/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  Arduino-compatible Stream class -- a hardware-independent abstract base
 *  for a byte source, adding polled/timeout-based parsing helpers on top
 *  of Print. Original implementation, not a port of ArduinoCore-avr/
 *  ArduinoCore-API's Stream (LGPL 2.1).
 */

#ifndef R01LIB_ARDUINO_STREAM_H
#define R01LIB_ARDUINO_STREAM_H

#include	"Print.h"

class Stream : public Print
{
public:
	/** @return number of bytes available to read without blocking */
	virtual int		available( void ) = 0;
	/** @return next byte, consuming it, or -1 if none available */
	virtual int		read( void ) = 0;
	/** @return next byte without consuming it, or -1 if none available */
	virtual int		peek( void ) = 0;

	/** Set how long the read.../parseInt/parseFloat/find... family below
	 *  will wait for more data before giving up.
	 * @param timeout timeout in milliseconds (default 1000)
	 */
	void	setTimeout( unsigned long timeout ) { _timeout = timeout; }

	/** Read up to length bytes, stopping early on timeout.
	 * @param buffer destination buffer
	 * @param length maximum bytes to read
	 * @return number of bytes actually read
	 */
	size_t	readBytes( char *buffer, size_t length );

	/** Read up to length bytes, stopping early on timeout or when
	 *  terminator is read (terminator itself is consumed but not stored).
	 * @param terminator byte that ends the read
	 * @param buffer destination buffer
	 * @param length maximum bytes to read
	 * @return number of bytes actually read
	 */
	size_t	readBytesUntil( char terminator, char *buffer, size_t length );

	/** Read bytes into a String until timeout (no explicit length limit).
	 * @return the bytes read (possibly empty, if nothing arrived before timeout)
	 */
	String	readString( void );

	/** Read bytes into a String until terminator is read (consumed but not
	 *  included) or timeout.
	 * @param terminator byte that ends the read
	 * @return the bytes read
	 */
	String	readStringUntil( char terminator );

	/** Skip any non-numeric bytes, then read a base-10 integer (optional
	 *  leading '-'), stopping at the first non-digit or on timeout.
	 * @return the parsed value, or 0 if none was found before timeout
	 */
	long	parseInt( void );

	/** Same as parseInt(), but also accepts a decimal point.
	 * @return the parsed value, or 0 if none was found before timeout
	 */
	float	parseFloat( void );

	/** Read and discard bytes until target is matched or timeout.
	 * @param target NUL-terminated byte sequence to search for
	 * @return true if target was found, false on timeout
	 */
	bool	find( const char *target );

	/** Read and discard bytes until the first length bytes of target are
	 *  matched or timeout.
	 * @param target byte sequence to search for
	 * @param length number of bytes of target to match
	 * @return true if found, false on timeout
	 */
	bool	find( const char *target, size_t length );

	/** Like find(), but also gives up (returning false) if terminator is
	 *  matched before target is.
	 * @param target NUL-terminated byte sequence to search for
	 * @param terminator NUL-terminated byte sequence that aborts the search if seen first
	 * @return true if target was found before terminator or timeout, false otherwise
	 */
	bool	findUntil( const char *target, const char *terminator );

protected:
	/** @return next byte, waiting up to _timeout ms for one to arrive; -1 on timeout */
	int		_timed_read( void );
	/** @return next byte without consuming it, waiting up to _timeout ms for one to arrive; -1 on timeout */
	int		_timed_peek( void );
	/** Shared implementation behind parseInt()/parseFloat().
	 * @param allow_decimal accept a decimal point (parseFloat()) or not (parseInt())
	 * @return the parsed value, or 0 if none was found before timeout
	 */
	double	_parseNumber( bool allow_decimal );

	unsigned long	_timeout	= 1000;
};

#endif // !R01LIB_ARDUINO_STREAM_H
