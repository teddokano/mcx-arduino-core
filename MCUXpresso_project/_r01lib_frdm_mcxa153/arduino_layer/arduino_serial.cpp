/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

/*
 * Note: r01lib.h defines a class named "Serial".
 * To avoid conflict with the global "SerialClass Serial" instance defined here,
 * we use a type alias so that "r01libSerial" refers to the r01lib Serial class.
 */

#include	"r01lib.h"

// Alias r01lib's Serial class before arduino.h redefines the name space
using r01libSerial = Serial;

#include	"arduino_serial.h"
#include	"arduino_io.h"

#include	<cstdio>
#include	<cstring>

// Global Arduino-compatible Serial instance
SerialClass	Serial( USBTX, USBRX );

SerialClass::SerialClass( int tx, int rx )
	: _tx( tx ), _rx( rx ), _baud( 115200 ), _serial( nullptr )
{
}

void SerialClass::begin( int baud )
{
	_baud = baud;
	if ( _serial )
		delete _serial;
	_serial = new r01libSerial( _tx, _rx, _baud );
}

// ---- print overloads ----

void SerialClass::print( const char *s )
{
	if ( !_serial ) return;
	while ( *s )
		_serial->putc( *s++ );
}

void SerialClass::print( char c )
{
	if ( !_serial ) return;
	_serial->putc( c );
}

void SerialClass::_print_num( long n, int base )
{
	if ( !_serial ) return;
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
	if ( !_serial ) return;
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

void SerialClass::print( double n )
{
	if ( !_serial ) return;
	char buf[ 32 ];
	snprintf( buf, sizeof(buf), "%g", n );
	print( buf );
}

void SerialClass::print( const std::string& s )  { print( s.c_str() ); }
void SerialClass::print( std::string_view s )
{
	if ( !_serial ) return;
	for ( char c : s ) _serial->putc( c );
}

// ---- println overloads ----

void SerialClass::println( void )                        { print( "\r\n" ); }
void SerialClass::println( const char *s )               { print(s); println(); }
void SerialClass::println( char c )                      { print(c); println(); }
void SerialClass::println( int n, int base )             { print(n, base); println(); }
void SerialClass::println( unsigned int n, int base )    { print(n, base); println(); }
void SerialClass::println( long n, int base )            { print(n, base); println(); }
void SerialClass::println( unsigned long n, int base )   { print(n, base); println(); }
void SerialClass::println( double n )                    { print(n); println(); }
void SerialClass::println( const std::string& s )        { print(s); println(); }
void SerialClass::println( std::string_view s )          { print(s); println(); }

// ---- printf ----

void SerialClass::printf( const char *format, ... )
{
	if ( !_serial ) return;
	char buf[ 256 ];
	va_list args;
	va_start( args, format );
	vsnprintf( buf, sizeof(buf), format, args );
	va_end( args );
	print( buf );
}

// ---- read / available / write ----

int SerialClass::read( void )
{
	if ( !_serial ) return -1;
	return _serial->getc();
}

int SerialClass::available( void )
{
	if ( !_serial ) return 0;
	return _serial->readable() ? 1 : 0;
}

void SerialClass::write( uint8_t c )
{
	if ( !_serial ) return;
	_serial->putc( c );
}
