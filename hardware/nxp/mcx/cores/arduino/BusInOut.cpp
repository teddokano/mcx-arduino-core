/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include "r01lib.h"
#include "BusInOut.h"

BusInOut::BusInOut(
				   uint8_t bit0,
				   uint8_t bit1,
				   uint8_t bit2,
				   uint8_t bit3,
				   uint8_t bit4,
				   uint8_t bit5,
				   uint8_t bit6,
				   uint8_t bit7
				   )
{	
	int b[ 8 ]	= { bit0, bit1, bit2, bit3, bit4, bit5, bit6, bit7 };
	uint8_t	last_bit	= -1;
	
	for ( int i = 0; i < 8; i++ )
	{
		if ( b[ i ] == nc )
		{
			_bits[ i ]	= NULL;
			continue;			
		}
		
		_bits[ i ]	= new DigitalInOut( b[ i ] );
			
		last_bit	= i;
	}

	_width	= last_bit + 1;
}

BusInOut::~BusInOut()
{	
	for ( int i = 0; i < _width; i++ )
	{
		if ( _bits[ i ] )
			delete _bits[ i ];
	}
}

void BusInOut::config( int conf )
{
	for ( int i = 0; i < _width; i++ )
	{
		if ( _bits[ i ] )
		{
			if ( DigitalInOut::INPUT == conf )
				_bits[ i ]->input();
			else
				_bits[ i ]->output();			
		}
	}
	
	_mode	= conf;
}

void BusInOut::value( uint8_t v )
{
	for ( int i = 0; i < _width; i++ )
	{
		if ( _bits[ i ] )		
			*(_bits[ i ])	= (v >> i) & 0x01;
	}
}

uint8_t BusInOut::value( void )
{
	uint8_t	r	= 0x00;
	
	for ( int i = 0; i < _width; i++ )
	{
		if ( _bits[ i ] )
			r	|= (*(_bits[ i ]) ? 0x01 : 0x00) << i;
	}
	
	return r;
}

BusInOut& BusInOut::operator=( uint8_t v )
{
	value( v );
	return *this;
}

BusInOut& BusInOut::operator=( BusInOut& )
{
	return *this;
}

BusInOut::operator int()
{
	return value();
}

BusIn::BusIn(
				uint8_t bit0,
				uint8_t bit1,
				uint8_t bit2,
				uint8_t bit3,
				uint8_t bit4,
				uint8_t bit5,
				uint8_t bit6,
				uint8_t bit7
				)
	: BusInOut( bit0, bit1, bit2, bit3, bit4, bit5, bit6, bit7 ){}
BusIn::~BusIn(){}

void BusIn::config( int conf )
{
	static_assert( true, "BusIn class doesn't support config() method call");
}

BusOut::BusOut(
				uint8_t bit0,
				uint8_t bit1,
				uint8_t bit2,
				uint8_t bit3,
				uint8_t bit4,
				uint8_t bit5,
				uint8_t bit6,
				uint8_t bit7
				)
	: BusInOut( bit0, bit1, bit2, bit3, bit4, bit5, bit6, bit7 )
{
	BusInOut::config( OUTPUT );
}

BusOut::~BusOut(){}

void BusOut::config( int conf )
{
	static_assert( true, "BusOut class doesn't support config() method call");
}



