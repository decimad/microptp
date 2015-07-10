#include <microptp/ptpclock.hpp>
#include <microptp/messages.hpp>
#include <microptp/ports/systemport.hpp>

namespace uptp {

	namespace states {

		Slave::Slave(PtpClock& clock)
			: clock_(clock), servo_(clock.get_system_port()), delay_req_id_(4434), delay_state_(clock.get_system_port())
		{
			clock_.master_tracker().best_master_changed = util::function<void()>(this, &Slave::on_best_master_changed);
			delay_state_.on_offset_update = util::function<void(uint32, int32)>(&servo_, &ClockServo::feed);
			servo_.output = util::function<void(int32)>(&clock.get_system_port(), &SystemPort::discipline);
			clock_.event_port()->on_transmit_completed = util::function<void(uint32, Time)>(this, &Slave::on_delay_request_transmitted);
		}

		Slave::~Slave()
		{
			clock_.master_tracker().best_master_changed.reset();
			delay_state_.on_offset_update.reset();
			clock_.event_port()->on_transmit_completed.reset();
		}

		void Slave::on_message(const msg::Header& header, PacketHandle packet_handle)
		{
			if (header.is(MessageTypes::Synch)) {
				msg::Sync sync;
				msg::deserialize(packet_handle.get_data(), sync);
				if(header.flag_field0 & uint8(msg::Header::Field0Flags::TwoStep)) {
					delay_state_.on_sync(header.sequence_id, packet_handle.time());
				} else {
					delay_state_.on_sync(header.sequence_id, packet_handle.time(), sync.origin_timestamp);
				}
				send_delay_request();	// no timers yet :(
			} else if( header.is(MessageTypes::FollowUp)) {
				msg::FollowUp follow_up;
				msg::deserialize(packet_handle.get_data(), follow_up);
				delay_state_.on_sync_followup(header.sequence_id, follow_up.precise_origin_timestamp);
			} else if (header.is(MessageTypes::DelayResp)) {
				msg::DelayResp delayresp;
				msg::deserialize(packet_handle.get_data(), delayresp);
//				if (delayresp.port_identity == clock_.master_tracker().best_foreign()->port_identity) {
					delay_state_.on_request_answered(delayresp.timestamp);
//				}
			}
		}

		void Slave::send_delay_request()
		{
			auto& port = clock_.event_port();
			auto buffer_handle = port->acquire_transmit_handle();

			msg::Header header;
			msg::DelayReq dreq;
			header.source_port_identity = clock_.get_identity();
			header.log_message_interval = 0x7F;
			header.control_field = 1;
			header.correction_field = 0;
			header.domain_number = 0;
			header.message_length = 44;
			header.version_ptp = 2;
			header.transport_specific = 8;
			header.message_type = static_cast<uint8>(MessageTypes::DelayRequest);
			header.sequence_id = 0xFFFF0000 | delay_req_id_;

			dreq.timestamp = Time(0ull);

			msg::serialize(buffer_handle.get_data(), header);
			msg::serialize(buffer_handle.get_data(), dreq);

			buffer_handle.set_size(44);

			port->send((224 << 0) | (0<<8) | (1<<16) | (129 << 24), 319, std::move(buffer_handle), 0xFFFF0000 | delay_req_id_);
		}

		void Slave::on_best_master_changed()
		{
		}

		void Slave::on_delay_req_timer()
		{
			// Timers to be implemented
		}

		void Slave::on_delay_request_transmitted(uint32 id, Time when)
		{
			delay_state_.on_request_sent(when);
			++delay_req_id_;
		}
	
	}

}
