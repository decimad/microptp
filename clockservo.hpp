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

		util::fixed_point<int32,-22> kn_;
		util::fixed_point<int32,-22> kp_;

		SystemPort& system_port_;
		uint8 offset_count_;
		uint8 delay_count_;
	};
	
}

#endif
