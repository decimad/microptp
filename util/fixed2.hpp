#ifndef UTIL_FIXED_2_HPP__
#define UTIL_FIXED_2_HPP__


#include <type_traits>
#include <utility>
#include "mathutil.hpp"

namespace util {
	
	// Bits:
	// < XXXXXXX FFFFFFFF.IIIIIIII.OOOOOOOO >

	template< typename T, int I, int F = sizeof(T)*8-I, unsigned int O = 0 >
	struct fixed {
		static constexpr int integer_bits    = I;
		static constexpr int fractional_bits = F;
		static constexpr int offset          = O;
		static constexpr int bits            = static_cast<int>(sizeof(T) * 8);
		static constexpr int free_bits       = bits - (integer_bits + fractional_bits) - offset;
		static constexpr int scaling         = -F+min(I, 0);

		static_assert(I > 0 || F > 0,    "No bits left, dude!");
		static_assert(I + F + O <= bits, "Not enough bits available.");

		static constexpr int significant_bits()
		{
			return I + F;
		}

		template<unsigned int Bit>
		static constexpr int exponent()
		{
			return -F + min(I, 0) - O + Bit;
		}

		static constexpr int max_exponent()
		{
			return exponent<O + significant_bits()>();
		}

		static constexpr int min_exponent()
		{
			return exponent<O>();
		}
		
		constexpr fixed()
			: value(0)
		{}

		constexpr fixed(T value_)
			: value(value_)
		{}

		template< int Shift >
		constexpr auto virtual_shift() {
			return fixed<T, I+Shift, F-Shift, O>(value);
		}
		
		template< int Shift >
		constexpr auto scaling_shift() {
			return fixed<T, I-max<int>(Shift-free_bits,0), F+min<int>(O+Shift,0), max<int>(O+Shift,0)>(shifted(value, Shift));
		}

		template< typename S >
		constexpr static fixed from(S value)
		{
			// Need to be more sophisticated here... detect powers of 2 and such which make negative integers or fractions possible (for constants)
			return fixed(static_cast<T>(round(scaled_exp2(value, -scaling)))<<offset);
		}

		template<typename S>
		constexpr S to()
		{
			return scaled_exp2<S>(value>>offset, scaling);
		}

		T value;
	};
	
	namespace detail {

		template<typename T, typename S>
		constexpr unsigned int integer_bits(S value)
		{
			//     simple log2 of the integer part     if signed we need a bit more         if value is positive and matches a power, we need one more again
			return log2_ceil(trunc(abs(value))) + (std::is_signed<T>::value ? 1 : 0) + ((value > 0 && is_power_of_2(trunc(value))) ? 1 : 0);
		}

		template<typename T, typename S, typename U>
		constexpr unsigned int integer_bits_interval(S s, U u)
		{
			return util::max(integer_bits<T>(s), integer_bits<T>(u));
		}

		template<typename T, typename S, typename U>
		constexpr unsigned int fractional_bits_interval(S s, U u)
		{
			return sizeof(T) * 8 - integer_bits_interval<T>(s, u);
		}

		template<typename T, typename S>
		constexpr unsigned int fractional_bits(S value)
		{
			return (std::is_integral<S>::value) ? (0) : (sizeof(T) * 8 - integer_bits<T>(value));
		}

		template<typename T >
		constexpr fixed<T, sizeof(T) * 8, 0> int_(T val)
		{
			return fixed<T, sizeof(T) * 8, 0>(val);
		}

		template< typename A, typename B >
		struct matching_shifts;

		// Find the best combination of shifts so the fractional points are aligned,
		// minimizing shift operations but only if no precision is lost
		template< typename T, int AI, int AF, unsigned int AO, int BI, int BF, unsigned int BO >
		struct matching_shifts< fixed<T, AI, AF, AO>, fixed<T, BI, BF, BO> > {
			// shift amount:    +-(AF-BF)
			//		if positive: must shift B up AF-BF, but can only shift B.free_bits, thus shift lhs down by LF-RF-rhs.free_bits
			using A = fixed<T, AI, AF, AO>;
			using B = fixed<T, BI, BF, BO>;

			constexpr static int AS = AO + AF;
			constexpr static int BS = BO + BF;

			// Goal: Try not to loose significant bits, minimize shifts, but prefer shifts or loosing bits

			//
			// A: < XXXXXXXXXX IIIIIIII.FFFFFFFF OOOOOO >   // perfect shift possible
			// B: < XXXXX IIIII.FFFFFFFFFFFFFF XXXXXXXX >   // perfect shift not possible
			//

			constexpr static bool a_perfect_match = (BS > AS) ? (A::free_bits >= BS - AS) : (A::offset >= AS - BS);
			constexpr static int perfect_shift_a = (BS - AS);

			constexpr static bool b_perfect_match = (AS > BS) ? (B::free_bits >= AS - BS) : (B::offset >= BS - AS);
			constexpr static int perfect_shift_b = (AS - BS);

			constexpr static bool combined_perfect_match = (AS > BS) ? (A::offset + B::free_bits >= AS - BS) : (A::free_bits + B::offset >= BS - AS);
			constexpr static int combined_shift_a = (AS > BS) ? (-A::offset) : (BS - AS - B::offset);
			constexpr static int combined_shift_b = (BS > AS) ? (-B::offset) : (AS - BS - A::offset);

			// if all of those are false, we need to move the one with bigger F to the lower F
			constexpr static int imperfect_shift_a = (AS < BS) ? A::free_bits : -(AS - BS - B::free_bits);
			constexpr static int imperfect_shift_b = (BS < AS) ? B::free_bits : -(BS - AS - A::free_bits);

			constexpr static int shift_a = 
				(a_perfect_match && (!b_perfect_match || perfect_shift_a < perfect_shift_b)) ? perfect_shift_a :
				((combined_perfect_match) ? combined_shift_a : imperfect_shift_a);
										 
			constexpr static int shift_b =
				(b_perfect_match && (!a_perfect_match || perfect_shift_b < perfect_shift_a)) ? perfect_shift_b :
				((combined_perfect_match) ? combined_shift_b : imperfect_shift_b);
		};

		constexpr int div2_even1(int value) {
			return (value & 1) ? (value / 2 + 1) : (value / 2);
		}

		constexpr int div2_even2(int value) {
			return value / 2;
		}

		template< typename DestType, typename FixedA, typename FixedB >
		struct product_shifts;

		template< typename DestType, typename TA, typename TB, int AI, int AF, unsigned int AO, int BI, int BF, unsigned int BO >
		struct product_shifts< DestType, fixed<TA, AI, AF, AO>, fixed<TB, BI, BF, BO> > {
			using A = fixed<TA, AI, AF, AO>;
			using B = fixed<TB, BI, BF, BO>;
		
			constexpr static int dest_bits = sizeof(DestType) * 8 + ((std::is_signed<DestType>::value && !std::is_signed<TA>::value && !std::is_signed<TB>::value) ? 1 : 0);
		
			constexpr static int overflow_bits = (AI + AF + BI + BF) - dest_bits;

			constexpr static bool perfect_possible = ((AI + AF + BI + BF) <= dest_bits);
			constexpr static bool perfect_always_possible = ((AI + AF + AO + BI + BF + BO) <= dest_bits);

			constexpr static bool perfect_possible_a_shifted = ((AI + AF + BI + BF + BO) <= dest_bits);	
			constexpr static bool perfect_possible_b_shifted = ((AI + AF + AO + BI + BF) <= dest_bits);

			// sig_a = AI+AF, sig_a' = sig_a - (-shift_a - AO) = sig_a + shift_a + AO
			// sig_b = BI+BF, sig_b' = sig_b - (-shift_b - BO) = sib_b + shift_b + BO
			//                sig_a' + sig_b' = dest_bits
			//                sig_a' = sig_b' (+1, iff dest_bits is not even, which doesn't happen)
			// =>
			// shift_a = dest/2 - sig_a - AO
			// shift_b = dest/2 - sig_b - BO
			// But cap to 0, since shifting up does not increase the precision, and if you're moving, move at least by the offset

			constexpr static int imperfect_shift_a_temp = min<int>(dest_bits/2 - AI - AF - AO, 0);
			constexpr static int imperfect_shift_b_temp = min<int>(dest_bits/2 - BI - BF - BO, 0);

			constexpr static int imperfect_shift_a = (imperfect_shift_a_temp != 0) ? min<int>(imperfect_shift_a_temp, -AO) : imperfect_shift_a_temp;
			constexpr static int imperfect_shift_b = (imperfect_shift_b_temp != 0) ? min<int>(imperfect_shift_b_temp, -BO) : imperfect_shift_b_temp;

			constexpr static int shift_a = (!perfect_possible) ? imperfect_shift_a : (
				(perfect_always_possible ? 0 : 
					(((perfect_possible_a_shifted && (!perfect_possible_b_shifted || (AO >= BO))) || !perfect_possible_b_shifted) ? AO : 0)
				));

			constexpr static int shift_b = (!perfect_possible) ? imperfect_shift_b : (
				(perfect_always_possible ? 0 :
					(((perfect_possible_b_shifted && (!perfect_possible_a_shifted || (BO > AO))) || !perfect_possible_a_shifted) ? BO : 0)		
				));				
		};

		template< typename DestType, typename OtherFixed, typename T, int I, int F, unsigned int O >
		constexpr auto mul_match(fixed<T, I, F, O> a)
		{
			return a.template scaling_shift<product_shifts<DestType, fixed<T, I, F, O>, OtherFixed>::shift_a>();
		}

		template< typename OtherFixed, typename T, int I, int F, unsigned int O >
		constexpr auto sum_match(fixed<T,I,F,O> a)
		{
			return a.template scaling_shift<matching_shifts<fixed<T, I, F, O>, OtherFixed>::shift_a>();
		}

		template< typename DestType, typename FixedA, typename FixedB >
		struct div_shifts;

		template< typename DestType, typename TA, typename TB, int AI, int AF, unsigned int AO, int BI, int BF, unsigned int BO >
		struct div_shifts< DestType, fixed<TA, AI, AF, AO>, fixed<TB, BI, BF, BO> > {
			using A = fixed<TA, AI, AF, AO>;
			using B = fixed<TB, BI, BF, BO>;

			constexpr static int destsize = sizeof(DestType) * 8 - ((std::is_signed<DestType>::value && !std::is_signed<TA>::value && !std::is_signed<TB>::value) ? 1 : 0);

			// exp(a, x) = -f(a) + min(i(a), 0) - o(a) + x
			//
			// exp(a/b, x) = exp(a, x) - exp(b, x) + x
			//
			// exp(a/b, size-1) =! exp(a, size-1) - exp(b, size-1) + size - 1 =! i(a) + f(b)
			//
			// i(a/b) = i(a) + f(b)
			//
			// -f(a/b) + min(i(a/b), 0) - o(a/b) + size - 1 = -f(a) + min(i(a), 0) - o(a) + size -1 + f(b) - min(i(b), 0) + o(b) - size + 1 + size - 1
			// -f(a/b) + min(i(a/b), 0) - o(a/b)            = -f(a) + min(i(a), 0) - o(a) + f(b) - min(i(b), 0) + o(b)
			//
			// case A: i(a) + f(b) >= 0: (o(a/b) =! 0)
			// -f(a/b)               = -f(a) + min(i(a), 0) - o(a) + f(b) - min(i(b), 0) + o(b)
			//  f(a/b)               =  f(a) - min(i(a), 0) + o(a) - f(b) + min(i(b), 0) - o(b)

			// case B: i(a) + f(b) < 0:
			// -f(a/b) + i(a) + f(b) = -f(a) + min(i(a), 0) - o(a) + f(b) - min(i(b), 0) + o(b)
			// -f(a/b)               = -f(a) + min(i(a), 0) - i(a) - o(a) - min(i(b), 0) + o(b)
			//  f(a/b)               =  f(a) - min(i(a), 0) + i(a) + o(a) + min(i(b), 0) - o(b)

			constexpr static int shift_a = A::free_bits;
			constexpr static int shift_b = -B::offset;

			constexpr static int RF = (AI + BF >= 0) ?
				(AF - min(AI, 0) + AO - BF + min(BI, 0) - BO) :
				(AF - min(AI, 0) + AO + AI + min(BI, 0) - BO);

			constexpr static int RI = destsize - RF;
			constexpr static unsigned int RO = 0;
		};

		template< typename OtherFixed, typename T, int I, int F, unsigned int O >
		constexpr auto div_match_nom(fixed<T, I, F, O> nom)
		{
			return nom.template scaling_shift<nom.free_bits>();
		}

		template< typename OtherFixed, typename T, int I, int F, unsigned int O >
		constexpr auto div_match_den(fixed<T, I, F, O> den)
		{
			return den.template scaling_shift<-den.offset>();
		}

	}

#define FIXED_TYPE(Type, Value) \
	::util::fixed<Type, ::util::detail::integer_bits<Type>(Value), ::util::detail::fractional_bits<Type>(Value)>

#define FIXED_VALUE(Type, Value) \
	::util::fixed<Type, ::util::detail::integer_bits<Type>(Value), ::util::detail::fractional_bits<Type>(Value)>::from(Value)

#define FIXED_RANGE_TYPE(Type, Min, Max) \
	::util::fixed<Type, ::util::detail::integer_bits_interval<Type>(Min,Max), ::util::detail::fractional_bits_interval<Type>(Min,Max)>


	namespace detail {


		template< typename T, int LI, int LF, unsigned int LO, int RI, int RF >
		constexpr auto operator_plus_impl(fixed<T, LI, LF, LO> lhs, fixed<T, RI, RF, LF + LO - RF> rhs)
		{
			return fixed<T, max(LI, RI), max(LF, RF), min(LO, LF + LO - RF)>(lhs.value + rhs.value);
		}

		template< typename T, int LI, int LF, unsigned int LO, int RI, int RF >
		constexpr auto operator_minus_impl(fixed<T, LI, LF, LO> lhs, fixed<T, RI, RF, LF + LO - RF> rhs)
		{
			return fixed<T, max(LI, RI), max(LF, RF), min(LO, LF + LO - RF)>(lhs.value - rhs.value);
		}

		template< typename DestType, typename T, int LI, int LF, unsigned int LO, int RI, int RF, unsigned int RO >
		constexpr auto mul_impl(fixed<T, LI, LF, LO> lhs, fixed<T, RI, RF, RO> rhs)
		{
			return fixed<DestType, LI+RI, LF+RF, LO+RO>(lhs.value * rhs.value);
		}

		template< typename DestType, typename LT, int LI, int LF, unsigned int LO, typename RT, int RI, int RF, unsigned int RO >
		constexpr auto div_impl(fixed<LT, LI, LF, LO> nom, fixed<RT, RI, RF, RO> den)
		{
			return fixed< DestType,
				detail::div_shifts<DestType, fixed<LT, LI, LF, LO>, fixed<RT, RI, RF, RO>>::RI,
				detail::div_shifts<DestType, fixed<LT, LI, LF, LO>, fixed<RT, RI, RF, RO>>::RF, 
				0 >(nom.value / den.value);
		}

	}

	//
	//
	//

	template< typename T, int LI, int LF, unsigned int LO, int RI, int RF, unsigned int RO >
	constexpr auto operator+(fixed<T, LI, LF, LO> lhs, fixed<T, RI, RF, RO> rhs)
	{
		// :]
		return detail::operator_plus_impl(
			detail::sum_match<fixed<T, RI, RF, RO>>(lhs),
			detail::sum_match<fixed<T, LI, LF, LO>>(rhs)
			);
	}

	template< typename T, int LI, int LF, unsigned int LO, int RI, int RF, unsigned int RO >
	constexpr auto operator-(fixed<T, LI, LF, LO> lhs, fixed<T, RI, RF, RO> rhs)
	{
		// :]
		return detail::operator_minus_impl(
			detail::sum_match<fixed<T, RI, RF, RO>>(lhs),
			detail::sum_match<fixed<T, LI, LF, LO>>(rhs)
			);
	}

	template<typename DestType, typename LT, int LI, int LF, unsigned int LO, typename RT, int RI, int RF, unsigned int RO >
	constexpr auto mul(fixed<LT, LI, LF, LO> lhs, fixed<RT, RI, RF, RO> rhs)
	{
		return detail::mul_impl<DestType>(
			detail::mul_match<DestType, fixed<RT, RI, RF, RO>>(lhs),
			detail::mul_match<DestType, fixed<LT, LI, LF, RO>>(rhs)
			);
	};

	template<typename DestType, typename LT, int LI, int LF, unsigned int LO, typename RT, int RI, int RF, unsigned  int RO >
	constexpr auto div(fixed<LT, LI, LF, LO> lhs, fixed<RT, RI, RF, RO> rhs)
	{
		return detail::div_impl<DestType>(
			detail::div_match_nom<DestType>(lhs),	// note that the matching is not dependent
			detail::div_match_den<DestType>(rhs)
		);
	}

}

#endif
