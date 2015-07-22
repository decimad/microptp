#include <microptp/ptpclock.hpp>
#include <microptp/messages.hpp>
#include <microptp/ports/systemport.hpp>
#include <stm/trace.h>

namespace uptp {

	namespace states {

		namespace slave_detail {

			//
			//	estimating drift
			//

			estimating_drift::estimating_drift()
				: num_syncs_received_(0)
			{
				uncorrected_offset_buffer_.set(0);
				one_way_delay_buffer_.set(0);
			}

			void estimating_drift::on_sync(Slave& state, Time master_time, Time slave_time)
			{
				if(num_syncs_received_ == 0) {
					first_sync_master_  = master_time;
					first_sync_slave_ = slave_time;
				}

				uncorrected_offset_buffer_.add((
							(master_time-first_sync_master_) - (slave_time-first_sync_slave_)
						).to_nanos());

				sync_master_ = master_time;
				sync_slave_ = slave_time;

				++num_syncs_received_;
			}

			void estimating_drift::on_delay(Slave& slave, Time master_time, Time slave_time)
			{
				// We're assuming that 8*one_way_delay doesn't reach a second!
				if(num_syncs_received_ > 1 ) {
					const auto nom    = (sync_master_ - first_sync_master_);
					const auto den    = (sync_slave_  - first_sync_slave_);
					auto drift =  util::fixed_point<int32, -30>((nom.to_nanos() << 28) / (den.to_nanos() >> 2));

					const auto& t0 = sync_master_;
					const auto& t1 = sync_slave_;

					const auto& t2 = slave_time;
					const auto& t3 = master_time;

					const int64 lhs = ((t3-t0 + t2-t1).to_nanos() / 2);
					const int64 rhs = (((t2-t1).to_nanos()>>2)*drift.value)>>30;

					int32 delay_nanos = static_cast<int32>(lhs + rhs);

#ifdef MICROPTP_DIAGNOSTICS
					if(util::abs(delay_nanos) > 50000000) {
						trace_printf(0, "Bad delay on estimating one way delay (>50ms)\n");
					}
#endif

					one_way_delay_buffer_.add(delay_nanos);

					if(num_syncs_received_ >= 8) {
						// We're really assuming that getting the first 8 syncs took less than 16 seconds here!
						auto drift_minus_one = util::sub<int32, -30>(drift, util::fixed_point<int32, 0>(1));
						auto ppb = util::mul_precise<int32, 0, int64>(drift_minus_one, 1000000000);

						//int32 ppb         = static_cast<int32>(((static_cast<int64>(drift) * 1000000000) >> 30) - 1000000000);
#ifdef MICROPTP_DIAGNOSTICS
						trace_printf(0, "Estimated ppb: %d\n", ppb.value);
#endif

						one_way_delay_buffer_.add(delay_nanos);
						int32 mean_one_way_delay = one_way_delay_buffer_.average();

#ifdef MICROPTP_DIAGNOSTICS
						if(util::abs(mean_one_way_delay) > 50000000) {
							trace_printf(0, "Bad mean delay on estimating one way delay (>50ms)\n");
						} else {
							trace_printf(0, "Mean one way delay: %d nanos\n", mean_one_way_delay);
						}
#endif

						// Fixme: use the mean offset + 1/2 * t * drift for a better estimate!
						//Time difftime = slave.uncorrected_offset_buffer_.average() + (sync_master_ - first_sync_master_).to_nanos()/2;
						int64 offset_nanos = uncorrected_offset_buffer_.average();
						Time mean_uncorrected_offset = Time(offset_nanos/1000000000, offset_nanos%1000000000) + first_sync_master_ - first_sync_slave_;

						auto test_time = util::mul_precise<int32, 0, int64>(drift_minus_one, (nom/2).to_nanos()).value;

						Time offset = mean_uncorrected_offset - Time(0, mean_one_way_delay) + Time(0, test_time);
						trace_printf(0, "Offsetting clock by %d secs %d nanos.\n", static_cast<int32>(offset.secs_), offset.nanos_);

						auto& port = slave.clock_.get_system_port();
						port.adjust_time(offset);
						port.discipline(ppb.value);
						slave.servo_.reset(ppb.value);		// initialize the servo integrator!
						slave.states_.to_state<pi_operational>(mean_one_way_delay);
					}
				}
			}

			//
			// pi_operational
			//

			pi_operational::pi_operational(int32 delay_nanos)
				: last_time_(0,0)
			{
				one_way_delay_buffer_.set(delay_nanos);
				uncorrected_offset_buffer_.set(0);
			}

			void pi_operational::on_sync(Slave& slave, Time master_time, Time slave_time)
			{
				sync_master_ = master_time;
				sync_slave_  = slave_time;
				Time offset  = master_time - slave_time;

				if(offset.secs_ != 0 || util::abs(offset.nanos_) > 50000000) {
					trace_printf(0, "Bad estimatation preset! (secs: %d nanos: %d)\n", static_cast<int32>(offset.secs_), offset.nanos_);
				}

				if(offset.secs_ == 0) {
					uncorrected_offset_buffer_.add(offset.nanos_);
				}
			}

			void pi_operational::on_delay(Slave& slave, Time master_time, Time slave_time)
			{
				const auto& t0 = sync_master_;
				const auto& t1 = sync_slave_;
				const auto& t2 = slave_time;
				const auto& t3 = master_time;

				const Time one_way_delay = ((t3-t0)-(t2-t1))/2;

				if((one_way_delay.secs_ != 0) || (util::abs(one_way_delay.nanos_) > 50000000)) {
					trace_printf(0, "PI Operational: Bad One-Way Delay: %d\n", one_way_delay.nanos_);
				}


				if(one_way_delay.secs_ == 0) {
					one_way_delay_buffer_.add(one_way_delay.nanos_);
				}

				if(last_time_.secs_ != 0) {
					int32 offset = uncorrected_offset_buffer_.average() - one_way_delay_buffer_.average();
					uint32 dt    = static_cast<uint32>((slave_time - last_time_).to_nanos());
					slave.servo_.feed(dt, offset);
				}

				last_time_ = slave_time;
			}

		}

		Slave::Slave(PtpClock& clock)
			: clock_(clock),
			  servo_(clock.get_system_port()),
			  delay_req_id_(4434),
			  sync_serial_(0),
			  sync_state_(slave_detail::SyncState::Initial),
			  dreq_state_(slave_detail::DreqState::Initial)
		{
			clock_.master_tracker().best_master_changed = util::function<void()>(this, &Slave::on_best_master_changed);
			servo_.output = util::function<void(int32)>(&clock.get_system_port(), &SystemPort::discipline);
			clock_.event_port()->on_transmit_completed = util::function<void(uint32, Time)>(this, &Slave::on_delay_request_transmitted);

			states_.to_state<slave_detail::estimating_drift>();
		}

		Slave::~Slave()
		{
			clock_.master_tracker().best_master_changed.reset();
			clock_.event_port()->on_transmit_completed.reset();
		}

		void Slave::on_message(const msg::Header& header, PacketHandle packet_handle)
		{
			if (header.is(MessageTypes::Synch)) {
				msg::Sync sync;
				msg::deserialize(packet_handle.get_data(), sync);

				if(header.flag_field0 & uint8(msg::Header::Field0Flags::TwoStep)) {
					on_sync(header.sequence_id, packet_handle.time());
				} else {
					on_sync(header.sequence_id, packet_handle.time(), sync.origin_timestamp);
				}
				send_delay_request();	// no timers yet :(
			} else if( header.is(MessageTypes::FollowUp)) {
				msg::FollowUp follow_up;
				msg::deserialize(packet_handle.get_data(), follow_up);
				on_sync_followup(header.sequence_id, follow_up.precise_origin_timestamp);
			} else if (header.is(MessageTypes::DelayResp)) {
				msg::DelayResp delayresp;
				msg::deserialize(packet_handle.get_data(), delayresp);
//				if (delayresp.port_identity == clock_.master_tracker().best_foreign()->port_identity) {
					on_request_answered(delayresp.timestamp);
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
			dreq_send_ = when;
			dreq_state_ = slave_detail::DreqState::DreqSent;
			++delay_req_id_;
		}

		void Slave::on_request_answered(const Time& dreq_receive)
		{
			if(dreq_state_ == slave_detail::DreqState::DreqSent) {
				if(states_.is_state<slave_detail::pi_operational>()) {
					states_.as_state<slave_detail::pi_operational>().on_delay(*this, dreq_receive, dreq_send_);
				} else if(states_.is_state<slave_detail::estimating_drift>()) {
					states_.as_state<slave_detail::estimating_drift>().on_delay(*this, dreq_receive, dreq_send_);
				}
				dreq_state_ = slave_detail::DreqState::Initial;
			}
		}

		void Slave::on_sync(uint16 serial, const Time& receive_time, const Time& send_time)
		{
			if(states_.is_state<slave_detail::pi_operational>()) {
				states_.as_state<slave_detail::pi_operational>().on_sync(*this, send_time, receive_time);
			} else if(states_.is_state<slave_detail::estimating_drift>()) {
				states_.as_state<slave_detail::estimating_drift>().on_sync(*this, send_time, receive_time);
			}
			sync_state_ = slave_detail::SyncState::Initial;
		}

		// Two-Step
		void Slave::on_sync(uint16 serial, const Time& receive_time)
		{
			sync_receive_ = receive_time;
			sync_serial_ = serial;
			sync_state_ = slave_detail::SyncState::SyncTwoStepReceived;
		}

		void Slave::on_sync_followup(uint16 serial, const Time& send_time)
		{
			if(sync_state_ == slave_detail::SyncState::SyncTwoStepReceived && serial == sync_serial_) {
				if(states_.is_state<slave_detail::pi_operational>()) {
					states_.as_state<slave_detail::pi_operational>().on_sync(*this, send_time, sync_receive_);
				} else if(states_.is_state<slave_detail::estimating_drift>()) {
					states_.as_state<slave_detail::estimating_drift>().on_sync(*this, send_time, sync_receive_);
				}
				sync_state_ = slave_detail::SyncState::Initial;
			} else {
				sync_state_ = slave_detail::SyncState::Initial;
			}
		}
	
	}

}
