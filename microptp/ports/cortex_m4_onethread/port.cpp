#include "microptp_config.hpp"
#ifdef MICROPTP_PORT_CORTEX_M4_ONETHREAD

#include <lwip/ip.h>
#include <lwip/tcpip.h>
#include <lwip/udp.h>
#include <lwip/igmp.h>
#include <stmlib/eth/lwip/custom_buffer.hpp>
#include <stmlib/eth/lwip_onethread/lwip_thread.hpp>
#include <stmlib/eth.hpp>
#include <stmlib/trace.h>
#include <microlib/variant.hpp>
#include <microlib/pool.hpp>
#include <microptp/ports/cortex_m4_onethread/port.hpp>

namespace uptp {

	//
	// UdpStruct
	//
	UdpStruct::UdpStruct(SystemPort* sysport, uint16 port)
		: sysport_(sysport), udpport_(port), pcb_(nullptr)
	{
		pcb_ = udp_new();
		if (pcb_) {
			if (udp_bind(pcb_, IP_ADDR_ANY, udpport_) != 0) {
				udp_remove(pcb_);
				pcb_ = nullptr;
			} else {
				udp_recv(pcb_, &UdpStruct::on_recv, this);
			}
		}
	}

	void UdpStruct::on_recv( void * arg, struct udp_pcb * upcb, struct pbuf * p, const ip_addr_t * addr, u16_t port)
	{
		UdpStruct& udp_struct = *reinterpret_cast<UdpStruct*>(arg);
		PacketHandle handle(eth::lwip::ptr_from_pbuf(p));

		if (udp_struct.on_received) {
			udp_struct.on_received(std::move(handle));
		}
	}

	// called in context of SystemPort-thread
	void UdpStruct::send( uint32 ip, uint16 port, PacketHandle handle, uint32 id )
	{
		transmit_id = id;
		handle.set_transmit_callback(ulib::function<void(uint64,eth::lwip::custom_buffer_ptr)>(this, &UdpStruct::transmit_completed_callback));
		udp_sendto(pcb_, handle.release_pbuf(), (const ip_addr_t*) &ip, port);
	}

	void UdpStruct::transmit_completed_callback(uint64 time, eth::lwip::custom_buffer_ptr buffer)
	{
		if (on_transmit_completed) {
			on_transmit_completed(transmit_id, Time(time));
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

	void SystemPort::command(ThreadCommands cmds)
	{
		if(cmds == ThreadCommands::EnableClock) {
			clock_.enable();
		} else if(cmds == ThreadCommands::DisableClock) {
			clock_.disable();
		}
	}

	void SystemPort::network_changed( ip_address ipaddr, std::array<unsigned char, 6>& macaddr )
	{
		clock_.on_network_changed(ipaddr, macaddr);

		//ip_address_ = addr;
		//clock_.on_network_changed();
	}

	// Port interface
	void SystemPort::init()
	{
	}

		TimerHandle SystemPort::make_timer(ulib::function<void()> func)
	{
		return timer_pool_.make(std::move(func));
	}

	TimerHandle SystemPort::make_timer()
	{
		return timer_pool_.make();
	}

	NetHandle SystemPort::make_udp(uint16 port)
	{
		return udp_pool_.make(this, port);
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
		// isn't this really an implementation detail inside the driver? maybe support Secs+Nanos AND Secs+Subsecs there as a type directly.
		uint32 subs = eth::ptp_nanos_to_subseconds(ulib::abs(delta.nanos_));
		uint32 secs = static_cast<uint32>(ulib::abs(delta.secs_));
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

	void PacketHandle::set_transmit_callback(ulib::function<void(uint64, eth::lwip::custom_buffer_ptr)> func)
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

	//
	// Timer Handle
	//
	Timer::Timer()
	{}

	Timer::Timer(ulib::function<void()> func)
		: callback(std::move(func))
	{}

	void Timer::start(uint32 timeout_msecs)
	{
		eth::lwip::LwipThread::get().add_timeout_thread(timeout_msecs, &Timer::timer_func, this);
	}

	void Timer::reset(uint32 timeout_msecs)
	{
		eth::lwip::LwipThread::get().update_timeout_thread(timeout_msecs, &Timer::timer_func, this);
	}

	void Timer::stop()
	{
		eth::lwip::LwipThread::get().remove_timeout_thread(&Timer::timer_func, this);
	}

	void Timer::timer_func(void* arg)
	{
		Timer* tim = static_cast<Timer*>(arg);
		if (tim->callback) {
			tim->callback();
		}
	}

	Timer::~Timer()
	{
		stop();
	}

}

#endif
