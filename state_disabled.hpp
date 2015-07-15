/*
 * state_disabled.hpp
 *
 *  Created on: 14.07.2015
 *      Author: Michael
 */

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
