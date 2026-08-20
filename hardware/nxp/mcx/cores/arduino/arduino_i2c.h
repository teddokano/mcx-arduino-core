/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_I2C_H
#define R01LIB_ARDUINO_I2C_H

#include <cstdint>
#include <cstddef>

class I2C;	// full definition: i2c.h (r01lib), pulled in by whichever
			// translation unit actually implements TwoWire's methods

/** Arduino-compatible I2C (Wire) class.
 *
 *  Wraps an r01lib I2C (or, when sda_pin/scl_pin are this board's I3C
 *  pins, an I3C running in legacy I2C_MODE) behind the classic
 *  Arduino two-phase Wire API: beginTransmission()/write()/
 *  endTransmission() buffer up a transaction locally, sent as one write()
 *  to the target on endTransmission(); requestFrom()/available()/read()
 *  work the same way for reads.
 */
class TwoWire
{
public:
	/** Construct on the given SDA/SCL pin pair. Hardware isn't touched
	 *  until begin().
	 * @param sda_pin SDA pin
	 * @param scl_pin SCL pin
	 */
	TwoWire( int sda_pin, int scl_pin );

	/** Initialize the bus. Lazily creates the underlying I2C or I3C
	 *  instance (I3C, in I2C_MODE, if sda_pin/scl_pin are this board's
	 *  I3C_SDA/I3C_SCL) and sets the bus frequency.
	 * @param baud SCL frequency in Hz (default 100kHz)
	 */
	void	begin( int baud = 100000 );

	/** Deinitialize the bus and free the underlying I2C/I3C instance. */
	void	end( void );

	/** Change the bus frequency at runtime (no-op before begin()).
	 * @param freq SCL frequency in Hz
	 */
	void	setClock( uint32_t freq );

	/** Start buffering a write transaction to the given target address.
	 * @param address target 7-bit I2C address
	 */
	void	beginTransmission( const uint8_t address );

	/** Queue one byte into the current transaction's write buffer.
	 * @param data byte to queue
	 * @return number of bytes now queued
	 */
	size_t	write( uint8_t data );

	/** Queue multiple bytes into the current transaction's write buffer.
	 * @param data bytes to queue
	 * @param length number of bytes
	 * @return number of bytes now queued
	 */
	size_t	write( const uint8_t *data, size_t length );

	/** Send the buffered write transaction started by beginTransmission().
	 * @param stop generate a STOP condition (true, default) or a repeated start (false)
	 * @return 0 on success, non-zero status code on failure
	 */
	uint8_t	endTransmission( bool stop = true );

	/** Read a block of data from a target address into an internal buffer,
	 *  to be consumed with available()/read().
	 * @param address target 7-bit I2C address
	 * @param length number of bytes to read
	 * @param stop generate a STOP condition (true, default) or a repeated start (false)
	 * @return number of bytes actually read (0 on failure)
	 */
	uint8_t	requestFrom( const uint8_t address, const size_t length, bool stop = true );

	/** @return number of bytes remaining to be read() from the last requestFrom() */
	int		available( void );

	/** @return next byte from the last requestFrom(), or -1 if none remain */
	int		read( void );

private:
	const int	_sda;
	const int	_scl;
	I2C			*i2c;
	uint8_t		targ_addr;
	int			baudrate;
	uint8_t		data_buf[ 128 ];
	size_t		data_buf_index;
	size_t		read_size;
};

/** Global TwoWire instance on this board's general-purpose I2C pins (I2C_SDA/I2C_SCL). */
extern TwoWire	Wire;

/** Global TwoWire instance on this board's I3C pins (I3C_SDA/I3C_SCL), run
 *  in legacy I2C_MODE -- see TwoWire::begin(). Typically wired to an
 *  on-board sensor rather than an external header.
 */
extern TwoWire	Wire1;

#ifdef	CPU_MCXN947VDF
/** Independent I2C instance on the MikroBus header (MB_SDA/MB_SCL), its own
 * physical peripheral (LPI2C3/FlexComm3) -- only possible on N947, which
 * has more than one physical I2C peripheral. A153 has only one (LPI2C0),
 * already used by Wire, so no independent Wire2 exists there.
 */
extern TwoWire	Wire2;
#endif

#endif // !R01LIB_ARDUINO_I2C_H
