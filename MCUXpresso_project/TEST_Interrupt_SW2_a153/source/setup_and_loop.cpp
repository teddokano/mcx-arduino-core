/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"arduino.h"

volatile bool	sw_pressed	= false;
bool			led_state	= true;

void callback( void )
{
	sw_pressed	= true;
}

void setup( void )
{
	Serial.begin( 115200 );
	Serial.println( "Hello, world!" );
	Serial.println( "Press SW2 to generate INT and toggle LED" );

	pin_mode( BLUE, OUTPUT );
	pin_mode( SW2, INPUT_PULLUP );
	
	attachInterrupt( digitalPinToInterrupt( SW2 ), callback, FALLING );
	digitalWrite( BLUE, led_state );
}

void loop( void )
{
	if ( sw_pressed )
	{
		sw_pressed	= false;
		led_state	= !led_state;
		digitalWrite( BLUE, led_state );
		Serial.println( "SW2 has been pressed\r\n" );
		delay( 100 );	//	debounce
	}
}
