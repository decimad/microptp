#include <microptp/ptpdatatypes.hpp>
#include <tuple>
#include <cstring> // need memcmp

namespace uptp {

	Time::Time()
	{}

	Time::Time(uint64 logical)
		: secs_(logical >> 32), nanos_(logical & 0xFFFFFFFF)
	{}

	Time::Time(uint32 msecs)
		: secs_(msecs / 1000), nanos_((msecs % 1000) * 1000 * 1000)
	{}

	Time::Time(int64 secs, int32 nanos)
		: secs_(secs), nanos_(nanos)
	{}

	Time::Time(const Time& other)
		: secs_(other.secs_), nanos_(other.nanos_)
	{
	}

	void Time::normalize()
	{
		if (nanos_ <= -1000000000) {
			nanos_ += -1000000000;
			--secs_;
		} else if (nanos_ > 1000000000) {
			nanos_ -= 1000000000;
			++secs_;
		}

		if(secs_ > 0 && nanos_ < 0) {
			--secs_;
			nanos_ = 1000000000 + nanos_;
		}

		if(secs_ < 0 && nanos_ > 0) {
			++secs_;
			nanos_ = -1000000000 + nanos_;
		}
	}

	Time operator-(const Time& a, const Time& b) {
		Time result;
		result.secs_ = a.secs_ - b.secs_;
		result.nanos_ = a.nanos_ - b.nanos_;
		result.normalize();
		return result;
	}

	Time operator+(const Time& a, const Time& b)
	{
		Time result;
		result.secs_ = a.secs_ + b.secs_;
		result.nanos_ = a.nanos_ + b.nanos_;
		result.normalize();
		return result;
	}

	Time operator/(const Time& time, int divisor)
	{
		Time result;
		result.nanos_ = time.nanos_ / divisor;
		result.nanos_ += 1000000000l * (time.secs_ % divisor) / divisor;
		result.secs_ = time.secs_ / divisor;
		return result;
	}

	Time operator-(const Time& time)
	{
		return Time(-time.secs_, -time.nanos_);
	}

	int64 Time::to_nanos() const
	{
		return 1000000000ll*secs_ + nanos_;
	}

	void Time::clear()
	{
		secs_ = 0;
		nanos_ = 0;
	}

	bool operator<(const Time& a, const Time& b)
	{
		if (a.secs_ < b.secs_) {
			return true;
		} else if (a.secs_ == b.secs_) {
			if (a.secs_ < 0) return a.nanos_ > b.nanos_;
			else return a.nanos_ < b.nanos_;
		} else {
			return false;
		}
	}

	bool operator>(const Time& a, const Time& b) {
		if (a.secs_ > b.secs_) {
			return true;
		} else if (a.secs_ == b.secs_) {
			if (a.secs_ < 0) return b.nanos_ > a.nanos_;
			else return a.nanos_ > b.nanos_;
		} else {
			return false;
		}
	}

	bool operator<=(const Time& a, const Time& b) {
		if (a.secs_ < b.secs_) {
			return true;
		} else if (a.secs_ == b.secs_) {
			if (a.secs_ < 0) return a.nanos_ >= b.nanos_;
			else return a.nanos_ <= b.nanos_;
		} else {
			return false;
		}
	}

	bool operator>=(const Time& a, const Time& b) {
		if (a.secs_ > b.secs_) {
			return true;
		} else if (a.secs_ == b.secs_) {
			if (a.secs_ < 0) return a.nanos_ <= b.nanos_;
			else return a.nanos_ >= b.nanos_;
		} else {
			return false;
		}
	}

	bool operator==(const Time& a, const Time& b) {
		return a.secs_ == b.secs_ && a.nanos_ == b.nanos_;
	}

	//
	// ClockQuality
	//
	bool operator<(const ClockQuality& lhs, const ClockQuality& rhs)
	{
		return std::tie(lhs.clock_class, lhs.clock_accuracy, lhs.offset_scaled_log_variance)
			< std::tie(rhs.clock_class, rhs.clock_accuracy, rhs.offset_scaled_log_variance);
	}

	bool operator==(const ClockQuality& lhs, const ClockQuality& rhs)
	{
		return std::tie(lhs.clock_class, lhs.clock_accuracy, lhs.offset_scaled_log_variance)
			== std::tie(rhs.clock_class, rhs.clock_accuracy, rhs.offset_scaled_log_variance);
	}

	bool operator>(const ClockQuality& lhs, const ClockQuality& rhs)
	{
		return rhs < lhs;
	}

	bool operator<=(const ClockQuality& lhs, const ClockQuality& rhs)
	{
		return (lhs < rhs) || (lhs == rhs);
	}

	bool operator>=(const ClockQuality& lhs, const ClockQuality& rhs)
	{
		return rhs <= lhs;
	}

	bool operator!=(const ClockQuality& lhs, const ClockQuality& rhs)
	{
		return !(lhs == rhs);
	}

	//
	// ClockIdentity
	//
	bool operator<(const ClockIdentity& lhs, const ClockIdentity& rhs)
	{
		return memcmp(&lhs.identity, &lhs.identity + 8, 8) == -1;
	}

	bool operator==(const ClockIdentity& lhs, const ClockIdentity& rhs)
	{
		memcmp(&lhs.identity, &lhs.identity + 8, 8) == 0;
	}

	bool operator>(const ClockIdentity& lhs, const ClockIdentity& rhs)
	{
		return rhs < lhs;
	}

	bool operator<=(const ClockIdentity& lhs, const ClockIdentity& rhs)
	{
		int ret = memcmp(&lhs.identity, &lhs.identity + 8, 8);
		return (ret == -1) || (ret == 0);
	}

	bool operator>=(const ClockIdentity& lhs, const ClockIdentity& rhs)
	{
		return rhs <= lhs;
	}

	bool operator!=(const ClockIdentity& lhs, const ClockIdentity& rhs)
	{
		return !(lhs == rhs);
	}

	//
	// PortIdentity
	//
	bool operator<(const PortIdentity& lhs, const PortIdentity& rhs)
	{
		return std::tie(lhs.clock, lhs.port) < std::tie(rhs.clock, rhs.port);
	}

	bool operator==(const PortIdentity& lhs, const PortIdentity& rhs)
	{
		return std::tie(lhs.clock, lhs.port) == std::tie(rhs.clock, rhs.port);
	}

	bool operator>(const PortIdentity& lhs, const PortIdentity& rhs)
	{
		return rhs < lhs;
	}

	bool operator<=(const PortIdentity& lhs, const PortIdentity& rhs)
	{
		return (lhs < rhs) || (lhs == rhs);
	}

	bool operator>=(const PortIdentity& lhs, const PortIdentity& rhs)
	{
		return rhs <= lhs;
	}

	bool operator!=(const PortIdentity& lhs, const PortIdentity& rhs)
	{
		return !(lhs == rhs);
	}

}
