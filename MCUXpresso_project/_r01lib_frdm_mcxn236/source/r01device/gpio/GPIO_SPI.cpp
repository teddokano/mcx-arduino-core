/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include "GPIO_SPI.h"

GPIO_SPI::GPIO_SPI( SPI& interface, uint8_t dev_address ) : spi( interface ), dev_addr( dev_address )
{
}

GPIO_SPI::~GPIO_SPI()
{
}

int GPIO_SPI::reg_w( uint8_t reg_adr, const uint8_t *data, uint16_t size )
{
	uint8_t	w_data[ size + 2 ];
	uint8_t	r_data[ size + 2 ];
	
	w_data[ 0 ]	= (dev_addr << 1);
	w_data[ 1 ]	= reg_adr | auto_increment;
	memcpy( w_data + 2, data, size );
	
	spi.write( w_data, r_data, size + 2 );
	
	return size;
}

int GPIO_SPI::reg_w( uint8_t reg_adr, uint8_t data )
{
	uint8_t	w_data[ 3 ];
	uint8_t	r_data[ 3 ];
	
	w_data[ 0 ]	= dev_addr << 1;
	w_data[ 1 ]	= reg_adr;
	w_data[ 2 ]	= data;
	
	spi.write( w_data, r_data, 3 );
	
	return 1;
}

int GPIO_SPI::reg_r( uint8_t reg_adr, uint8_t *data, uint16_t size )
{
	uint8_t	w_data[ size + 2 ]	= { 0 };
	uint8_t	r_data[ size + 2 ];

	w_data[ 0 ]	= (dev_addr << 1) | 0x1;
	w_data[ 1 ]	= reg_adr | auto_increment;

	spi.write( w_data, r_data, size + 2 );
	
	memcpy( data, r_data + 2, size );

	return size;
}

uint8_t GPIO_SPI::reg_r( uint8_t reg_adr )
{
	uint8_t	w_data[ 3 ];
	uint8_t	r_data[ 3 ];
	
	w_data[ 0 ]	= (dev_addr << 1) | 0x1;
	w_data[ 1 ]	= reg_adr;
	w_data[ 2 ]	= 0;
	
	spi.write( w_data, r_data, 3 );
	
	return r_data[ 2 ];
} 

void GPIO_SPI::write_r8( uint8_t reg, uint8_t val )
{
	reg_w( reg, val );
}

void GPIO_SPI::write_r16( uint8_t reg, uint16_t val )
{
	uint8_t	buff[ 2 ];
	
	buff[ 0 ]	= val >> 8;
	buff[ 1 ]	= val & 0xFF;
	
	reg_w( reg, buff, sizeof( buff ) );
}

uint8_t GPIO_SPI::read_r8( uint8_t reg )
{
	return reg_r( reg );	
}

uint16_t GPIO_SPI::read_r16( uint8_t reg )
{
	uint8_t	buff[ 2 ];

	reg_r( reg, buff, sizeof( buff ) );
	
	return (buff[ 0 ] << 8) | buff[ 1 ];
}

void GPIO_SPI::bit_op8( uint8_t reg, uint8_t mask, uint8_t value )
{
	uint8_t	v	= read_r8( reg );

	v	&= mask;
	v	|= value;
	
	write_r8( reg, v );
}

void GPIO_SPI::bit_op16( uint8_t reg, uint16_t mask, uint16_t value )
{
	uint16_t v	= read_r16( reg );

	v	&= mask;
	v	|= value;

	write_r16( reg, v );
}

