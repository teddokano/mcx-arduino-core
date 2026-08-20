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

/** SPI clock polarity/phase modes (CPOL/CPHA), passed to SPISettings/setDataMode() */
enum spi_mode {
	SPI_MODE0 = 0,	/**< CPOL:0, CPHA:0 */
	SPI_MODE1,		/**< CPOL:0, CPHA:1 */
	SPI_MODE2,		/**< CPOL:1, CPHA:0 */
	SPI_MODE3,		/**< CPOL:1, CPHA:1 */
};

// Legacy (pre-1.6) clock divider constants, values matching classic AVR
// SPI.h's register-encoding scheme (not linear) -- setClockDivider() maps
// each back to the actual divide-by-N factor it names.
/** @name Legacy setClockDivider() constants -- divide r01lib SPI::clock_freq() by N */
///@{
#define	SPI_CLOCK_DIV4		0x00
#define	SPI_CLOCK_DIV16		0x01
#define	SPI_CLOCK_DIV64		0x02
#define	SPI_CLOCK_DIV128	0x03
#define	SPI_CLOCK_DIV2		0x04
#define	SPI_CLOCK_DIV8		0x05
#define	SPI_CLOCK_DIV32		0x06
///@}

/** Default SPI chip-select pin (this board's Arduino-header CS). */
constexpr int SS	= ARD_CS;

/** Bundles the clock/bitOrder/dataMode settings for one
 *  beginTransaction()/endTransaction() pair.
 */
class SPISettings
{
public:
	/** Default: clock 0 (see beginTransaction() -- only takes effect if it
	 *  actually differs from the bus's current clock), MSBFIRST, SPI_MODE0.
	 */
	SPISettings() : clock( 0 ), bitOrder( MSBFIRST ), dataMode( SPI_MODE0 ) {}

	/** @param freq SCLK frequency in Hz
	 *  @param order LSBFIRST or MSBFIRST
	 *  @param mode SPI_MODE0..SPI_MODE3
	 */
	SPISettings( uint32_t freq, int order, int mode );

	uint32_t	clock;		/**< SCLK frequency in Hz */
	int			bitOrder;	/**< LSBFIRST or MSBFIRST */
	int			dataMode;	/**< SPI_MODE0..SPI_MODE3 */
};

// r01lib の SPI クラスを forward 宣言（SPIClass SPI インスタンスとの名前衝突を回避）
class SPI;

/** Arduino-compatible SPI class, wrapping an r01lib SPI instance. */
class SPIClass
{
public:
	// Defaults to the Arduino-header SPI pins (D10-D13); pass a different
	// pin set (e.g. MB_MOSI/MB_MISO/MB_SCK/MB_CS) to talk to a different
	// physical SPI instance -- see the global `SPI1` for the MikroBus one.
	/** @param mosi MOSI pin
	 *  @param miso MISO pin
	 *  @param sclk SCLK pin
	 *  @param cs default chip-select pin (driven by begin()/end(); a
	 *            sketch can still bit-bang any pin itself as CS instead)
	 */
	SPIClass( int mosi = ARD_MOSI, int miso = ARD_MISO, int sclk = ARD_SCK, int cs = ARD_CS );

	/** Initialize the underlying r01lib SPI instance (idempotent -- a second call is a no-op). */
	void	 begin( void );

	/** Deinitialize and free the underlying r01lib SPI instance. */
	void	 end( void );

	/** Apply clock/bitOrder/dataMode for the transaction about to start
	 *  (only the settings that actually differ from the bus's current
	 *  ones are re-applied to hardware).
	 * @param settings clock/bitOrder/dataMode to use
	 */
	void	 beginTransaction( SPISettings settings );

	/** Transfer one byte (simultaneous write+read).
	 * @param data byte to send
	 * @return byte received
	 */
	uint8_t  transfer( uint8_t data );

	/** Transfer two bytes as a 16-bit value, byte order per the last-applied bitOrder.
	 * @param data value to send
	 * @return value received
	 */
	uint16_t transfer16( uint16_t data );

	/** Transfer a buffer in place (simultaneous write+read; received bytes overwrite buf).
	 * @param buf buffer to send from and receive into
	 * @param count number of bytes to transfer
	 */
	void	 transfer( void *buf, size_t count );

	/** Ends the current transaction. No-op (this implementation doesn't need
	 *  matching begin/end cleanup), provided for API completeness.
	 */
	void	 endTransaction( void );

	/*
	 *  usingInterrupt()/notUsingInterrupt() are no-ops here: on AVR-style
	 *  cores they let beginTransaction()/endTransaction() temporarily mask
	 *  a specific external interrupt that might reenter a SPI transfer.
	 *  Declared for sketch compatibility only.
	 */
	void	 usingInterrupt( uint8_t interruptNumber )    { (void)interruptNumber; }
	void	 notUsingInterrupt( uint8_t interruptNumber ) { (void)interruptNumber; }

	/*
	 *  Legacy pre-1.6 API: configures bit order/mode/clock outside of a
	 *  beginTransaction()/endTransaction() pair, applied immediately and
	 *  persisting until changed again. Superseded by SPISettings, kept for
	 *  sketch compatibility. Each calls begin() first if it hasn't been
	 *  called yet.
	 */
	/** @param order LSBFIRST or MSBFIRST */
	void	setBitOrder( uint8_t order );
	/** @param mode SPI_MODE0..SPI_MODE3 */
	void	setDataMode( uint8_t mode );
	/** @param divider one of the SPI_CLOCK_DIVn constants above; divides
	 *                 the r01lib SPI's own input clock (clock_freq()), not
	 *                 F_CPU as classic AVR's divider table does
	 */
	void	setClockDivider( uint8_t divider );

private:
	void	txrx( uint8_t *buf, size_t count );

	const int	_mosi;
	const int	_miso;
	const int	_sclk;
	const int	_cs;
	SPI			*spi	= nullptr;

	uint32_t	_last_clock	= 0;
	int			_last_mode	= -1;
	int			_last_order	= -1;
};

/** Global SPI instance on this board's Arduino-header SPI pins (ARD_MOSI/ARD_MISO/ARD_SCK/ARD_CS). */
extern SPIClass	SPI;

/** Global SPI instance on this board's MikroBus header pins, its own
 *  independent physical SPI peripheral (see each board's PIN_MAPPING_*.md
 *  for which one).
 */
extern SPIClass	SPI1;

#endif // !R01LIB_ARDUINO_SPI_H
