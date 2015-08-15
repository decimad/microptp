/*
 * state_slave.hpp
 *
 *  Created on: 10.07.2015
 *      Author: Michael
 */

#ifndef MICROPTP_STATE_SLAVE_HPP__
#define MICROPTP_STATE_SLAVE_HPP__
#include <microptp/state_base.hpp>
#include <microptp/ptpdatatypes.hpp>

namespace uptp {

	class PtpClock;

	namespace states {

		class Slave;

		namespace slave_detail {

			struct estimating_drift {
				// we're estimating the drift before setting the time and starting the PI servo for reducing the initial shock
				// PTP-Delay-Calculation introduces an error term linear in drift!
				estimating_drift();

				void on_sync (Slave& state,  Time master_time, Time slave_time);
				void on_delay(Slave& state,  Time master_time, Time slave_time);

				uint16 num_syncs_received_;
				Time   first_sync_master_;
				Time   first_sync_slave_;

				Time sync_master_;
				Time sync_slave_;

				ulib::circular_averaging_buffer<int32, 8> one_way_delay_buffer_;		// if the drift is bad, delay can actually be negative!
				ulib::circular_averaging_buffer<int64,  8> uncorrected_offset_buffer_;
			};

			struct pi_operational {
				pi_operational(int32 delay_nanos);

				void on_sync(Slave& state, Time master_time, Time slave_time);
				void on_delay(Slave& state, Time master_time, Time slave_time);

				Time sync_master_;
				Time sync_slave_;
				Time last_time_;

				ulib::circular_averaging_buffer<int32, 4> one_way_delay_buffer_;		// if the drift is bad, delay can actually be negative!
				ulib::circular_averaging_buffer<int32, 4> uncorrected_offset_buffer_;
			};


			enum class SyncState {
				Initial,
				SyncTwoStepReceived,
			};

			enum class DreqState {
				Initial,
				DreqSent
			};
		}

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

			// One-Step
			void on_sync         (uint16 serial, const Time& receive_time, const Time& send_time);

			// Two-Step
			void on_sync         (uint16 serial, const Time& receive_time);
			void on_sync_followup(uint16 serial, const Time& send_time);

			void do_delay_request();
			void on_request_sent    (const Time& dreq_send);
			void on_request_answered(const Time& dreq_receive);

		private:
			friend class slave_detail::estimating_drift;
			friend class slave_detail::pi_operational;

			ulib::state_machine<slave_detail::estimating_drift, slave_detail::pi_operational> states_;

			Time sync_receive_;	/* sync receive time from slave */
			slave_detail::SyncState sync_state_;
			uint16 sync_serial_;

			Time dreq_send_;	/* delay request time from slave */
			Time dreq_receive_;	/* delay reques answer from master */
			uint16 delay_req_id_;
			slave_detail::DreqState dreq_state_;

			ClockServo servo_;
			PtpClock& clock_;

			//PtpDelayState delay_state_;
			//TimerHandleType delay_req_timer_;
		};

	}
}

#endif /* SYSTEM_MICROPTP_STATE_SLAVE_HPP_ */
