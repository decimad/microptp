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
		kp_ = util::to_fixed<int32, -8>(0.5);
		kn_ = util::to_fixed<int32, -8>(0.01);
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
