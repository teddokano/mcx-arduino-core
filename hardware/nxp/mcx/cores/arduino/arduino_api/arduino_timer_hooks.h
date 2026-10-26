/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  Hooks into interrupts the core owns, for the bundled MsTimer2 and
 *  TimerOne libraries. Internal: not part of the public API, and not
 *  included from Arduino.h.
 */

#ifndef R01LIB_ARDUINO_TIMER_HOOKS_H
#define R01LIB_ARDUINO_TIMER_HOOKS_H

#include	<stdint.h>

/** Called from SysTick_Handler() (the lowest interrupt priority) with the
 *  milliseconds this tick counted: 1, or more after interrupts were off
 *  for over 1 ms (never 0). nullptr when nothing is hooked (MsTimer2).
 */
extern void	(* volatile systick_hook)( uint32_t ms );

#if defined( CPU_MCXC444VLH )
/** Called from PIT_IRQHandler() when PIT channel 1 has expired, its flag
 *  already cleared. Channel 0 is tone()'s, and the two channels share the
 *  one PIT interrupt. nullptr when nothing is hooked (TimerOne).
 */
extern void	(* volatile pit_channel1_hook)( void );

/** Turns the PIT on, as PIT_Init() does, but leaves both channels as they
 *  are (PIT_Init() stops them), so that tone() and TimerOne can each start
 *  it without stopping the other's channel.
 */
void	pit_module_init( void );
#endif

#endif	// R01LIB_ARDUINO_TIMER_HOOKS_H
