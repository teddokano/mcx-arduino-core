/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"r01lib.h"
#include	"arduino_i2c.h"

TwoWire	Wire(  I2C_SDA, I2C_SCL );
TwoWire	Wire1( I3C_SDA, I3C_SCL );

TwoWire::TwoWire( int sda_pin, int scl_pin ) : _sda( sda_pin ), _scl( scl_pin ), i2c( nullptr ){}

void TwoWire::begin( int baud )
{
	baudrate	= baud;
	if ( !i2c )
		i2c	= new I2C( _sda, _scl );
	i2c->frequency( baudrate );
	
	i2c->err_callback( nullptr );
}

void TwoWire::beginTransmission( const uint8_t address )
{
	targ_addr		= address;
	data_buf_index	= 0;
}

size_t TwoWire::write( uint8_t data )
{
	data_buf[ data_buf_index++ ]	= data;

	return	data_buf_index;
}

size_t TwoWire::write( const uint8_t *data, size_t length )
{
    memcpy( &data_buf[ data_buf_index ], data, length );
	data_buf_index += length;

	return	data_buf_index;
}

uint8_t TwoWire::endTransmission( bool stop )
{
	return	i2c->write( targ_addr, data_buf, data_buf_index, stop );
}

uint8_t	TwoWire::requestFrom( const uint8_t address, const size_t length, bool stop )
{
	data_buf_index	= 0;
	read_size		= length;

	return	i2c->read( address, data_buf, length );
}

int TwoWire::available( void )
{
	return (int)(read_size - data_buf_index);
}

int TwoWire::read( void )
{
	if ( data_buf_index >= read_size )
		return -1;
	return	data_buf[ data_buf_index++ ];
}
