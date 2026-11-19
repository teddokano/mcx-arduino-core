/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

/** @file mcx_watchdog_used.h
 *  Included by <avr/wdt.h> and the WDT library's WDT.h: defines
 *  mcx_sketch_wdt_event(), through which the core hands the watchdog its
 *  events (see mcx_watchdog.h). Weak, so that every file of a sketch can
 *  include it. Internal.
 */

#ifndef MCX_WATCHDOG_USED_H
#define MCX_WATCHDOG_USED_H

#include	"mcx_watchdog.h"

#ifdef __cplusplus
extern "C"
#endif
__attribute__((weak)) void mcx_sketch_wdt_event( int event )
{
	mcx_wdt_event( event );
}

#endif	// MCX_WATCHDOG_USED_H
