//          Copyright Michael Steinberg 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MICROPTP_PTPDATATYPES_HPP__
#define MICROPTP_PTPDATATYPES_HPP__

#include <array>
#include <microptp/types.hpp>

namespace uptp {

	namespace detail {

		struct UInt48 {
			uint32 upper;
			uint16 lower;
		};

	}

	using enum8 = uint8;

	using uint48 = detail::UInt48;

	struct PtpTime {
		uint48 secondsField;
		uint32 nanosecondsField;
	};
	
	struct Time {
		Time();
		Time(uint32 msecs);
		Time(uint64 logical);
		Time(int64 secs, int32 nanos);
		Time(const Time&);

		void clear();
		void normalize();
		int64 to_nanos() const;

		int64 secs_;
		int32 nanos_;
	};

	Time operator-(const Time&, const Time&);
	Time operator/(const Time&, int32);
	Time operator+(const Time&, const Time&);
	Time operator-(const Time&);

	bool operator<(const Time&, const Time&);
	bool operator>(const Time&, const Time&);
	bool operator<=(const Time&, const Time&);
	bool operator>=(const Time&, const Time&);
	bool operator==(const Time&, const Time&);

	struct ClockQuality {
		uint16 offset_scaled_log_variance;
		uint8 clock_class;
		enum8 clock_accuracy;
	};

	bool operator<(const ClockQuality&, const ClockQuality&);
	bool operator==(const ClockQuality&, const ClockQuality&);
	bool operator>(const ClockQuality&, const ClockQuality&);
	bool operator<=(const ClockQuality&, const ClockQuality&);
	bool operator>=(const ClockQuality&, const ClockQuality&);
	bool operator!=(const ClockQuality&, const ClockQuality&);
	
	struct ClockIdentity {
		ClockIdentity()
		{
		}

		ClockIdentity(const std::array<uint8, 6>& mac_addr)
		{
			update(mac_addr);
		}

		void update(const std::array<uint8, 6>& mac_addr)
		{
			std::copy(mac_addr.begin(), mac_addr.begin() + 3, identity.begin());
			identity[3] = 0xFF;
			identity[4] = 0xFE;
			std::copy(mac_addr.begin() + 3, mac_addr.end(), identity.begin() + 5);
		}

		uint8& operator[](uint8 index)
		{
			return identity[index];
		}

		std::array<uint8, 8> identity;
	};

	bool operator<(const ClockIdentity&, const ClockIdentity&);
	bool operator==(const ClockIdentity&, const ClockIdentity&);
	bool operator>(const ClockIdentity&, const ClockIdentity&);
	bool operator<=(const ClockIdentity&, const ClockIdentity&);
	bool operator>=(const ClockIdentity&, const ClockIdentity&);
	bool operator!=(const ClockIdentity&, const ClockIdentity&);

	struct PortIdentity {
		PortIdentity()
		{}

		PortIdentity(const std::array<uint8, 6>& mac_addr, uint16 port_)
			: clock(mac_addr), port(port_)
		{}

		ClockIdentity clock;
		uint16 port;
	};

	bool operator<(const PortIdentity&, const PortIdentity&);
	bool operator==(const PortIdentity&, const PortIdentity&);
	bool operator>(const PortIdentity&, const PortIdentity&);
	bool operator<=(const PortIdentity&, const PortIdentity&);
	bool operator>=(const PortIdentity&, const PortIdentity&);
	bool operator!=(const PortIdentity&, const PortIdentity&);
	
	struct ClockData {
		ClockIdentity identity_;
		ClockQuality quality_;

	};

	struct UtcOffset {
		uint16 offset_;
		bool valid_;
	};


}

#endif
