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
struct WireTarget;	// target (slave) mode state, arduino_i2c.cpp; allocated
					// only by begin(address)

/** Same feature macro as ArduinoCore-avr, so libraries that guard their
 *  setWireTimeout() calls with it pick it up here too. */
#define	WIRE_HAS_TIMEOUT

/** Wire has end(). The same macro, and value, as ArduinoCore-avr's, which
 *  libraries check before calling end(). */
#define	WIRE_HAS_END	1

/** Size of each of TwoWire's two buffers: the most one endTransmission()
 *  sends or one requestFrom() reads. AVR's is 32. */
#define	WIRE_BUFFER_SIZE	128

/** Arduino-compatible I2C (Wire) class.
 *
 *  Wraps an r01lib I2C (or, when sda_pin/scl_pin are this board's I3C
 *  pins, an I3C running in legacy I2C_MODE; not on FRDM-MCXA156, where
 *  every TwoWire is an LPI2C) behind the classic
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
 *
 *  begin(address) also makes it a target (slave), as on AVR: onReceive()
 *  and onRequest() handlers run, from the interrupt, when a controller
 *  writes to or reads from that address, and the bus can still be used
 *  as a controller at the same time. Not on Wire1 where it runs on the
 *  I3C peripheral (FRDM-MCXA153 and FRDM-MCXN947), and not on
 *  FRDM-MCXN947's Wire2, whose LPI2C doesn't respond as a target (see
 *  begin(uint8_t)).
 */
class TwoWire : public Stream
{
public:
	/** Construct on the given SDA/SCL pin pair. Hardware isn't touched
	 *  until begin().
	 *
	 *  This is how the core makes Wire, Wire1 and Wire2, and it is not
	 *  meant for sketches: the pins are r01lib's raw pin values, which
	 *  arduino_i2c.cpp sees since it does not include the Arduino pin
	 *  renumbering. In a sketch, names such as MB_SDA or A4 are Arduino pin
	 *  numbers, a different value, so a TwoWire made from them stops in
	 *  panic() at begin(). Use Wire, Wire1 or Wire2.
	 * @param sda_pin SDA pin, as r01lib's raw pin value
	 * @param scl_pin SCL pin, as r01lib's raw pin value
	 */
	TwoWire( int sda_pin, int scl_pin );

	/** Join the bus as the controller (master), at 100kHz; setClock()
	 *  changes that. Lazily creates the underlying I2C or I3C instance
	 *  (I3C, in I2C_MODE, if sda_pin/scl_pin are this board's
	 *  I3C_SDA/I3C_SCL). On the LPI2C buses (Wire, Wire2) it also turns on
	 *  the pins' internal pull-ups, as AVR and UNO R4 do.
	 */
	void	begin( void );

	/** Join the bus as a target (slave) at the given address, as well as
	 *  a controller, as in every official core. Controller transfers keep
	 *  working alongside. Calls panic() for an address over 127, and on
	 *  Wire1 where it runs on the I3C peripheral (FRDM-MCXA153 and
	 *  FRDM-MCXN947), and on FRDM-MCXN947's Wire2 (LPI2C3), which never
	 *  sees the bus as a target, though it works as a controller and is
	 *  set up the same as Wire's LPI2C2, which does.
	 *
	 *  A later begin() with no address goes back to controller only.
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

	/** Deinitialize the bus, target side included, turn the internal
	 *  pull-ups off again (as AVR does), and free the underlying I2C/I3C
	 *  instance. */
	void	end( void );

	/** Not in AVR's API (ESP32 has the same one): move this bus to A4/A5,
	 *  or back to its own pins, for shields made for UNO boards whose I2C
	 *  is on A4/A5. Call it before begin(), or after end().
	 *
	 *  A4/A5 have an I2C on FRDM-MCXA156 (LPI2C1; Wire or Wire1 can move
	 *  there), FRDM-MCXN236 (LPI2C2, Wire1's own; Wire or Wire1) and
	 *  FRDM-MCXC444 (I2C1, Wire's own; Wire only, as Wire1's own pins
	 *  have no Arduino pin number to move it back by), and none on
	 *  FRDM-MCXA153 and FRDM-MCXN947. One peripheral runs one bus at a
	 *  time: on FRDM-MCXN236, Wire on A4/A5 and Wire1 can't both be
	 *  begun. A begin() that would take a peripheral another TwoWire is
	 *  running stops in panic().
	 *
	 *  The pins it leaves go back to plain GPIO inputs, if this bus had
	 *  them and nothing has taken them since. So do another TwoWire's,
	 *  left on the same peripheral by its end(), when begin() starts it
	 *  here (FRDM-MCXN236: Wire on A4/A5 after Wire1, or the other way).
	 *
	 *  begin() turns on the pins' internal pull-ups, as it does on every
	 *  pin pair. FRDM-MCXA156 and FRDM-MCXN236 have no pull-up resistors
	 *  on A4/A5 (FRDM-MCXC444 has 4.7k), so give the bus external ones
	 *  there unless the shield has them.
	 * @param sda SDA pin (A4, or this bus's own SDA)
	 * @param scl SCL pin (A5, or this bus's own SCL)
	 * @return true if the bus is now on these pins; false, with the pins
	 *         unchanged, if this board has no I2C there, after begin()
	 *         (until end()), or if another TwoWire is running on the
	 *         peripheral behind them
	 */
	bool	setPins( int sda, int scl );

	/** Set the handler called when a controller has written to this
	 *  target, once the write ends (STOP or repeated start). Inside it,
	 *  available()/read() give the bytes written, at most
	 *  WIRE_BUFFER_SIZE; a controller writing more is NAKed from there on.
	 *
	 *  Runs from the LPI2C interrupt, as on AVR: keep it short, and keep
	 *  Serial output in it small, since a full Serial TX buffer waits on
	 *  an interrupt that can't run until this one returns.
	 * @param handler function taking the number of bytes received
	 */
	void	onReceive( void (*handler)( int ) );

	/** Set the handler called when a controller reads from this target.
	 *  Inside it, write() queues the reply (up to WIRE_BUFFER_SIZE bytes).
	 *  A controller that reads more than was queued gets 0xFF for the rest,
	 *  as on AVR; with no handler set, all of it is 0xFF. Runs from the
	 *  interrupt, the same as onReceive()'s handler.
	 * @param handler function taking no arguments
	 */
	void	onRequest( void (*handler)( void ) );

	/** Change the bus frequency at runtime (no-op before begin()).
	 * @param freq SCL frequency in Hz
	 */
	void	setClock( uint32_t freq );

	/** Start buffering a write transaction to the given target address.
	 * @param address target 7-bit I2C address
	 */
	void	beginTransmission( const uint8_t address );

	/** Queue one byte into the current transaction's write buffer, or,
	 *  inside an onRequest() handler, into the reply to the controller.
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
	 * @return as AVR's: 0 success, 2 the address was not acknowledged, 3 a data
	 *  byte was not acknowledged, 4 another error (bus busy, arbitration lost,
	 *  ...), 5 timeout (setWireTimeout()). The LPI2C (Wire on every board but
	 *  FRDM-MCXC444, and the other LPI2C buses) can't tell a data byte's NAK
	 *  from the address's, so it gives 2 for both. lastStatus() has the
	 *  driver's own status code
	 */
	uint8_t	endTransmission( bool stop = true );

	/** Not in the official API: the driver's status code from the last
	 *  endTransmission() or requestFrom() (an MCUXpresso SDK status_t, 0 on
	 *  success), for telling apart what endTransmission()'s 4 covers.
	 *  endTransmission() returned this, cut to 8 bits, before v0.11.0
	 */
	int32_t	lastStatus( void ) const { return last_status; }

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
	 *  No effect on Wire1 where it runs on the I3C peripheral rather than
	 *  LPI2C (FRDM-MCXA153 and FRDM-MCXN947): it is not covered, so
	 *  getWireTimeoutFlag() stays false there. FRDM-MCXA156's Wire1 is an
	 *  LPI2C and is covered.
	 *
	 * @param timeout limit in microseconds, 0 to disable (default 25ms)
	 * @param reset_with_timeout re-initialize the bus hardware after a timeout
	 */
	void	setWireTimeout( uint32_t timeout = 25000, bool reset_with_timeout = false );

	/** @return true if a transfer has timed out since the flag was last cleared */
	bool	getWireTimeoutFlag( void );

	/** Clear the flag getWireTimeoutFlag() reports. */
	void	clearWireTimeoutFlag( void );

	/** Internal: target-side event from the LPI2C slave interrupt. */
	void	target_event( void *xfer );

	/** Internal, for the mcxPinState library: the pins this bus is on now
	 *  (setPins() moves them), as r01lib's raw pin values. Not part of the
	 *  API. */
	int		raw_sda( void ) const { return _sda; }
	int		raw_scl( void ) const { return _scl; }	///< as raw_sda()

private:
	void	start( int baud );
	void	target_arm( void );
	void	target_stop( void );
	bool	on_i3c_pins( void ) const;
	void	check_timeout( int status );
	void	release_pads( void );
	const char	*peripheral( void ) const;
	static TwoWire	*running_on( const char *periph, const TwoWire *except );

	const int	own_sda;	// the pins it was made on, raw r01lib values
	const int	own_scl;
	int			_sda;		// the pins it is on now (setPins())
	int			_scl;
	uint8_t		pad_mux;	// the pins' ALT once begin() muxed them, 0xFF before
	I2C			*i2c;
	uint8_t		targ_addr;
	int			baudrate;
	uint8_t		tx_buf[ WIRE_BUFFER_SIZE ];
	size_t		tx_size;
	uint8_t		rx_buf[ WIRE_BUFFER_SIZE ];	// what requestFrom() read
	const uint8_t	*rx_data;	// what read() reads: rx_buf, or what a
								// controller last wrote to this target
	size_t		rx_index;
	size_t		rx_size;
	WireTarget	*target;
	void		(*user_onReceive)( int );
	void		(*user_onRequest)( void );
	uint32_t	timeout_us;
	bool		reset_on_timeout;
	bool		timed_out;
	int32_t		last_status;
};

/** Global TwoWire instance on this board's general-purpose I2C pins (I2C_SDA/I2C_SCL). */
extern TwoWire	Wire;

/** Global TwoWire instance on this board's I3C pins (I3C_SDA/I3C_SCL), run
 *  in legacy I2C_MODE -- see TwoWire::begin(). Typically wired to an
 *  on-board sensor rather than an external header.
 *
 *  On FRDM-MCXA156 it is the MikroBus I2C (MB_SDA/MB_SCL) instead, on its
 *  own LPI2C3: that board's on-board sensor is on Wire's D18/D19. On
 *  FRDM-MCXN236, which has no I3C sensor, it is the MikroBus I2C too, on
 *  LPI2C2, the same bus as the on-board accelerometer.
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
