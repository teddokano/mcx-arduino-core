/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include        "r01lib.h"
#include        "arduino.h"

#define MAX_DIGITAL_PINS    128

static DigitalInOut*    digital_pins[ MAX_DIGITAL_PINS ]    = {};
static InterruptIn*     interrupt_pins[ MAX_DIGITAL_PINS ]  = {};

void pinMode( int pin_num, int mode )
{
#ifdef  ARDUINO_PIN_RENUMBERING
		pin_num = arduino_pin_by_number[ pin_num ];
#endif

		if ( pin_num < 0 || pin_num >= MAX_DIGITAL_PINS )
				return;

		int     dir       = (mode == OUTPUT || mode == OUTPUT_OPENDRAIN) ? DigitalInOut::OUTPUT : DigitalInOut::INPUT;
		int     pin_mode  = DigitalInOut::PullNone;

		if ( mode == INPUT_PULLUP )
				pin_mode = DigitalInOut::PullUp;
		else if ( mode == INPUT_PULLDOWN )
				pin_mode = DigitalInOut::PullDown;
		else if ( mode == OUTPUT_OPENDRAIN )
				pin_mode = DigitalInOut::OpenDrain;

		if ( digital_pins[ pin_num ] != nullptr )
		{
				if ( dir == OUTPUT )
						digital_pins[ pin_num ]->output();
				else
						digital_pins[ pin_num ]->input();

				digital_pins[ pin_num ]->mode( pin_mode );
		}
		else
		{
				digital_pins[ pin_num ] = new DigitalInOut( pin_num, dir, 0, pin_mode );

				if ( digital_pins[ pin_num ] == nullptr )
						panic( "error @ new, in pinMode()" );
		}
}

void digitalWrite( int pin_num, bool state )
{
#ifdef  ARDUINO_PIN_RENUMBERING
		pin_num = arduino_pin_by_number[ pin_num ];
#endif

		if ( pin_num < 0 || pin_num >= MAX_DIGITAL_PINS )
				return;

		if ( digital_pins[ pin_num ] != nullptr )
				*(digital_pins[ pin_num ]) = state;
}

bool digitalRead( int pin_num )
{
#ifdef  ARDUINO_PIN_RENUMBERING
		pin_num = arduino_pin_by_number[ pin_num ];
#endif

		if ( pin_num < 0 || pin_num >= MAX_DIGITAL_PINS )
				return false;

		if ( digital_pins[ pin_num ] != nullptr )
				return *(digital_pins[ pin_num ]);

		return false;
}

void attachInterrupt( int pin_num, void (*callback)(void), int mode )
{
#ifdef  ARDUINO_PIN_RENUMBERING
		pin_num = arduino_pin_by_number[ pin_num ];
#endif

		if ( pin_num < 0 || pin_num >= MAX_DIGITAL_PINS )
				return;

		InterruptIn* int_pin    = interrupt_pins[ pin_num ];

		if ( int_pin == nullptr )
		{
				int_pin    = new InterruptIn( pin_num );

				if ( int_pin == nullptr )
						panic( "error @ new, in attachInterrupt()" );

				interrupt_pins[ pin_num ]    = int_pin;
		}

		switch ( mode )
		{
				case    RISING:
						int_pin->rise( callback );
						break;

				case    FALLING:
						int_pin->fall( callback );
						break;

				case    CHANGE:
						int_pin->rise( callback );
						int_pin->fall( callback );
						break;

				case    LOW:
						int_pin->low( callback );
						break;

				default:
						panic( "error @ attachInterrupt(), unknown mode" );
						break;
		}
}

void detachInterrupt( int pin_num )
{
#ifdef  ARDUINO_PIN_RENUMBERING
		pin_num = arduino_pin_by_number[ pin_num ];
#endif

		if ( pin_num < 0 || pin_num >= MAX_DIGITAL_PINS )
				return;

		if ( interrupt_pins[ pin_num ] != nullptr )
				interrupt_pins[ pin_num ]->disable();
}

int digitalPinToInterrupt( int pin_num )
{
		return  pin_num;
}

void shiftOut( int dataPin, int clockPin, int bitOrder, uint8_t val )
{
		for ( int i = 0; i < 8; i++ )
		{
				if ( bitOrder == LSBFIRST )
						digitalWrite( dataPin, !!(val & (1 << i)) );
				else
						digitalWrite( dataPin, !!(val & (1 << (7 - i))) );

				digitalWrite( clockPin, HIGH );
				digitalWrite( clockPin, LOW );
		}
}

uint8_t shiftIn( int dataPin, int clockPin, int bitOrder )
{
		uint8_t	value	= 0;

		for ( int i = 0; i < 8; i++ )
		{
				digitalWrite( clockPin, HIGH );

				if ( bitOrder == LSBFIRST )
						value |= digitalRead( dataPin ) << i;
				else
						value |= digitalRead( dataPin ) << (7 - i);

				digitalWrite( clockPin, LOW );
		}

		return	value;
}

unsigned long pulseIn( int pin_num, bool state, unsigned long timeout )
{
		unsigned long	start_wait	= micros();

		while ( digitalRead( pin_num ) == state )
				if ( (micros() - start_wait) > timeout )
						return	0;

		while ( digitalRead( pin_num ) != state )
				if ( (micros() - start_wait) > timeout )
						return	0;

		unsigned long	start_pulse	= micros();

		while ( digitalRead( pin_num ) == state )
				if ( (micros() - start_pulse) > timeout )
						return	0;

		return	micros() - start_pulse;
}

unsigned long pulseInLong( int pin_num, bool state, unsigned long timeout )
{
		return	pulseIn( pin_num, state, timeout );
}
