//          Copyright Michael Steinberg 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <microptp/messages.hpp>
#include <microptp/util/msgutil.hpp>

namespace uptp {

	namespace msg {
		
		namespace {

			template<size_t Offset>
			void serialize(net_buffer buff, const ClockIdentity& host)
			{
				buff.serialize<uint8[8], Offset>(host.identity);
			}

			template<size_t Offset>
			void deserialize(net_const_buffer buff, ClockIdentity& host)
			{
				buff.deserialize<uint8[8], Offset>(host.identity);
			}

			template<size_t Offset>
			void serialize(net_buffer buff, const PortIdentity& host)
			{
				serialize<Offset>(buff, host.clock);
				buff.serialize<uint16, Offset + 8>(host.port);
			}

			template<size_t Offset>
			void deserialize(net_const_buffer buff, PortIdentity& host)
			{
				deserialize<Offset>(buff, host.clock);
				buff.deserialize<uint16, Offset + 8>(host.port);
			}

			template<size_t Offset>
			void serialize(net_buffer buff, const Time& time)
			{
				buff.serialize<uint16, Offset, uint16>((time.secs_ >> 32) & 0xFFFF)
					.serialize<uint32, Offset + 2, uint32>(time.secs_ & 0xFFFFFFFF)
					.serialize<uint32, Offset + 6, uint32>(time.nanos_);
			}

			template<size_t Offset>
			void deserialize(net_const_buffer buff, Time& time)
			{
				uint16 temp16;
				uint32 temp32;

				buff.deserialize<uint16, Offset>(temp16)
					.deserialize<uint32, Offset + 2>(temp32)
					.deserialize<uint32, Offset + 6>(time.nanos_);

				time.secs_ = (static_cast<uint64>(temp16) << 32) | temp32;
			}

			template<size_t Offset>
			void serialize(net_buffer buff, const ClockQuality& quality)
			{
				buff.serialize<uint8, Offset>(quality.clock_class)
					.serialize<uint8, Offset + 1>(quality.clock_accuracy)
					.serialize<uint16, Offset + 2>(quality.offset_scaled_log_variance);
			}

			template<size_t Offset>
			void deserialize(net_const_buffer buff, ClockQuality& quality)
			{
				buff.deserialize<uint8, Offset>(quality.clock_class)
					.deserialize<uint8, Offset + 1>(quality.clock_accuracy)
					.deserialize<uint16, Offset + 2>(quality.offset_scaled_log_variance);
			}

		}

		//
		// Header
		//
		void serialize(net_buffer buff, const Header& host)
		{
			buff
				.serialize<uint4u,  0>(host.transport_specific)
				.serialize<uint4l,  0>(host.message_type)
				.serialize<uint4u,  1>(0)
				.serialize<uint4l,  1>(host.version_ptp)
				.serialize<uint16,  2>(host.message_length)
				.serialize<uint8,   4>(host.domain_number)
				.serialize<uint8,   5>(0)
				.serialize<uint8,   6>(host.flag_field0)
				.serialize<uint8,   7>(host.flag_field1)
				.serialize<uint64,  8>(host.correction_field)
				.serialize<uint32, 16>(0)
				.serialize_nested<PortIdentity, &serialize<20>>(host.source_port_identity)
				.serialize<uint16, 30>(host.sequence_id)
				.serialize<uint8,  32>(host.control_field)
				.serialize<int8,   33>(host.log_message_interval);
		}

		void deserialize(net_const_buffer buff, Header& host)
		{
			buff
				.deserialize<uint4u,  0>(host.transport_specific)
				.deserialize<uint4l,  0>(host.message_type)
				.deserialize<uint4l,  1>(host.version_ptp)
				.deserialize<uint16,  2>(host.message_length)
				.deserialize<uint8,   4>(host.domain_number)
				.deserialize<uint8,   6>(host.flag_field0)
				.deserialize<uint8,   7>(host.flag_field1)
				.deserialize<uint64,  8>(host.correction_field)
				.deserialize_nested <PortIdentity, &deserialize<20>>(host.source_port_identity)
				.deserialize<uint16, 30>(host.sequence_id)
				.deserialize<uint8,  32>(host.control_field)
				.deserialize<int8,   33>(host.log_message_interval);
		}

		//
		// Announce
		// 
		void serialize(net_buffer buff, const Announce& host)
		{
			buff
				.serialize_nested<Time, &serialize<34>>(host.origin_timestamp)
				.serialize<uint16, 44>(host.current_utc_offset)
				.serialize<uint8,  47>(host.grandmaster_priority1)
				.serialize_nested<ClockQuality, &serialize<48>>(host.grandmaster_clock_quality)
				.serialize<uint8, 52>(host.grandmaster_priority2)
				.serialize_nested<ClockIdentity, &serialize<53>>(host.grandmaster_identity)
				.serialize<uint16, 61>(host.steps_removed) // unaligned write!
				.serialize<uint8,  63>(host.time_source);
		}

		void deserialize(net_const_buffer buff, Announce& host)
		{
			buff
				.deserialize_nested<Time, &deserialize<34>>(host.origin_timestamp)
				.deserialize<uint16, 44>(host.current_utc_offset)
				.deserialize<uint8, 47>(host.grandmaster_priority1)
				.deserialize_nested<ClockQuality, &deserialize<48>>(host.grandmaster_clock_quality)
				.deserialize<uint8, 52>(host.grandmaster_priority2)
				.deserialize_nested<ClockIdentity, &deserialize<53>>(host.grandmaster_identity)
				.deserialize<uint16, 61>(host.steps_removed)	// unaligned read!
				.deserialize<uint8, 63>(host.time_source);
		}

		//
		// Sync
		//
		void serialize(net_buffer buff, const Sync& host)
		{
			serialize<34>(buff, host.origin_timestamp);
		}

		void deserialize(net_const_buffer buff, Sync& host)
		{
			deserialize<34>(buff, host.origin_timestamp);
			//buff.deserialize_nested<Time, &deserialize<34>>(host.origin_timestamp);
		}

		//
		// Follow-Up
		//
		void serialize(net_buffer buff, const FollowUp& host)
		{
			serialize<34>(buff, host.precise_origin_timestamp);
		}

		void deserialize(net_const_buffer buff, FollowUp& host)
		{
			deserialize<34>(buff, host.precise_origin_timestamp);
		}

		//
		// PDelayReq
		//
		void serialize(net_buffer buff, const PDelayReq& host)
		{
			serialize<34>(buff, host.timestamp);
		}

		void deserialize(net_const_buffer buff, PDelayReq& host)
		{
			deserialize<34>(buff, host.timestamp);
		}

		//
		// PDelayResp
		//
		void serialize(net_buffer buff, const PDelayResp& host)
		{
			serialize<34>(buff, host.port_identity);
			serialize<44>(buff, host.timestamp);
		}

		void deserialize(net_const_buffer buff, PDelayResp& host)
		{
			deserialize<34>(buff, host.port_identity);
			deserialize<44>(buff, host.timestamp);
		}

		//
		// PDelayRespFollowUp
		//
		void serialize(net_buffer buff, const PDelayRespFollowUp& host)
		{
			serialize<34>(buff, host.precise_origin_timestamp);
		}

		void deserialize(net_const_buffer buff, PDelayRespFollowUp& host)
		{
			deserialize<34>(buff, host.precise_origin_timestamp);
		}

		//
		// DelayReq
		//
		void serialize(net_buffer buff, const DelayReq& host)
		{
			serialize<34>(buff, host.timestamp);
		}

		void deserialize(net_const_buffer buff, DelayReq& host)
		{
			deserialize<34>(buff, host.timestamp);
		}

		//
		// DelayResp
		//
		void serialize(net_buffer buff, const DelayResp& host)
		{
			serialize<34>(buff, host.timestamp);
			serialize<44>(buff, host.port_identity);
		}

		void deserialize(net_const_buffer buff, DelayResp& host)
		{
			deserialize<34>(buff, host.timestamp);
			deserialize<44>(buff, host.port_identity);
		}

	}
}
