/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"arduino_serial.h"
#include	"arduino_io.h"
#include	"arduino.h"		// for millis(), used by the Stream-style helpers' timeouts

#include	<cstdio>
#include	<cstring>

// Global Arduino-compatible Serial instances (no heap, directly inherit r01lib Serial)
// Serial:  USB-bridged UART (USBTX/USBRX, not part of the pin-renumbering table)
// Serial1: hardware UART on D0(RX)/D1(TX) -- arduino_pin_by_number[] converts the
//          renumbered D0/D1 indices back to the raw physical pin macros
//          Serial::resolve_pins() expects (same trick pinMode()/digitalWrite() use).
SerialClass	Serial(  USBTX, USBRX );
SerialClass	Serial1( arduino_pin_by_number[ D1 ], arduino_pin_by_number[ D0 ] );

// ---- print overloads ----

namespace {

/*
 *  snprintf()/printf() have no format specifier for arbitrary radixes (no
 *  %b for binary, and nothing at all for bases other than 8/10/16), so
 *  BIN -- and any other non-DEC/HEX/OCT base a sketch might pass, matching
 *  real Arduino's support for any radix 2-36 -- has to be hand-converted.
 */
void _utoa_radix( unsigned long long value, int base, char *buf, size_t bufsize )
{
	static const char	digits[]	= "0123456789abcdefghijklmnopqrstuvwxyz";

	if ( base < 2 || base > 36 )
		base	= 10;

	char	tmp[ 66 ];
	int		i	= 0;

	if ( value == 0 )
		tmp[ i++ ]	= '0';

	while ( value > 0 && i < (int)sizeof(tmp) - 1 )
	{
		tmp[ i++ ]	= digits[ value % (unsigned)base ];
		value		/= (unsigned)base;
	}

	size_t	j	= 0;
	while ( i > 0 && j < bufsize - 1 )
		buf[ j++ ]	= tmp[ --i ];
	buf[ j ]	= '\0';
}

}	// namespace

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
	else if ( n < 0 )
	{
		putc( '-' );
		_utoa_radix( (unsigned long long)( -(long long)n ), base, buf, sizeof(buf) );
	}
	else
		_utoa_radix( (unsigned long long)n, base, buf, sizeof(buf) );
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
		_utoa_radix( (unsigned long long)n, base, buf, sizeof(buf) );
	print( buf );
}

void SerialClass::_print_num64( long long n, int base )
{
	char buf[ 66 ];
	if ( base == DEC )
		snprintf( buf, sizeof(buf), "%lld", n );
	else if ( base == HEX )
		snprintf( buf, sizeof(buf), "%llx", (unsigned long long)n );
	else if ( base == OCT )
		snprintf( buf, sizeof(buf), "%llo", (unsigned long long)n );
	else if ( n < 0 )
	{
		putc( '-' );
		_utoa_radix( (unsigned long long)( -n ), base, buf, sizeof(buf) );
	}
	else
		_utoa_radix( (unsigned long long)n, base, buf, sizeof(buf) );
	print( buf );
}

void SerialClass::_print_unum64( unsigned long long n, int base )
{
	char buf[ 66 ];
	if ( base == DEC )
		snprintf( buf, sizeof(buf), "%llu", n );
	else if ( base == HEX )
		snprintf( buf, sizeof(buf), "%llx", n );
	else if ( base == OCT )
		snprintf( buf, sizeof(buf), "%llo", n );
	else
		_utoa_radix( n, base, buf, sizeof(buf) );
	print( buf );
}

void SerialClass::print( int n, int base )                { _print_num( n, base ); }
void SerialClass::print( unsigned int n, int base )       { _print_unum( n, base ); }
void SerialClass::print( long n, int base )               { _print_num( n, base ); }
void SerialClass::print( unsigned long n, int base )      { _print_unum( n, base ); }
void SerialClass::print( long long n, int base )          { _print_num64( n, base ); }
void SerialClass::print( unsigned long long n, int base ) { _print_unum64( n, base ); }

void SerialClass::print( double n, int digits )
{
	_print_double( n, digits );
}

void SerialClass::print( const std::string& s )  { print( s.c_str() ); }
void SerialClass::print( std::string_view s )
{
	for ( char c : s ) putc( c );
}
void SerialClass::print( const String& s )        { print( s.c_str() ); }
void SerialClass::print( const __FlashStringHelper *pstr )
{
	print( reinterpret_cast<const char *>( pstr ) );
}

// ---- println overloads ----

void SerialClass::println( void )                        { print( "\r\n" ); }
void SerialClass::println( const char *s )               { print(s); println(); }
void SerialClass::println( char c )                      { print(c); println(); }
void SerialClass::println( int n, int base )             { print(n, base); println(); }
void SerialClass::println( unsigned int n, int base )    { print(n, base); println(); }
void SerialClass::println( long n, int base )            { print(n, base); println(); }
void SerialClass::println( unsigned long n, int base )   { print(n, base); println(); }
void SerialClass::println( long long n, int base )       { print(n, base); println(); }
void SerialClass::println( unsigned long long n, int base ) { print(n, base); println(); }
void SerialClass::println( double n, int digits )        { print(n, digits); println(); }
void SerialClass::println( const std::string& s )        { print(s); println(); }
void SerialClass::println( std::string_view s )          { print(s); println(); }
void SerialClass::println( const String& s )              { print(s); println(); }
void SerialClass::println( const __FlashStringHelper *pstr ) { print(pstr); println(); }

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

// ---- Stream-style helpers ----

int SerialClass::_timed_read( void )
{
	unsigned long	start	= millis();
	int				c;

	do
	{
		c	= read();
		if ( c >= 0 )
			return	c;
	} while ( ( millis() - start ) < _timeout );

	return	-1;
}

int SerialClass::_timed_peek( void )
{
	unsigned long	start	= millis();
	int				c;

	do
	{
		c	= peek();
		if ( c >= 0 )
			return	c;
	} while ( ( millis() - start ) < _timeout );

	return	-1;
}

size_t SerialClass::readBytes( char *buffer, size_t length )
{
	size_t	count	= 0;

	while ( count < length )
	{
		int	c	= _timed_read();
		if ( c < 0 )
			break;
		buffer[ count++ ]	= (char)c;
	}

	return	count;
}

size_t SerialClass::readBytesUntil( char terminator, char *buffer, size_t length )
{
	size_t	count	= 0;

	while ( count < length )
	{
		int	c	= _timed_read();
		if ( c < 0 || c == terminator )
			break;
		buffer[ count++ ]	= (char)c;
	}

	return	count;
}

String SerialClass::readString( void )
{
	String	result;
	int		c;

	while ( ( c = _timed_read() ) >= 0 )
		result.concat( (char)c );

	return	result;
}

String SerialClass::readStringUntil( char terminator )
{
	String	result;
	int		c;

	while ( ( c = _timed_read() ) >= 0 && c != terminator )
		result.concat( (char)c );

	return	result;
}

double SerialClass::_parseNumber( bool allow_decimal )
{
	bool	negative	= false;
	double	value		= 0;
	double	frac		= 0;
	double	frac_scale	= 1;
	bool	in_frac		= false;

	int	c	= _timed_peek();

	// discard anything that isn't the start of a number
	while ( c >= 0 && c != '-' && ( c < '0' || c > '9' ) && !( allow_decimal && c == '.' ) )
	{
		read();
		c	= _timed_peek();
	}

	if ( c == '-' )
	{
		negative	= true;
		read();
		c	= _timed_peek();
	}

	while ( c >= 0 )
	{
		if ( c >= '0' && c <= '9' )
		{
			if ( in_frac )
			{
				frac_scale	*= 0.1;
				frac		+= ( c - '0' ) * frac_scale;
			}
			else
			{
				value	= value * 10 + ( c - '0' );
			}
			read();
		}
		else if ( allow_decimal && c == '.' && !in_frac )
		{
			in_frac	= true;
			read();
		}
		else
		{
			break;
		}

		c	= _timed_peek();
	}

	value	+= frac;
	return	negative ? -value : value;
}

long SerialClass::parseInt( void )
{
	return	(long)_parseNumber( false );
}

float SerialClass::parseFloat( void )
{
	return	(float)_parseNumber( true );
}

bool SerialClass::find( const char *target )
{
	size_t	target_len	= strlen( target );

	if ( target_len == 0 )
		return	true;

	size_t	matched	= 0;

	while ( true )
	{
		int	c	= _timed_read();
		if ( c < 0 )
			return	false;

		if ( (char)c == target[ matched ] )
		{
			matched++;
			if ( matched == target_len )
				return	true;
		}
		else
		{
			matched	= ( (char)c == target[ 0 ] ) ? 1 : 0;
		}
	}
}
