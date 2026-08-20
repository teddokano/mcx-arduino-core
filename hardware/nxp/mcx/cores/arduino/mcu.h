/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_MCU_H
#define R01LIB_MCU_H

#include "r01lib.h"

/** Chip-level bring-up: boot pins, boot clocks, peripheral clock
 *  attach/enable, and (per-CPU) debug console init. Called exactly once,
 *  automatically, by Obj's constructor the first time any r01lib
 *  peripheral object is created -- not normally called directly.
 */
void	init_mcu( void );

/** Busy-wait for at least the given duration.
 * @param delayTime_sec delay in seconds (fractional; e.g. 0.001 = 1ms)
 */
void	wait( double delayTime_sec );

/** Busy-wait for at least the given duration.
 * @param milloseconds delay in milliseconds
 */
void	wait_ms( unsigned int milloseconds );

/** Busy-wait for at least the given duration.
 * @param microseconds delay in microseconds
 */
void	wait_us( unsigned int microseconds );

/** Report a fatal error and hang.
 *
 *  Prints @p s, then blinks the on-board RGB LEDs in SOS Morse code
 *  ("... --- ...") forever. Never returns.
 *
 * @param s error message to print before entering the blink loop
 */
void 	panic( const char *s );


#endif // R01LIB_MCU_H
