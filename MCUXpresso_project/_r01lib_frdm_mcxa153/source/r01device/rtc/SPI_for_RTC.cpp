/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include	"rtc/RTC_NXP.h"

SPI_for_RTC::SPI_for_RTC( SPI& interface ) : spi( interface ) {}
SPI_for_RTC::~SPI_for_RTC() {}

void SPI_for_RTC::txrx( uint8_t *data, int size )
{
	uint8_t	r_data[ size ];
	
	spi.write( data, r_data, size );
	memcpy( data, r_data, size );
}

int SPI_for_RTC::reg_w( uint8_t reg_adr, const uint8_t *data, uint16_t size )
{
	uint8_t	v[ size + 1 ];
	
	v[ 0 ]	= reg_adr;
	memcpy( v + 1, data, size );
	
	txrx( v, sizeof( v ) );
	
	return 0;
}

int SPI_for_RTC::reg_w( uint8_t reg_adr, uint8_t data )
{
	uint8_t	v[]	= { reg_adr, data };
	
	txrx( v, sizeof( v ) );

	return 0;
}

int SPI_for_RTC::reg_r( uint8_t reg_adr, uint8_t *data, uint16_t size )
{
	uint8_t	v[ size + 1 ];
	
	for ( int i = 0; i < size + 1; i++ ) v[ i ]	= 0xFF;
	
	v[ 0 ]	= (uint8_t)(reg_adr | 0x80);
	
	txrx( v, sizeof( v ) );
	
	memcpy( data, v + 1, size );

	return 0;
}

uint8_t	SPI_for_RTC::reg_r( uint8_t reg_adr )
{
	uint8_t	v[]	= { (uint8_t)(reg_adr | 0x80), 0xFF };
	
	txrx( v, sizeof( v ) );
	
	return v[ 1 ];
}

void SPI_for_RTC::write_r8( uint8_t reg, uint8_t val )
{
	reg_w( reg, val );
}

uint8_t SPI_for_RTC::read_r8( uint8_t reg )
{
	return reg_r( reg );
}

void SPI_for_RTC::bit_op8(  uint8_t reg,  uint8_t mask,  uint8_t value )
{
	uint8_t	tmp	= reg_r( reg ) & mask;
	reg_w( reg, tmp | value );
}
