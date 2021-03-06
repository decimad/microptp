//          Copyright Michael Steinberg 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <microptp/config.hpp>

namespace uptp {

	constexpr FIXED_RANGE(0, 0.01, 32) Config::kn_;
	constexpr FIXED_RANGE(0, 0.1, 32)  Config::kp_;

}