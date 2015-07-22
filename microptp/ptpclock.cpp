#include <microptp/ptpclock.hpp>
#include <microptp/ports/systemport.hpp>
#include <stm/trace.h>

namespace uptp {

	PtpClock::PtpClock(SystemPort& system_port, const Config& cfg)
		: master_tracker_(cfg), system_port_(system_port), cfg_(cfg), clock_identity_(cfg.mac_addr, 0)
	{
		statemachine_.to_state<states::Initializing>(*this);
	}

	SystemPort& PtpClock::get_system_port()
	{
		return system_port_;
	}

	MasterTracker& PtpClock::master_tracker()
	{
		return master_tracker_;
	}

	void PtpClock::enable()
	{
		if(is<states::Disabled>()) {
			to_state<states::Listening>();
		}
	}

	void PtpClock::disable()
	{
		if(!is<states::Disabled>()) {
			to_state<states::Disabled>();
		}
	}

	void PtpClock::on_general_message(PacketHandle packet)
	{
		msg::Header header;
		msg::deserialize(packet.get_data(), header);
		
		if (header.is(MessageTypes::Announce)) {
			msg::Announce announce;
			msg::deserialize(packet.get_data(), announce);
			master_tracker_.announce_master(header, announce);
		} else {
			auto* state = statemachine_.get_state_interface<states::PtpStateBase>();
			if (state) {
				state->on_message(header, std::move(packet));
			}
		}
	}

	void PtpClock::on_event_message(PacketHandle packet)
	{
		msg::Header header;
		msg::deserialize(packet.get_data(), header);

		auto* state = statemachine_.get_state_interface<states::PtpStateBase>();
		if (state) {
			state->on_message(header, std::move(packet));
		}

	}

	void PtpClock::on_network_changed() {
		auto& sys = get_system_port();

		event_port_   = sys.make_udp(319);
		general_port_ = sys.make_udp(320);

		if( event_port_ && general_port_ ) {
			sys.join_multicast((224 << 0) | (0<<8) | (1<<16) | (129 << 24));

			event_port_->on_received   = util::function<void(PacketHandle)>(this, &PtpClock::on_event_message);
			general_port_->on_received = util::function<void(PacketHandle)>(this, &PtpClock::on_general_message);

			//statemachine_.to_state<states::Listening>(*this);
		}

	}

	NetHandle& PtpClock::event_port()
	{
		return event_port_;
	}

	NetHandle& PtpClock::general_port()
	{
		return general_port_;
	}

	PortIdentity& PtpClock::get_identity()
	{
		return clock_identity_;
	}

}