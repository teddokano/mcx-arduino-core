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
	
	spi.write( data, r_data, size );
	memcpy( data, r_data, size );
}

SPIClass	SPI_;
