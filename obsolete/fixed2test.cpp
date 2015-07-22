#include <microptp/util/fixed2.hpp>
#include <tuple>
#include <functional>

using int32 = int;
using uint32 = unsigned int;
using int64 = long long;
using uint64 = unsigned long long;

template<typename T>
constexpr bool highest_bit_set(T val)
{
	return (val & (1 << (sizeof(T) * 8 - 1))) != 0;
}

void FIXED_VALUE_test()
{
	// These would currently fail for cases there the fraction part is a limited power of two
	// and it would detect those. Need to improve! 0.5 should really be f[0,1]... 0.25 should be f[-1,2] and so on...
	constexpr auto fixed1 = FIXED_VALUE(int, -234.2333453);
	static_assert(highest_bit_set(fixed1.value), "Not using all precision");

	constexpr auto fixed2 = FIXED_VALUE(unsigned int, 234.2333453);
	static_assert(highest_bit_set(fixed2.value), "Not using all precision");

	constexpr auto fixed3 = FIXED_VALUE(unsigned int, 234);
	static_assert(!highest_bit_set(fixed3.value), "Encoding zero fraction");
}

void log_test()
{
	static_assert(util::log2_ceil (8) == 3, "Bad log");
	static_assert(util::log2_floor(8) == 3, "Bad log");
	static_assert(util::log2_ceil (8.2) == 4, "Bad log");
	static_assert(util::log2_floor(8.2) == 3, "Bad log");
}

void safe_abs_test()
{
	static_assert(util::safe_abs<short>(-32768) == 32767, "safe abs fail.");
}

void type_test()
{
	FIXED_TYPE(unsigned int, 3251) my_value;
}

void range_test()
{
	constexpr double start = -544.53;
	constexpr double end = -3.463;

	using my_type = FIXED_RANGE_TYPE(int, start, end);
	constexpr auto end_val   = my_type::from(end).to<int>();
	constexpr auto start_val = my_type::from(start).to<int>();

	static_assert(my_type::from(end).to<int>()   == util::round(end),   "fixed range miscalculated");
	static_assert(my_type::from(start).to<int>() == util::round(start), "fixed range miscalculated");
}

void shift_test()
{
	constexpr auto val = util::fixed<int, 17, 5>(0).scaling_shift<2>();
}


void plus_test()
{
	using A = util::fixed<int, 17,  5>;
	using B = util::fixed<int, 5, 17>;
	constexpr auto result = A::from(34) + B::from(2.532);
	constexpr auto value = result.to<double>();
}

void minus_test()
{
	
	using A = util::fixed<int, 17, 5>;
	using B = util::fixed<int, 5, 17>;
	constexpr auto result2 = A::from(2) - B::from(3.532);
	constexpr auto value = result2.to<double>();
}

void binary_digits_test()
{
	constexpr int digits = util::binary_digits(3.5);
}

void multiply_test()
{
	using A = util::fixed<int, 9, 5, 5>;
	using B = util::fixed<int, 4, 17>;

	using C = util::fixed<int, -3, util::binary_digits(3.5)-3>;
	constexpr auto c = C::from(0.01);	// 6 bits payload ;)
	constexpr auto c_val = c.to<double>();
	
	constexpr double a_double = 543.21;
	constexpr double b_double = 3.567423234;

	constexpr auto a = A::from(a_double);
	constexpr auto a_val = a.to<double>();

	constexpr auto b = B::from(b_double);
	constexpr auto b_val = b.to<double>();

	constexpr auto result = util::mul<int>(a,b);
	constexpr auto conversion = result.to<double>();
	constexpr auto error = conversion - a_double * b_double;
}

void negative_fractional_test()
{
	using A = util::fixed<int, 4, -2>;
	
	constexpr auto a = A::from(61);
	constexpr auto a_val = a.to<int>();
}

void negative_integer_test()
{
	using A = util::fixed<int, -4, 6>;
	constexpr auto scaling = A::scaling;

	constexpr auto a = A::from(0.091);
	constexpr auto a_val = a.to<double>();
}

void div_test()
{
	using A = util::fixed<int, 9, 5>;
	using B = util::fixed<int, 4, 17>;

	constexpr auto fixed_a = A::from(255.32);
	constexpr auto fixed_a_value = fixed_a.to<double>();

	constexpr auto fixed_b = B::from(0.0242);
	constexpr auto fixed_b_value = fixed_b.to<double>();


	constexpr auto exponent = A::max_exponent();
	constexpr auto max_integer_bits = A::max_exponent() - B::min_exponent();

	constexpr auto max_integer = util::exp2<int64>(A::max_exponent() - B::min_exponent());

	using C = util::fixed<int, 0, util::binary_digits(3.5)>;

	constexpr int RI = util::detail::div_shifts<int, A, B>::RI;
	constexpr int RF = util::detail::div_shifts<int, A, B>::RF;

	constexpr auto div_result = util::fixed<int, RI, RF>(fixed_a.value / fixed_b.value);
	constexpr double test = div_result.to<double>();

	constexpr auto div_super_test = util::div<int>(fixed_a, fixed_b);
	constexpr auto div_super_test_value = div_super_test.to<double>();
}

void func()
{
	constexpr auto shifted = util::shifted(5, -2);
	constexpr auto result = util::log2_ceil(9.2);

	constexpr auto value = util::safe_abs<short>(-32768);

	constexpr auto fixed = util::fixed<int, 8>::from(-43.46);
	constexpr auto exp5  = fixed.to<int>();

	constexpr auto exp4 = util::exp2<short>(0);

	constexpr auto exp = util::scaled_exp2(3, 2);

	constexpr auto asfasf = util::safe_abs(-4.25);

	constexpr auto asfa = util::is_power_of_2(4.4);

	constexpr auto teas = util::log2_floor(4.4);

	constexpr util::fixed<int, 35, -3> val(4);
	
	constexpr auto log = util::log2_ceil(4.2);
	constexpr auto bits = util::detail::integer_bits<int>(-234.2);
	constexpr auto fixed2 = FIXED_VALUE(int, 234.2333453);
	constexpr bool highest_set = (fixed2.value & (1 << 31)) != 0;

	constexpr auto asfdt = fixed2.to<double>();
}
