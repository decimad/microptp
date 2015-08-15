#ifndef MICROPTP_PORTS_CORTEX_M4_ONETHREAD_PORT_HPP__
#define MICROPTP_PORTS_CORTEX_M4_ONETHREAD_PORT_HPP__

#include <microptp_config.hpp>
#ifdef MICROPTP_PORT_CORTEX_M4_ONETHREAD

#include <microptp/ptpdatatypes.hpp>
#include <microptp/ptpclock.hpp>
#include <microptp/ports/cortex_m4_onethread/port_types.hpp>
#include <microptp/uptp.hpp>
#include <microlib/pool.hpp>
#include <microlib/functional.hpp>
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

		void network_changed( ip_address addr, std::array<uint8, 6>& mac_address );

		// Port interface
	public:
		using packet_handle_type = PacketHandle;

		void init();

		TimerHandle make_timer();
		TimerHandle make_timer(ulib::function<void()> func);

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
		ulib::pool<UdpStruct, 4> udp_pool_;
		ulib::pool<Timer, 5> timer_pool_; // a watchdog timer per remote master plus a delay_req timer

		PtpClock clock_;
		ip_addr_t ip_address_;		
	};
		
}

#endif
#endif
