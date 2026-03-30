/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"arduino.h"

#define	TARGET_ADDRESS	0x4C

void setup( void )
{
	Serial.begin( 115200 );
	Serial.println( "Hello, world!" );

	Wire.begin();

	pin_mode( BLUE, OUTPUT );
}

void loop( void )
{
	Wire.beginTransmission( TARGET_ADDRESS );
	Wire.write( 0x00 );
	Wire.endTransmission( false );

	int8_t	buf[ 2 ]	= { 0, 0 };
	int		i	= 0;

	Wire.requestFrom( TARGET_ADDRESS, 2 );

	while ( Wire.available() )
		buf[ i++ ]	= Wire.read();

	Serial.print( buf[ 0 ] );
	Serial.print( buf[ 1 ] );
	Serial.print( "       " );

	
	Serial.print( "P3T1085 temperature = " );
	Serial.print( (double)( ( ((int16_t)(buf[ 0 ]) << 8) | buf[ 1 ] ) / 256.0 ) );
	Serial.println( "℃" );

	
	
	Serial.println( "LED - ON" );
	digitalWrite( BLUE, LOW );
	delay( 500 );

	Serial.println( "LED - OFF" );
	digitalWrite( BLUE, HIGH );
	delay( 500 );
}
