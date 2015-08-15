//          Copyright Michael Steinberg 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

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




