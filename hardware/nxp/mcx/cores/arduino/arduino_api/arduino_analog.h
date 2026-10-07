/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_ANALOG_H
#define R01LIB_ARDUINO_ANALOG_H

#include	<stdint.h>

/** Read an analog input pin. Lazily creates the pin's AnalogIn instance on
 *  first use. Calls panic() if pin_num isn't one of this board's
 *  analog-capable pins, except A3 on FRDM-MCXN236 (the blue LED's pin),
 *  which returns -1 instead.
 * @param pin_num analog pin (A0..A5, board-dependent which are wired)
 * @return raw ADC reading, scaled to analogReadResolution() bits (10 by default)
 */
int		analogRead( int pin_num );

/** Set a PWM output pin's duty cycle. Lazily creates the pin's PwmOut
 *  instance on first use, with the default 1kHz period -- or, if the other
 *  pin of its FlexPWM submodule is already running, at that pin's period.
 *  After pinMode() has made the pin GPIO, this puts it back on FlexPWM.
 *  On a pin with no PWM it drives the pin HIGH or LOW instead, as AVR's
 *  core does.
 * @param pin_num PWM pin (PWM0..PWM5, and D3/D5/D6/D9 on FRDM-MCXA156 and FRDM-MCXN236)
 * @param value duty cycle, scaled to analogWriteResolution() bits (8 by default, so 0..255)
 */
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
 *  PWM2/PWM3, PWM4/PWM5; on FRDM-MCXA156 also D6/D9), and a submodule's
 *  period register is shared by both channels -- changing one pin's
 *  frequency changes its paired pin's frequency too, and the paired pin
 *  keeps its pulse width in microseconds as above. analogWrite() on
 *  either pin afterwards sets its duty at the shared frequency. See this
 *  board's PIN_MAPPING_*.md.
 *
 * @param pin_num PWM pin (PWM0..PWM5, and D3/D5/D6/D9 on FRDM-MCXA156 and FRDM-MCXN236)
 * @param frequency new PWM frequency in Hz; call this before analogWrite() to set duty at the new rate
 */
void	analogWriteFrequency( int pin_num, uint32_t frequency );

/*
 *  analogReference() is a no-op on this board: the LPADC's reference
 *  voltage source is fixed in hardware (VDDA, see AnalogIn.cpp) and isn't
 *  switchable, unlike AVR-family boards. Declared for sketch compatibility
 *  only -- calling it does nothing.
 *
 * @param mode ignored
 */
void	analogReference( uint8_t mode );

/** @name analogReference() mode names
 *  Accepted and ignored (see above), so sketches that pass them compile.
 *  DEFAULT/INTERNAL/EXTERNAL take ArduinoCore-avr's values; AR_DEFAULT/
 *  AR_INTERNAL/AR_EXTERNAL are the names the UNO R4 and other 32-bit cores
 *  use instead.
 */
///@{
#define	DEFAULT		1
#define	INTERNAL	3
#define	EXTERNAL	0
#define	AR_DEFAULT	DEFAULT
#define	AR_INTERNAL	INTERNAL
#define	AR_EXTERNAL	EXTERNAL
///@}

/** Change analogRead()'s return value scaling.
 * @param bits result resolution, clamped to 1..16
 */
void	analogReadResolution( int bits );

/** Change analogWrite()'s duty-cycle value scaling.
 * @param bits duty resolution, clamped to 1..16 (so max duty value is (1<<bits)-1)
 */
void	analogWriteResolution( int bits );

#endif // !R01LIB_ARDUINO_ANALOG_H
