#include "r01lib.h"
#include "arduino_i2c.h"

I2C	i2c( I2C_SDA, I2C_SCL );	//	SDA, SCL

void WireClass::begin( int baud )
{
	baudrate	= baud;
	i2c.frequency( baudrate );
}

void WireClass::beginTransmission( const uint8_t address )
{
	targ_addr		= address;
	data_buf_index	= 0;
}

size_t WireClass::write( uint8_t data )
{
	data_buf[ data_buf_index++ ]	= data;

	return	data_buf_index;
}

size_t WireClass::write( const uint8_t *data, size_t length )
{
    memcpy( &data_buf[ data_buf_index ], data, length );
	data_buf_index += length;

	return	data_buf_index;
}

uint8_t WireClass::endTransmission( bool stop )
{
	return	i2c.write( targ_addr, data_buf, data_buf_index, stop );
}

uint8_t	WireClass::requestFrom( const uint8_t address, const size_t length, bool stop )
{
	data_buf_index	= 0;
	read_size		= length;
	
	return	i2c.read( address, data_buf, length );
}

int WireClass::available( void )
{
	return (int)(read_size - data_buf_index);
}

int WireClass::read( void )
{
	if ( data_buf_index >= read_size )
		return -1;
	return	data_buf[ data_buf_index++ ];
}

WireClass	Wire;
