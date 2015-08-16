#ifndef SYSTEM_MICROPTP_PORTS_CORTEX_M4_ONETHREAD_UTIL_HPP_
#define SYSTEM_MICROPTP_PORTS_CORTEX_M4_ONETHREAD_UTIL_HPP_

#include <microptp_config.hpp>
#ifdef MICROPTP_PORT_CORTEX_M4_ONETHREAD

#include <microlib/intrusive_pool.hpp>
#include <microlib/functional.hpp>
#include <microptp/ptpdatatypes.hpp>
#include <lwip/ip.h>
#include <stmlib/eth/lwip/custom_buffer.hpp>

namespace uptp {

	class SystemPort;

	class PacketHandle {
	public:
		PacketHandle();
		PacketHandle(PacketHandle&&);
		PacketHandle& operator=(PacketHandle&&);

		PacketHandle(const PacketHandle&) = delete;
		PacketHandle& operator=(const PacketHandle&) = delete;

		~PacketHandle();

	public:
		// Handle Simulation
		PacketHandle* operator->();
		const PacketHandle* operator->() const;

		PacketHandle& operator*();
		const PacketHandle& operator*() const;

		explicit operator bool() const;

	public:
		// [PacketRep]
		void* get_data();
		const void* get_data() const;

		size_t capacity() const;
		void set_size(size_t size);

		Time time() const;

		// Used by System Port only
	public:
		void time_to_logical();
		void time_to_physical();
		PacketHandle(eth::lwip::custom_buffer_ptr buffer);
		pbuf* release_pbuf();
		void set_transmit_callback(ulib::function<void(uint64, eth::lwip::custom_buffer_ptr)>);

	private:
		eth::lwip::custom_buffer_ptr buffer_;
	};

	using ReceivePacketHandle = PacketHandle;
	using TransmitPacketHandle = PacketHandle;

	struct recv_data_struct;

	class UdpStruct {
	public:
		UdpStruct(SystemPort* sysport, uint16 port);

	public:
		// [NetRep]
		void send( uint32 ip, uint16 port, PacketHandle handle, uint32 id );

		ulib::function<void(uint32 id, Time timestamp)> on_transmit_completed;
		ulib::function<void(PacketHandle)> on_received;

		static PacketHandle acquire_transmit_handle();

		explicit operator bool() const;

	private:
		void transmit_completed_callback(uint64 time, eth::lwip::custom_buffer_ptr buffer);
		
		// FIXME: temporal hack... should really store the id inside the buffers.
		uint32 transmit_id;

		static void create_udp( void* arg );
		static void on_recv( void * arg, struct udp_pcb * upcb, struct pbuf * p, const ip_addr_t * addr, u16_t port);

		udp_pcb* pcb_;
		SystemPort* sysport_;
		uint16 udpport_;
	};

	using NetHandle = ulib::pool_ptr<UdpStruct>;

	class Timer {
	public:
		Timer(const Timer&) = delete;
		void operator=(const Timer&) = delete;
		
		Timer();
		Timer(ulib::function<void()> func);

		~Timer();

	public:
		// [TimerRep]
		void start(uint32 msecs);
		void reset(uint32 msecs);
		void stop();

		ulib::function<void()> callback;
		
	private:
		static void timer_func(void* arg);
		
	};

	using TimerHandle = ulib::pool_ptr<Timer>;

	using ip_address = ip_addr_t;
}

#endif
#endif
