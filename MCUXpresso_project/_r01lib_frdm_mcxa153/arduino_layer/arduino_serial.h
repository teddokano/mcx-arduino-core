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
#include	"Serial.h"

// Number base definitions
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

/**
 * @brief Arduino-compatible Serial class for NXP MCX BSP.
 *
 * Inherits r01lib's Serial directly (no heap allocation, no wrapper).
 * Pinned to USBTX/USBRX. Call begin(baud) to initialise.
 */
class SerialClass : public Serial
{
public:
	SerialClass() : Serial( USBTX, USBRX ) {}

	void	begin( int baud ) { this->baud( baud ); }

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

	int		available( void ) { return readable() ? 1 : 0; }
	int		read( void )      { return getc(); }
	void	write( uint8_t c ){ putc( c ); }

	inline operator bool( void ) { return true; }

private:
	void	_print_num( long n, int base );
	void	_print_unum( unsigned long n, int base );
	void	_print_double( double val, int digits );
};

extern SerialClass	Serial;

#endif // !R01LIB_ARDUINO_SERIAL_H
