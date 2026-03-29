/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"r01lib.h"
#include	"arduino.h"

#include	<map>

std::map<int, DigitalInOut*>	digital_pins;

void pin_mode( int pin_num, int mode )
{
#ifdef	ARDUINO_PIN_RENUMBERING
	pin_num	= arduino_pin_by_number[ pin_num ];
#endif
	
	int	dir		= mode & 0x1;
	int	pull	= mode & DigitalInOut::PullUp;

	auto	it	= digital_pins.find( pin_num );
	
	if ( it != digital_pins.end() )
	{
		if ( dir == OUTPUT )
			(digital_pins[ pin_num ])->output();
		else
			(digital_pins[ pin_num ])->input();
	}
	else
	{	
		digital_pins[ pin_num ]	= new DigitalInOut( pin_num, dir, 0, pull );

		if ( digital_pins[ pin_num ] == nullptr )
			panic( "error @ new, in pin_mode()" );
	}
}

void digitalWrite( int pin_num, bool state )
{
#ifdef	ARDUINO_PIN_RENUMBERING
	pin_num	= arduino_pin_by_number[ pin_num ];
#endif
	
	auto	it	= digital_pins.find( pin_num );
	
	if ( it != digital_pins.end() )
		*(digital_pins[ pin_num ])	= state;
}

bool digitalRead( int pin_num )
{
#ifdef	ARDUINO_PIN_RENUMBERING
	pin_num	= arduino_pin_by_number[ pin_num ];
#endif
	
	auto	it	= digital_pins.find( pin_num );
	
	if ( it != digital_pins.end() )
		return	*(digital_pins[ pin_num ]);
	else
		return	false;
}

void attachInterrupt( int pin_num, void (*callback)(void), int mode )
{
#ifdef	ARDUINO_PIN_RENUMBERING
	pin_num	= arduino_pin_by_number[ pin_num ];
#endif
	
	InterruptIn* int_pin	= new InterruptIn( pin_num );

	if ( int_pin == nullptr )
		panic( "error @ new, in attachInterrupt()" );

	switch ( mode )
	{
		case	RISING:
			int_pin->rise( callback );
			break;

		case	FALLING:
			int_pin->fall( callback );
			break;

		case	CHANGE:
			int_pin->rise( callback );
			int_pin->fall( callback );
			break;

		default:
			panic( "error @ attachInterrupt(), unknown mode" );
			break;
	}
}

int digitalPinToInterrupt( int pin_num )
{
	return	pin_num;
}
