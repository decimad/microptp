#ifndef MICROPTP_PORTS_SYSTEMPORT_DEFAULTS_HPP__
#define MICROPTP_PORTS_SYSTEMPORT_DEFAULTS_HPP__

#include <microptp/types.hpp>
#include <microptp/ports/cortex_m4/cortex_m4_util.hpp>

namespace uptp {

	namespace system {

		// If you replace the standard implementations with system
		// specific ops, define UPTP_NO_DEFAULT_BYTEORDER

		uint8 to_host_byteorder(uint8);
		uint16 to_host_byteorder(uint16);
		uint32 to_host_byteorder(uint32);
		uint64 to_host_byteorder(uint64);

		int8 to_host_byteorder(int8);
		int16 to_host_byteorder(int16);
		int32 to_host_byteorder(int32);
		int64 to_host_byteorder(int64);

		uint8 to_net_byteorder(uint8);
		uint16 to_net_byteorder(uint16);
		uint32 to_net_byteorder(uint32);
		uint64 to_net_byteorder(uint64);

		int8 to_net_byteorder(int8);
		int16 to_net_byteorder(int16);
		int32 to_net_byteorder(int32);
		int64 to_net_byteorder(int64);

	}

}

#endif
