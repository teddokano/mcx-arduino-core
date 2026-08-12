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
	virtual int		available( void ) = 0;
	virtual int		read( void ) = 0;
	virtual int		peek( void ) = 0;

	void	setTimeout( unsigned long timeout ) { _timeout = timeout; }

	size_t	readBytes( char *buffer, size_t length );
	size_t	readBytesUntil( char terminator, char *buffer, size_t length );

	String	readString( void );
	String	readStringUntil( char terminator );

	long	parseInt( void );
	float	parseFloat( void );

	bool	find( const char *target );
	bool	find( const char *target, size_t length );
	bool	findUntil( const char *target, const char *terminator );

protected:
	int		_timed_read( void );
	int		_timed_peek( void );
	double	_parseNumber( bool allow_decimal );

	unsigned long	_timeout	= 1000;
};

#endif // !R01LIB_ARDUINO_STREAM_H
