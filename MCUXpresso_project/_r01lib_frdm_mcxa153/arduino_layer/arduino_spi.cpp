/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"r01lib.h"

// r01lib の SPI クラスを別名で退避（arduino_spi.h で SPIClass SPI を宣言するため）
using r01libSPI = SPI;

#include	"arduino_spi.h"

static r01libSPI	*spi	= nullptr;	//	MOSI, MISO, SCLK, CS (lazy init)

SPISettings::SPISettings( uint32_t freq, int order, int mode ) : clock( freq ), bitOrder( order ), dataMode( mode )
{
}

void SPIClass::begin( void )
{
	if ( !spi )
		spi	= new r01libSPI( ARD_MOSI, ARD_MISO, ARD_SCK, ARD_CS );
}

void SPIClass::end( void )
{
	delete spi;
	spi			= nullptr;
	_last_clock	= 0;
	_last_mode	= -1;
	_last_order	= -1;
}

void	SPIClass::beginTransaction( SPISettings settings )
{
	if ( settings.clock != _last_clock )
	{
		spi->frequency( settings.clock );
		_last_clock	= settings.clock;
	}
	if ( settings.dataMode != _last_mode )
	{
		spi->mode( settings.dataMode );
		_last_mode	= settings.dataMode;
	}
	if ( settings.bitOrder != _last_order )
	{
		spi->bit_order( (uint8_t)settings.bitOrder );
		_last_order	= settings.bitOrder;
	}
}

void SPIClass::endTransaction( void )
{
}

uint8_t SPIClass::transfer( uint8_t data )
{
	txrx( &data, 1 );
	return data;
}

uint16_t SPIClass::transfer16( uint16_t data )
{
	union { uint16_t val; struct { uint8_t lo; uint8_t hi; } b; } t;

	t.val	= data;

	// MSBFIRST: high byte goes out first on the wire; LSBFIRST: low byte first
	if ( _last_order == MSBFIRST )
	{
		t.b.hi	= transfer( t.b.hi );
		t.b.lo	= transfer( t.b.lo );
	}
	else
	{
		t.b.lo	= transfer( t.b.lo );
		t.b.hi	= transfer( t.b.hi );
	}

	return	t.val;
}

void SPIClass::transfer( void *buf, size_t count )
{
	txrx( static_cast<uint8_t *>( buf ), count );
}

void SPIClass::txrx( uint8_t *data, size_t size )
{
	static constexpr int	READ_BUFFER_SIZE	= 128;

	uint8_t	r_data[ READ_BUFFER_SIZE ];

	spi->write( data, r_data, (int)size );
	memcpy( data, r_data, size );
}

SPIClass	SPI;
