/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"r01lib.h"
#include	"Arduino.h"

#define	MAX_ANALOG_PINS		128
#define	PWM_PERIOD_US		1000	// 1kHz, common Arduino-like PWM rate

static AnalogIn*	analog_in_pins[ MAX_ANALOG_PINS ]	= {};
static PwmOut*		pwm_out_pins[ MAX_ANALOG_PINS ]		= {};

// Defaults match classic Arduino (10bit analogRead, 8bit analogWrite).
// Adjustable via analogReadResolution()/analogWriteResolution() (Due/Zero/
// MKR-style extension); LPADC's raw reading is always 16bit internally.
static int	adc_resolution_bits	= 10;
static int	pwm_resolution_bits	= 8;

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

	return	(int)( analog_in_pins[ pin_num ]->read_u16() >> ( 16 - adc_resolution_bits ) );
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

	int	max_value	= ( 1 << pwm_resolution_bits ) - 1;

	if ( value < 0 )
		value	= 0;
	else if ( value > max_value )
		value	= max_value;

	pwm_out_pins[ pin_num ]->write( (float)value / (float)max_value );
}

void analogWriteFrequency( int pin_num, uint32_t frequency )
{
#ifdef	ARDUINO_PIN_RENUMBERING
	pin_num	= arduino_pin_by_number[ pin_num ];
#endif

	if ( pin_num < 0 || pin_num >= MAX_ANALOG_PINS )
		return;

	if ( frequency < 1 )
		frequency	= 1;

	if ( pwm_out_pins[ pin_num ] == nullptr )
	{
		pwm_out_pins[ pin_num ]	= new PwmOut( pin_num );

		if ( pwm_out_pins[ pin_num ] == nullptr )
			panic( "error @ new, in analogWriteFrequency()" );
	}

	pwm_out_pins[ pin_num ]->period_us( (int)( 1000000UL / frequency ) );
}

void analogReference( uint8_t mode )
{
	(void)mode;	// no-op: this board's ADC reference voltage is fixed in hardware
}

void analogReadResolution( int bits )
{
	if ( bits < 1 )
		bits	= 1;
	else if ( bits > 16 )
		bits	= 16;	// LPADC's native resolution

	adc_resolution_bits	= bits;
}

void analogWriteResolution( int bits )
{
	if ( bits < 1 )
		bits	= 1;
	else if ( bits > 16 )
		bits	= 16;

	pwm_resolution_bits	= bits;
}
