/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"r01lib.h"
#include	"arduino.h"

#define	MAX_ANALOG_PINS		128
#define	PWM_PERIOD_US		1000	// 1kHz, common Arduino-like PWM rate

static AnalogIn*	analog_in_pins[ MAX_ANALOG_PINS ]	= {};
static PwmOut*		pwm_out_pins[ MAX_ANALOG_PINS ]		= {};

int analogRead( int pin_num )
{
#ifdef	ARDUINO_PIN_RENUMBERING
	pin_num	= arduino_pin_by_number[ pin_num ];
#endif

	if ( pin_num < 0 || pin_num >= MAX_ANALOG_PINS )
		return	0;

	if ( analog_in_pins[ pin_num ] == nullptr )
	{
		analog_in_pins[ pin_num ]	= new AnalogIn( pin_num );

		if ( analog_in_pins[ pin_num ] == nullptr )
			panic( "error @ new, in analogRead()" );
	}

	return	(int)( analog_in_pins[ pin_num ]->read_u16() >> 6 );	// 16bit -> 10bit
}

void analogWrite( int pin_num, int value )
{
#ifdef	ARDUINO_PIN_RENUMBERING
	pin_num	= arduino_pin_by_number[ pin_num ];
#endif

	if ( pin_num < 0 || pin_num >= MAX_ANALOG_PINS )
		return;

	if ( pwm_out_pins[ pin_num ] == nullptr )
	{
		pwm_out_pins[ pin_num ]	= new PwmOut( pin_num );

		if ( pwm_out_pins[ pin_num ] == nullptr )
			panic( "error @ new, in analogWrite()" );

		pwm_out_pins[ pin_num ]->period_us( PWM_PERIOD_US );
	}

	if ( value < 0 )
		value	= 0;
	else if ( value > 255 )
		value	= 255;

	pwm_out_pins[ pin_num ]->write( (float)value / 255.0f );
}
