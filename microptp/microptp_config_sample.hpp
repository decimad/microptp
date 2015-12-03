#ifndef CONFIG_MICROPTP_CONFIG_HPP__
#define CONFIG_MICROPTP_CONFIG_HPP__

#include <config/config.h>

#ifndef STMLIB_LWIP_ONETHREAD
#define MICROPTP_PORT_CORTEX_M4
#else
#define MICROPTP_PORT_CORTEX_M4_ONETHREAD
#endif

#include <stmlib/trace.h>

#define PRINT(Args...) trace_printf(0, Args)

#ifdef _DEBUG
#define TRACE(...) trace_printf(0, __VA_ARGS__)
#else
#define TRACE(...)
#endif

#endif
