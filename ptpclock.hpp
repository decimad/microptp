#ifndef MICROPTP_PTPCLOCK_HPP__
#define MICROPTP_PTPCLOCK_HPP__

#include <microptp/util/statemachine.hpp>
#include <microptp/util/string.hpp>
#include <microptp/ports/systemport_defaults.hpp>
#include <microptp/messages.hpp>
#include <microptp/clockservo.hpp>
#include <microptp/ptpdatatypes.hpp>
#include <microptp/mastertracker.hpp>
#include <microptp/uptp.hpp>

namespace uptp {

	class PtpClock;
	class SystemPort;

	namespace states {

		class PtpStateBase
		{
		public:
			virtual void on_message(const msg::Header& header, PacketHandle) = 0;
			virtual ~PtpStateBase() {}
		};

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

		class Disabled : public PtpStateBase
		{
		public:
			Disabled(PtpClock& clock);
		};

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

		class Slave : public PtpStateBase
		{
		public:
			Slave(PtpClock& clock);
			~Slave();

			void on_delay_req_timer();
			void on_message(const msg::Header&, PacketHandle) override;
			void on_best_master_changed();

		private:
			void send_delay_request();
			void on_delay_request_transmitted(uint32 id, Time time);

			PtpDelayState delay_state_;
			uint16 delay_req_id_;
			//TimerHandleType delay_req_timer_;
			ClockServo servo_;
			PtpClock& clock_;
		};

		class PeerSlave : public PtpStateBase
		{
		public:
			PeerSlave(PtpClock& clock);

			void on_pdelay_req_timer();

		private:
			ClockServo servo_;

		};

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

		SystemPort& get_system_port();
		Config& get_cfg();

		MasterTracker& master_tracker();

		void send_delay_req(util::function<void(const Time&)> completion_func);

		void on_network_changed();
		void on_general_message(PacketHandle packet);
		void on_event_message(PacketHandle packet);

		NetHandle& event_port();
		NetHandle& general_port();

		PortIdentity& get_identity();

		template< typename State, typename... Args >
		void to_state(Args&&... args)
		{
			statemachine_.template to_state<State>(*this, std::forward<Args>(args)...);
		}

	private:
		NetHandle event_port_;
		NetHandle general_port_;

		SystemPort& system_port_;
		Config cfg_;
		PortIdentity clock_identity_;
		MasterTracker master_tracker_;
		
		void init_net();

		PortIdentity port_identity_;
		//util::static_string<32> user_description;

		util::state_machine<
			states::Initializing,
			//states::Faulty,
			//states::Disabled,
			states::Listening,
			states::Slave,
			//states::Master,
			//states::Passive,
			states::Uncalibrated
		> statemachine_;

	};


}

#endif
