//          Copyright Michael Steinberg 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SYSTEM_MICROPTP_MICROPTP_CONFIG_HPP_
#define SYSTEM_MICROPTP_MICROPTP_CONFIG_HPP_

#include <microptp_config.hpp>
#include <microptp/ptpdatatypes.hpp>
#include <fixed/fixed.hpp>

#ifndef PRINT
#define PRINT(...)
#endif

#ifndef TRACE
#define TRACE(...)
#endif

#ifndef UPTP_SLAVE_ONLY
#define UPTP_SLAVE_ONLY 1
#endif

namespace uptp {

	struct Config {
#if !UPTP_SLAVE_ONLY
		ClockQuality clock_quality;
#endif
		static const bool two_step = true;
		static const bool any_domain = false;
		static const uint8 preferred_domain = 0;

		static constexpr auto kp_ = FIXED_RANGE(0, 0.1, 32)::from(0.005);
		static constexpr auto kn_ = FIXED_RANGE(0, 0.01, 32)::from(0.0005);
	};

}

#endif /* SYSTEM_MICROPTP_MICROPTP_CONFIG_HPP_ */
