/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"r01lib.h"

// Alias to avoid name collision: SPIClass uses "SPI" as member type,
// but r01lib defines a class called "SPI" and we also define "SPIClass SPI".
using r01libSPI = SPI;

#include	"arduino_spi.h"
#include	"arduino_io.h"

SPIClass	SPI;

SPIClass::SPIClass() : _spi( nullptr )
{
}

void SPIClass::begin( void )
{
	if ( !_spi )
		_spi = new r01libSPI( SPI_MOSI, SPI_MISO, SPI_SCLK, SPI_CS );
}

void SPIClass::end( void )
{
	if ( _spi )
	{
		delete _spi;
		_spi = nullptr;
	}
}

void SPIClass::beginTransaction( SPISettings settings )
{
	if ( _spi )
		_spi->frequency( settings._clock );
}

void SPIClass::endTransaction( void )
{
	// no-op
}

uint8_t SPIClass::transfer( uint8_t data )
{
	if ( !_spi ) return 0;
	uint8_t rx = 0;
	// spi.h: write( uint8_t *wp, uint8_t *rp, int length )  — 3 arguments
	_spi->write( &data, &rx, 1 );
	return rx;
}

uint16_t SPIClass::transfer16( uint16_t data )
{
	uint8_t hi = transfer( (uint8_t)(data >> 8) );
	uint8_t lo = transfer( (uint8_t)(data & 0xFF) );
	return ( (uint16_t)hi << 8 ) | lo;
}
