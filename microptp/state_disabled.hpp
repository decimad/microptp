//          Copyright Michael Steinberg 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SYSTEM_MICROPTP_STATE_DISABLED_HPP_
#define SYSTEM_MICROPTP_STATE_DISABLED_HPP_

#include <microptp/state_base.hpp>

namespace uptp {

	class PtpClock;

namespace states {

	class Disabled : public PtpStateBase
	{
	public:
		Disabled(PtpClock& clock);

		void on_message(const msg::Header&, PacketHandle) override;

	};

} }



#endif /* SYSTEM_MICROPTP_STATE_DISABLED_HPP_ */
