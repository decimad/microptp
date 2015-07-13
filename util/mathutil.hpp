#ifndef UTIL_MATHUTIL_HPP__
#define UTIL_MATHUTIL_HPP__

#include <limits>

namespace util {

	using largest_unsigned_type = unsigned long long;
	using largest_signed_type = long long;

	template<typename T>
	constexpr std::enable_if_t<std::is_integral<T>::value, T> exp2(unsigned int exponent)
	{
		return 1 << exponent;
	}

	template<typename T>
	constexpr std::enable_if_t<std::is_floating_point<T>::value, T> exp2(unsigned int exponent)
	{
		return (exponent==0) ? 1 : (exp2<T>(exponent-1) * 2);
	}
	
	template< typename T >
	constexpr T shifted(T value, int amount)
	{
		return (amount >= 0) ? (value << amount) : (value >> (-amount));
	}
	
	template< typename T >
	constexpr T sign(T val)
	{
		return (val > 0) ? 1 : ((val < 0) ? -1 : 0);
	}


	template< typename T >
	constexpr std::enable_if_t<!std::is_integral<T>::value, T> 
		scaled_exp2(T value, int exponent)
	{
		return (exponent >= 0) ? (value * exp2<T>(exponent)) : (value / exp2<T>(-exponent));
	}

	template< typename T >
	constexpr std::enable_if_t<std::is_integral<T>::value, T>
		scaled_exp2(T value, int exponent)
	{
		return (exponent >= 0) ? 
					(value << exponent)
					: 
						((value & (1<<(-exponent-1))) ? // rounding
							((value >> -exponent)+1) : (value >> -exponent)	
						);
	}


	template< typename T>
	constexpr std::enable_if_t<std::is_integral<T>::value, unsigned int>
		log2_floor(T value)
	{
		return (value <= 1) ? 0 : (1 + log2_floor(value >> 1));
	}

	template<typename T>
	constexpr std::enable_if_t<std::is_integral<T>::value, unsigned int>
		log2_ceil(T value)
	{
		return
			(value <= 1) ? 0 :
			(((value & 1) && (value > 1)) ? (2 + log2_floor(value >> 1)) : (1 + log2_ceil(value >> 1)));
	}

	template< typename T >
	constexpr T floor(T value)
	{
		return
			static_cast<T>(
				(value - largest_signed_type(value) == 0) ? largest_signed_type(value) :
				((value >= 0) ? largest_signed_type(value) : largest_signed_type(value) - 1)
				);
	}

	template< typename T >
	constexpr T ceil(T value)
	{
		return static_cast<T>(
			(value - largest_signed_type(value) == 0) ? int(value) :
			((value >= 0) ? (largest_signed_type(value) + 1) : largest_signed_type(value))
			);
	}

	template<typename T>
	constexpr std::enable_if_t<std::is_floating_point<T>::value, unsigned int>
		log2_floor(T value)
	{
		return log2_floor(largest_unsigned_type(floor(value)));
	}

	template< typename T >
	constexpr std::enable_if_t<std::is_integral<T>::value, T> round(T value)
	{
		return value;
	}

	template< typename T >
	constexpr std::enable_if_t<std::is_floating_point<T>::value, T> round(T value)
	{
		return static_cast<T>(static_cast<largest_signed_type>(value+sign(value)*T(0.5)));
	}

	template<typename T>
	constexpr std::enable_if_t<std::is_floating_point<T>::value, unsigned int>
		log2_ceil(T value)
	{
		return log2_ceil(largest_unsigned_type(ceil(value)));
	}

	template< typename T >
	constexpr T clamp(T value, T min, T max)
	{
		return (value > max) ? max : ((value < min) ? min : value);
	}
	
	template<typename T>
	constexpr std::enable_if_t< std::is_floating_point<T>::value || std::is_signed<T>::value, T>
		abs(T value)
	{
		return (value >= 0) ? value : -value;
	}

	template<typename T>
	constexpr std::enable_if_t< std::is_integral<T>::value && !std::is_signed<T>::value, T>
		abs(T value)
	{
		return value;
	}

	template<typename T>
	constexpr std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value, T> 
		safe_abs(T value)
	{
		return abs<T>(clamp<T>(value, -std::numeric_limits<T>::max(), std::numeric_limits<T>::max()));
	}

	template<typename T>
	constexpr std::enable_if_t<std::is_unsigned<T>::value || std::is_floating_point<T>::value, T> 
		safe_abs(T value)
	{
		return abs(value);
	}

	template<typename T>
	constexpr T trunc(T value)
	{
		return static_cast<T>(static_cast<largest_signed_type>((value)));
	}

	template<typename T>
	constexpr bool is_power_of_2(T value)
	{
		return exp2<T>(log2_floor(safe_abs(value))) == safe_abs(value);
	}

	template<typename T>
	constexpr T max(T lhs, T rhs)
	{
		return (lhs > rhs) ? lhs : rhs;
	}

	template<typename T>
	constexpr T min(T lhs, T rhs)
	{
		return (lhs > rhs) ? rhs : lhs;
	}

	constexpr int binary_digits(double decimal)
	{
		return static_cast<int>(ceil(3.3219280948873623478703194294894 * decimal));
	}

}

#endif
