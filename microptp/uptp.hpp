#ifndef MICROPTP_UPTP_HPP__
#define MICROPTP_UPTP_HPP__

#include <microptp/ptpdatatypes.hpp>

//#define MICROPTP_DIAGNOSTICS

namespace uptp {

	class SystemPort;
	class PacketHandle;

	struct Config {
#ifndef UPTP_SLAVE_ONLY
		ClockQuality clock_quality;
#endif
		static const bool two_step = true;
		static const bool any_domain = false;
		static const uint8 preferred_domain = 0;

		std::array<uint8, 6> mac_addr;
	};

}

#endif
