/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"arduino_serial.h"
#include	"arduino_io.h"

#include	<cstdio>
#include	<cstring>

// Global Arduino-compatible Serial instance (no heap, directly inherits r01lib Serial)
SerialClass	Serial;

// ---- print overloads ----

void SerialClass::print( const char *s )
{
	while ( *s )
		putc( *s++ );
}

void SerialClass::print( char c )
{
	putc( c );
}

void SerialClass::_print_num( long n, int base )
{
	char buf[ 34 ];
	if ( base == DEC )
		snprintf( buf, sizeof(buf), "%ld", n );
	else if ( base == HEX )
		snprintf( buf, sizeof(buf), "%lx", (unsigned long)n );
	else if ( base == OCT )
		snprintf( buf, sizeof(buf), "%lo", (unsigned long)n );
	else
		snprintf( buf, sizeof(buf), "%ld", n );
	print( buf );
}

void SerialClass::_print_unum( unsigned long n, int base )
{
	char buf[ 34 ];
	if ( base == DEC )
		snprintf( buf, sizeof(buf), "%lu", n );
	else if ( base == HEX )
		snprintf( buf, sizeof(buf), "%lx", n );
	else if ( base == OCT )
		snprintf( buf, sizeof(buf), "%lo", n );
	else
		snprintf( buf, sizeof(buf), "%lu", n );
	print( buf );
}

void SerialClass::print( int n, int base )           { _print_num( n, base ); }
void SerialClass::print( unsigned int n, int base )  { _print_unum( n, base ); }
void SerialClass::print( long n, int base )          { _print_num( n, base ); }
void SerialClass::print( unsigned long n, int base ) { _print_unum( n, base ); }

void SerialClass::print( double n, int digits )
{
	_print_double( n, digits );
}

void SerialClass::print( const std::string& s )  { print( s.c_str() ); }
void SerialClass::print( std::string_view s )
{
	for ( char c : s ) putc( c );
}

// ---- println overloads ----

void SerialClass::println( void )                        { print( "\r\n" ); }
void SerialClass::println( const char *s )               { print(s); println(); }
void SerialClass::println( char c )                      { print(c); println(); }
void SerialClass::println( int n, int base )             { print(n, base); println(); }
void SerialClass::println( unsigned int n, int base )    { print(n, base); println(); }
void SerialClass::println( long n, int base )            { print(n, base); println(); }
void SerialClass::println( unsigned long n, int base )   { print(n, base); println(); }
void SerialClass::println( double n, int digits )        { print(n, digits); println(); }
void SerialClass::println( const std::string& s )        { print(s); println(); }
void SerialClass::println( std::string_view s )          { print(s); println(); }

// ---- helpers ----

void SerialClass::_print_double( double val, int digits )
{
	if ( val < 0.0 ) { putc( '-' ); val = -val; }
	long integer	= (long)val;
	double frac		= val - integer;
	char buf[ 32 ];
	sprintf( buf, "%ld.", integer );
	print( buf );
	for ( int i = 0; i < digits; i++ )
	{
		frac	*= 10;
		int d	= (int)frac;
		putc( '0' + d );
		frac	-= d;
	}
}
