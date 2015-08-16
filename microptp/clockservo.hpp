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
	
	class PtpClock;

	class ClockServo {
	public:
		ClockServo(PtpClock& clock);
		~ClockServo();

		void reset( int32 integrator );
		void feed( uint32 dt, int32 offset );

		ulib::function<void(int32)> output;

	private:
		FIXED_RANGE(-10000000, 10000000, 32) integrator_state_;
		PtpClock& clock_;
	};
	
}

#endif
