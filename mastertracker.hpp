#ifndef MICROPTP_MASTERTRACKER_HPP__
#define MICROPTP_MASTERTRACKER_HPP__

#include <microptp/uptp.hpp>
#include <microptp/util/functional.hpp>
#include <microptp/ptpdatatypes.hpp>
#include <microptp/messages.hpp>
#include <algorithm>

namespace uptp {

	struct MasterDescriptor {
		MasterDescriptor(const msg::Header& h, const msg::Announce& a);
		void update(const msg::Header& h, const msg::Announce& a);

		PortIdentity port_identity;		
		ClockIdentity grandmaster_clock;
		ClockQuality grandmaster_clock_quality;

		uint16 stepsRemoved;
		uint8 grandmasterPriority1;
		uint8 grandmasterPriority2;

		int8 log_message_interval;

		uint8 domainNumber;
		enum8 timeSource;
	};

	int bmc_compare(const MasterDescriptor& a, const MasterDescriptor& b, const Config& cfg);


	template< typename T, size_t Size >
	struct inefficient_pool {

		size_t insert(const T& val) {
			for (size_t i = 0; i < Size;++i) {
				if (!taken_[i]) {
					new (&array_[i]) T(val);
					taken_[i] = true;
					++count_;
					return i;
				}
			}

			return size_t(-1);
		}

		template< typename... Args >
		size_t emplace(Args&&... args)
		{
			for (size_t i = 0; i < Size;++i) {
				if (!taken_[i]) {
					new (&array_[i]) T(std::forward<Args>(args)...);
					taken_[i] = true;
					++count_;
					return i;
				}
			}

			return size_t(-1);
		}

		void remove(size_t index) {
			if (taken_[index]) {
				reinterpret_cast<T*>(&array_[index])->~T();
				taken_[index] = false;
				--count_;
			}
		}

		bool is_taken(size_t index) const
		{
			return taken_[index];
		}

		T& operator[](size_t index) {
			return reinterpret_cast<T&>(array_[index]);
		}

		const T& operator[](size_t index) const {
			return reinterpret_cast<const T&>(array_[index]);
		}

		template< typename Condition >
		size_t find_if(Condition&& condition) {
			for (size_t i = 0; i < Size; ++i) {
				if (taken_[i] && condition(*reinterpret_cast<T*>(&array_[i]))) return i;
			}

			return -1;
		}

		~inefficient_pool() {
			for (size_t i = 0; i < Size; ++i) {
				if (taken_[i]) {
					reinterpret_cast<T*>(&array_[i])->~T();
				}
			}
		}

	private:
		using storage_type = std::aligned_storage_t<sizeof(T),std::alignment_of<T>::value>;
		std::array<storage_type, Size> array_;
		std::array<bool, Size> taken_;
		size_t count_;
	};

	template< size_t Size >
	struct BmcComparator {
		BmcComparator(const inefficient_pool<MasterDescriptor, Size>& pool, const Config& config)
			: pool_(pool), config_(config)
		{}

		bool operator()(size_t indexa, size_t indexb)
		{
			return bmc_compare(pool_[indexa], pool_[indexb], config_) == -1;
		}

		const inefficient_pool<MasterDescriptor, Size>& pool_;
		const Config config_;
	};


	template< typename T, size_t Size, typename Compare = std::less<T> >
	struct inefficient_sorted_array : private Compare {

		template< typename... Args >
		inefficient_sorted_array(Args&&... args)
			: Compare(std::forward<Args>(args)...), count_(0)
		{
		}

		void remove_index(size_t index) {
			for (size_t i = index; i < count_ - 1; ++i) {
				array_[i] = array_[i + 1];
			}
			--count_;
		}

		void remove_val(const T& val) {
			for (size_t i = 0; i < Size; ++i) {
				if (array_[i] == val) {
					remove_index(i);
					return;
				}
			}
		}

		size_t size() const
		{
			return count_;
		}

		void resort()
		{
			std::sort(&array_[0], &array_[count_], static_cast<Compare&>(*this));
		}

		void insert(const T& val) {
			if(count_ != Size) {
				array_[count_] = val;
				++count_;
				resort();
			}
		}

		const T& front() const
		{
			return array_[0];
		}

		const T& back() const
		{
			return array_[count_ - 1];
		}

	private:
		std::array<T, Size> array_;
		size_t count_;
	};


	class MasterTracker {
	public:
		MasterTracker(const Config&);
		~MasterTracker();

		void announce_master(const msg::Header& header, const msg::Announce& announce);

		util::function<void()> foreign_set_changed;
		util::function<void()> best_master_changed;

		size_t num_foreigns() const;
		const MasterDescriptor* best_foreign() const;

	private:
		MasterDescriptor* find(const PortIdentity&);

		uint8 find_master(const PortIdentity&);
		uint8 find_master(const MasterDescriptor&);
		
		void remove_master(const PortIdentity&);
		void remove_master(uint8 index);

		uint8 best_master() const;

		static constexpr size_t max_masters_ = 10;

		inefficient_pool<MasterDescriptor, max_masters_> foreign_masters_;
		inefficient_sorted_array<uint8, max_masters_, BmcComparator<max_masters_>> sorted_masters_;
	};

}

#endif
