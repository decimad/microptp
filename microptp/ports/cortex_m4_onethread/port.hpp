#ifndef MICROPTP_PORTS_CORTEX_M4_INTHREAD_HPP__
#define MICROPTP_PORTS_CORTEX_M4_INHTREAD_HPP__

#include <microptp_config.hpp>
#ifdef MICROPTP_PORT_CORTEX_M4_ONETHREAD

#include <microptp/ptpdatatypes.hpp>
#include <microptp/ptpclock.hpp>
#include <microptp/ports/cortex_m4_onethread/port_types.hpp>
#include <microptp/uptp.hpp>
#include <thread.hpp>

namespace uptp {

	struct sysport_message;

	class SystemPort
	{
	public:
		SystemPort(const Config& cfg);
		~SystemPort();

		enum class ThreadCommands {
			EnableClock = 0,
			DisableClock = 1,
			Finish = 2
		};

	public:
		void start();

		void ip_addr_changed( ip_addr_t addr );

		// Port interface
	public:
		using packet_handle_type = PacketHandle;

		void init();

		void make_timer (size_t id, uint32 millis);
		void close_timer(size_t id);

		NetHandle make_udp(uint16 port);

		void join_multicast(uint32 multicast_addr);
		void leave_multicast();

		Time get_time();
		void set_time(Time absolute);
		void adjust_time(Time delta);
		void discipline(int32 ppb);

		void close();

	public:
		void command( ThreadCommands );

	private:
		PtpClock clock_;
		ip_addr_t ip_address_;
	};
		
}

#define UPTP_SLAVE_ONLY
#endif
#endif
