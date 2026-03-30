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

void SerialClass::print( double n, int digits )
{
	if ( !_serial ) return;
	if ( n < 0.0 ) { _serial->putc( '-' ); n = -n; }
	long integer	= (long)n;
	double frac		= n - integer;
	char buf[ 32 ];
	sprintf( buf, "%ld.", integer );
	print( buf );
	for ( int i = 0; i < digits; i++ )
	{
		frac	*= 10;
		int d	= (int)frac;
		_serial->putc( '0' + d );
		frac	-= d;
	}
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
void SerialClass::println( double n, int digits )    { print(n, digits); println(); }
void SerialClass::println( const std::string& s )        { print(s); println(); }
void SerialClass::println( std::string_view s )          { print(s); println(); }

// ---- printf ----

//	Helper: output a double value with given decimal digits
void SerialClass::_print_double( double val, int digits )
{
	if ( val < 0.0 ) { _serial->putc( '-' ); val = -val; }
	long integer	= (long)val;
	double frac		= val - integer;
	char buf[ 32 ];
	sprintf( buf, "%ld.", integer );
	for ( char *p = buf; *p; p++ ) _serial->putc( *p );
	for ( int i = 0; i < digits; i++ )
	{
		frac	*= 10;
		int d	= (int)frac;
		_serial->putc( '0' + d );
		frac	-= d;
	}
}

void SerialClass::printf( const char *format, ... )
{
	if ( !_serial ) return;
	va_list args;
	va_start( args, format );

	const char *p	= format;
	while ( *p )
	{
		if ( *p != '%' ) { _serial->putc( *p++ ); continue; }

		//	Collect format specifier
		const char *spec_start	= p++;
		while ( *p && !strchr( "fFeEgGdiouxXcsp%lL", *p ) ) p++;
		if ( *p == 'l' || *p == 'L' ) p++;		//	skip length modifier
		char type	= *p ? *p++ : '\0';

		//	Parse precision from spec (default=6)
		int digits	= 6;
		const char *pp	= spec_start + 1;
		while ( *pp && *pp != '.' && *pp != 'f' && *pp != 'g' && *pp != 'e'
		        && *pp != 'F' && *pp != 'G' && *pp != 'E' ) pp++;
		if ( *pp == '.' ) { pp++; digits = 0; while ( *pp >= '0' && *pp <= '9' ) digits = digits * 10 + (*pp++ - '0'); }

		if ( type == 'f' || type == 'F' || type == 'e' || type == 'E' || type == 'g' || type == 'G' )
		{
			_print_double( va_arg( args, double ), digits );
		}
		else if ( type == '%' )
		{
			_serial->putc( '%' );
		}
		else
		{
			//	Non-float: reconstruct spec and use snprintf
			char spec_buf[ 32 ];
			int spec_len	= (int)(p - spec_start);
			if ( spec_len < (int)sizeof(spec_buf) )
			{
				strncpy( spec_buf, spec_start, spec_len );
				spec_buf[ spec_len ]	= '\0';
			}
			else { spec_buf[ 0 ] = '\0'; }

			char out[ 64 ];
			switch ( type )
			{
				case 'd': case 'i':                          snprintf( out, sizeof(out), spec_buf, va_arg( args, int ) );            break;
				case 'o': case 'u': case 'x': case 'X':     snprintf( out, sizeof(out), spec_buf, va_arg( args, unsigned int ) );   break;
				case 'c':                                    snprintf( out, sizeof(out), spec_buf, va_arg( args, int ) );            break;
				case 's':                                    snprintf( out, sizeof(out), spec_buf, va_arg( args, const char* ) );   break;
				case 'p':                                    snprintf( out, sizeof(out), spec_buf, va_arg( args, void* ) );          break;
				default:                                     out[ 0 ] = '\0'; break;
			}
			print( out );
		}
	}
	va_end( args );
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
