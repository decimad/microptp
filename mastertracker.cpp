#include <microptp/uptp.hpp>
#include <microptp/ptpdatatypes.hpp>
#include <microptp/mastertracker.hpp>
#include <microptp/messages.hpp>

namespace uptp {

	using namespace msg;

	namespace {
		template< typename T >
		int compare(const T& a, const T& b)
		{
			if (a > b) return 1;
			else if (a == b) return 0;
			else return -1;
		}

		template< typename T >
		int match_exclusive(const T& a, const T& b, const T& pref)
		{
			if (a == pref && b == pref) return 0;
			if (a == pref) return -1;
			if (b == pref) return 1;
			return 0;
		}
	}

	int bmc_compare(const MasterDescriptor& a, const MasterDescriptor& b, const Config& cfg)
	{
		int result;

		if (result = compare(a.grandmaster_clock, b.grandmaster_clock)) {
			int grandmaster_compare_result = result;

			// OPTIONAL domain comparison / any domain 
			if ( cfg.any_domain ) {
				// part 1: preferred domain wins
				const uint8 domainNumber = 0;
				if (result = match_exclusive(a.domainNumber, b.domainNumber, cfg.preferred_domain)) return result;
				if (result = compare(a.domainNumber, b.domainNumber)) return result;
			}

			if (result = compare(a.grandmasterPriority1, b.grandmasterPriority1)) return result;
			if (result = compare(a.grandmaster_clock_quality, b.grandmaster_clock_quality)) return result;
			if (result = compare(a.grandmasterPriority2, b.grandmasterPriority2)) return result;
			return grandmaster_compare_result;
		} else {
			if (a.stepsRemoved + 1 < b.stepsRemoved) return -1;
			if (a.stepsRemoved > b.stepsRemoved + 1) return 1;
			
			if (result = compare(a.port_identity, b.port_identity)) return result;
			
			return 0;
		}
	}
		
	//
	// MasterDescriptor
	//
	MasterDescriptor::MasterDescriptor(const msg::Header& h, const msg::Announce& a)
	{
		update(h, a);
	}

	void MasterDescriptor::update(const msg::Header& h, const msg::Announce& a)
	{
		port_identity = h.source_port_identity;
		grandmaster_clock = a.grandmaster_identity;
		grandmaster_clock_quality = a.grandmaster_clock_quality;

		stepsRemoved = a.steps_removed+1;
		grandmasterPriority1 = a.grandmaster_priority1;
		grandmasterPriority2 = a.grandmaster_priority2;

		log_message_interval = h.log_message_interval;

		domainNumber = h.domain_number;
		timeSource = a.time_source;
	}


	//
	// MasterTracker
	//

	MasterTracker::MasterTracker(const Config& config)
		: sorted_masters_(foreign_masters_, config)
	{
	}

	MasterTracker::~MasterTracker()
	{
	}

	void MasterTracker::announce_master(const msg::Header& header, const msg::Announce& announce)
	{
		size_t old_best = sorted_masters_.size() ? sorted_masters_.front() : -1;
		auto index = foreign_masters_.find_if([&](const MasterDescriptor& desc) { return desc.port_identity == header.source_port_identity; });
		if (index == size_t(-1)) {
			index = foreign_masters_.emplace(header, announce);
			if(index != size_t(-1)) {
				sorted_masters_.insert(index);
			}
		} else {
//			foreign_masters_[index].update(header, announce);
//			sorted_masters_.resort();
		}
		if (sorted_masters_.front() != old_best && best_master_changed) {
			best_master_changed();
		}
	}

	uint8 MasterTracker::find_master(const PortIdentity& identity) {
		return foreign_masters_.find_if([&](const MasterDescriptor& desc) { return desc.port_identity == identity; });
	}

	uint8 MasterTracker::find_master(const MasterDescriptor& desc) {
		return foreign_masters_.find_if([&](const MasterDescriptor& desc2) { return &desc2 == &desc; });
	}

	void MasterTracker::remove_master(const PortIdentity& identity) {
		auto index = find_master(identity);
		if (index != -1) {
			remove_master(index);
		}
	}
	
	void MasterTracker::remove_master(uint8 index) {
		size_t old_best = sorted_masters_.front();
		sorted_masters_.remove_val(index);
		foreign_masters_.remove(index);
		if (sorted_masters_.front() != old_best && best_master_changed) {
			best_master_changed();
		}
	}

	const MasterDescriptor* uptp::MasterTracker::best_foreign() const
	{
		if(sorted_masters_.size()) {
			return &foreign_masters_[sorted_masters_.front()];
		} else {
			return nullptr;
		}
	}

}
