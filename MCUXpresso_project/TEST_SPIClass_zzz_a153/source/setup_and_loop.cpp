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

	SPI_.begin();

	pin_mode( BLUE, OUTPUT );
}

void loop( void )
{
	uint8_t	data[]	= { 0x00, 0x01, 0x02, 0x03, 0xFC, 0xFD, 0xFE, 0xFF };
	
	SPI_.beginTransaction( SPISettings( 1000000, MSBFIRST, SPI_MODE0 ) );
	digitalWrite( SS, LOW );
	SPI_.transfer( data, sizeof( data ) );
	digitalWrite( SS, HIGH );
	
	for ( auto b : data )
	{
		Serial.print( "0x" );
		if ( b < 0x10 )
			Serial.print( '0' );
		Serial.print( b, HEX );
		Serial.print( ' ' );
	}
	
	Serial.println( "LED - ON" );
	digitalWrite( BLUE, LOW );
	delay( 500 );

	Serial.println( "LED - OFF" );
	digitalWrite( BLUE, HIGH );
	delay( 500 );
}
