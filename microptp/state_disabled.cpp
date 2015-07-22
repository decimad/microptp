/*
 * state_disabled.cpp
 *
 *  Created on: 14.07.2015
 *      Author: Michael
 */

#include <microptp/state_disabled.hpp>
#include <microptp/ptpclock.hpp>
#include <microptp/ports/systemport.hpp>

namespace uptp { namespace states {

	//
	// Yup... disabled means... nothing
	//

	Disabled::Disabled(PtpClock& clock)
	{
		clock.get_system_port().discipline(0);
	}

	void Disabled::on_message(const msg::Header&, PacketHandle)
	{

	}

} }




