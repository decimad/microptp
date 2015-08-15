//          Copyright Michael Steinberg 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MICROPTP_CLOCKSERVO_HPP__
#define MICROPTP_CLOCKSERVO_HPP__

#pragma once
#include <microptp/config.hpp>
#include <microlib/functional.hpp>
#include <fixed/fixed.hpp>

namespace uptp {
	
	class SystemPort;

	class ClockServo {
	public:
		ClockServo(SystemPort& clock);
		~ClockServo();

		void reset( int32 integrator );
		void feed( uint32 dt, int32 offset );

		ulib::function<void(int32)> output;

	private:
		FIXED_RANGE(-10000000, 10000000, 32) integrator_state_;
		FIXED_RANGE(0, 0.01, 32) kn_;
		FIXED_RANGE(0, 0.1, 32) kp_;

		SystemPort& system_port_;
	};
	
}

#endif
