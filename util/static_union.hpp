#ifndef MICROPTP_UTIL_STATIC_UNION_HPP
#define MICROPTP_UTIL_STATIC_UNION_HPP

#include <utility>
#include <type_traits>
#include <microptp/util/util.hpp>
#include <microptp/util/utiltypes.hpp>

namespace util {

	template< typename T, size_t Current = 0, typename... Types>
	struct type_to_index;

	template< typename T, size_t Current, typename Type0, typename... Types >
	struct type_to_index< T, Current, Type0, Types... > {
		static const size_t value = std::is_same<Type0, T>::value ? Current : type_to_index<T, Current + 1, Types...>::value;
	};

	template< typename T, size_t current >
	struct type_to_index<T, current>
	{
		static const size_t value = -1;
	};

	template< size_t Index, typename... Types >
	struct index_to_type;

	template< size_t Index, typename Type0, typename... Types >
	struct index_to_type< Index, Type0, Types... >
	{
		using type = typename index_to_type< Index - 1, Types... >::type;
	};

	template< typename Type0, typename... Types >
	struct index_to_type< 0, Type0, Types... >
	{
		using type = Type0;
	};

	template< size_t Index, typename... Types >
	using index_to_type_t = typename index_to_type<Index, Types...>::type;

	template< typename... Types >
	struct static_union {
	private:
		static constexpr size_t Size = sizeof...(Types);

		template< size_t Index >
		void destruct_impl(size_t index, util::size_t_type<Index> ) {
			if (Index == index) {
				using type = typename index_to_type<Index, Types...>::type;
				auto ptr = reinterpret_cast<type*>(&storage_);
				ptr->~type();
				current_type = -1;
			} else {
				destruct_impl(index, util::size_t_type<Index+1>());
			}
		}

		void destruct_impl(size_t index, util::size_t_type<sizeof...(Types)>) {
			current_type = -1;
		}

		void destruct(size_t index) {
			destruct_impl(index, util::size_t_type<0>());
		}

	public:
		void clear() {
			if(current_type != -1) destruct(current_type);
		}

		template< typename ToType, typename... Args >
		ToType& to_type(Args&&... args)
		{
			static_assert(type_to_index<ToType, 0, Types...>::value != -1, "Not my type.");

			if (current_type != -1) {
				destruct(current_type);
			}

			new (&storage_) ToType(std::forward<Args>(args)...);
			current_type = type_to_index<ToType, 0, Types...>::value;

			return *reinterpret_cast<ToType*>(&storage_);
		}

		template< typename T >
		bool is() const
		{
			return current_type == type_to_index<T, 0, Types...>::value;
		}

		template< typename T >
		T& as() {
			return *reinterpret_cast<T*>(&storage_);
		}

		template< typename T >
		const T& as() const {
			return *reinterpret_cast<const T*>(&storage_);
		}

	private:

		template< typename Iface, size_t Index >
		Iface* get_interface_impl(size_t current_type, util::size_t_type<Index>) {
			if (Index == current_type) {
				return &as<index_to_type_t<Index, Types...>>();
			} else {
				return get_interface_impl<Iface>(current_type, util::size_t_type<Index+1>());
			}
		}

		template<typename Iface>
		Iface* get_interface_impl(size_t current_type, util::size_t_type<Size>) {
			return nullptr;
		}

	public:
		template< typename Iface >
		Iface* get_interface()
		{
			return get_interface_impl<Iface>(current_type, util::size_t_type<0>());
		}

		static_union()
			: current_type(-1)
		{}

		~static_union()
		{
			clear();
		}

	private:
		using storage_type = typename std::aligned_storage<max_size<Types...>::value, max_alignment<Types...>::value>::type;
		storage_type storage_;

		size_t current_type = -1;
	};

}

#endif
