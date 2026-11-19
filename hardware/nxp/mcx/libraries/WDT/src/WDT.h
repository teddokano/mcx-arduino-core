/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

/** @file WDT.h
 *  The Arduino UNO R4's WDT library interface, on this board's watchdog
 *  (the core's mcx_watchdog.h, the same one as <avr/wdt.h>):
 *
 *      #include <WDT.h>
 *
 *      void setup() {
 *        if (!WDT.begin(2000)) { ... }   // 2 seconds
 *      }
 *      void loop() {
 *        WDT.refresh();
 *        ...
 *      }
 *
 *  As on the UNO R4, begin() works once (a second call changes nothing),
 *  and there is no way to stop the watchdog. While it runs, panic() and a
 *  HardFault end in a watchdog reset, once their message is out.
 *  Different from the UNO R4's: the range of begin() (1ms to 67108ms here)
 *  and what getCounter() returns (milliseconds left, not the hardware's
 *  count).
 */

#ifndef MCX_WDT_H
#define MCX_WDT_H

#include	<stdint.h>
#include	"mcx_watchdog.h"
#include	"mcx_watchdog_used.h"

class WDTimer
{
public:
	/** Start the watchdog, once.
	 * @param timeout_ms its time, 1 to 67108 milliseconds
	 * @return 1 if it runs (also when it ran already: the time then stays
	 *         as it was), 0 for a time out of the range */
	int begin( uint32_t timeout_ms )
	{
		if ( _timeout )
			return	1;
		if ( !mcx_wdt_enable( timeout_ms ) )
			return	0;
		_timeout	= timeout_ms;
		return	1;
	}

	/** Feed the watchdog: its time starts again. Does nothing before begin(). */
	void refresh( void )
	{
		if ( _timeout )
			mcx_wdt_reset();
	}

	/** @return the time given to begin(), in milliseconds; 0 before it */
	uint32_t getTimeout( void ) const
	{
		return	_timeout;
	}

	/** @return milliseconds left before the watchdog resets the board; 0
	 *  before begin(). (The UNO R4's returns its hardware's count.) */
	uint32_t getCounter( void ) const
	{
		return	_timeout ? mcx_wdt_remaining() : 0;
	}

private:
	uint32_t	_timeout	= 0;
};

/** The watchdog, as the UNO R4's WDT. */
inline WDTimer	WDT;

#endif	// MCX_WDT_H
