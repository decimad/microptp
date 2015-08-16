//          Copyright Michael Steinberg 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MICROPTP_MESSAGES_HPP__
#define MICROPTP_MESSAGES_HPP__

#include <microptp/ptpdatatypes.hpp>
#include <microptp/util/msgutil.hpp>

namespace uptp {

	enum class MessageTypes : uint8 {
		Synch = 0,
		DelayRequest = 1,
		PeerDelayReq = 2,
		PeerDelayResp = 3,
		FollowUp = 8,
		DelayResp = 9,
		PeerDelayRespFollowUp = 10,
		Announce = 11,
		Signaling = 12,
		Management = 13
	};

	namespace msg {

		//
		// Message types / Header
		// Note that the layout does not match PTP layout, reserved stuff is omitted
		// and members are reordered for better packing
		//
		
		struct Header {
			enum class Field0Flags {
				AlternateMaster = 0x01,
				TwoStep = 0x02,
				Unicast = 0x04,
				ProfileSpecific1 = 0x20,
				ProfileSpecific2 = 0x40,
				Security = 0x80,
			};

			enum class Field1Flags {
				Li61 = 1,
				Li59 = 2,
				UtcValid = 4,
				PtpTimescale = 8,
				TimeTraceable = 16,
				FrequencyTraceable = 32
			};


			PortIdentity source_port_identity;
			int64 correction_field;
			uint16 message_length;
			uint16 sequence_id;
			uint8 transport_specific;
			uint8 message_type;
			uint8 version_ptp;
			uint8 domain_number;
			uint8 flag_field0;
			uint8 flag_field1;
			uint8 control_field;
			int8 log_message_interval;

			bool is(MessageTypes type) const {
				return message_type == static_cast<uint8>(type);
			}
		};
		
		struct Announce {
			Time origin_timestamp;
			ClockQuality grandmaster_clock_quality;
			ClockIdentity grandmaster_identity;
			int16 current_utc_offset;
			uint16 steps_removed;
			
			uint8 grandmaster_priority1;
			uint8 grandmaster_priority2;
		
			enum class TimeSource : uint8 {
				AtomicClock = 0x10,
				GPS = 0x20,
				TerrestrialRadio = 0x30,
				PTP = 0x40,
				NTP = 0x50,
				HandSet = 0x60,
				Other = 0x90,
				InternalOscillator = 0xA0
			};

			enum8 time_source;
		};

		struct Sync
		{
			Time origin_timestamp;
		};

		struct FollowUp
		{
			Time precise_origin_timestamp;
		};

		struct DelayReq
		{
			Time timestamp;
		};

		struct DelayResp
		{
			Time timestamp;
			PortIdentity port_identity;
		};

		struct PDelayReq : public DelayReq
		{
		};

		struct PDelayResp : public DelayResp
		{
		};

		struct PDelayRespFollowUp : public FollowUp
		{
		};

		void serialize(net_buffer buff, const Header&);
		void deserialize(net_const_buffer buff, Header&);

		void serialize(net_buffer buff, const Announce&);
		void deserialize(net_const_buffer buff, Announce&);

		void serialize(net_buffer buff, const Sync&);
		void deserialize(net_const_buffer buff, Sync&);

		void serialize(net_buffer buff, const FollowUp&);
		void deserialize(net_const_buffer buff, FollowUp&);

		void serialize(net_buffer buff, const DelayReq&);
		void deserialize(net_const_buffer buff, DelayReq&);
	
		void serialize(net_buffer buff, const DelayResp&);
		void deserialize(net_const_buffer buff, DelayResp&);
	
		void serialize(net_buffer buff, const PDelayReq&);
		void deserialize(net_const_buffer buff, PDelayReq&);

		void serialize(net_buffer buff, const PDelayResp&);
		void deserialize(net_const_buffer buff, PDelayResp&);

		void serialize(net_buffer buff, const PDelayRespFollowUp&);
		void deserialize(net_const_buffer buff, PDelayRespFollowUp&);
	}

}

#endif
