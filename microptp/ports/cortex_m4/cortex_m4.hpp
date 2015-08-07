#ifndef MICROPTP_PORTS_CORTEX_M4_HPP__
#define MICROPTP_PORTS_CORTEX_M4_HPP__

#include <microptp/ptpdatatypes.hpp>
#include <microptp/ptpclock.hpp>
#include <microptp/ports/cortex_m4/cortex_m4_util.hpp>
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

		void make_timer(size_t id, uint32 millis);
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

		template<typename T>
		void post_message(util::pool_ptr<T> msg) {
			auto* bare_ptr = msg.get_payload();
			msg->lifetime_ = std::move(msg);
			post_message(bare_ptr);
		}

		void post_message(sysport_message* msg);

		void post_ip_addr_changed( ip_addr_t );
		void post_thread_command( ThreadCommands );
		void post_callback( void(*)(uintptr_t, uintptr_t), uintptr_t, uintptr_t );

	private:
		// Mailbox
		Mailbox mailbox_;

		friend class sysport_default_message;

		void on_command(ThreadCommands);
		void on_ip_addr_changed(ip_addr_t arg);

	private:
		static msg_t threadfunc(void* the_port);

		util::static_thread<&SystemPort::threadfunc, 2048> thread_;	// warning: the mailbox buffer needs to fit into the stack!
		PtpClock clock_;
		ip_addr_t ip_address_;
	};
		
}

#define UPTP_SLAVE_ONLY
#endif
