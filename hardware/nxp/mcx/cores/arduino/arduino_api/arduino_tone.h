/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_TONE_H
#define R01LIB_ARDUINO_TONE_H

/** Generate a square wave on the given digital pin (CTIMER0-driven GPIO
 *  toggle, works on any digital pin -- unlike analogWrite(), not limited
 *  to the PWM-capable pins). Only one tone can be active at a time; a new
 *  call takes over CTIMER0/the pin regardless of what was previously
 *  playing.
 *
 * @param pin_num pin to output the tone on
 * @param frequency tone frequency in Hz (0 is a no-op)
 * @param duration (optional) length to play in milliseconds; 0 (default) plays indefinitely until noTone()
 */
void	tone( int pin_num, unsigned int frequency, unsigned long duration = 0 );

/** Stop the tone currently playing on the given pin (no-op if a different
 *  pin, or no pin, is currently playing).
 * @param pin_num pin to stop
 */
void	noTone( int pin_num );

#endif // !R01LIB_ARDUINO_TONE_H
