#ifndef SYSTEM_MICROPTP_PORTS_CORTEX_M4_UTIL_HPP_
#define SYSTEM_MICROPTP_PORTS_CORTEX_M4_UTIL_HPP_

#include <microlib/intrusive_pool.hpp>
#include <microlib/functional.hpp>
#include <microptp/ptpdatatypes.hpp>
#include <lwip/api.h>
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

		void* get_data();
		const void* get_data() const;

		size_t capacity() const;
		void set_size(size_t size);

		Time time() const;

		void time_to_logical();
		void time_to_physical();
		PacketHandle(eth::lwip::custom_buffer_ptr buffer);
		explicit operator bool() const;

		pbuf* release_pbuf();

		~PacketHandle();

		void set_transmit_callback(util::function<void(uint64, eth::lwip::custom_buffer_ptr)>);

	private:
		eth::lwip::custom_buffer_ptr buffer_;
	};

	using ReceivePacketHandle = PacketHandle;
	using TransmitPacketHandle = PacketHandle;

	struct recv_data_struct;

	class UdpStruct {
	public:
		UdpStruct(SystemPort* sysport, uint16 port);

		void send( uint32 ip, uint16 port, PacketHandle handle, uint32 id );

		util::function<void(uint32 id, Time timestamp)> on_transmit_completed;
		util::function<void(PacketHandle)> on_received;

		explicit operator bool() const;

		static PacketHandle acquire_transmit_handle();

	private:
		void on_transmit_completed_tcpthread(uint64 time, eth::lwip::custom_buffer_ptr buffer);
		void on_transmit_completed_portthread(uint32 id, uint64 time);

		// FIXME: temporal hack... should really store the id inside the buffers.
		uint32 transmit_id;

		static void create_udp( void* arg );
		static void on_recv_tcpthread( void * arg, struct udp_pcb * upcb, struct pbuf * p, struct ip_addr * addr, u16_t port);

		friend class udp_messages;
		void on_recv_portthread( recv_data_struct& data );

		udp_pcb* pcb_;
		SystemPort* sysport_;
		uint16 udpport_;
	};

	using NetHandle = util::pool_ptr<UdpStruct>;

}

#endif
