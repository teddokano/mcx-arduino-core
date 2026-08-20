/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"Print.h"

#include	<cstdio>
#include	<cstring>

// ---- write() ----

size_t Print::write( const uint8_t *buffer, size_t size )
{
	size_t	n	= 0;

	while ( size-- )
		n	+= write( *buffer++ );

	return	n;
}

size_t Print::write( const char *buffer, size_t size )
{
	return	write( (const uint8_t *)buffer, size );
}

size_t Print::write( const char *str )
{
	return	str ? write( (const uint8_t *)str, strlen( str ) ) : 0;
}

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

size_t Print::print( const char *s )
{
	return	write( s );
}

size_t Print::print( char c )
{
	return	write( (uint8_t)c );
}

size_t Print::_print_num( long n, int base )
{
	char	buf[ 34 ];
	if ( base == DEC )
		snprintf( buf, sizeof(buf), "%ld", n );
	else if ( base == HEX )
		snprintf( buf, sizeof(buf), "%lx", (unsigned long)n );
	else if ( base == OCT )
		snprintf( buf, sizeof(buf), "%lo", (unsigned long)n );
	else if ( n < 0 )
	{
		size_t	bytes	= write( '-' );
		_utoa_radix( (unsigned long long)( -(long long)n ), base, buf, sizeof(buf) );
		return	bytes + print( buf );
	}
	else
		_utoa_radix( (unsigned long long)n, base, buf, sizeof(buf) );
	return	print( buf );
}

size_t Print::_print_unum( unsigned long n, int base )
{
	char	buf[ 34 ];
	if ( base == DEC )
		snprintf( buf, sizeof(buf), "%lu", n );
	else if ( base == HEX )
		snprintf( buf, sizeof(buf), "%lx", n );
	else if ( base == OCT )
		snprintf( buf, sizeof(buf), "%lo", n );
	else
		_utoa_radix( (unsigned long long)n, base, buf, sizeof(buf) );
	return	print( buf );
}

size_t Print::_print_num64( long long n, int base )
{
	char	buf[ 66 ];
	if ( base == DEC )
		snprintf( buf, sizeof(buf), "%lld", n );
	else if ( base == HEX )
		snprintf( buf, sizeof(buf), "%llx", (unsigned long long)n );
	else if ( base == OCT )
		snprintf( buf, sizeof(buf), "%llo", (unsigned long long)n );
	else if ( n < 0 )
	{
		size_t	bytes	= write( '-' );
		_utoa_radix( (unsigned long long)( -n ), base, buf, sizeof(buf) );
		return	bytes + print( buf );
	}
	else
		_utoa_radix( (unsigned long long)n, base, buf, sizeof(buf) );
	return	print( buf );
}

size_t Print::_print_unum64( unsigned long long n, int base )
{
	char	buf[ 66 ];
	if ( base == DEC )
		snprintf( buf, sizeof(buf), "%llu", n );
	else if ( base == HEX )
		snprintf( buf, sizeof(buf), "%llx", n );
	else if ( base == OCT )
		snprintf( buf, sizeof(buf), "%llo", n );
	else
		_utoa_radix( n, base, buf, sizeof(buf) );
	return	print( buf );
}

size_t Print::print( int n, int base )                { return _print_num( n, base ); }
size_t Print::print( unsigned int n, int base )        { return _print_unum( n, base ); }
size_t Print::print( long n, int base )                { return _print_num( n, base ); }
size_t Print::print( unsigned long n, int base )       { return _print_unum( n, base ); }
size_t Print::print( long long n, int base )           { return _print_num64( n, base ); }
size_t Print::print( unsigned long long n, int base )  { return _print_unum64( n, base ); }

size_t Print::print( double n, int digits )
{
	return	_print_double( n, digits );
}

size_t Print::print( const std::string &s )  { return write( s.c_str(), s.length() ); }
size_t Print::print( std::string_view s )
{
	size_t	n	= 0;
	for ( char c : s )
		n	+= write( (uint8_t)c );
	return	n;
}
size_t Print::print( const String &s )        { return write( s.c_str(), s.length() ); }
size_t Print::print( const __FlashStringHelper *pstr )
{
	return	print( reinterpret_cast<const char *>( pstr ) );
}

size_t Print::print( const Printable &p )
{
	return	p.printTo( *this );
}

// ---- println overloads ----

size_t Print::println( void )                        { return print( "\r\n" ); }
size_t Print::println( const char *s )               { size_t n = print(s); return n + println(); }
size_t Print::println( char c )                      { size_t n = print(c); return n + println(); }
size_t Print::println( int n, int base )             { size_t r = print(n, base); return r + println(); }
size_t Print::println( unsigned int n, int base )    { size_t r = print(n, base); return r + println(); }
size_t Print::println( long n, int base )            { size_t r = print(n, base); return r + println(); }
size_t Print::println( unsigned long n, int base )   { size_t r = print(n, base); return r + println(); }
size_t Print::println( long long n, int base )       { size_t r = print(n, base); return r + println(); }
size_t Print::println( unsigned long long n, int base ) { size_t r = print(n, base); return r + println(); }
size_t Print::println( double n, int digits )        { size_t r = print(n, digits); return r + println(); }
size_t Print::println( const std::string &s )        { size_t n = print(s); return n + println(); }
size_t Print::println( std::string_view s )          { size_t n = print(s); return n + println(); }
size_t Print::println( const String &s )              { size_t n = print(s); return n + println(); }
size_t Print::println( const __FlashStringHelper *pstr ) { size_t n = print(pstr); return n + println(); }
size_t Print::println( const Printable &p )               { size_t n = print(p); return n + println(); }

// ---- helpers ----

size_t Print::_print_double( double val, int digits )
{
	size_t	bytes	= 0;

	if ( val < 0.0 ) { bytes += write( '-' ); val = -val; }

	long	integer	= (long)val;
	double	frac	= val - integer;
	char	buf[ 32 ];
	snprintf( buf, sizeof(buf), "%ld.", integer );
	bytes	+= print( buf );

	for ( int i = 0; i < digits; i++ )
	{
		frac	*= 10;
		int	d	= (int)frac;
		bytes	+= write( (uint8_t)('0' + d) );
		frac	-= d;
	}

	return	bytes;
}
