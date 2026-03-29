/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_IO_H
#define R01LIB_ARDUINO_IO_H

#define	ARDUINO_PIN_RENUMBERING


constexpr int	INPUT			= DigitalInOut::INPUT;
constexpr int	OUTPUT			= DigitalInOut::OUTPUT;
constexpr int	INPUT_PULLUP	= INPUT | DigitalInOut::PullUp;

constexpr bool	HIGH	= true;
constexpr bool	LOW		= false;

void	pin_mode( int pin_num, int mode );
void	digitalWrite( int pin_num, bool state );
bool	digitalRead( int pin_num );

constexpr int	RISING	= 0;
constexpr int	FALLING	= 1;
constexpr int	CHANGE	= 2;

void	attachInterrupt( int int_num, void (*callback)(void), int mode );
int		digitalPinToInterrupt( int pin_num );

#ifdef	ARDUINO_PIN_RENUMBERING

const int	arduino_pin_by_number[]	=
{
	D0,
	D1,
	D2,
	D3,
	D4,
	D5,
	D6,
	D7,
	D8,
	D9,
	D10,
	D11,
	D12,
	D13,
	D18,
	D19,
	A0,
	A1,
	A2,
	A3,
	A4,
	A5,
	SW2,
	SW3,
	MB_AN,
	MB_RST,
	MB_CS,
	MB_SCK,
	MB_MISO,
	MB_MOSI,
	MB_PWM,
	MB_INT,
	MB_RX,
	MB_TX,
	MB_SCL,
	MB_SDA,
	RED,
	GREEN,
	BLUE,

	I3C_SDA,
	I3C_SCL,
	I2C_SDA,
	I2C_SCL,
	SPI_CS,
	SPI_MOSI,
	SPI_MISO,
	SPI_SCLK,
	ARD_CS,
	ARD_MOSI,
	ARD_MISO,
	ARD_SCK,
};

#undef	D0
#undef	D1
#undef	D2
#undef	D3
#undef	D4
#undef	D5
#undef	D6
#undef	D7
#undef	D8
#undef	D9
#undef	D10
#undef	D11
#undef	D12
#undef	D13
#undef	D18
#undef	D19
#undef	A0
#undef	A1
#undef	A2
#undef	A3
#undef	A4
#undef	A5
#undef	SW2
#undef	SW3
#undef	MB_AN
#undef	MB_RST
#undef	MB_CS
#undef	MB_SCK
#undef	MB_MISO
#undef	MB_MOSI
#undef	MB_PWM
#undef	MB_INT
#undef	MB_RX
#undef	MB_TX
#undef	MB_SCL
#undef	MB_SDA
#undef	RED
#undef	GREEN
#undef	BLUE

#undef	I3C_SDA
#undef	I3C_SCL
#undef	I2C_SDA
#undef	I2C_SCL
#undef	SPI_CS
#undef	SPI_MOSI
#undef	SPI_MISO
#undef	SPI_SCLK
#undef	ARD_CS
#undef	ARD_MOSI
#undef	ARD_MISO
#undef	ARD_SCK

enum ArduinoPinNum {
	D0	= 0,
	D1,
	D2,
	D3,
	D4,
	D5,
	D6,
	D7,
	D8,
	D9,
	D10,
	D11,
	D12,
	D13,
	D18,
	D19,
	A0,
	A1,
	A2,
	A3,
	A4,
	A5,
	SW2,
	SW3,
	MB_AN,
	MB_RST,
	MB_CS,
	MB_SCK,
	MB_MISO,
	MB_MOSI,
	MB_PWM,
	MB_INT,
	MB_RX,
	MB_TX,
	MB_SCL,
	MB_SDA,
	RED,
	GREEN,
	BLUE,

	I3C_SDA,
	I3C_SCL,
	I2C_SDA,
	I2C_SCL,
	SPI_CS,
	SPI_MOSI,
	SPI_MISO,
	SPI_SCLK,
	ARD_CS,
	ARD_MOSI,
	ARD_MISO,
	ARD_SCK,
};

#endif // ARDUINO_PIN_RENUMBERING

#endif // !R01LIB_ARDUINO_IO_H
