/** NXP Analog Front End class library for MCX
 *
 *  @author  Tedd OKANO
 *
 *  Copyright: 2023 - 2024 Tedd OKANO
 *  Released under the MIT license
 */

#include "AFE_NXP.h"
#include <bit>

SPI_for_AFE::SPI_for_AFE( SPI& spi, bool spi_addr ) : _spi( spi ), dev_ad( spi_addr )
{
}

SPI_for_AFE::~SPI_for_AFE()
{
}

void SPI_for_AFE::txrx( uint8_t *data, int size )
{
	static constexpr int	READ_BUFFER_SIZE	= command_length + 16 * 3; //	(2[command] + 16[logical_channels] * 3[bytes])

	uint8_t	r_data[ READ_BUFFER_SIZE ];
	
	data[ 0 ]	|= dev_ad ? 0x80 : 0x00;
	
	_spi.write( data, r_data, size );
	memcpy( data, r_data, size );
}

void SPI_for_AFE::write_r16( uint16_t reg )
{
	reg	<<= 1;

	uint8_t	v[]	= { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };
	txrx( v, sizeof( v ) );
}

void SPI_for_AFE::write_r16( uint16_t reg, uint16_t val )
{
	reg	<<= 1;

	uint8_t	v[]	= { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF), (uint8_t)(val >> 8), (uint8_t)val };
	txrx( v, sizeof( v ) );
}

uint16_t SPI_for_AFE::read_r16( uint16_t reg )
{
	constexpr int	array_size	= command_length + sizeof( uint16_t );
	
	reg	<<= 1;
	reg	 |= 0x4000;

	uint8_t	v[ array_size ]	= { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF), 0xFF, 0xFF };
	txrx( v, array_size );
	
//	printf( "0x%04X\r\n", byteswap16( *((uint16_t*)(v + command_length)) ) );
//	return byteswap16( *((uint16_t*)(v + command_length)) );

	return get_data16( v + command_length );
}

void SPI_for_AFE::write_r24( uint16_t reg, uint32_t val )
{
	reg	<<= 1;

	uint8_t	v[]	= { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF), (uint8_t)(val >> 16), (uint8_t)(val >> 8), (uint8_t)val };
	txrx( v, sizeof( v ) );
}

int32_t SPI_for_AFE::read_r24( uint16_t reg )
{
	constexpr int	array_size		= command_length + sizeof( uint32_t );
	constexpr int	transfer_size	= array_size - 1;	// since the data is 24 bits

	reg	<<= 1;
	reg	 |= 0x4000;

	uint8_t	v[ array_size ]	= { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };
	txrx( v, transfer_size );
	
	return get_data24( v + command_length );
}

void SPI_for_AFE::burst( uint32_t *data, int length, int width )
{
	constexpr int	data_byte_size		= 3;
	constexpr int	logical_chanels		= 16;
	constexpr int	total_data_length	= data_byte_size * logical_chanels;

	uint8_t		v[ command_length + total_data_length ];	
	uint16_t	reg	  = (0x2005 << 1) | 0x4000;	// CMD_BURST_DATA

	v[ 0 ]	= (uint8_t)(reg >> 8);
	v[ 1 ]	= (uint8_t)(reg & 0xFF);
	
	txrx( v, command_length + length * width );
	
	for ( auto i = 0; i < length; i++ )
		*data++	= get_data24( v + command_length + i * width );
}


