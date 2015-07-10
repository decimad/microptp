#include <microptp/clockservo.hpp>
#include <microptp/util/fixedpoint.hpp>
#include <microptp/ports/systemport.hpp>
#include <stm/trace.h>

namespace uptp {

	//
	// PtpDelayState::estimating_drift
	//

	// we're estimating the drift before setting the time and starting the PI servo to reduce the initial shock
	// PTP-Delay-Calculation introduces an error term linear in drift!
	PtpDelayState::estimating_drift::estimating_drift()
		: num_syncs_received_(0)
	{
	}

	void PtpDelayState::estimating_drift::on_sync(PtpDelayState& state, Time master_time, Time slave_time)
	{
		if(num_syncs_received_ == 0) {
			first_sync_master_  = master_time;
			first_sync_slave_ = slave_time;
		}

		state.uncorrected_offset_buffer_.add((slave_time - master_time).to_nanos());

		sync_master_ = master_time;
		sync_slave_ = slave_time;

		++num_syncs_received_;
	}

	void PtpDelayState::estimating_drift::on_delay(PtpDelayState& state, Time master_time, Time slave_time)
	{
		// We're assuming that 8*one_way_delay doesn't reach a second!

		if(num_syncs_received_ >= 8) {
			// We're really assuming that getting the first 8 syncs took less than 16 seconds here!
			const auto nom    = (sync_master_ - first_sync_master_);
			const auto den    = (sync_slave_ - first_sync_slave_);
			const int32 drift = ((nom.to_nanos() << 28) / (den.to_nanos() >> 4))>>2;	// 2.30 fixed

			const auto& t0 = sync_master_;
			const auto& t1 = sync_slave_;

			const auto& t2 = slave_time;
			const auto& t3 = master_time;

			int32 ppb         = static_cast<int32>(((static_cast<int64>(drift) * 1000000000) >> 30) - 1000000000);
			int32 delay_nanos = static_cast<int32>(((t3-t0+t2-t1).to_nanos() >> 1) - ((t2-t1).to_nanos() << 30) / drift);

			// Fixme: use the mean offset + 1/2 * t * drift for a better estimate!
			Time  offset      = t1 - t0 - Time(0, delay_nanos);

			// our best estimate
			state.one_way_delay_buffer_.set(delay_nanos);
			state.uncorrected_offset_buffer_.set(0);

			state.port_.adjust_time(-offset);
			state.port_.discipline(ppb);
			state.states_.to_state<pi_operational>();
		}
	}

	//
	// PtpDelayState::pi_operational
	//

	PtpDelayState::pi_operational::pi_operational()
		: last_time_(0,0)
	{

	}

	void PtpDelayState::pi_operational::on_sync(PtpDelayState& state, Time master_time, Time slave_time)
	{
		sync_master_ = master_time;
		sync_slave_  = slave_time;
		Time offset = slave_time - master_time;
		state.uncorrected_offset_buffer_.add(offset.nanos_);
	}

	void PtpDelayState::pi_operational::on_delay(PtpDelayState& state, Time master_time, Time slave_time)
	{
		const auto& t0 = sync_master_;
		const auto& t1 = sync_slave_;
		const auto& t2 = slave_time;
		const auto& t3 = master_time;

		const Time one_way_delay = ((t3-t0)-(t2-t1))/2;
		state.one_way_delay_buffer_.add(one_way_delay.nanos_);

		if(last_time_.secs_ != 0 && state.on_offset_update) {
			auto offset = state.uncorrected_offset_buffer_.average() - state.one_way_delay_buffer_.average();
			auto dt = static_cast<uint32>((slave_time - last_time_).to_nanos());
			state.on_offset_update( dt, -offset );
		}

		last_time_ = slave_time;
	}

	//
	// PtpDelayState
	//

	PtpDelayState::PtpDelayState(SystemPort& port)
		: sync_state_(SyncState::Initial), dreq_state_(DreqState::Initial), port_(port)
	{
		states_.to_state<estimating_drift>();
	}

	PtpDelayState::~PtpDelayState()
	{
	}

	void PtpDelayState::on_sync(uint16 serial, const Time& receive_time, const Time& send_time)
	{
		if(states_.is_state<pi_operational>()) {
			states_.as_state<pi_operational>().on_sync(*this, send_time, receive_time);
		} else if(states_.is_state<estimating_drift>()) {
			states_.as_state<estimating_drift>().on_sync(*this, send_time, receive_time);
		}
		sync_state_ = SyncState::Initial;
	}

	// Two-Step
	void PtpDelayState::on_sync(uint16 serial, const Time& receive_time)
	{
		sync_receive_ = receive_time;
		sync_serial_ = serial;
		sync_state_ = SyncState::SyncTwoStepReceived;
	}

	void PtpDelayState::on_sync_followup(uint16 serial, const Time& send_time)
	{
		if(sync_state_ == SyncState::SyncTwoStepReceived && serial == sync_serial_) {
			if(states_.is_state<pi_operational>()) {
				states_.as_state<pi_operational>().on_sync(*this, send_time, sync_receive_);
			} else if(states_.is_state<estimating_drift>()) {
				states_.as_state<estimating_drift>().on_sync(*this, send_time, sync_receive_);
			}
			sync_state_ = SyncState::Initial;
		} else {
			sync_state_ = SyncState::Initial;
		}
	}

	// One-Step
	void PtpDelayState::on_request_sent(const Time& dreq_send)
	{
		dreq_send_ = dreq_send;
		dreq_state_ = DreqState::DreqSent;
	}

	void PtpDelayState::on_request_answered(const Time& dreq_receive)
	{
		if(dreq_state_ == DreqState::DreqSent) {
			if(states_.is_state<pi_operational>()) {
				states_.as_state<pi_operational>().on_delay(*this, dreq_receive, dreq_send_);
			} else if(states_.is_state<estimating_drift>()) {
				states_.as_state<estimating_drift>().on_delay(*this, dreq_receive, dreq_send_);
			}
			dreq_state_ = DreqState::Initial;
		}
	}

	void PtpDelayState::reset() {
		sync_state_ = SyncState::Initial;
		dreq_state_ = DreqState::Initial;

		states_.to_state<estimating_drift>();
	}

	const Time& PtpDelayState::get_offset() const
	{
		return Time();
	}

	const Time& PtpDelayState::get_delay() const
	{
		return Time();
	}
	

	//
	// ClockServo
	//

	ClockServo::ClockServo(SystemPort& system_port)
		: system_port_(system_port), offset_count_(0), delay_count_(0)
	{
		kp_ = util::to_fixed<int32, -8>(0.5);
		kn_ = util::to_fixed<int32, -8>(0.01);
		integrator_state_ = 0;
	}
	
	ClockServo::~ClockServo()
	{
	}

	void ClockServo::reset( int32 set_value )
	{
		integrator_state_ = set_value << 22;
	}

	void ClockServo::feed(uint32 dt_nanos, int32 offset_nanos)
	{
		util::fixed_point<int32,-30> dt_fixed = ((static_cast<uint64>(dt_nanos) << 32) / 1000000000ul)>>2;	// 2.30
		auto scaled_off = util::mul_precise<int32, -8, int64>(dt_fixed, offset_nanos);
		auto atten_off  = util::mul_precise<int32, -8, int64>(scaled_off, kn_);
		integrator_state_ = util::add<int32, -8>(integrator_state_, atten_off);
		auto output_val = util::add<int32, -8>(util::mul_precise<int32, -8, int64>(kp_, offset_nanos), integrator_state_);

		if(output) {
			auto val = output_val.value >> 8;
			trace_printf(0, "Offset: %10d PI output: %10d ppb (integrator: %10d)\n", offset_nanos, val, (integrator_state_.value>>8));
			output( val );
		}
	}

}
