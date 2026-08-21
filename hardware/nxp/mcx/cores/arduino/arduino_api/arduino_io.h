/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_IO_H
#define R01LIB_ARDUINO_IO_H

#include	<stdint.h>

/** When defined (always, in this build), the pin macros pulled in from
 *  r01lib's io.h (D0..D19, A0..A5, MB_*, SPI_*, ARD_*, PWM0..PWM5) are
 *  renumbered below into a small consecutive ArduinoPinNum enum, and
 *  arduino_pin_by_number[] is built to map back from that enum to the
 *  original raw r01lib pin value pinMode()/digitalWrite()/etc. actually
 *  operate on.
 */
#define	ARDUINO_PIN_RENUMBERING

/** pinMode() direction values */
constexpr int	INPUT		= DigitalInOut::INPUT;   // = 0
constexpr int	OUTPUT		= DigitalInOut::OUTPUT;  // = 1
constexpr int	INPUT_PULLUP	= 0x10;                  // INPUTかつPullUp（OUTPUT=1と衝突しない値）
constexpr int	INPUT_PULLDOWN	= 0x20;                  // INPUTかつPullDown
constexpr int	OUTPUT_OPENDRAIN	= 0x30;              // OUTPUTかつOpenDrain

/** digitalWrite()/digitalRead() logic levels */
constexpr bool	HIGH	= true;
constexpr bool	LOW		= false;

/** Pin the on-board LED that most example sketches blink is wired to. */
#define	LED_BUILTIN	GREEN

/** Configure a pin's direction and pull/drive mode. Lazily creates (or
 *  reconfigures) the pin's underlying DigitalInOut instance, always
 *  re-muxing it to ALT0 (plain GPIO) first regardless of whatever
 *  peripheral last owned it -- this is what lets a pin be handed back and
 *  forth between GPIO and I2C/I3C/SPI/UART use across a sketch.
 *
 * @param pin_num Arduino pin number (D0.., A0.., etc.)
 * @param mode INPUT, OUTPUT, INPUT_PULLUP, INPUT_PULLDOWN, or OUTPUT_OPENDRAIN
 */
void	pinMode( int pin_num, int mode );

/** Drive a digital output pin high or low. pinMode() must have been
 *  called on the pin first (as OUTPUT); otherwise this is a no-op.
 *
 * @param pin_num Arduino pin number
 * @param state HIGH or LOW
 */
void	digitalWrite( int pin_num, bool state );

/** Read a digital input pin's current level. pinMode() must have been
 *  called on the pin first; otherwise this returns LOW.
 *
 * @param pin_num Arduino pin number
 * @return HIGH or LOW
 */
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
/** @return this pin's raw GPIO peripheral base, or nullptr if pinMode() hasn't been called on it yet
 * @param pin_num Arduino pin number
 */
GPIO_Type*			digitalPinToPort( int pin_num );

/** @return this pin's bit mask (1 << bit) within its GPIO port, or 0 if pinMode() hasn't been called on it yet
 * @param pin_num Arduino pin number
 */
uint32_t			digitalPinToBitMask( int pin_num );

/** @return the given GPIO port's data-output register (PDOR), read/write, or nullptr if port is nullptr */
volatile uint32_t*	portOutputRegister( GPIO_Type *port );

/** @return the given GPIO port's data-input register (PDIR), read-only, or nullptr if port is nullptr */
volatile uint32_t*	portInputRegister( GPIO_Type *port );

/** @return the given GPIO port's data-direction register (PDDR, 1=OUTPUT), or nullptr if port is nullptr */
volatile uint32_t*	portModeRegister( GPIO_Type *port );

/** digitalPinToInterrupt() never actually returns this -- every valid pin
 *  on this core can have an interrupt attached -- provided only so sketches
 *  that check for it against digitalPinToInterrupt()'s result still compile.
 */
constexpr int	NOT_AN_INTERRUPT	= -1;

// Values match real Arduino exactly (CHANGE=1/FALLING=2/RISING=3, with 0
// deliberately left free for LOW -- attachInterrupt(pin, isr, LOW) reuses
// the digital-level LOW constant above as its 4th mode). This project used
// to number these RISING=0/FALLING=1/CHANGE=2, which collided LOW's value
// with RISING and made attachInterrupt(..., LOW) silently behave as RISING.
constexpr int	CHANGE	= 1;
constexpr int	FALLING	= 2;
constexpr int	RISING	= 3;

/** Register a callback to run on a digital pin's edge (or level, for LOW).
 *  Lazily creates the pin's InterruptIn instance if this is the first
 *  attach() on it; a later call re-registers on the existing instance
 *  (no leak).
 *
 * @param int_num Arduino pin number (despite the name, a pin number, not
 *                 an interrupt index -- pass digitalPinToInterrupt(pin) or
 *                 the pin number directly, they're equivalent here)
 * @param callback function to call on the event
 * @param mode RISING, FALLING, CHANGE, or LOW (level-triggered)
 */
void	attachInterrupt( int int_num, void (*callback)(void), int mode );

/** Disable a pin's interrupt and clear its registered callback (reverses attachInterrupt()).
 * @param int_num Arduino pin number
 */
void	detachInterrupt( int int_num );

/** @return int_num is already the pin's interrupt number on this core (any
 *          valid pin supports interrupts), so this is the identity function
 * @param pin_num Arduino pin number
 */
int		digitalPinToInterrupt( int pin_num );

/** Bit-bang one byte out on dataPin, clocked by clockPin (a software SPI-like transfer).
 * @param dataPin pin to drive with each bit
 * @param clockPin pin pulsed high-then-low after each bit is set
 * @param bitOrder LSBFIRST or MSBFIRST
 * @param val byte to shift out
 */
void	shiftOut( int dataPin, int clockPin, int bitOrder, uint8_t val );

/** Bit-bang one byte in from dataPin, clocked by clockPin (a software SPI-like transfer).
 * @param dataPin pin to sample after each clock pulse
 * @param clockPin pin pulsed high-then-low to clock in each bit
 * @param bitOrder LSBFIRST or MSBFIRST
 * @return the byte read
 */
uint8_t	shiftIn( int dataPin, int clockPin, int bitOrder );

/** Measure the length of a pulse on a digital pin.
 *
 *  Waits for any pulse already in progress to end, then waits for the
 *  pin to reach @p state, then measures how long it stays there.
 *
 * @param pin_num Arduino pin number
 * @param state the pulse's active level to measure (HIGH or LOW)
 * @param timeout maximum time to wait, in microseconds (default 1s)
 * @return pulse length in microseconds, or 0 on timeout
 */
unsigned long	pulseIn( int pin_num, bool state, unsigned long timeout = 1000000UL );

/** Same as pulseIn() -- provided for API compatibility with classic Arduino's
 *  long-pulse variant; this implementation has no separate long-pulse path.
 * @param pin_num Arduino pin number
 * @param state the pulse's active level to measure (HIGH or LOW)
 * @param timeout maximum time to wait, in microseconds (default 1s)
 * @return pulse length in microseconds, or 0 on timeout
 */
unsigned long	pulseInLong( int pin_num, bool state, unsigned long timeout = 1000000UL );

#ifdef	ARDUINO_PIN_RENUMBERING

/*
 *  I3C_SDA/I3C_SCL/I2C_SDA/I2C_SCL are aliases of other names in this same
 *  table (e.g. I2C_SDA is literally "#define I2C_SDA D18" in r01lib's own
 *  io.h) -- a plain textual substitution, re-expanded wherever it's used,
 *  not an independent value. Simply leaving I2C_SDA out of this table's
 *  #undef/enum step (as tried initially) does NOT decouple it: it still
 *  expands to D18, and D18 itself gets renumbered below, so I2C_SDA would
 *  silently inherit D18's *new* small-int value regardless. Captured here,
 *  before anything below is #undef'd, so these constexprs freeze today's
 *  raw r01lib value -- then further down they're #undef'd and redefined to
 *  literally *be* these frozen constants, breaking the alias chain for
 *  good. (Found via real-hardware testing: this exact bug reappeared as an
 *  SOS panic on FRDM-MCXN947's r01lib_I3C example after the first attempt,
 *  since N947's I3C_SDA aliases MB_RX -- a renumbered name -- while A153's
 *  happens to alias a raw physical pin macro (P0_16) that was never
 *  renumbered, masking the same underlying bug there.)
 */
constexpr int	_raw_I3C_SDA	= I3C_SDA;
constexpr int	_raw_I3C_SCL	= I3C_SCL;
constexpr int	_raw_I2C_SDA	= I2C_SDA;
constexpr int	_raw_I2C_SCL	= I2C_SCL;

/** Maps an ArduinoPinNum enum value (D0, A2, MB_SDA, ...) back to the raw
 *  r01lib physical pin value pinMode()/digitalWrite()/digitalRead()/etc.
 *  actually operate on. Indexed by the enum value itself.
 */
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

	SPI_CS,
	SPI_MOSI,
	SPI_MISO,
	SPI_SCLK,
	ARD_CS,
	ARD_MOSI,
	ARD_MISO,
	ARD_SCK,

	PWM0,
	PWM1,
	PWM2,
	PWM3,
	PWM4,
	PWM5,
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

#undef	SPI_CS
#undef	SPI_MOSI
#undef	SPI_MISO
#undef	SPI_SCLK
#undef	ARD_CS
#undef	ARD_MOSI
#undef	ARD_MISO
#undef	ARD_SCK

/*
 *  I3C_SDA/I3C_SCL/I2C_SDA/I2C_SCL are redefined here to the raw values
 *  frozen above, NOT given a fresh sequential enum slot like the names
 *  above -- unlike D0-D19, MB_*, SPI_*, ARD_* (all genuinely usable as
 *  ordinary pinMode()/digitalWrite() pins, or required for third-party
 *  compatibility like MOSI/MISO/SCK below), these four are only ever used
 *  as raw r01lib pin values: constructor arguments to I2C/I3C (internal
 *  arduino_i2c.cpp plumbing, unaffected either way since it never includes
 *  this header, or "Arduino_incompatible_API" examples constructing r01lib
 *  objects directly). Fixing them to their raw value means the same name
 *  means the same value everywhere, whether or not <Arduino.h> has been
 *  included -- closing off the "renumbered here, raw there" mismatch that
 *  caused two real SOS-panic bugs (Serial1 on MikroBus, and the
 *  r01lib_I3C example).
 */
#undef	I3C_SDA
#undef	I3C_SCL
#undef	I2C_SDA
#undef	I2C_SCL
#define	I3C_SDA	_raw_I3C_SDA
#define	I3C_SCL	_raw_I3C_SCL
#define	I2C_SDA	_raw_I2C_SDA
#define	I2C_SCL	_raw_I2C_SCL

#undef	PWM0
#undef	PWM1
#undef	PWM2
#undef	PWM3
#undef	PWM4
#undef	PWM5

/** Small consecutive Arduino pin numbers -- the values pinMode()/
 *  digitalWrite()/digitalRead()/etc. actually take as pin_num, translated
 *  back to a raw r01lib pin value via arduino_pin_by_number[] before use.
 */
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

	SPI_CS,
	SPI_MOSI,
	SPI_MISO,
	SPI_SCLK,
	ARD_CS,
	ARD_MOSI,
	ARD_MISO,
	ARD_SCK,

	PWM0,
	PWM1,
	PWM2,
	PWM3,
	PWM4,
	PWM5,
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

/** Standard SPI/Wire pin identifiers, same convention as
 *  ArduinoCore-avr's/ArduinoCore-samd's pins_arduino.h/variant.h --
 *  aliases for this board's own I2C_SDA/I2C_SCL and default-SPI pins
 *  (ARD_CS/ARD_MOSI/ARD_MISO/ARD_SCK), for third-party code that
 *  references the PIN_* names directly instead of going through the
 *  Wire/SPI objects. Same values on both boards (I2C_SDA/I2C_SCL and
 *  ARD_*, unlike A0-A5, are uniform ArduinoPinNum entries -- see their
 *  own definitions above).
 */
#define	PIN_WIRE_SDA	I2C_SDA
#define	PIN_WIRE_SCL	I2C_SCL
#define	PIN_SPI_SS		ARD_CS
#define	PIN_SPI_MOSI	ARD_MOSI
#define	PIN_SPI_MISO	ARD_MISO
#define	PIN_SPI_SCK		ARD_SCK

/** Total digital pins (D0-D13, D18, D19 -- D14-D17 are simply never
 *  defined, same "gap" as this board's other examples). Same on both
 *  boards: ArduinoPinNum is a single shared enum, not per-board.
 */
#define	NUM_DIGITAL_PINS	16

/** Working analog-input pins. Genuinely differs by board: FRDM-MCXA153
 *  has A0-A5 all wired to LPADC channels, but FRDM-MCXN947's A0/A1 have
 *  no LPADC channel routed to them at all (fixed to io.h's DISABLED_PIN
 *  sentinel -- see PIN_MAPPING_N947.md) -- analogRead() on either panics.
 *  A portable library assuming NUM_ANALOG_INPUTS pins starting at A0
 *  still won't work correctly on N947 purely from this count being
 *  right (A0 itself is the missing one, not one past the working range),
 *  but reporting the real working count here is still strictly more
 *  honest than the alternative of claiming 6 on a board where 2 of those
 *  6 don't work.
 */
#if	defined( FRDM_MCXA153 )
#define	NUM_ANALOG_INPUTS	6
#elif	defined( FRDM_MCXN947 )
#define	NUM_ANALOG_INPUTS	4
#endif

/** Whether analogWrite() works on the given ArduinoPinNum. On this board
 *  PWM capability lives entirely on the dedicated PWM0-PWM5 pins (a
 *  separate namespace from D0-D19, unlike AVR boards where PWM shares
 *  the D-pin numbers) -- so this is a plain range check against the
 *  enum above, true only for PWM0..PWM5, correctly false for every
 *  D-pin/A-pin/etc regardless of board.
 */
#define	digitalPinHasPWM( p )	( ( (p) >= PWM0 ) && ( (p) <= PWM5 ) )

#endif // ARDUINO_PIN_RENUMBERING

#endif // !R01LIB_ARDUINO_IO_H
