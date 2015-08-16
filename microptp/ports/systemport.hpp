#ifndef MICROPTP_PORTS_SYSTEMPORT_HPP__
#define MICROPTP_PORTS_SYSTEMPORT_HPP__

#include "microptp_config.hpp"

#ifdef MICROPTP_PORT_CORTEX_M4
#include <microptp/ports/cortex_m4/port.hpp>
#endif

#ifdef MICROPTP_PORT_CORTEX_M4_ONETHREAD
#include <microptp/ports/cortex_m4_onethread/port.hpp>
#endif

#include <microptp/ports/systemportapi.hpp>
#include <microptp/ports/systemport_defaults.hpp>

#endif

/* System Port API

	The system port has to provide three Handle classes:

	Every class ending with "Handle" herafter shall have these semantics:
	[Handle Semantics]
		- Default Initialization
		- Move semantics
		- explicit operator bool
		- operator-> const/nonconst
		- operator*  const/nonconst
		
	- NetHandle
		[Handle Semantics] referencing a [NetRep]

	- PacketHandle
		[Handle Semantics] referencing a [PacketRep]

	- TimerHandle
		[Handle Semantics] referencing a [TimerRep]

	- [NetRep] class referenced by NetHandle
			
		// Called by the system port once a packet has been transmitted, arguments are
		// the packet id provided by the ptp clock instance during the send command
		// and the timestamp of the transmission.
		ulib::function<void(uint32 id, Time timestamp)> on_transmit_completed;

		// Called by the system port when a packet has been received on the
		// udp port represented by this NetHandle.
		ulib::function<void(PacketHandle)> on_received;

		// Indicating if this NetHandle is initialized and functional
		explicit operator bool()

		// Acquire a packet handle for sending
		PacketHandle acquire_transmit_handle();

		// Transmit a data packet [handle] to [ip]:[port]
		// the id is used in the subsequent on_transmit_completed callback
		void send( uint32 ip, uint16 port, PacketHandle handle, uint32 id );

	- [PacketRep]

		// return pointer to data buffer referenced by this packet rep
		void* get_data();

		// return pointer to data buffer referenced by this packet rep
		const void* get_data() const;

		// return capacity of data buffer referenced by this packet rep
		// only used for transmissions on handles returned by
		// [NetHandle]::acquire_transmit_handle
		size_t capacity() const;

		// called to indicate the packet size
		// only used for transmissions on handles returned by
		// [NetHandle]::acquire_transmit_handle
		void set_size(size_t size);

		// Return the timestamp of receival for packets
		// pushed by [NetRep]::on_received
		Time time() const;

	- [TimerRep]

		// start the timer with [msecs] milliseconds timeout
		void start(uint32 msecs);

		// reset the timer with [msecs] milliseconds timout
		void reset(uint32 msecs);

		// stop the timer
		void stop();

		// callbed by the timer on timeout event
		ulib::function<void()> callback;

	- SystemPort

		// Create a [TimerRep] and return a TimerHandle to it
		TimerHandle make_timer();

		// Create a [TimerRep] initalized with given callback funcion
		// and return a TimerHandle to it
		TimerHandle make_timer(ulib::function<void()> func);

		// Create a [NetRep] bound to port [port] and return
		// a NetHandle to it
		NetHandle make_udp(uint16 port);

		// Called to join a given multicast network on the network interface
		void join_multicast(uint32 multicast_addr);

		// Called to leave the multicast network
		void leave_multicast();

		// Get the current time
		Time get_time();

		// Set the time absolute
		void set_time(Time absolute);

		// Offset the time
		void adjust_time(Time delta);

		// Discipline the clock in parts per billion
		void discipline(int32 ppb);

*/