#ifndef MICROPTP_CLOCKSERVO_HPP__
#define MICROPTP_CLOCKSERVO_HPP__

#pragma once
#include <microptp/uptp.hpp>
#include <microptp/ptpdatatypes.hpp>
#include <microlib/functional.hpp>
#include <microlib/circular_buffer.hpp>
#include <microlib/statemachine.hpp>
#include <fixed/fixed.hpp>

namespace uptp {
	
	class ClockServo {
	public:
		ClockServo(SystemPort& clock);
		~ClockServo();

		void reset( int32 integrator );
		void feed( uint32 dt, int32 offset );

		util::function<void(int32)> output;

	private:
		FIXED_RANGE(-10000000, 10000000, 32) integrator_state_;
		FIXED_RANGE(0, 0.01, 32) kn_;
		FIXED_RANGE(0, 0.1, 32) kp_;

		SystemPort& system_port_;
	};
	
}

#endif
