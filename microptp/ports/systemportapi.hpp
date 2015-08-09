/*
 * systemportapi.hpp
 *
 *  Created on: 08.08.2015
 *      Author: Michael
 */
#ifndef MICROPTP_PORTS_SYSTEMPORTAPI_HPP__
#define MICROPTP_PORTS_SYSTEMPORTAPI_HPP__

#include <microptp_config.hpp>

#ifdef MICROPTP_PORT_CORTEX_M4
#include <microptp/ports/cortex_m4/port_types.hpp>
#endif

#ifdef MICROPTP_PORT_CORTEX_M4_ONETHREAD
#include <microptp/ports/cortex_m4_onethread/port_types.hpp>
#endif

#endif



