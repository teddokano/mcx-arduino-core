/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include	"misc/potentiometer/AD5161.h"

AD5161_base::AD5161_base() : _value( 0 ){}
AD5161_base::~AD5161_base(){}
void AD5161_base::begin( void ){}

AD5161_I2C::AD5161_I2C( I2C& interface, uint8_t i2c_address ) : AD5161_base(), I2C_device( interface, i2c_address ){}
AD5161_I2C::~AD5161_I2C(){}

void AD5161_I2C::value( uint8_t v )
{
	write_r8( 0x00, v );
}

uint8_t AD5161_I2C::value( void )
{
	uint8_t data	= 0;
	rx( &data, 1 );
	
	return data;
}

AD5161_I2C& AD5161_I2C::operator=( uint8_t v )
{
	value( v );
	return *this;
}

AD5161_I2C& AD5161_I2C::operator=( AD5161_I2C& )
{
	return *this;
}

AD5161_I2C::operator int()
{
	return value();
}


AD5161_SPI::AD5161_SPI( SPI& interface ) : AD5161_base(), spi( interface ){}
AD5161_SPI::~AD5161_SPI(){}

uint8_t AD5161_SPI::value( uint8_t v )
{
	uint8_t	r;
	
	spi.write( &v, &r, 1 );
	_value	= v;
	
	return r;
}

uint8_t AD5161_SPI::value( void )
{
	return value( _value );
}

AD5161_SPI& AD5161_SPI::operator=( uint8_t v )
{
	value( v );
	return *this;
}

AD5161_SPI& AD5161_SPI::operator=( AD5161_SPI& )
{
	return *this;
}

AD5161_SPI::operator int()
{
	return value();
}

