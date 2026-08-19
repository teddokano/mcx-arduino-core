/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_ANALOG_H
#define R01LIB_ARDUINO_ANALOG_H

#include	<stdint.h>

int		analogRead( int pin_num );
void	analogWrite( int pin_num, int value );

/*
 *  analogWriteFrequency() is not part of the official Arduino API --
 *  it's a de facto extension some vendor cores provide (e.g. Teensy's
 *  analogWriteFrequency(pin, freq)), included here for the same reason:
 *  official Arduino never standardized PWM frequency control since AVR's
 *  timers can't do it cleanly.
 *
 *  Sets the given pin's PWM period to 1/frequency, preserving its
 *  current pulse width in microseconds (same semantics as PwmOut::
 *  period()/period_us() -- the duty cycle *ratio* is not preserved
 *  across a frequency change, only the absolute pulse width, clamped to
 *  the new period if it no longer fits).
 *
 *  Caveat: PWM0-5 pair up two-to-a-FlexPWM-submodule (PWM0/PWM1,
 *  PWM2/PWM3, PWM4/PWM5), and a submodule's period register is shared by
 *  both channels -- changing one pin's frequency changes its paired
 *  pin's frequency too. See this board's PIN_MAPPING_*.md.
 */
void	analogWriteFrequency( int pin_num, uint32_t frequency );

/*
 *  analogReference() is a no-op on this board: the LPADC's reference
 *  voltage source is fixed in hardware (VDDA, see AnalogIn.cpp) and isn't
 *  switchable, unlike AVR-family boards. Declared for sketch compatibility
 *  only -- calling it does nothing.
 */
void	analogReference( uint8_t mode );

void	analogReadResolution( int bits );
void	analogWriteResolution( int bits );

#endif // !R01LIB_ARDUINO_ANALOG_H
