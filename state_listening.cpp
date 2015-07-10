#include <microptp/ptpclock.hpp>
//#include <microptp/ports/systemport.hpp>

namespace uptp {

	namespace states {

		Listening::Listening(PtpClock& clock)
		: clock_(clock)
		{
			clock_.master_tracker().best_master_changed = util::function<void()>(this, &Listening::on_best_master_changed);
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
