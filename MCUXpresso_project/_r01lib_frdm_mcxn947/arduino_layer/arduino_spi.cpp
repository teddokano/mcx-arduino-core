#include "r01lib.h"
#include "arduino_spi.h"

SPI	spi( ARD_MOSI, ARD_MISO, ARD_SCK, ARD_CS );	//	MOSI, MISO, SCLK, CS

SPISettings::SPISettings( uint32_t freq, int order, int mode ) : clock( freq ), bitOrder( order ), dataMode( mode )
{
}

void SPIClass::begin( void )
{
}

void	SPIClass::beginTransaction( SPISettings settings )
{
	spi.frequency( settings.clock );
	spi.mode( settings.dataMode );
}

void SPIClass::endTransaction( void )
{
}

uint8_t SPIClass::transfer( uint8_t data )
{
	txrx( &data, 1 );
	return data;
}

void SPIClass::transfer( void *buf, size_t count )
{
	txrx( static_cast<uint8_t *>( buf ), count );
}

void SPIClass::txrx( uint8_t *data, size_t size )
{
	static constexpr int	READ_BUFFER_SIZE	= 128;

	uint8_t	r_data[ READ_BUFFER_SIZE ];

	spi.write( data, r_data, (int)size );
	memcpy( data, r_data, size );
}

SPIClass	SPI_;
