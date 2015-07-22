#include <microptp/clockservo.hpp>
#include <microptp/ports/systemport.hpp>
#include <stm/trace.h>

namespace uptp {

	//
	// ClockServo
	//

	ClockServo::ClockServo(SystemPort& system_port)
		: system_port_(system_port), offset_count_(0), delay_count_(0)
	{
		// -28.60 * 4.28 = -24.88 (go to -24.56 and premultiply to save the multiplication every feed)
		kp_ = kp_.from(0.1);
		kn_ = kn_.from(0.001);
		integrator_state_ = 0;

		// manual calcs to compare lib with manual
		kn_man_ = 26843545;	        // 4.28
		kp_man_ = 268435;           // 4.28
		integrator_state_man_ = 0;	// 24.8
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
		using namespace fix;

		// integrator_state_' = integrator_state_ + dt * kn_ * offset_nanos
		// [output] = [ppb].
		// => [kn_] = [ppb / offset_nanos]
		// => [kp_] = [ppb / offset_nanos / s]

		// manual calcs to compare lib with manual
		constexpr auto seconds_factor_man = 1152921504;							                         // -28.60
		const auto dt_fixed_man       = (uint32)((uint64(dt_nanos)*seconds_factor_man)>>32);	         // 32.0  * -28.60 =  4.60 (>> 32 =  4.28)
		const auto dtfm_kn            = (uint32)((uint64(dt_fixed_man)*kn_man_)>>30);                    //  4.28 *   4.28 = 16.56 (>> 30 = 16.16)
		const auto delta_int_man      = (uint32)((uint64(dtfm_kn) * offset_nanos) >> 8);                 // 16.16 *  32.0  = 48.16 (>> 16 = 32.0)
		const auto prop_man           = static_cast<int32>((int64(kp_man_) * int64(offset_nanos))>>28);  // 4.28 * 32.0 = 36.28 (>>28 = 36.0 and it will fit...)
		integrator_state_man_         = integrator_state_man_ + delta_int_man;
		const auto result_man         = prop_man + (integrator_state_man_>>8);

		constexpr auto seconds_factor = ufixed<-28, 60>::from(1.e-9);
		const auto dt_fixed           = mul<fits< 2,30>>(ufixed<32,0>(dt_nanos), seconds_factor);	// convert to seconds (we can scale kn_ accordingly beforehands, btw)
		const auto temp               = mul<fits<22,10>>(dt_fixed, sfixed<32,0>(offset_nanos));
		const auto delta_int          = mul<fits<22,10>>(temp, kn_);
		integrator_state_             = add<fits<28, 4>>(delta_int, integrator_state_);
		const auto proportional       = mul<fits<22,10>>(kp_, sfixed<32,0>(offset_nanos));
		const auto result             = add<>(proportional, integrator_state_);

		if(output) {
			output( result.to<int>() );
		}

		//util::fixed_point<int32, -30> dt_fixed = ((static_cast<int64>(dt_nanos) << 30) / 1000000000ul);


//		auto scaled_off = util::fixed_point<int32, -10>((static_cast<int64>(dt_fixed.value)*static_cast<int64>(offset_nanos))>>20);
//		auto scaled_off                        = util::mul_precise<int32, -16, int64>(dt_fixed, offset_nanos);
//		int scaled_off_value = util::to<int32>(scaled_off);
//		auto atten_off                         = util::mul_precise<int32,  -8, int64>(scaled_off, kn_);
//		int delta_integrator = util::to<int32>(atten_off);
//		integrator_state_                      = util::add        <int32,  -8>       (integrator_state_, atten_off);
//		auto proportional                      = util::mul_precise<int32,  -8, int64>(kp_, offset_nanos);
//		auto value                             = util::to<int32>(util::add<int32, -8>(proportional, integrator_state_));

//		if(output) {
#ifdef MICROPTP_DIAGNOSTICS
			trace_printf(0, "Offset: %10d ns ahead. PI output (speeding up by): %10d ppb + (integrator: %10d) (scaled_off: %10d, delta_int: %10d)\n", offset_nanos, value-util::to<int32>(integrator_state_), util::to<int32>(integrator_state_), scaled_off_value, delta_integrator );
#endif
//			output( value );
//		}
	}

}
