/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"arduino.h"

void setup( void )
{
	Serial.begin( 115200 );
	Serial.println( "Hello, world!" );

	pin_mode( BLUE, OUTPUT );
}

void loop( void )
{
	Serial.println( "LED - ON" );
	digitalWrite( BLUE, LOW );
	delay( 500 );

	Serial.println( "LED - OFF" );
	digitalWrite( BLUE, HIGH );
	delay( 500 );
}
