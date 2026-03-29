/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_SPI_H
#define R01LIB_ARDUINO_SPI_H

#include	<stdint.h>

#define SPI_MODE0	0
#define SPI_MODE1	1
#define SPI_MODE2	2
#define SPI_MODE3	3

#define MSBFIRST	1
#define LSBFIRST	0

class SPISettings
{
public:
	SPISettings( uint32_t clock = 1000000, uint8_t bitOrder = MSBFIRST, uint8_t dataMode = SPI_MODE0 )
		: _clock( clock ), _bitOrder( bitOrder ), _dataMode( dataMode ) {}
	uint32_t	_clock;
	uint8_t		_bitOrder;
	uint8_t		_dataMode;
};

// Forward-declare r01lib's SPI class to avoid name collision
class SPI;

class SPIClass
{
public:
	SPIClass();

	void		begin( void );
	void		end( void );
	void		beginTransaction( SPISettings settings );
	void		endTransaction( void );
	uint8_t		transfer( uint8_t data );
	uint16_t	transfer16( uint16_t data );

private:
	SPI*		_spi;   // r01lib SPI (forward-declared above)
};

extern SPIClass	SPI;

#endif // !R01LIB_ARDUINO_SPI_H
