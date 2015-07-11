#include <lwip/tcpip.h>
#include <lwip/udp.h>
#include <lwip/igmp.h>
#include <stm/util/intrusive_pool.hpp>
#include <stm/eth/lwip/custom_buffer.hpp>
#include <stm/eth.hpp>
#include <microptp/util/static_union.hpp>
#include <microptp/ports/cortex_m4.hpp>
#include <chmboxes.h>	// Mailbox for UPTP thread
#include <chbsem.h>
#include <stm/trace.h>


namespace uptp {

	struct sysport_message {
		virtual void operator()(SystemPort* sysport) = 0;
		virtual void free() = 0;
		virtual ~sysport_message() {}
	};

	struct func_struct {
		using callback_function = void (*)(uintptr_t arg1, uintptr_t arg2);

		// would really need some thread safety here
		callback_function func;
		uintptr_t arg1;
		uintptr_t arg2;
	};

	struct sysport_default_message : public sysport_message
	{
		util::static_union< func_struct, SystemPort::ThreadCommands, ip_addr_t > payload;

		// fixme, use douple dispatch...
		void operator()(SystemPort* sysport) {
			if(payload.is<SystemPort::ThreadCommands>()) {
				auto& ref = payload.as<SystemPort::ThreadCommands>();
				sysport->on_command(ref);
			} else if(payload.is<ip_addr_t>()) {
				auto& ref = payload.as<ip_addr_t>();
				sysport->on_ip_addr_changed(ref);
			} else if(payload.is<func_struct>()) {
				auto& ref = payload.as<func_struct>();
				ref.func(ref.arg1, ref.arg2);
			}
		}

		util::pool_ptr<sysport_default_message> lifetime_;

		void free() override {
			lifetime_.clear();
		}
	};

	util::pool<sysport_default_message, 4, util::SysLock> default_message_pool;

	struct recv_data_struct
	{
		PacketHandle packet;
		ip_addr addr;
		UdpStruct* udp_struct;
	};

	struct transmit_complete_struct {
		UdpStruct* udp;
		uint32 id;
		uint64 time;
	};

	struct udp_messages : public sysport_message
	{
		util::static_union< recv_data_struct, transmit_complete_struct > payload;

		void operator()(SystemPort* port)
		{
			if(payload.is< recv_data_struct >()) {
				auto& ref = payload.as< recv_data_struct >();
				ref.udp_struct->on_recv_portthread(ref);
			} else if(payload.is<transmit_complete_struct>()) {
				auto& ref = payload.as<transmit_complete_struct>();
				ref.udp->on_transmit_completed_portthread(ref.id, ref.time);
			}
		}

		void free() override {
			lifetime_.clear();
		}

		~udp_messages() {
		}

		util::pool_ptr<udp_messages> lifetime_;
	};

	util::pool<udp_messages, 4, util::SysLock> udp_message_pool;

	//
	// UdpStruct
	//
	UdpStruct::UdpStruct(SystemPort* sysport, uint16 port)
		: sysport_(sysport), udpport_(port), pcb_(nullptr)
	{
		tcpip_callback(&UdpStruct::create_udp, this);
	}

	void UdpStruct::create_udp(void* arg)
	{
		UdpStruct& ref = *reinterpret_cast<UdpStruct*>(arg);
		ref.pcb_ = udp_new();
		if( ref.pcb_ ) {
			if(udp_bind(ref.pcb_, IP_ADDR_ANY, ref.udpport_) != 0) {
				udp_remove(ref.pcb_);
				ref.pcb_ = nullptr;
			} else {
				udp_recv(ref.pcb_, &UdpStruct::on_recv_tcpthread, &ref);
			}
		}

	}

	// called in context of tcpip-thread
	void UdpStruct::on_recv_tcpthread( void * arg, struct udp_pcb * upcb, struct pbuf * p, struct ip_addr * addr, u16_t port)
	{
		UdpStruct& udp_struct = *reinterpret_cast<UdpStruct*>(arg);
		PacketHandle handle(eth::lwip::ptr_from_pbuf(p));

		// relay over udp_message() -> mailbox -> udp_message::operator() -> to on_recv_portthread
		auto msg = udp_message_pool.make();
		if(msg) {
			auto& ref = msg->payload.to_type<recv_data_struct>();
			ref.addr = *addr;
			ref.packet = std::move(handle);
			ref.udp_struct = &udp_struct;
			auto* bare_ptr = msg.get_payload();
			msg->lifetime_ = std::move(msg);
			udp_struct.sysport_->post_message(bare_ptr);
		}
	}

	void UdpStruct::on_recv_portthread( recv_data_struct& data )
	{
		if(on_received) {
			on_received(std::move(data.packet));
		}
	}


	// Message payload to transfer send command from
	// portthread to tcp thread
	struct send_struct {
		udp_pcb* upcb;
		PacketHandle handle;
		ip_addr addr;
		uint16 port;
		BinarySemaphore sema;
	};

	void send_tcpthread(void* ptr);

	// called in context of SystemPort-thread
	void UdpStruct::send( uint32 ip, uint16 port, PacketHandle handle, uint32 id )
	{
		transmit_id = id;

		handle.set_transmit_callback(util::function<void(uint64,eth::lwip::custom_buffer_ptr)>(this, &UdpStruct::on_transmit_completed_tcpthread));

		send_struct args;
		args.upcb = pcb_;
		args.handle = std::move(handle);
		args.addr.addr = ip;
		args.port = port;
		// blocking call
		chBSemInit(&args.sema, 1);
		tcpip_callback(send_tcpthread, &args);
		chBSemWait(&args.sema);
	}

	void send_tcpthread(void* ptr)
	{
		auto* data = reinterpret_cast<send_struct*>(ptr);
		auto local = std::move(data->handle);
		auto* pb = local.release_pbuf();
		udp_sendto(data->upcb, pb, &data->addr, data->port );
		chBSemReset((&data->sema), 0);
	}

	void UdpStruct::on_transmit_completed_tcpthread(uint64 time, eth::lwip::custom_buffer_ptr buffer)
	{
		auto msg = udp_message_pool.make();
		if(msg) {
			auto& ref = msg->payload.to_type<transmit_complete_struct>();
			ref.udp = this;
			ref.time = time;
			ref.id = transmit_id;
			auto* bare_pointer = msg.get_payload();
			msg->lifetime_ = std::move(msg);
			sysport_->post_message(bare_pointer);
		}
	}

	void UdpStruct::on_transmit_completed_portthread(uint32 id, uint64 time)
	{
		if(on_transmit_completed) {
			on_transmit_completed(id, Time(time));
		}
	}


	UdpStruct::operator bool() const {
		return pcb_ != nullptr;
	}

	PacketHandle UdpStruct::acquire_transmit_handle()
	{
		auto ptr = eth::lwip::request_buffer();
		if(ptr) {
			ptr->reset_udp();
		}
		return std::move(ptr);
	}


	util::pool<UdpStruct, 4> udp_pool;

	//
	// SystemPort
	//

	SystemPort::SystemPort(const Config& cfg)
		: clock_(*this, cfg)
	{

	}

	SystemPort::~SystemPort()
	{

	}

	void SystemPort::start()
	{
		thread_.run(NORMALPRIO, this);
	}

	msg_t SystemPort::threadfunc(void* arg)
	{
		SystemPort& port = *static_cast<SystemPort*>(arg);

		msg_t mailbox_buff[16];
		chMBInit(&port.mailbox_, mailbox_buff, 16);

		while(true) {
			msg_t msg;
			auto result = chMBFetch(&port.mailbox_, &msg, MS2ST(1000) );
			if(result == RDY_OK) {
				auto* port_msg = reinterpret_cast<sysport_message*>(msg);
				(*port_msg)(&port);
				port_msg->free();
			} else if(result == RDY_RESET) {
				break;
			}
		}

		return 0;
	}

	void SystemPort::on_command(ThreadCommands cmds)
	{

	}

	void SystemPort::on_ip_addr_changed( ip_addr_t addr )
	{
		ip_address_ = addr;
		clock_.on_network_changed();
	}

	void SystemPort::post_message(sysport_message* msg)
	{
		chMBPost(&mailbox_, reinterpret_cast<msg_t>(msg), TIME_INFINITE);
	}


	void SystemPort::ip_addr_changed( ip_addr_t addr )
	{
		auto msg = default_message_pool.make();
		if(msg) {
			auto ref = msg->payload.to_type<ip_addr_t>();
			ref = addr;
			auto* bare_ptr = msg.get_payload();
			msg->lifetime_ = std::move(msg);
			post_message(bare_ptr);
		}
	}


	void SystemPort::post_callback( void(*func)(uintptr_t, uintptr_t), uintptr_t arg1, uintptr_t arg2)
	{
		auto msg = default_message_pool.make();
		if(msg) {
			auto ref = msg->payload.to_type<func_struct>();
			ref.func = func;
			ref.arg1 = arg1;
			ref.arg2 = arg2;
			auto* bare_ptr = msg.get_payload();
			msg->lifetime_ = std::move(msg);
			post_message(bare_ptr);
		}
	}

	// Port interface
	void SystemPort::init()
	{
	}

	void SystemPort::make_timer(size_t id, uint32 millis)
	{

	}

	void SystemPort::close_timer(size_t id)
	{

	}

	NetHandle SystemPort::make_udp(uint16 port)
	{
		return udp_pool.make(this, port);
	}

	void SystemPort::join_multicast(uint32 multicast_addr)
	{
		auto result = igmp_joingroup( &ip_address_, (ip_addr_t*)&multicast_addr );
	}

	void SystemPort::leave_multicast()
	{

	}

	Time SystemPort::get_time()
	{
		return Time();
	}

	void SystemPort::set_time(Time absolute)
	{

	}

	void SystemPort::adjust_time(Time delta)
	{
		uint32 subs = eth::ptp_nanos_to_subseconds(util::abs(delta.nanos_));
		uint32 secs = static_cast<uint32>(util::abs(delta.secs_));
		if(delta.secs_ < 0 || delta.nanos_ < 0) {
			subs |= 1<<31;
		} else {
			trace_printf(0, "System Port: adding %d secs %d subs.\n", secs, subs);
		}

		eth::ptp_update_time(secs, subs);
	}

	void SystemPort::discipline(int32 ppb)
	{
		eth::ptp_discipline(ppb);
	}

	void SystemPort::close()
	{

	}

	//
	// PacketHandle
	//

	PacketHandle::PacketHandle()
		: buffer_(nullptr)
	{

	}

	PacketHandle::PacketHandle(eth::lwip::custom_buffer_ptr buffer)
		: buffer_(std::move(buffer))
	{
	}

	PacketHandle::~PacketHandle()
	{
		if(buffer_) {
			eth::lwip::custom_buffer_reuse_chain(std::move(buffer_));
		}
	}

	PacketHandle::PacketHandle(PacketHandle&& other)
		: buffer_(std::move(other.buffer_))
	{
	}

	void PacketHandle::set_transmit_callback(util::function<void(uint64, eth::lwip::custom_buffer_ptr)> func)
	{
		if(buffer_) {
			auto& ref = buffer_->enhance().to_type<eth::lwip::transmit_callback>();
			ref.func = func;
		}
	}

	PacketHandle& PacketHandle::operator=(PacketHandle&& other)
	{
		buffer_ = std::move(other.buffer_);
		return *this;
	}

	void PacketHandle::time_to_logical()
	{
		if(buffer_->is_enhanced() && buffer_->enhanced()->is<uint64>()) {
			auto& ref = buffer_->enhanced()->as<uint64>();
			ref = eth::ptp_hardware_to_logical(ref);
		}
	}

	void PacketHandle::time_to_physical()
	{
		if(buffer_->is_enhanced() && buffer_->enhanced()->is<uint64>()) {
			auto& ref = buffer_->enhanced()->as<uint64>();
			ref = eth::ptp_logical_to_hardware(ref);
		}
	}

	Time PacketHandle::time() const
	{
		if(buffer_->is_enhanced() && buffer_->enhanced()->is<uint64>()) {
			return Time(buffer_->enhanced()->as<uint64>());
		} else {
			return Time();
		}
	}

	void* PacketHandle::get_data()
	{
		return buffer_->data();
	}

	PacketHandle::operator bool() const
	{
		return buffer_ ? true : false;
	}

	const void* PacketHandle::get_data() const
	{
		return buffer_->data();
	}

	pbuf* PacketHandle::release_pbuf()
	{
		return &(buffer_.release_type()->pbuf);
	}

	size_t PacketHandle::capacity() const
	{
		return buffer_->capacity();
	}

	void PacketHandle::set_size(size_t size)
	{
		buffer_->pbuf.len     = size;
		buffer_->pbuf.tot_len = size;
	}

}
