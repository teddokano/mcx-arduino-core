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

	pin_mode( 0, OUTPUT );
	pin_mode( 1, OUTPUT );
}

void loop( void )
{
	static int	count	= 0;

	digitalWrite( 0, count & 0x1 );
	digitalWrite( 1, count & 0x2 );
	//delay( 1 );

	count++;
}
