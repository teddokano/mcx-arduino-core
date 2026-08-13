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
 *  analogReference() is a no-op on this board: the LPADC's reference
 *  voltage source is fixed in hardware (VDDA, see AnalogIn.cpp) and isn't
 *  switchable, unlike AVR-family boards. Declared for sketch compatibility
 *  only -- calling it does nothing.
 */
void	analogReference( uint8_t mode );

void	analogReadResolution( int bits );
void	analogWriteResolution( int bits );

#endif // !R01LIB_ARDUINO_ANALOG_H
