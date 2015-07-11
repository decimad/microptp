#ifndef MICROPTP_UTIL_FIXEDPOINT_HPP__
#define MICROPTP_UTIL_FIXEDPOINT_HPP__

#include <limits>
#include <microptp/util/util.hpp>

namespace util {

	template< typename T, unsigned int B, int E >
	struct fixed_desc {
		using value_type = T;
		static const int exp  = E;
		static const int base = B;
	};

	// Returns absolute value (remember that this might convert -128 back to -128 in a signed char f.e...)
	template< typename T >
	constexpr T abs(T value)
	{
		return (value < 0) ? (-value) : (value);
	}
	
	namespace detail {
		constexpr int log_bigger(double value, double base) {
			return (value > 1) ? (1 + log_bigger(value / base, base)) : 0;
		}

		constexpr int log_smaller(double value, double base) {
			return (value < 1) ? (-1 + log_smaller(value * base, base)) : 0;
		}

	}

	constexpr int log(double value, double base) {
		return (value > 1) ? detail::log_bigger(value, base) : detail::log_smaller(value, base);
	}

	// Clamp value to interval [min, max]
	template<typename T>
	constexpr T clamp(T value, T min, T max)
	{
		return (value > max) ? max : ((value < min) ? min : value);
	}

	// clamps value to the data range for which abs(value) is safe (consider -128 in a signed char).
	template<typename T>
	constexpr std::enable_if_t<std::is_signed<T>::value, T> safe_clamp(T value)
	{
		return (value == std::numeric_limits<T>::min()) ? (std::numeric_limits<T>::min() + 1) : value;
	}

	template<typename T>
	constexpr std::enable_if_t<!std::is_signed<T>::value, T> safe_clamp(T value)
	{
		return value;
	}

	//
	// exp
	//
	template< typename T >
	constexpr T exp(unsigned int base, unsigned int exponent)
	{
		return (exponent == 0) ? 1 : (T(base) * (exp<T>(base, exponent - 1)));
	}

	//
	// safe_abs
	// Returns absolute value, respecting that -max is not representable... might wanna clamp to -max+1 really.	
	template< typename T >
	constexpr T safe_abs(T val) {
		return abs(safe_clamp(val));
	}
	
	//
	// scale
	// 

	namespace detail {
		template< typename DestType, typename T >
		constexpr DestType scale_up(T value, unsigned int base, unsigned int exponent)
		{
			return static_cast<DestType>(value * exp<T>(base, exponent));
		}

		template< typename DestType, typename T >
		constexpr DestType scale_down(T value, unsigned int base, unsigned int exponent)
		{
			return static_cast<DestType>((value + exp<T>(base, exponent) / 2) / exp<T>(base, exponent));
		}
	}
		
	// Result = Value * Base^Exponent (Uses rounding, if Exponent < 0)
	template< typename DestType, typename T >
	constexpr DestType scale(T value, int base, int exponent)
	{
		return (exponent >= 0) ? detail::scale_up<DestType>(value, base, exponent) : detail::scale_down<DestType>(value, base, safe_abs(exponent));
	}
	
	// Fixed-Point Number. Real value = num.value * B^Exponent
	template< typename T, int Exponent, unsigned int B = 2 >
	struct fixed_point {
		constexpr fixed_point()
			: value(0)
		{}

		constexpr fixed_point(T value_)
			: value(value_)
		{}

		fixed_point& operator=(const fixed_point& other)
		{
			value = other.value;
			return *this;
		}


		T value;
	};

	
	template< typename DestType, int Z, typename lhs_T, typename rhs_T, int X, int Y, unsigned int B>
	constexpr fixed_point<DestType, Z, B>
	add(fixed_point<lhs_T, X, B> lhs, fixed_point<rhs_T, Y, B> rhs ) {
		return fixed_point<DestType, Z, B>( scale<DestType>(lhs.value, B, X-Z) + scale<DestType>(rhs.value, B, Y-Z) );
	}

	template< typename DestType, int Z, typename lhs_T, typename rhs_T, int X, int Y, unsigned int B>
	constexpr fixed_point<DestType, Z>
	sub(fixed_point<lhs_T, X, B> lhs, fixed_point<rhs_T, Y, B> rhs) {
		return fixed_point<DestType, Z>( scale<DestType>(lhs.value, B, X - Z) - scale<DestType>(rhs.value, B, Y - Z));
	}
	
	template< typename ZT, int Z, int XPre = 0, int YPre = 0, typename LT, typename RT, int X, int Y, unsigned int B, typename Enable = std::enable_if_t<!std::is_integral<std::decay_t<RT>>::value>>
	constexpr fixed_point<ZT, Z, B>
	mul(fixed_point<LT, X, B> lhs, fixed_point<RT, Y, B> rhs) {
		return fixed_point<ZT, Z>(
			scale<ZT>( scale<ZT>(lhs.value, B, XPre) * scale<ZT>(rhs.value, B, YPre), B, X + Y - XPre - YPre - Z)
		);
	}

	template< typename ZT, int Z, int XPre = 0, int YPre = 0, int X, unsigned int B, typename LT, typename RT, typename Enable = std::enable_if_t<std::is_integral<std::decay_t<RT>>::value>>
	constexpr fixed_point<ZT, Z, B>
	mul(fixed_point<LT, X, B> lhs, RT&& rhs)
	{
		return fixed_point<ZT, Z>(
			scale<ZT>(scale<ZT>(lhs.value, B, XPre) * scale<ZT>(rhs, B, YPre), B, X - XPre - YPre - Z)
			);
	}

	template< typename ZT, int Z, typename IT, int X, unsigned int B, typename LT, typename RT,  typename Enable = std::enable_if_t<std::is_integral<std::decay_t<RT>>::value>>
	constexpr fixed_point<ZT, Z, B>
	mul_precise(fixed_point<LT, X, B> lhs, RT&& rhs)
	{
		return fixed_point<ZT, Z>(
			scale<ZT>(static_cast<IT>(lhs.value) * static_cast<IT>(rhs), B, X - Z)
			);
	}

	template< typename ZT, int Z, typename IT, int X, int Y, unsigned int B, typename LT, typename RT>
	constexpr fixed_point<ZT, Z, B>
		mul_precise(fixed_point<LT, X, B> lhs, fixed_point<RT, Y, B> rhs)
	{
		return fixed_point<ZT, Z>(
			scale<ZT>(static_cast<IT>(lhs.value) * static_cast<IT>(rhs.value), B, X + Y - Z)
			);
	}

	template< typename DestType, int Z, int XPre = 0, int YPre = 0, typename lhs_T, typename rhs_T, int X, int Y, unsigned int B>
	constexpr fixed_point<DestType, Z, B>
	div(fixed_point<lhs_T, X, B> lhs, fixed_point<rhs_T, Y, B> rhs)
	{
		return fixed_point<DestType, Z>(
			scale<DestType>(scale<DestType>(lhs.value, B, XPre) / scale<DestType>(rhs.value, B, YPre), B, X - Y - XPre + YPre - Z)
			);
	}
	
	
	template< typename To, typename T, int E, unsigned int B = 2 >
	constexpr To to(fixed_point<T, E, B> value)
	{
		return (E >= 0) ? To(value.value) * exp<T>(B, E) :
			To(value.value) / exp<T>(B, safe_abs(E));
	}


	//
	// to_fixed
	//
	
	namespace detail {

		template< typename T, int E, unsigned int B >
		constexpr fixed_point<T, E, B> to_fixed_positive(double val) {
			return fixed_point<T, E, B>(static_cast<T>(val / exp<T>(B, E)));
		}
		
		template<typename T, int E, unsigned int B>
		constexpr fixed_point<T,E,B> to_fixed_negative(double val) {
			return fixed_point<T,E,B>(static_cast<T>(val / (1.0 / exp<T>(B, -E))));
		}

	}
	
	template< typename T, int E, unsigned int B = 2 >
	constexpr fixed_point<T, E, B> to_fixed(double val)
	{
		return (E >= 0) ? detail::to_fixed_positive<T,E,B>(val) : detail::to_fixed_negative<T,E,B>(val);
	}

	constexpr int significant_bits_from_range(double start, double end, unsigned int base)
	{
		return log(max(abs(start), abs(end)), base);
	}

	template< typename T>
	constexpr int exponent_from_range(double start, double end, unsigned int base=2)
	{
		return -int(sizeof(T) * 8) + significant_bits_from_range(start, end, base);
	}
			
	template<typename T, int decimal, int fractional>
	using fixed = fixed_point<T, -fractional >;


}

#endif
