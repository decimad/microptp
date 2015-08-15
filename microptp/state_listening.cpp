//          Copyright Michael Steinberg 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <microptp/ptpclock.hpp>

namespace uptp {

	namespace states {

		Listening::Listening(PtpClock& clock)
		: clock_(clock)
		{
			if(clock_.master_tracker().best_foreign()) {
				on_best_master_changed();
			} else {
				clock_.master_tracker().best_master_changed = ulib::function<void()>(this, &Listening::on_best_master_changed);
			}
		}

		void Listening::on_message(const msg::Header&, PacketHandle)
		{
		}

		void Listening::on_best_master_changed()
		{
			clock_.to_state<Slave>();
		}

		Listening::~Listening()
		{
			clock_.master_tracker().best_master_changed.reset();
		}

	}

}
