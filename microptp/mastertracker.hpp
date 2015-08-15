#ifndef MICROPTP_MASTERTRACKER_HPP__
#define MICROPTP_MASTERTRACKER_HPP__

#include <microptp/config.hpp>
#include <microlib/functional.hpp>
#include <microlib/sorted_static_vector.hpp>
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

		TimerHandle watchdog;
	};

	int bmc_compare(const MasterDescriptor& a, const MasterDescriptor& b, const Config& cfg);

	struct BmcComparator {
		BmcComparator(const Config& config)
			: config_(config)
		{}

		bool operator()(const MasterDescriptor& a, const MasterDescriptor& b) const
		{
			return bmc_compare(a, b, config_) == -1;
		}
		
		bool operator()(const ulib::pool_ptr<MasterDescriptor>& a, const ulib::pool_ptr<MasterDescriptor>& b) const
		{
			return operator()(*a, *b);
		}

		const Config& config_;
	};


	class MasterTracker {
	public:
		static constexpr size_t max_masters_ = 10;
		using sorted_storage_type = ulib::sorted_static_vector<ulib::pool_ptr<MasterDescriptor>, max_masters_, BmcComparator>;
		using sorted_iterator = typename sorted_storage_type::iterator;

		MasterTracker(const Config&);
		~MasterTracker();

		void announce_master(const msg::Header& header, const msg::Announce& announce);

		ulib::function<void()> foreign_set_changed;
		ulib::function<void()> best_master_changed;

		size_t num_foreigns() const;
		const MasterDescriptor* best_foreign() const;

	private:
		MasterDescriptor* find(const PortIdentity&);

		sorted_iterator find_master(const PortIdentity&);
		sorted_iterator find_master(const MasterDescriptor&);		
		void erase(sorted_iterator it);
	
		uint8 best_master() const;

		const Config& config_;

		ulib::pool<MasterDescriptor, max_masters_> foreign_masters_;
		sorted_storage_type sorted_masters_;
	};

}

#endif
