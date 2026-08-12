/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_SERIAL_H
#define R01LIB_ARDUINO_SERIAL_H

#include	<string>
#include	<string_view>
#include	<stdarg.h>
#include	<stdint.h>
#include	<cstring>
#include	"Serial.h"
#include	"arduino_string.h"

// Number base definitions
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

/**
 * @brief Arduino-compatible Serial class for NXP MCX BSP.
 *
 * Inherits r01lib's Serial directly (no heap allocation, no wrapper).
 * Call begin(baud) to initialise.
 */
class SerialClass : public Serial
{
public:
	SerialClass( int tx_pin, int rx_pin ) : Serial( tx_pin, rx_pin ) {}

	/*
	 *  attach() registers a (no-op) RX callback purely to switch getc()/
	 *  readable() from raw single-byte hardware-register polling over to
	 *  the interrupt-driven ring buffer (see Serial::_irq_handler()) --
	 *  without it the RX interrupt is never enabled at all, and bytes
	 *  arriving faster than the sketch calls read() get silently
	 *  overwritten in the 1-deep hardware receive register.
	 */
	void	begin( int baud ) { this->baud( baud ); attach( []{}, RxIrq ); }

	void	print( const char *s );
	void	print( int n, int base = DEC );
	void	print( unsigned int n, int base = DEC );
	void	print( long n, int base = DEC );
	void	print( unsigned long n, int base = DEC );
	void	print( long long n, int base = DEC );
	void	print( unsigned long long n, int base = DEC );
	void	print( double n, int digits = 2 );
	void	print( char c );
	void	print( const std::string& s );
	void	print( std::string_view s );
	void	print( const String& s );
	void	print( const __FlashStringHelper *pstr );

	void	println( void );
	void	println( const char *s );
	void	println( int n, int base = DEC );
	void	println( unsigned int n, int base = DEC );
	void	println( long n, int base = DEC );
	void	println( unsigned long n, int base = DEC );
	void	println( long long n, int base = DEC );
	void	println( unsigned long long n, int base = DEC );
	void	println( double n, int digits = 2 );
	void	println( char c );
	void	println( const std::string& s );
	void	println( std::string_view s );
	void	println( const String& s );
	void	println( const __FlashStringHelper *pstr );

	int		available( void ) { return (int)Serial::available(); }
	int		read( void )      { return getc(); }
	int		peek( void )      { return Serial::peek(); }
	void	flush( void )     { Serial::flush(); }

	/*
	 *  All four write() overloads are declared together here (rather than
	 *  relying on `using Serial::write;` to pull in r01lib's own bulk
	 *  write(const uint8_t*, size_t)) so they share Arduino's size_t-
	 *  returns-bytes-written contract -- r01lib's version returns
	 *  status_t (0 = success), which would silently read as "0 bytes
	 *  written" if exposed directly to sketches expecting a byte count.
	 */
	size_t	write( uint8_t c )                              { putc( c ); return 1; }
	size_t	write( const uint8_t *buffer, size_t size )      { Serial::write( buffer, size ); return size; }
	size_t	write( const char *buffer, size_t size )         { return write( (const uint8_t *)buffer, size ); }
	size_t	write( const char *str )                         { return str ? write( (const uint8_t *)str, strlen( str ) ) : 0; }

	inline operator bool( void ) { return true; }

	// ---- Stream-style helpers (polled, using millis()-based timeout) ----

	void	setTimeout( unsigned long timeout ) { _timeout = timeout; }

	size_t	readBytes( char *buffer, size_t length );
	size_t	readBytesUntil( char terminator, char *buffer, size_t length );

	String	readString( void );
	String	readStringUntil( char terminator );

	long	parseInt( void );
	float	parseFloat( void );

	bool	find( const char *target );

private:
	void	_print_num( long n, int base );
	void	_print_unum( unsigned long n, int base );
	void	_print_num64( long long n, int base );
	void	_print_unum64( unsigned long long n, int base );
	void	_print_double( double val, int digits );

	int		_timed_read( void );
	int		_timed_peek( void );
	double	_parseNumber( bool allow_decimal );

	unsigned long	_timeout	= 1000;
};

extern SerialClass	Serial;
extern SerialClass	Serial1;	// hardware UART on D0(RX)/D1(TX), separate from the USB-bridged Serial

#endif // !R01LIB_ARDUINO_SERIAL_H
