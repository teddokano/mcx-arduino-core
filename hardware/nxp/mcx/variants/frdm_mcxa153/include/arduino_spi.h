/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_SPI_H
#define R01LIB_ARDUINO_SPI_H

enum endian {
	MSBFIRST = 0,
	LSBFIRST = 1,
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
	void	begin( void );
	void	beginTransaction( SPISettings settings );
	uint8_t transfer( uint8_t data );
	void	transfer( void *buf, size_t count );
	void	endTransaction( void );
	
private:
	void	txrx( uint8_t *buf, size_t count );

};

extern SPIClass	SPI;

#endif // !R01LIB_ARDUINO_SPI_H
