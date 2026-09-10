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
	// Keep the pin as the sketch wrote it: pinMode()/digitalWrite() do
	// their own renumbering, so handing them the raw value below would
	// renumber it a second time.
	const int	arduino_pin	= pin_num;

#ifdef	ARDUINO_PIN_RENUMBERING
	pin_num	= arduino_pin_by_number[ pin_num ];
#endif

	if ( pin_num < 0 || pin_num >= MAX_ANALOG_PINS )
		return;

	int	max_value	= ( 1 << pwm_resolution_bits ) - 1;

	if ( value < 0 )
		value	= 0;
	else if ( value > max_value )
		value	= max_value;

	// FlexPWM reaches only the six dedicated PWM0-PWM5 pins on either
	// board -- no D-pin on N947 has a FlexPWM alternate function at all,
	// and on A153 only D3/D7 do, on channels PWM5/PWM4 already use. So
	// analogWrite() on a pin a sketch written for a classic Arduino would
	// expect to work (analogWrite(9, ...) and friends) cannot produce
	// PWM here, and used to reach PwmOut's constructor, which panic()s --
	// turning a routine porting mistake into a dead sketch flashing SOS.
	//
	// Fall back to digitalWrite instead, which is what AVR's own core
	// does for a pin with no timer behind it (wiring_analog.c: `case
	// NOT_ON_TIMER: if (val < 128) digitalWrite(pin, LOW) else HIGH`).
	// The threshold is the midpoint of the *current* write resolution,
	// not a hardcoded 128, so it still means "half" after
	// analogWriteResolution() has changed the scale.
	//
	// (The other reference core, UNO R4's renesas, silently does nothing
	// in this situation. Either beats panicking; something observable was
	// chosen over silence because it tells you PWM isn't happening.)
	if ( !PwmOut::is_pwm_pin( pin_num ) )
	{
		pinMode( arduino_pin, OUTPUT );	// this core needs it before digitalWrite
		digitalWrite( arduino_pin, value >= ( ( max_value + 1 ) / 2 ) );
		return;
	}

	if ( pwm_out_pins[ pin_num ] == nullptr )
	{
		pwm_out_pins[ pin_num ]	= new PwmOut( pin_num );

		if ( pwm_out_pins[ pin_num ] == nullptr )
			panic( "error @ new, in analogWrite()" );

		pwm_out_pins[ pin_num ]->period_us( PWM_PERIOD_US );
	}

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

	// Same guard as analogWrite()'s, but with nothing to fall back to:
	// "period" has no meaning for a plain GPIO, so a pin FlexPWM can't
	// reach is simply ignored rather than panicking in PwmOut.
	if ( !PwmOut::is_pwm_pin( pin_num ) )
		return;

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
