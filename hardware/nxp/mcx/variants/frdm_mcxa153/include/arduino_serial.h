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

// Number base definitions
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

// Forward-declare r01lib's Serial to avoid including r01lib.h here
// (which would create circular dependency via arduino.h)
class Serial;

/**
 * @brief Arduino-compatible Serial wrapper around r01lib's Serial class.
 *
 * Provides begin()/print()/println()/printf() API.
 * The internal r01lib Serial instance is created lazily in begin().
 */
class SerialClass
{
public:
	SerialClass( int tx, int rx );

	void	begin( int baud );

	void	print( const char *s );
	void	print( int n, int base = DEC );
	void	print( unsigned int n, int base = DEC );
	void	print( long n, int base = DEC );
	void	print( unsigned long n, int base = DEC );
	void	print( double n, int digits = 2 );
	void	print( char c );
	void	print( const std::string& s );
	void	print( std::string_view s );

	void	println( void );
	void	println( const char *s );
	void	println( int n, int base = DEC );
	void	println( unsigned int n, int base = DEC );
	void	println( long n, int base = DEC );
	void	println( unsigned long n, int base = DEC );
	void	println( double n, int digits = 2 );
	void	println( char c );
	void	println( const std::string& s );
	void	println( std::string_view s );

	void	printf( const char *format, ... );

	int		read( void );
	int		available( void );
	void	write( uint8_t c );

private:
	int		_tx;
	int		_rx;
	int		_baud;
	Serial*	_serial;   // r01lib Serial (forward-declared above)

	void	_print_num( long n, int base );
	void	_print_unum( unsigned long n, int base );
	void	_print_double( double val, int digits );
};

extern SerialClass	Serial;

#endif // !R01LIB_ARDUINO_SERIAL_H
