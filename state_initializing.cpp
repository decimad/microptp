#include <microptp/ptpclock.hpp>
//#include <microptp/ports/systemport.hpp>

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
		}

	}

}
