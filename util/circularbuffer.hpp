#ifndef MICROPTP_UTIL_CIRCULARBUFFER_HPP_
#define MICROPTP_UTIL_CIRCULARBUFFER_HPP_
#include <array>

namespace util {

	template< typename T, size_t Size >
	struct circular_buffer {
		circular_buffer()
			: front_(0)
		{}

		void reset(T value = 0)
		{
			for(auto& elem : arr_) {
				elem = value;
			}

			front_ = 0;
		}

		void set(T value = 0)
		{
			for(auto& elem : arr_) {
				elem = value;
			}
			front_ = Size;
		}

		void add(T value) {
			arr_[front_%Size] = value;
			++front_;
		}

		T& operator[](size_t index) {
			return arr_[(front_-index)%Size];
		}

		T operator[](size_t index) const {
			return arr_[(front_-index)%Size];
		}

		void offset_all(T off) {
			for(auto& elem : arr_) {
				elem += off;
			}
		}

	private:
		std::array<T, Size> arr_;
		size_t front_;
	};

	template< typename T, size_t Size >
	struct circular_averaging_buffer {
		circular_averaging_buffer()
			: front_(0), sum_(0)
		{}

		void reset(T value = 0)
		{
			for(auto& elem : arr_) {
				elem = value;
			}

			front_ = 0;
		}

		void set(T value = 0)
		{
			for(auto& elem : arr_) {
				elem = value;
			}
			front_ = Size;
			sum_ = Size * value;
		}

		void add(T value) {
			sum_ -= arr_[(front_-Size-1)%Size];
			arr_[front_%Size] = value;
			sum_ += value;
			++front_;
		}

		T average() const {
			return sum_ / Size;
		}

		T& operator[](size_t index) {
			return arr_[(front_-index)%Size];
		}

		T operator[](size_t index) const {
			return arr_[(front_-index)%Size];
		}

		void offset_all(T off) {
			for(auto& elem : arr_) {
				elem += off;
			}
			sum_ += Size*off;
		}

	private:
		std::array<T, Size> arr_;
		T sum_;
		size_t front_;
	};


}

#endif
