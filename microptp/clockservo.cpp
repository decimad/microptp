//          Copyright Michael Steinberg 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <microptp/config.hpp>
#include <microptp/clockservo.hpp>
#include <microptp/ports/systemport.hpp>

namespace uptp {

	//
	// ClockServo
	//

	ClockServo::ClockServo(PtpClock& clock)
		: clock_(clock)
	{
		integrator_state_ = 0;
	}
	
	ClockServo::~ClockServo()
	{
	}

	void ClockServo::reset( int32 set_value )
	{
		integrator_state_ = integrator_state_.from(set_value);
	}

	void ClockServo::feed(uint32 dt_nanos, int32 offset_nanos)
	{
		// we're tolerating 1000 usecs offset before going back to synch state.
		constexpr auto seconds_factor = FIXED_CONSTANT(1.e-9, 32);
		using dt_nanos_type  = FIXED_RANGE_I(-00000000,2200000000);
		using off_nanos_type = FIXED_RANGE_I(-20000000,20000000);

		// integrator_state_' = integrator_state_ + dt * kn_ * offset_nanos
		// [output] = [ppb].
		// => [kn_] = [ppb / offset_nanos]
		// => [kp_] = [ppb / offset_nanos / s]

		integrator_state_ += seconds_factor * dt_nanos_type(dt_nanos) * off_nanos_type(offset_nanos) * clock_.get_config().kn_;
		const auto proportional = off_nanos_type(offset_nanos) * clock_.get_config().kp_;
		const auto result       = proportional + integrator_state_;

		if(output) {
			TRACE("Offset: %10d ns ahead. PI output (speeding up by): %10d ppb + (integrator: %10d)\n", offset_nanos, result.to<int32>()-integrator_state_.to<int32>(), integrator_state_.to<int32>());
			output( result.to<int>() );
		}
	}

}
