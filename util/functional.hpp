#ifndef MICROPTP_UTIL_FUNCTIONAL_HPP__
#define MICROPTP_UTIL_FUNCTIONAL_HPP__

#include <memory>
#include <microptp/util/util.hpp>

namespace util {


	template< typename Prototype >
	class function;

	template< typename Ret, typename... Args >
	class function< Ret(Args...) >
	{
	private:
		struct function_impl {
			virtual Ret call( Args... args ) const = 0;
			virtual void clone_construct(void* dest) const = 0;
			virtual ~function_impl() {};
		};
		
		template< typename Class, typename TargetRet, typename... TargetArgs >
		struct method_impl : public function_impl
		{
			using target_ptr_type = TargetRet(Class::*)(TargetArgs...);

			method_impl(Class* ptr, target_ptr_type target)
				: ptr_(ptr), target_(target)
			{}

			Ret call(Args... args) const override {
				return (ptr_->*target_)(std::move(args)...);
			}

			void clone_construct(void* dest) const override
			{
				new (dest) method_impl(ptr_, target_);
			}

			target_ptr_type target_;
			Class* ptr_;
		};

		template< typename TargetRet, typename... TargetArgs >
		struct free_impl : public function_impl
		{
			using target_ptr_type = TargetRet (*) (TargetArgs...);

			free_impl(target_ptr_type target)
				: target_(target)
			{
			}

			Ret call(Args... args) const override {
				return target_(args...);
			}

			void clone_construct(void* dest) const override
			{
				new (dest) free_impl(target_);
			}

			target_ptr_type target_;
		};
		
		using storage_type =
			std::aligned_storage_t<
				max_size<method_impl<function,void>, free_impl<void>>::value,
				max_alignment<method_impl<function,void>, free_impl<void>>::value
			>;


	public:
		function()
			: initialized_(false)
		{}

		function(const function& other)
			: initialized_(false)
		{
			*this = other;
		}

		template< typename Class, typename TargetRet, typename... TargetArgs >
		function(Class* ptr, TargetRet (Class::*func)(TargetArgs...) )
			: initialized_(true)
		{
			new (&storage_) method_impl<Class, TargetRet, TargetArgs...>(ptr, func);
		}

		template< typename TargetRet, typename... TargetArgs >
		function( TargetRet(*target)(TargetArgs...))
			: initialized_(true)
		{
			new (&storage_) free_impl<TargetRet, TargetArgs...>(target);
		}

		function& operator=(const function& other)
		{
			reset();
			auto* ptr = other.get_impl();
			if (ptr) {
				ptr->clone_construct(&storage_);
				initialized_ = true;
			}

			return *this;
		}

		explicit operator bool() const {
			return initialized_;
		}

		template< typename... CallArgs >
		Ret operator()(CallArgs&&... args) const
		{
			return get_impl()->call(std::forward<CallArgs>(args)...);
		}

		void reset()
		{
			if (initialized_) {
				get_impl()->~function_impl();
				initialized_ = false;
			}
		}

		~function()
		{
			reset();
		}

	private:
		function_impl* get_impl() {
			if (initialized_) {
				return reinterpret_cast<function_impl*>(&storage_);
			}
			else {
				return nullptr;
			}
		}

		const function_impl* get_impl() const {
			if (initialized_) {
				return reinterpret_cast<const function_impl*>(&storage_);
			}
			else {
				return nullptr;
			}
		}

	private:
		storage_type storage_;
		bool initialized_;
	};

}

#endif
