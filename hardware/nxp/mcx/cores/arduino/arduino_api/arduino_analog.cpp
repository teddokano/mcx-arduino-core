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

// Sketches speak Arduino's renumbering; the pin tables and peripheral
// classes below speak r01lib's raw pin values. Out of range becomes -1,
// which every caller below already treats as "no such pin".
static int raw_pin( int pin_num )
{
#ifdef	ARDUINO_PIN_RENUMBERING
	constexpr int	count	= (int)( sizeof( arduino_pin_by_number ) / sizeof( arduino_pin_by_number[0] ) );

	return	( 0 <= pin_num && pin_num < count ) ? arduino_pin_by_number[ pin_num ] : -1;
#else
	return	pin_num;
#endif
}

// Set by pinMode() (through analog_pin_to_gpio()) on a pin whose PwmOut it
// switched to GPIO; the next analogWrite() puts the pin back on FlexPWM
static bool		pwm_pin_released[ MAX_ANALOG_PINS ]	= {};

// arduino_io.cpp: drops the pin's pinMode() GPIO object, if it has one
bool	digital_pin_drop( int raw_pin );

// The PwmOut driving this pin, made on first use. nullptr means FlexPWM
// cannot reach it: PWM0-PWM5 on every board, and D3/D5/D6/D9 on
// FRDM-MCXA156 (on FRDM-MCXN236 those are PWM0-PWM5 pins themselves).
// Asking PwmOut directly instead would panic() -- hence is_pwm_pin().
static PwmOut* pwm_for( int pin )
{
	if ( pin < 0 || pin >= MAX_ANALOG_PINS || !PwmOut::is_pwm_pin( pin ) )
		return	nullptr;

	if ( pwm_out_pins[ pin ] == nullptr )
	{
		//	A pin whose submodule partner is already running joins it at
		//	its period, which analogWriteFrequency() may have set: putting
		//	the default back here would change the partner's frequency.
		bool	joins_running	= PwmOut::shares_running_period( pin );

		pwm_out_pins[ pin ]	= new PwmOut( pin );

		if ( pwm_out_pins[ pin ] == nullptr )
			panic( "error @ new, in analogWrite()" );

		if ( !joins_running )
			pwm_out_pins[ pin ]->period_us( PWM_PERIOD_US );
	}

	return	pwm_out_pins[ pin ];
}

//	Called by pinMode() before it switches a pin to GPIO. A PwmOut on the
//	pin stops owning it, and analogWrite() takes it back.
void analog_pin_to_gpio( int raw_pin )
{
	if ( raw_pin < 0 || raw_pin >= MAX_ANALOG_PINS || pwm_out_pins[ raw_pin ] == nullptr )
		return;

	pwm_out_pins[ raw_pin ]->release_pin();
	pwm_pin_released[ raw_pin ]	= true;
}

int analogRead( int pin_num )
{
	int	pin	= raw_pin( pin_num );

	if ( pin < 0 || pin >= MAX_ANALOG_PINS )
		return	0;

#if	defined( FRDM_MCXN236 )
	//	A3 is also the blue LED's pin, which cannot be cut off, so the core
	//	keeps it a digital pin. The chip does have an ADC input there, so
	//	rather than stopping in panic() as for a pin with none, say so with
	//	a value no reading at any resolution can take.
	if ( pin == arduino_pin_by_number[ A3 ] )
		return	-1;
#endif

	if ( analog_in_pins[ pin ] == nullptr )
	{
		analog_in_pins[ pin ]	= new AnalogIn( pin );

		if ( analog_in_pins[ pin ] == nullptr )
			panic( "error @ new, in analogRead()" );
	}

	return	(int)( analog_in_pins[ pin ]->read_u16() >> ( 16 - adc_resolution_bits ) );
}

void analogWrite( int pin_num, int value )
{
	const int	max_value	= ( 1 << pwm_resolution_bits ) - 1;

	if ( value < 0 )
		value	= 0;
	else if ( value > max_value )
		value	= max_value;

	int	pin	= raw_pin( pin_num );

	if ( PwmOut *pwm = pwm_for( pin ) )
	{
		//	The pin is FlexPWM's from here on: a pinMode() GPIO object
		//	made before would still claim it, and a pinMode() since the
		//	last analogWrite() has switched it to GPIO
		digital_pin_drop( pin );

		if ( pwm_pin_released[ pin ] )
		{
			pwm->claim_pin();
			pwm_pin_released[ pin ]	= false;
		}

		pwm->write( (float)value / (float)max_value );
		return;
	}

	//	Landing here is the normal case, not an error: no D-pin on N947
	//	reaches FlexPWM at all, and on A153 only D3/D7 do, on the channels
	//	PWM5/PWM4 already own -- so analogWrite( 9, ... ), written for a
	//	classic Arduino, can be PWM only on FRDM-MCXA156 and FRDM-MCXN236
	//	(D3/D5/D6/D9 there). Elsewhere, drive the pin
	//	high or low instead, as AVR's core does for a pin with no timer
	//	behind it (wiring_analog.c, "case NOT_ON_TIMER"). The midpoint
	//	follows analogWriteResolution() rather than AVR's hardcoded 128.
	//	pin_num is still the sketch's own numbering here -- pinMode() and
	//	digitalWrite() renumber it themselves.
	pinMode( pin_num, OUTPUT );	//	this core needs it before digitalWrite()
	digitalWrite( pin_num, value >= ( max_value + 1 ) / 2 );
}

void analogWriteFrequency( int pin_num, uint32_t frequency )
{
	PwmOut	*pwm	= pwm_for( raw_pin( pin_num ) );

	//	Unlike analogWrite(), there is nothing to fall back to -- a plain
	//	GPIO has no period. This is also this core's own extension rather
	//	than a standard Arduino call, so no ported sketch reaches it by
	//	accident: asking for it on a pin that cannot do PWM is a mistake
	//	worth saying out loud, as every other unsupported-pin case here does.
	if ( pwm == nullptr )
		panic( "analogWriteFrequency: pin has no PWM -- use PWM0-PWM5 (or D3/D5/D6/D9 on FRDM-MCXA156/N236)" );

	if ( frequency < 1 )
		frequency	= 1;

	pwm->period_us( (int)( 1000000UL / frequency ) );
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
