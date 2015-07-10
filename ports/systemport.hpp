#ifndef MICROPTP_PORTS_SYSTEMPORT_HPP__
#define MICROPTP_PORTS_SYSTEMPORT_HPP__

#include <microptp/ports/cortex_m4_util.hpp>
#include <microptp/ports/cortex_m4.hpp>
#include <microptp/ports/systemport_defaults.hpp>

#endif

/*
* Documentation of Environment API
*
*/

	// Necessary interface for all ports:

	/*
	class PacketHandle {
	public:
		PacketHandle();
		PacketHandle(PacketHandle&&);
		PacketHandle& operator=(PacketHandle&&);

		const void* get_data() const;
		void* get_data();
	};

	class SystemPort {		
	public:
		using packet_handle_type = PacketHandle;

		void init();

		void make_timer(size_t id, uint32 millis);
		void close_timer(size_t id);

		void init_udp( uint16 port);
		void join_multicast(uint32 multicast_addr);
		void leave_multicast();
		void close_udp();

		void send(PacketHandleType, uint32 packet_id);

		Time get_time();
		void set_time(Time absolute);
		void adjust_time(Time delta);
		void discipline(int32 ppb);

		void close();

		util::function<void(PacketHandleType)> on_message;		
		util::function<void(uint32, const Time&)> send_completed;
		util::function<void(uint32)> on_timer;
	};
	*/
