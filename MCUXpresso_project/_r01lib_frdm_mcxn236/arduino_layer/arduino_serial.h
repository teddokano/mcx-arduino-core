/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_SERIAL_H
#define R01LIB_ARDUINO_SERIAL_H

#include	<string>
#include	<string_view>

#include    <stdarg.h>

// Number base definitions
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

class SerialClass
{
public:
    void	begin( int baud );
	
    void	print( const char *s );
    void	print( int n, int base = DEC );
    void	print( unsigned int n, int base = DEC );
    void	print( long n, int base = DEC );
    void	print( unsigned long n, int base = DEC );
    void	print( double n );
    void	print( char c );
    void	print( const std::string& s );
    void	print( std::string_view s );

	void	println( void );
    void	println( const char *s );
    void	println( int n, int base = DEC );
    void	println( unsigned int n, int base = DEC );
    void	println( long n, int base = DEC );
    void	println( unsigned long n, int base = DEC );
    void	println( double n );
    void	println( char c );
    void	println( const std::string& s );
    void	println( std::string_view s );

	void	printf( const char *format, ... );

private:
    void	mcu_init_stdio( int baud );
};

extern SerialClass	Serial;

#endif // !R01LIB_ARDUINO_SERIAL_H
