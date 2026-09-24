/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_I2C_H
#define R01LIB_ARDUINO_I2C_H

#include <cstdint>
#include <cstddef>

#include "Stream.h"

class I2C;	// full definition: i2c.h (r01lib), pulled in by whichever
			// translation unit actually implements TwoWire's methods

/** Same feature macro as ArduinoCore-avr, so libraries that guard their
 *  setWireTimeout() calls with it pick it up here too. */
#define	WIRE_HAS_TIMEOUT

/** Size of each of TwoWire's two buffers: the most one endTransmission()
 *  sends or one requestFrom() reads. AVR's is 32. */
#define	WIRE_BUFFER_SIZE	128

/** Arduino-compatible I2C (Wire) class.
 *
 *  Wraps an r01lib I2C (or, when sda_pin/scl_pin are this board's I3C
 *  pins, an I3C running in legacy I2C_MODE) behind the classic
 *  Arduino two-phase Wire API: beginTransmission()/write()/
 *  endTransmission() buffer up a transaction locally, sent as one write()
 *  to the target on endTransmission(); requestFrom()/available()/read()
 *  work the same way for reads.
 *
 *  A Stream, as in every official core: print() goes into the transaction
 *  being built, and the parse/find/readBytes helpers read what the last
 *  requestFrom() got, so a Wire can be handed to code that takes a Print&
 *  or Stream&. The outgoing and incoming data have separate buffers, so
 *  starting a transaction does not throw away bytes not yet read.
 */
class TwoWire : public Stream
{
public:
	/** Construct on the given SDA/SCL pin pair. Hardware isn't touched
	 *  until begin().
	 * @param sda_pin SDA pin
	 * @param scl_pin SCL pin
	 */
	TwoWire( int sda_pin, int scl_pin );

	/** Join the bus as the controller (master), at 100kHz; setClock()
	 *  changes that. Lazily creates the underlying I2C or I3C instance
	 *  (I3C, in I2C_MODE, if sda_pin/scl_pin are this board's
	 *  I3C_SDA/I3C_SCL).
	 */
	void	begin( void );

	/** Join the bus as a target (slave) at the given address: what
	 *  begin(address) means in every official core. Not supported yet,
	 *  so this calls panic() with a message saying so, rather than
	 *  letting a sketch written for a target run as something else.
	 * @param address the target's own 7-bit address
	 */
	void	begin( uint8_t address );

	/** Same as begin(uint8_t) for 0 to 127.
	 *
	 *  Before v0.7.0 this core's only begin() took the SCL frequency here,
	 *  so a larger value is still taken as one, for sketches written that
	 *  way; no I2C address is that large. Deprecated: call begin() and
	 *  then setClock() instead, which works on every core.
	 * @param address the target's own 7-bit address, or (deprecated) an
	 *        SCL frequency in Hz
	 */
	void	begin( int address );

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
	 * @return 1, or 0 if the buffer (WIRE_BUFFER_SIZE bytes) is already
	 *         full, in which case the byte is dropped and getWriteError()
	 *         is set, as on AVR
	 */
	size_t	write( uint8_t data ) override;

	/** Queue multiple bytes into the current transaction's write buffer.
	 * @param data bytes to queue
	 * @param length number of bytes
	 * @return number of bytes queued, less than length if the buffer filled up
	 */
	size_t	write( const uint8_t *data, size_t length ) override;

	/** @name write() of an integer queues its low byte
	 *  As on AVR. Without these, `Wire.write(0)` is ambiguous: 0 converts
	 *  to uint8_t and to a const char* equally well. */
	///@{
	size_t	write( unsigned long n ) { return write( (uint8_t)n ); }
	size_t	write( long n )          { return write( (uint8_t)n ); }
	size_t	write( unsigned int n )  { return write( (uint8_t)n ); }
	size_t	write( int n )           { return write( (uint8_t)n ); }
	///@}

	/** Print's write( const char * ) and friends, hidden by the overloads above */
	using	Print::write;

	/** Send the buffered write transaction started by beginTransmission().
	 * @param stop generate a STOP condition (true, default) or a repeated start (false)
	 * @return 0 on success, non-zero status code on failure
	 */
	uint8_t	endTransmission( bool stop = true );

	/** Read a block of data from a target address into an internal buffer,
	 *  to be consumed with available()/read().
	 * @param address target 7-bit I2C address
	 * @param length number of bytes to read, at most WIRE_BUFFER_SIZE (more
	 *        is cut down to that, as AVR cuts down to its 32)
	 * @param stop generate a STOP condition (true, default) or a repeated start (false)
	 * @return number of bytes actually read (0 on failure)
	 */
	uint8_t	requestFrom( const uint8_t address, const size_t length, bool stop = true );

	/** Write a register address, then read from the target after a repeated
	 *  start: AVR's five-argument form, for the usual "select a register,
	 *  read it" transfer in one call.
	 * @param address target 7-bit I2C address
	 * @param quantity number of bytes to read
	 * @param iaddress register address, sent most significant byte first
	 * @param isize number of bytes of iaddress to send, 0 to 3 (more is cut
	 *        down to 3, as on AVR). 0 sends nothing and just reads
	 * @param sendStop generate a STOP condition after the read (true) or not
	 * @return number of bytes actually read; 0 if the target did not
	 *         acknowledge the register address, where AVR goes on to
	 *         attempt the read anyway
	 */
	uint8_t	requestFrom( uint8_t address, uint8_t quantity, uint32_t iaddress, uint8_t isize, uint8_t sendStop );

	/** @return number of bytes remaining to be read() from the last requestFrom() */
	int		available( void ) override;

	/** @return next byte from the last requestFrom(), or -1 if none remain */
	int		read( void ) override;

	/** @return next byte from the last requestFrom() without consuming it, or -1 if none remain */
	int		peek( void ) override;

	/** Does nothing: every transfer has finished by the time
	 *  endTransmission() or requestFrom() returns. The same as AVR's. */
	void	flush( void ) override {}

	/** Abort a transfer instead of hanging when a target holds the bus.
	 *
	 *  Same signature and defaults as ArduinoCore-avr. Disabled until
	 *  called, as on AVR. Can be called before or after begin().
	 *
	 *  What is timed is how long SCL or SDA stays low in one stretch (the
	 *  LPI2C's hardware pin-low timeout), not the whole transaction: a
	 *  target clock-stretching or holding SDA past the limit trips it; a
	 *  long transfer that keeps the bus moving does not. The hardware
	 *  counter caps the limit: ~87ms on FRDM-MCXN947 at any speed, but on
	 *  FRDM-MCXA153 ~87ms at 100kHz and only ~21.8ms at 400kHz, so even the
	 *  25ms default is clamped there. Longer values are clamped to the cap.
	 *
	 *  No effect on Wire1: that bus runs on the I3C peripheral rather than
	 *  LPI2C, and is not covered, so getWireTimeoutFlag() stays false there.
	 *
	 * @param timeout limit in microseconds, 0 to disable (default 25ms)
	 * @param reset_with_timeout re-initialize the bus hardware after a timeout
	 */
	void	setWireTimeout( uint32_t timeout = 25000, bool reset_with_timeout = false );

	/** @return true if a transfer has timed out since the flag was last cleared */
	bool	getWireTimeoutFlag( void );

	/** Clear the flag getWireTimeoutFlag() reports. */
	void	clearWireTimeoutFlag( void );

private:
	void	start( int baud );
	bool	on_i3c_pins( void ) const;
	void	check_timeout( int status );

	const int	_sda;
	const int	_scl;
	I2C			*i2c;
	uint8_t		targ_addr;
	int			baudrate;
	uint8_t		tx_buf[ WIRE_BUFFER_SIZE ];
	size_t		tx_size;
	uint8_t		rx_buf[ WIRE_BUFFER_SIZE ];
	size_t		rx_index;
	size_t		rx_size;
	uint32_t	timeout_us;
	bool		reset_on_timeout;
	bool		timed_out;
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
