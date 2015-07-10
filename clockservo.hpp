#ifndef MICROPTP_CLOCKSERVO_HPP__
#define MICROPTP_CLOCKSERVO_HPP__

#pragma once
#include <microptp/uptp.hpp>
#include <microptp/ptpdatatypes.hpp>
#include <microptp/util/functional.hpp>
#include <microptp/util/circularbuffer.hpp>
#include <microptp/util/statemachine.hpp>
#include <microptp/util/fixedpoint.hpp>

namespace uptp {

	namespace detail {
		
		template< size_t y_size, uint32... Coeffs >
		struct iir {

			std::array<uint32, sizeof...(Coeffs)> memory_;
		};

		template< uint32... Coeffs >
		struct fir {
			std::array<uint32, sizeof...(Coeffs)> memory_;
		};

	}
	
	class PtpDelayState {
	public:
		struct estimating_drift {
			// we're estimating the drift before setting the time and starting the PI servo for reducing the initial shock
			// PTP-Delay-Calculation introduces an error term linear in drift!
			estimating_drift();

			void on_sync(PtpDelayState& state,  Time master_time, Time slave_time);
			void on_delay(PtpDelayState& state, Time master_time, Time slave_time);

			uint16 num_syncs_received_;
			Time   first_sync_master_;
			Time   first_sync_slave_;

			Time sync_master_;
			Time sync_slave_;
		};

		struct pi_operational {
			pi_operational();

			void on_sync(PtpDelayState& state, Time master_time, Time slave_time);
			void on_delay(PtpDelayState& state, Time master_time, Time slave_time);

			Time sync_master_;
			Time sync_slave_;
			Time last_time_;
		};

		enum class SyncState {
			Initial,
			SyncTwoStepReceived,
		};

		enum class DreqState {
			Initial,
			DreqSent
		};

		PtpDelayState(SystemPort& port);
		~PtpDelayState();

		// One-Step
		void on_sync         (uint16 serial, const Time& receive_time, const Time& send_time);
		
		// Two-Step
		void on_sync         (uint16 serial, const Time& receive_time);
		void on_sync_followup(uint16 serial, const Time& send_time);

		// One-Step
		void do_delay_request();
		void on_request_sent    (const Time& dreq_send);
		void on_request_answered(const Time& dreq_receive);

		void reset();

		const Time& get_offset() const;
		const Time& get_delay() const;

		util::function<void(uint32 /*dt_nanos*/, int32 /*offset_nanos*/)> on_offset_update;

	private:
		util::state_machine<estimating_drift, pi_operational> states_;
		util::circular_averaging_buffer<int32, 8> one_way_delay_buffer_;		// if the drift is bad, delay can actually be negative!
		util::circular_averaging_buffer<int32, 8> uncorrected_offset_buffer_;

		// Syncs are shared by either state
		Time sync_receive_;	/* sync receive time from slave */
		SystemPort& port_;
		SyncState sync_state_;
		uint16 sync_serial_;

		Time dreq_send_;	/* delay request time from slave */
		Time dreq_receive_;	/* delay reques answer from master */
		uint16 dreq_serial_;
		DreqState dreq_state_;
	};
	
	class ClockServo {
	public:
		using fixed_type = util::fixed_point<int32, -8>; // 16.16

		ClockServo(SystemPort& clock);
		~ClockServo();

		void reset( int32 integrator );
		void feed( uint32 dt, int32 offset );

		util::function<void(int32)> output;

	private:
		fixed_type integrator_state_;

		fixed_type kn_;
		fixed_type kp_;

		SystemPort& system_port_;
		uint8 offset_count_;
		uint8 delay_count_;
	};
	
}

#endif
