#include <microptp/ports/systemport.hpp>

namespace uptp {

	namespace system {

#ifndef UPTP_NO_DEFAULT_BYTEORDER
#if UPTP_HOST_BIG_ENDIAN
		uint8 to_host_byteorder(uint8 val)
		{
			return val;
		}

		uint16 to_host_byteorder(uint16 val)
		{
			return val;
		}

		uint32 to_host_byteorder(uint32 val)
		{
			return val;
		}

		uint64 to_host_byteorder(uint64 val)
		{
			return val;
		}

		int8 to_host_byteorder(int8 val)
		{
			return val;
		}

		int16 to_host_byteorder(int16 val)
		{
			return val;
		}

		int32 to_host_byteorder(int32 val)
		{
			return val;
		}

		int64 to_host_byteorder(int64 val)
		{
			return val;
		}

		uint8 to_net_byteorder(uint8 val)
		{
			return val;
		}

		uint16 to_net_byteorder(uint16 val)
		{
			return val;
		}

		uint32 to_net_byteorder(uint32 val)
		{
			return val;
		}

		uint64 to_net_byteorder(uint64 val)
		{
			return val;
		}

		int8 to_net_byteorder(int8 val)
		{
			return val;
		}

		int16 to_net_byteorder(int16 val)
		{
			return val;
		}

		int32 to_net_byteorder(int32 val)
		{
			return val;
		}

		int64 to_net_byteorder(int64 val)
		{
			return val;
		}

#else
		uint8 to_host_byteorder(uint8 val)
		{
			return val;
		}

		uint16 to_host_byteorder(uint16 val)
		{
			return (val >> 8) | (val << 8);
		}

		uint32 to_host_byteorder(uint32 val)
		{
			return (static_cast<uint32>(to_host_byteorder(static_cast<uint16>(val)))<<16) | to_host_byteorder(static_cast<uint16>(val >> 16));
		}

		uint64 to_host_byteorder(uint64 val)
		{
			return (static_cast<uint64>(to_host_byteorder(static_cast<uint32>(val)))<<32) | to_host_byteorder(static_cast<uint32>(val >> 32));
		}

		int8 to_host_byteorder(int8 val)
		{
			return val;
		}

		int16 to_host_byteorder(int16 val)
		{
			return (val >> 8) | (val << 8);
		}

		int32 to_host_byteorder(int32 val)
		{
			return (static_cast<int32>(to_host_byteorder(static_cast<int16>(val)))<<16) | to_host_byteorder(static_cast<int16>(val >> 16));
		}

		int64 to_host_byteorder(int64 val)
		{
			return (static_cast<uint64>(to_host_byteorder(static_cast<int32>(val)))<<32) | to_host_byteorder(static_cast<int32>(val >> 32));
		}

		uint8 to_net_byteorder(uint8 val)
		{
			return to_host_byteorder(val);
		}
		
		uint16 to_net_byteorder(uint16 val)
		{
			return to_host_byteorder(val);
		}
		
		uint32 to_net_byteorder(uint32 val)
		{
			return to_host_byteorder(val);
		}

		uint64 to_net_byteorder(uint64 val)
		{
			return to_host_byteorder(val);
		}

		int8 to_net_byteorder(int8 val) {
			return to_host_byteorder(val);
		}

		int16 to_net_byteorder(int16 val)
		{
			return to_host_byteorder(val);
		}

		int32 to_net_byteorder(int32 val)
		{
			return to_host_byteorder(val);
		}

		int64 to_net_byteorder(int64 val)
		{
			return to_host_byteorder(val);
		}
#endif
#endif

	}

}
