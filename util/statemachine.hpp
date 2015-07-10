#ifndef MICROPTP_UTIL_STATEMACHINE_HPP__
#define MICROPTP_UTIL_STATEMACHINE_HPP__

#include <microptp/util/static_union.hpp>

namespace util {

	template< typename... States >
	struct state_machine {

		state_machine()
		{
		}

		template<typename ToState, typename... Args>
		void to_state(Args&&... args)
		{
			storage_.template to_type<ToState>( std::forward<Args>(args)... );
		}
		
		template< typename Iface >
		Iface* get_state_interface()
		{
			return storage_.template get_interface<Iface>();
		}

		template< typename T >
		bool is_state() const
		{
			return storage_.template is<T>();
		}

		template< typename T >
		T& as_state()
		{
			return storage_.template as<T>();
		}

		template< typename T >
		const T& as_state() const
		{
			return storage_.template as<T>();
		}

	private:
		static_union<States...> storage_;
	};

}

#endif
