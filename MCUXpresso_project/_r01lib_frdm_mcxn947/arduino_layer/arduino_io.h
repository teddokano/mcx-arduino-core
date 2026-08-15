/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_IO_H
#define R01LIB_ARDUINO_IO_H

#include	<stdint.h>

#define	ARDUINO_PIN_RENUMBERING

constexpr int	INPUT		= DigitalInOut::INPUT;   // = 0
constexpr int	OUTPUT		= DigitalInOut::OUTPUT;  // = 1
constexpr int	INPUT_PULLUP	= 0x10;                  // INPUTかつPullUp（OUTPUT=1と衝突しない値）
constexpr int	INPUT_PULLDOWN	= 0x20;                  // INPUTかつPullDown
constexpr int	OUTPUT_OPENDRAIN	= 0x30;              // OUTPUTかつOpenDrain

constexpr bool	HIGH	= true;
constexpr bool	LOW		= false;

#define	LED_BUILTIN	GREEN

void	pinMode( int pin_num, int mode );
void	digitalWrite( int pin_num, bool state );
bool	digitalRead( int pin_num );

/*
 *  Fast-GPIO / direct-register-access family, for third-party libraries
 *  that bit-bang a pin and want to skip digitalWrite()'s per-call
 *  overhead (e.g. NeoPixel-style drivers). pinMode() must be called on
 *  the pin first -- these read back the GPIO_Type/bit already resolved
 *  by the pin's (lazily-created) DigitalInOut instance, they don't do
 *  pin setup themselves. Returns nullptr/0 for a pin that hasn't been
 *  configured with pinMode() yet.
 */
GPIO_Type*			digitalPinToPort( int pin_num );
uint32_t			digitalPinToBitMask( int pin_num );
volatile uint32_t*	portOutputRegister( GPIO_Type *port );
volatile uint32_t*	portInputRegister( GPIO_Type *port );
volatile uint32_t*	portModeRegister( GPIO_Type *port );

constexpr int	NOT_AN_INTERRUPT	= -1;

// Values match real Arduino exactly (CHANGE=1/FALLING=2/RISING=3, with 0
// deliberately left free for LOW -- attachInterrupt(pin, isr, LOW) reuses
// the digital-level LOW constant above as its 4th mode). This project used
// to number these RISING=0/FALLING=1/CHANGE=2, which collided LOW's value
// with RISING and made attachInterrupt(..., LOW) silently behave as RISING.
constexpr int	CHANGE	= 1;
constexpr int	FALLING	= 2;
constexpr int	RISING	= 3;

void	attachInterrupt( int int_num, void (*callback)(void), int mode );
void	detachInterrupt( int int_num );
int		digitalPinToInterrupt( int pin_num );

void	shiftOut( int dataPin, int clockPin, int bitOrder, uint8_t val );
uint8_t	shiftIn( int dataPin, int clockPin, int bitOrder );

unsigned long	pulseIn( int pin_num, bool state, unsigned long timeout = 1000000UL );
unsigned long	pulseInLong( int pin_num, bool state, unsigned long timeout = 1000000UL );

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

	PWM_0,
	PWM_1,
	PWM_2,
	PWM_3,
	PWM_4,
	PWM_5,
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

#undef	PWM_0
#undef	PWM_1
#undef	PWM_2
#undef	PWM_3
#undef	PWM_4
#undef	PWM_5

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

	PWM_0,
	PWM_1,
	PWM_2,
	PWM_3,
	PWM_4,
	PWM_5,
};

// Bare MOSI/MISO/SCK, aliased to this board's default SPI pins (ARD_MOSI/
// ARD_MISO/ARD_SCK, the same ones the global `SPI` instance uses). Every
// other Arduino core (AVR, SAMD, ESP32, Renesas UNO R4, ...) provides these
// as standard board-pin identifiers; without them, any third-party library
// that references MOSI/MISO/SCK directly (e.g. the official SD library)
// fails to compile here.
#define	MOSI	ARD_MOSI
#define	MISO	ARD_MISO
#define	SCK		ARD_SCK

#endif // ARDUINO_PIN_RENUMBERING

#endif // !R01LIB_ARDUINO_IO_H
