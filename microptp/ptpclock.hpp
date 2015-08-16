//          Copyright Michael Steinberg 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MICROPTP_PTPCLOCK_HPP__
#define MICROPTP_PTPCLOCK_HPP__

#include <microlib/statemachine.hpp>
#include <microlib/string.hpp>
#include <microptp/ports/systemportapi.hpp>
#include <microptp/messages.hpp>
#include <microptp/clockservo.hpp>
#include <microptp/ptpdatatypes.hpp>
#include <microptp/mastertracker.hpp>
#include <microptp/uptp.hpp>
#include <microptp/state_slave.hpp>
#include <microptp/state_disabled.hpp>

namespace uptp {

	class PtpClock;
	class SystemPort;

	namespace states {

		class Initializing : public PtpStateBase
		{
		public:
			Initializing(PtpClock& clock);
			~Initializing();

			void on_announce_timeout();
			void on_best_master_changed();
			void on_message(const msg::Header& header, PacketHandle) override;

		private:
			PtpClock& clock_;
		};

		class Faulty : public PtpStateBase
		{
		public:
			Faulty(PtpClock& clock);
		};
		/*
		class Disabled : public PtpStateBase
		{
		public:
			Disabled(PtpClock& clock);
		};
*/

		class Listening : public PtpStateBase
		{
		public:
			Listening(PtpClock& clock);
			~Listening();

			void on_message(const msg::Header&, PacketHandle) override;
			void on_best_master_changed();

		private:
			PtpClock& clock_;
		};

		/*
		class PeerSlave : public PtpStateBase
		{
		public:
			PeerSlave(PtpClock& clock);

			void on_pdelay_req_timer();

		private:
			ClockServo servo_;

		};
*/

		class Master : public PtpStateBase
		{
		public:

			void on_announce_timer();
			void on_delay_req();
			
		private:
			ClockQuality quality;
			uint8 priority1;
			uint8 priority2;
		};

		class Passive : public PtpStateBase
		{
		public:
			Passive(PtpClock&);
		};

		class Uncalibrated : public PtpStateBase
		{
		public:
			Uncalibrated(PtpClock&);
		};

	}

	class PtpClock
	{
	public:
		PtpClock(SystemPort& env, const Config& cfg);

	public:
		void enable();
		void disable();

	public:
		// Invoked by systemport
		void on_network_changed(ip_address ipaddr, const std::array<uint8, 6>& macaddr);
		void on_general_message(PacketHandle packet);
		void on_event_message(PacketHandle packet);

	public:
		SystemPort& get_system_port();
		Config& get_config();
		MasterTracker& master_tracker();

		NetHandle& event_port();
		NetHandle& general_port();

		PortIdentity& get_identity();

	public:
		// State machine
		template< typename State, typename... Args >
		void to_state(Args&&... args)
		{
			statemachine_.template to_state<State>(*this, std::forward<Args>(args)...);
		}

		template< typename T >
		bool is() const {
			return statemachine_.template is_state<T>();
		}

	public:
		void send_delay_req(ulib::function<void(const Time&)> completion_func);

	private:
		NetHandle event_port_;
		NetHandle general_port_;

		SystemPort& system_port_;
		Config config_;
		PortIdentity port_identity_;
		MasterTracker master_tracker_;
		
		void init_net();

		ulib::state_machine<
			states::Initializing,
			//states::Faulty,
			states::Disabled,
			states::Listening,
			states::Slave
			//states::Master,
			//states::Passive,
			//states::Uncalibrated
		> statemachine_;

	};


}

#endif
