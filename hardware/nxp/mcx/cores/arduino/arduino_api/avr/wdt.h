/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

/** @file avr/wdt.h
 *  avr-libc's watchdog interface, on this board's watchdog (see
 *  mcx_watchdog.h for how each board does it):
 *
 *      #include <avr/wdt.h>
 *
 *      void setup() {
 *        if (MCUSR & _BV(WDRF)) { ... }   // the last reset was the watchdog's
 *        MCUSR = 0;
 *        wdt_enable(WDTO_2S);
 *      }
 *      void loop() {
 *        wdt_reset();
 *        ...
 *      }
 *
 *  Differences from AVR:
 *  - The watchdog can't be stopped once started (the hardware can't be, on
 *    any of these boards). wdt_disable() stops it resetting the board: the
 *    core feeds it from then on.
 *  - While it is enabled, panic() and a HardFault end in a watchdog reset,
 *    once their message is out.
 *  - MCUSR holds the cause of the last reset only (WDRF, PORF, EXTRF,
 *    BORF); on AVR the flags add up over resets until cleared. Writing it
 *    only changes this copy.
 *  - On FRDM-MCXC444, a sketch that includes this header has the COP left
 *    on from reset (1024ms), fed by the core: one that keeps interrupts off
 *    for a second or more is reset by it, enabled or not.
 */

#ifndef MCX_AVR_WDT_H
#define MCX_AVR_WDT_H

#include	"../mcx_watchdog.h"
#include	"../mcx_watchdog_used.h"

/** @name The times wdt_enable() takes, as in avr-libc
 *  On every board here they are exact (15ms is 15ms). */
///@{
#define	WDTO_15MS	0
#define	WDTO_30MS	1
#define	WDTO_60MS	2
#define	WDTO_120MS	3
#define	WDTO_250MS	4
#define	WDTO_500MS	5
#define	WDTO_1S		6
#define	WDTO_2S		7
#define	WDTO_4S		8
#define	WDTO_8S		9
///@}

/** Start the watchdog, or change its time.
 * @param value WDTO_15MS to WDTO_8S; a larger value is taken as WDTO_8S */
static inline void wdt_enable( unsigned char value )
{
	static const unsigned short	ms[]	= { 15, 30, 60, 120, 250, 500, 1000, 2000, 4000, 8000 };

	mcx_wdt_enable( ms[ ( value < 10 ) ? value : 9 ] );
}

/** Feed the watchdog: its time starts again. */
static inline void wdt_reset( void )
{
	mcx_wdt_reset();
}

/** Stop the watchdog resetting the board (it keeps running, fed by the
 *  core). */
static inline void wdt_disable( void )
{
	mcx_wdt_disable();
}

/** @name The cause of the last reset, as AVR's MCUSR and its bits */
///@{
#define	MCUSR	( *mcx_reset_flags() )
#define	PORF	MCX_RESET_PORF
#define	EXTRF	MCX_RESET_EXTRF
#define	BORF	MCX_RESET_BORF
#define	WDRF	MCX_RESET_WDRF
///@}

#endif	// MCX_AVR_WDT_H
