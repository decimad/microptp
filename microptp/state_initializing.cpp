//          Copyright Michael Steinberg 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <microptp/ptpclock.hpp>

namespace uptp {

	namespace states {

		Initializing::Initializing( PtpClock& clock )
			: clock_(clock)
		{
			clock_.to_state<Disabled>();
		}

		Initializing::~Initializing()
		{
		}

		void Initializing::on_message(const msg::Header& header, PacketHandle packet_handle)
		{
			(void) header;
			(void) packet_handle;
		}

	}

}
