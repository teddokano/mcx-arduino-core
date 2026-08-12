/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  Arduino-compatible Print class -- a hardware-independent abstract base
 *  for anything bytes can be written to. This is an original, from-scratch
 *  implementation matching the Arduino Print API surface, not a port of
 *  ArduinoCore-avr/ArduinoCore-API's Print.cpp (which is LGPL 2.1).
 *
 *  A derived class only needs to implement write(uint8_t) (and optionally
 *  the bulk write(const uint8_t*, size_t) for efficiency) to get every
 *  print()/println() overload below for free -- matching how real Arduino
 *  libraries expect to inherit from Print.
 */

#ifndef R01LIB_ARDUINO_PRINT_H
#define R01LIB_ARDUINO_PRINT_H

#include	<stdint.h>
#include	<stddef.h>
#include	<string>
#include	<string_view>

#include	"arduino_string.h"
#include	"Printable.h"

// Number base definitions, shared by print()/println() and String's
// numeric constructors/concat.
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

class Print
{
public:
	virtual ~Print() {}

	virtual size_t	write( uint8_t c ) = 0;
	virtual size_t	write( const uint8_t *buffer, size_t size );
	size_t	write( const char *buffer, size_t size );
	size_t	write( const char *str );

	virtual int		availableForWrite( void ) { return 0; }

	size_t	print( const char *s );
	size_t	print( char c );
	size_t	print( const std::string &s );
	size_t	print( std::string_view s );
	size_t	print( const String &s );
	size_t	print( const __FlashStringHelper *pstr );
	size_t	print( const Printable &p );
	size_t	print( int n, int base = DEC );
	size_t	print( unsigned int n, int base = DEC );
	size_t	print( long n, int base = DEC );
	size_t	print( unsigned long n, int base = DEC );
	size_t	print( long long n, int base = DEC );
	size_t	print( unsigned long long n, int base = DEC );
	size_t	print( double n, int digits = 2 );

	size_t	println( void );
	size_t	println( const char *s );
	size_t	println( char c );
	size_t	println( const std::string &s );
	size_t	println( std::string_view s );
	size_t	println( const String &s );
	size_t	println( const __FlashStringHelper *pstr );
	size_t	println( const Printable &p );
	size_t	println( int n, int base = DEC );
	size_t	println( unsigned int n, int base = DEC );
	size_t	println( long n, int base = DEC );
	size_t	println( unsigned long n, int base = DEC );
	size_t	println( long long n, int base = DEC );
	size_t	println( unsigned long long n, int base = DEC );
	size_t	println( double n, int digits = 2 );

private:
	size_t	_print_num( long n, int base );
	size_t	_print_unum( unsigned long n, int base );
	size_t	_print_num64( long long n, int base );
	size_t	_print_unum64( unsigned long long n, int base );
	size_t	_print_double( double val, int digits );
};

#endif // !R01LIB_ARDUINO_PRINT_H
