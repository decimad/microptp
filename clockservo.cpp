#include <microptp/clockservo.hpp>
#include <microptp/util/fixedpoint.hpp>
#include <microptp/ports/systemport.hpp>
#include <stm/trace.h>

namespace uptp {

	//
	// ClockServo
	//

	ClockServo::ClockServo(SystemPort& system_port)
		: system_port_(system_port), offset_count_(0), delay_count_(0)
	{
		kp_ = util::to_fixed<int32, -22>(0.1);
		kn_ = util::to_fixed<int32, -22>(0.001);
		integrator_state_ = 0;
	}
	
	ClockServo::~ClockServo()
	{
	}

	void ClockServo::reset( int32 set_value )
	{
		integrator_state_ = set_value << 8;
	}

	void ClockServo::feed(uint32 dt_nanos, int32 offset_nanos)
	{
		// integrator_state_' = integrator_state_ + dt * kn_ * offset_nanos
		// [output] = [ppb].
		// => [kn_] = [ppb / offset_nanos]
		// => [kp_] = [ppb / offset_nanos / s]

		util::fixed_point<int32, -30> dt_fixed = ((static_cast<int64>(dt_nanos) << 30) / 1000000000ul);
		auto scaled_off = util::fixed_point<int32, -10>((static_cast<int64>(dt_fixed.value)*static_cast<int64>(offset_nanos))>>20);
//		auto scaled_off                        = util::mul_precise<int32, -16, int64>(dt_fixed, offset_nanos);
		int scaled_off_value = util::to<int32>(scaled_off);
		auto atten_off                         = util::mul_precise<int32,  -8, int64>(scaled_off, kn_);
		int delta_integrator = util::to<int32>(atten_off);
		integrator_state_                      = util::add        <int32,  -8>       (integrator_state_, atten_off);
		auto proportional                      = util::mul_precise<int32,  -8, int64>(kp_, offset_nanos);
		auto value                             = util::to<int32>(util::add<int32, -8>(proportional, integrator_state_));

		if(output) {
#ifdef MICROPTP_DIAGNOSTICS
			trace_printf(0, "Offset: %10d ns ahead. PI output (speeding up by): %10d ppb + (integrator: %10d) (scaled_off: %10d, delta_int: %10d)\n", offset_nanos, value-util::to<int32>(integrator_state_), util::to<int32>(integrator_state_), scaled_off_value, delta_integrator );
#endif
			output( value );
		}
	}

}
