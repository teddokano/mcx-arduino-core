/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  TimerOne's interface: a timer with a period in microseconds that can
 *  call a function from its interrupt. Written from scratch to the TimerOne
 *  library's interface (none of its source, CC BY 3.0, is copied), so
 *  sketches written for it build unchanged.
 *
 *  On AVR it runs on the 16-bit Timer1, and its longest period is about
 *  8.4 s. Here it runs on CTIMER1 on the MCX A and N chips (12 MHz,
 *  longest period about 357 s), and on PIT channel 1 on FRDM-MCXC444 (the
 *  24 MHz bus clock, about 178 s). A longer period is cut to the longest.
 *  Both count a clock made from the same oscillator as the core clock, so
 *  the periods agree with millis() and micros(), as on AVR.
 *
 *  The PWM functions (pwm(), setPwmDuty() and disablePwm()) are not
 *  provided: on AVR they drive pins 9 and 10 from Timer1, which have no
 *  counterpart here. Use analogWrite() and analogWriteFrequency() on the
 *  PWM pins instead.
 */

#ifndef MCX_TIMERONE_H
#define MCX_TIMERONE_H

#include	<Arduino.h>

class TimerOne
{
public:
	/** Sets the period and starts the timer from 0. Any other call made
	 *  before this one first does initialize() with the default period.
	 * @param microseconds period (1 s if left out)
	 */
	void	initialize( unsigned long microseconds = 1000000 );

	/** Changes the period, and leaves the timer running (it starts it if it
	 *  was stopped), as on AVR. The next period is the new one; on
	 *  FRDM-MCXC444 it is counted from now.
	 * @param microseconds period
	 */
	void	setPeriod( unsigned long microseconds );

	/** Starts the timer from the beginning of a period. */
	void	start();

	/** Stops the timer where it is. resume() goes on from there. */
	void	stop();

	/** Same as start(). */
	void	restart();

	/** Goes on from where stop() left the timer. */
	void	resume();

	/** Calls isr at the end of every period, from the interrupt, at the
	 *  lowest priority (as millis()'s SysTick): other interrupts, Serial's
	 *  among them, still run while it does. Keep it short, as on AVR.
	 *  A period that ended while no function was attached is called for
	 *  at once, as on AVR.
	 * @param isr function to call
	 */
	void	attachInterrupt( void (*isr)() );

	/** setPeriod( microseconds ) (when microseconds is more than 0),
	 *  then attachInterrupt( isr ).
	 * @param isr function to call
	 * @param microseconds period
	 */
	void	attachInterrupt( void (*isr)(), unsigned long microseconds );

	/** Stops calling the function. The timer keeps running. */
	void	detachInterrupt();

	/** The function the interrupt calls. */
	static void	(*isrCallback)();

	/** The function isrCallback holds until one is attached: does nothing. */
	static void	isrDefaultUnused();

	//	Not provided: AVR drives pins 9 and 10 from Timer1 for these. Use
	//	analogWrite() and analogWriteFrequency() on the PWM pins instead
	void	pwm( char pin, unsigned int duty )								= delete;
	void	pwm( char pin, unsigned int duty, unsigned long microseconds )	= delete;
	void	setPwmDuty( char pin, unsigned int duty )						= delete;
	void	disablePwm( char pin )											= delete;
};

extern TimerOne	Timer1;

#endif	// MCX_TIMERONE_H
