/*
 * state_base.hpp
 *
 *  Created on: 10.07.2015
 *      Author: Michael
 */

#ifndef MICROPTP_STATE_BASE_HPP_
#define MICROPTP_STATE_BASE_HPP_

namespace uptp {

	class PacketHandle;

	namespace msg {

		struct Header;

	}

	namespace states {

		class PtpStateBase
		{
		public:
			virtual void on_message(const msg::Header& header, PacketHandle) = 0;
			virtual ~PtpStateBase() {}
		};

	}
}

#endif /* SYSTEM_MICROPTP_STATE_BASE_HPP_ */
