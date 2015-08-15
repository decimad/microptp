//          Copyright Michael Steinberg 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

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
