//          Copyright Michael Steinberg 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <microptp/config.hpp>
#include <microptp/ptpclock.hpp>
#include <microptp/messages.hpp>
#include <microptp/ports/systemport.hpp>

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
				using namespace fix;
				using large_nanos_type = FIXED_RANGE_I(0, 16000000000ull);
				using small_nanos_type = FIXED_RANGE_I(0,2000000000u);
				using drift_type = FIXED_RANGE(0.99, 1.01, 32);

				// We're assuming that 8*one_way_delay doesn't reach a second!
				if(num_syncs_received_ >= 1 ) {
					const auto& t0 = sync_master_;
					const auto& t1 = sync_slave_;

					const auto& t2 = slave_time;
					const auto& t3 = master_time;

					const int32 delay_nanos = (t3-t0 + t2-t1).to_nanos() / 2;
					one_way_delay_buffer_.add(delay_nanos);

					if(ulib::abs(delay_nanos) > 50000000) {
						TRACE("Bad delay on estimating one way delay (>50ms)\n");
					}

					if(num_syncs_received_ >= 8) {
						const auto nom    = (sync_master_ - first_sync_master_);
						const auto den    = (sync_slave_  - first_sync_slave_);

						const large_nanos_type nom_nanos(nom.to_nanos());
						const drift_type drift = div<fits<1,31>, positive>(nom_nanos, large_nanos_type(den.to_nanos()));

						// We're really assuming that getting the first 8 syncs took less than 16 seconds here!
						const auto drift_minus_one = drift-FIXED_CONSTANT_I(1);
						const auto ppb_fixed = (drift_minus_one*FIXED_CONSTANT_I(1000000000u));
						const auto ppb = ppb_fixed.to<int32>();

						TRACE("Estimated ppb: %d\n", ppb);

						int32 mean_one_way_delay = one_way_delay_buffer_.average();

						if(ulib::abs(mean_one_way_delay) > 50000000) {
							TRACE("Bad mean delay on estimating one way delay (>50ms)\n");
						} else {
							TRACE("Mean one way delay: %d nanos\n", mean_one_way_delay);
						}

						// Fixme: use the mean offset + 1/2 * t * drift for a better estimate!
						int64 offset_mean = uncorrected_offset_buffer_.average();
						Time mean_uncorrected_offset = Time(offset_mean/1000000000, offset_mean%1000000000) + first_sync_master_ - first_sync_slave_;

						const int64 corrected_half = fix::virtual_shift<-1>(drift_minus_one * nom_nanos).to<int64>();
						Time offset = mean_uncorrected_offset + Time(0, mean_one_way_delay) /*+ Time(corrected_half/1000000000, corrected_half%1000000000)*/;
						TRACE("Offsetting clock by %d secs %d nanos.\n", static_cast<int32>(offset.secs_), offset.nanos_);

						auto& port = slave.clock_.get_system_port();
						port.adjust_time(offset);
						port.discipline(ppb);
						slave.servo_.reset(ppb);		// initialize the servo integrator!
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

				//one_way_delay_filter_.feed(delay_nanos);
			}

			void pi_operational::on_sync(Slave& slave, Time master_time, Time slave_time)
			{
				sync_master_ = master_time;
				sync_slave_  = slave_time;
				Time offset  = master_time - slave_time;

				if(offset.secs_ != 0 || ulib::abs(offset.nanos_) > 50000000) {
					TRACE("Bad estimatation preset! (secs: %d nanos: %d)\n", static_cast<int32>(offset.secs_), offset.nanos_);
				}

				if(offset.secs_ == 0) {
					//uncorrected_offset_filter_.feed(offset.nanos_);
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

				if((one_way_delay.secs_ != 0) || (ulib::abs(one_way_delay.nanos_) > 50000000)) {
					TRACE("PI Operational: Bad One-Way Delay: %d\n", one_way_delay.nanos_);
				}

				if(one_way_delay.secs_ == 0) {
					//one_way_delay_filter_.feed(one_way_delay.nanos_);
					one_way_delay_buffer_.add(one_way_delay.nanos_);
				}

				if(last_time_.secs_ != 0) {
					int32 offset = uncorrected_offset_buffer_.average() + one_way_delay_buffer_.average();
					//int32 offset = uncorrected_offset_filter_.get() + one_way_delay_filter_.get();
					uint32 dt    = static_cast<uint32>((slave_time - last_time_).to_nanos());
					slave.servo_.feed(dt, offset);
				}

				last_time_ = slave_time;
			}

		}

		Slave::Slave(PtpClock& clock)
			: clock_(clock),
			  servo_(clock),
			  delay_req_id_(4434),
			  sync_serial_(0),
			  sync_state_(slave_detail::SyncState::Initial),
			  dreq_state_(slave_detail::DreqState::Initial)
		{
			clock_.master_tracker().best_master_changed = ulib::function<void()>(this, &Slave::on_best_master_changed);
			servo_.output = ulib::function<void(int32)>(&clock.get_system_port(), &SystemPort::discipline);
			clock_.event_port()->on_transmit_completed = ulib::function<void(uint32, Time)>(this, &Slave::on_delay_request_transmitted);

			states_.to_state<slave_detail::estimating_drift>();
		}

		Slave::~Slave()
		{
			clock_.master_tracker().best_master_changed.reset();
			servo_.output.reset();
			clock_.event_port()->on_transmit_completed.reset();
		}

		void Slave::on_message(const msg::Header& header, PacketHandle packet_handle)
		{
			if (header.is(MessageTypes::Synch)) {
				msg::Sync sync;
				msg::deserialize(packet_handle->get_data(), sync);

				if(header.flag_field0 & uint8(msg::Header::Field0Flags::TwoStep)) {
					on_sync(header.sequence_id, packet_handle->time());
				} else {
					on_sync(header.sequence_id, packet_handle->time(), sync.origin_timestamp);
				}
				send_delay_request();	// no timers yet :(
			} else if( header.is(MessageTypes::FollowUp)) {
				msg::FollowUp follow_up;
				msg::deserialize(packet_handle->get_data(), follow_up);
				on_sync_followup(header.sequence_id, follow_up.precise_origin_timestamp);
			} else if (header.is(MessageTypes::DelayResp)) {
				msg::DelayResp delayresp;
				msg::deserialize(packet_handle->get_data(), delayresp);
				auto& source_identity = header.source_port_identity;
				auto& best_identity   = clock_.master_tracker().best_foreign()->port_identity;
				auto& dresp_identity  = delayresp.port_identity;
				auto& this_identity   = clock_.get_identity();

				if ( source_identity == best_identity && dresp_identity == this_identity )
				{
					on_request_answered(delayresp.timestamp);
				}
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

			msg::serialize(buffer_handle->get_data(), header);
			msg::serialize(buffer_handle->get_data(), dreq);

			buffer_handle->set_size(44);

			port->send((224 << 0) | (0<<8) | (1<<16) | (129 << 24), 319, std::move(buffer_handle), 0xFFFF0000 | delay_req_id_);
		}

		void Slave::on_best_master_changed()
		{
			if(clock_.master_tracker().best_foreign()) {
				clock_.to_state<Slave>();
			} else {
				clock_.to_state<Listening>();
			}
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
				states_.dispatch_self <
					ulib::case_<slave_detail::pi_operational, METHOD(&slave_detail::pi_operational::on_delay)>,
					ulib::case_<slave_detail::estimating_drift, METHOD(&slave_detail::estimating_drift::on_delay)>
				>(*this, dreq_receive, dreq_send_);
				
				dreq_state_ = slave_detail::DreqState::Initial;
			}
		}

		void Slave::on_sync(uint16 serial, const Time& receive_time, const Time& send_time)
		{
			states_.dispatch_self <
				ulib::case_<slave_detail::pi_operational,   METHOD(&slave_detail::pi_operational::on_sync)>,
				ulib::case_<slave_detail::estimating_drift, METHOD(&slave_detail::estimating_drift::on_sync)>
			>(*this, send_time, receive_time);

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
				states_.dispatch_self <
					ulib::case_<slave_detail::pi_operational, METHOD(&slave_detail::pi_operational::on_sync)>,
					ulib::case_<slave_detail::estimating_drift, METHOD(&slave_detail::estimating_drift::on_sync)>
				>(*this, send_time, sync_receive_);

				sync_state_ = slave_detail::SyncState::Initial;
			} else {
				sync_state_ = slave_detail::SyncState::Initial;
			}
		}
	
	}

}
