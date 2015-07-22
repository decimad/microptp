#ifndef MICROPTP_UTIL_HPP__
#define MICROPTP_UTIL_HPP__
#include <type_traits>
#include <cstddef>

namespace util {

	template< typename... Types >
	struct max_alignment;

	template< typename Type0, typename... Types >
	struct max_alignment< Type0, Types... > {
		static const size_t value = (std::alignment_of<Type0>::value > max_alignment< Types... >::value) ? std::alignment_of<Type0>::value : max_alignment< Types... >::value;
	};

	template< >
	struct max_alignment<> {
		static const size_t value = 0;
	};

	template< typename... Types >
	struct max_size;

	template< typename Type0, typename... Types >
	struct max_size< Type0, Types... > {
		static const size_t value = (sizeof(Type0) > max_size< Types... >::value) ? sizeof(Type0) : max_size< Types... >::value;
	};

	template< >
	struct max_size<> {
		static const size_t value = 0;
	};

	template< typename T>
	constexpr T max(T a, T b) {
		return (a > b) ? a : b;
	}

	template< typename T>
	constexpr T min(T a, T b) {
		return (a < b) ? a : b;
	}

	template<typename T>
	constexpr T abs(T value)
	{
		return (value >= 0) ? value : -value;
	}

}

#endif
