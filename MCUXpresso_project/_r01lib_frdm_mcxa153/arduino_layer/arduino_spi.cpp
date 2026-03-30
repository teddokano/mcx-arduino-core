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

void	SPIClass::beginTransaction( SPISettings settings )
{
	static uint32_t	last_clock	= 0;
	static int		last_mode	= -1;

	if ( settings.clock != last_clock )
	{
		spi->frequency( settings.clock );
		last_clock	= settings.clock;
	}
	if ( settings.dataMode != last_mode )
	{
		spi->mode( settings.dataMode );
		last_mode	= settings.dataMode;
	}
}

uint8_t SPIClass::transfer( uint8_t data )
{
	txrx( &data, 1 );
	return data;
}

void SPIClass::transfer( uint8_t *data, int length )
{
	txrx( data, length );
}

void SPIClass::txrx( uint8_t *data, int size )
{
	static constexpr int	READ_BUFFER_SIZE	= 128;

	uint8_t	r_data[ READ_BUFFER_SIZE ];
	
	spi->write( data, r_data, size );
	memcpy( data, r_data, size );
}

SPIClass	SPI;
