/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_SPI_H
#define R01LIB_ARDUINO_SPI_H

// Matches the LSBFIRST=0/MSBFIRST=1 values already #define'd in arduino.h
// (standard Arduino convention). Kept as a local enum too since this header
// doesn't include arduino.h (arduino.h includes this, not the other way
// around) -- values must stay in sync with those macros or SPISettings'
// inline default constructor below and a sketch's explicit MSBFIRST/
// LSBFIRST would silently disagree once arduino.h's macros take over.
enum endian {
	LSBFIRST = 0,
	MSBFIRST = 1,
};

enum spi_mode {
	SPI_MODE0 = 0,
	SPI_MODE1,
	SPI_MODE2,
	SPI_MODE3,
};

constexpr int SS	= ARD_CS;

class SPISettings
{
public:
	SPISettings() : clock( 0 ), bitOrder( MSBFIRST ), dataMode( SPI_MODE0 ) {}
	SPISettings( uint32_t freq, int order, int mode );

	uint32_t	clock;
	int			bitOrder;
	int			dataMode;
};

// r01lib の SPI クラスを forward 宣言（SPIClass SPI インスタンスとの名前衝突を回避）
class SPI;

class SPIClass
{
public:
	void	 begin( void );
	void	 end( void );
	void	 beginTransaction( SPISettings settings );
	uint8_t  transfer( uint8_t data );
	uint16_t transfer16( uint16_t data );
	void	 transfer( void *buf, size_t count );
	void	 endTransaction( void );

	/*
	 *  usingInterrupt()/notUsingInterrupt() are no-ops here: on AVR-style
	 *  cores they let beginTransaction()/endTransaction() temporarily mask
	 *  a specific external interrupt that might reenter a SPI transfer.
	 *  Declared for sketch compatibility only.
	 */
	void	 usingInterrupt( uint8_t interruptNumber )    { (void)interruptNumber; }
	void	 notUsingInterrupt( uint8_t interruptNumber ) { (void)interruptNumber; }

private:
	void	txrx( uint8_t *buf, size_t count );

	uint32_t	_last_clock	= 0;
	int			_last_mode	= -1;
	int			_last_order	= -1;
};

extern SPIClass	SPI;

#endif // !R01LIB_ARDUINO_SPI_H
