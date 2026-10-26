/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  MsTimer2's interface: calls a function every given number of
 *  milliseconds, from an interrupt. Written from scratch to the MsTimer2
 *  library's interface (none of its source, LGPL 2.1 or later, is copied), so
 *  sketches written for it build unchanged.
 *
 *  On AVR it runs on Timer2. Here it runs on the core's 1 ms SysTick
 *  interrupt, which millis() already uses, so it takes no timer and no
 *  pin, and works the same on every board.
 */

#ifndef MCX_MSTIMER2_H
#define MCX_MSTIMER2_H

#include	<Arduino.h>

namespace MsTimer2 {
	/** The period set(), in milliseconds (never 0). */
	extern unsigned long			msecs;
	/** The function set() gave. */
	extern void						(*func)();
	/** Milliseconds counted toward the next call. */
	extern volatile unsigned long	count;
	/** Nonzero while func runs. */
	extern volatile char			overflowing;

	/** Stops the timer, and sets func to be called every ms milliseconds
	 *  once start() runs. A period of 0 is taken as 1.
	 *
	 *  func runs in the SysTick interrupt, at the lowest priority: other
	 *  interrupts, Serial's among them, still run while it does, and
	 *  millis() reads the time of this tick. Keep it short, as on AVR.
	 * @param ms period in milliseconds
	 * @param f function to call
	 */
	void	set( unsigned long ms, void (*f)() );

	/** Starts the timer: the first call comes ms milliseconds later. */
	void	start();

	/** Stops the timer. start() begins a fresh period. */
	void	stop();

	/** Counts one millisecond, and calls func when the period is up. Called
	 *  by the timer; a call missed because func ran long is made up for at
	 *  the next milliseconds, as on AVR.
	 */
	void	_overflow();
}

#endif	// MCX_MSTIMER2_H
