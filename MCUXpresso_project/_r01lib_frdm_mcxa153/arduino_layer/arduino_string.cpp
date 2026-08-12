/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"arduino_string.h"
#include	"mcu.h"

#include	<cstdio>
#include	<cstring>
#include	<cstdlib>
#include	<cctype>

namespace {

// Integer-arithmetic double -> decimal string, same technique as
// SerialClass::_print_double() -- nano.specs' snprintf() doesn't support
// %f/%e/%g, so this can't just be sprintf("%f", ...).
void dtoa( double val, unsigned char decimalPlaces, char *out, size_t outsize )
{
	char	*p		= out;
	char	*end	= out + outsize - 1;

	if ( val < 0.0 && p < end )
	{
		*p++	= '-';
		val		= -val;
	}

	long	integer	= (long)val;
	double	frac	= val - integer;

	int	n	= snprintf( p, (size_t)(end - p), "%ld", integer );
	p	+= ( n > 0 ) ? n : 0;

	if ( decimalPlaces > 0 && p < end )
		*p++	= '.';

	for ( unsigned char i = 0; i < decimalPlaces && p < end; i++ )
	{
		frac	*= 10;
		int	d	= (int)frac;
		*p++	= (char)( '0' + d );
		frac	-= d;
	}

	*p	= '\0';
}

}	// namespace

void String::_init( void )
{
	_buf	= nullptr;
	_len	= 0;
}

bool String::_alloc_copy( const char *src, unsigned int len )
{
	char	*newbuf	= new char[ len + 1 ];

	if ( newbuf == nullptr )
		panic( "error @ new, in String" );

	if ( src != nullptr )
		memcpy( newbuf, src, len );
	newbuf[ len ]	= '\0';

	delete[] _buf;
	_buf	= newbuf;
	_len	= len;

	return	true;
}

String::String( void )
{
	_init();
}

String::String( const char *cstr )
{
	_init();
	_alloc_copy( cstr, cstr ? (unsigned int)strlen( cstr ) : 0 );
}

String::String( const String &s )
{
	_init();
	_alloc_copy( s._buf, s._len );
}

String::String( String &&s ) noexcept
{
	_buf	= s._buf;
	_len	= s._len;
	s._buf	= nullptr;
	s._len	= 0;
}

String::String( char c )
{
	_init();
	char	tmp[ 2 ]	= { c, '\0' };
	_alloc_copy( tmp, 1 );
}

String::String( const std::string &s )
{
	_init();
	_alloc_copy( s.c_str(), (unsigned int)s.length() );
}

String::String( int value, unsigned char base )
{
	_init();
	*this	= String( (long)value, base );
}

String::String( unsigned int value, unsigned char base )
{
	_init();
	*this	= String( (unsigned long)value, base );
}

String::String( long value, unsigned char base )
{
	_init();
	char	buf[ 34 ];
	const char	*fmt	= ( base == 16 ) ? "%lx" : ( base == 8 ) ? "%lo" : "%ld";
	snprintf( buf, sizeof(buf), fmt, value );
	_alloc_copy( buf, (unsigned int)strlen( buf ) );
}

String::String( unsigned long value, unsigned char base )
{
	_init();
	char	buf[ 34 ];
	const char	*fmt	= ( base == 16 ) ? "%lx" : ( base == 8 ) ? "%lo" : "%lu";
	snprintf( buf, sizeof(buf), fmt, value );
	_alloc_copy( buf, (unsigned int)strlen( buf ) );
}

String::String( float value, unsigned char decimalPlaces )
{
	_init();
	char	buf[ 48 ];
	dtoa( (double)value, decimalPlaces, buf, sizeof(buf) );
	_alloc_copy( buf, (unsigned int)strlen( buf ) );
}

String::String( double value, unsigned char decimalPlaces )
{
	_init();
	char	buf[ 48 ];
	dtoa( value, decimalPlaces, buf, sizeof(buf) );
	_alloc_copy( buf, (unsigned int)strlen( buf ) );
}

String::~String()
{
	delete[] _buf;
}

String& String::operator=( const String &rhs )
{
	if ( this != &rhs )
		_alloc_copy( rhs._buf, rhs._len );
	return	*this;
}

String& String::operator=( String &&rhs ) noexcept
{
	if ( this != &rhs )
	{
		delete[] _buf;
		_buf		= rhs._buf;
		_len		= rhs._len;
		rhs._buf	= nullptr;
		rhs._len	= 0;
	}
	return	*this;
}

String& String::operator=( const char *cstr )
{
	_alloc_copy( cstr, cstr ? (unsigned int)strlen( cstr ) : 0 );
	return	*this;
}

bool String::reserve( unsigned int size )
{
	(void)size;
	return	true;
}

void String::getBytes( unsigned char *buf, unsigned int bufsize ) const
{
	if ( buf == nullptr || bufsize == 0 )
		return;

	unsigned int	n	= ( _len < bufsize - 1 ) ? _len : ( bufsize - 1 );

	memcpy( buf, c_str(), n );
	buf[ n ]	= '\0';
}

void String::toCharArray( char *buf, unsigned int bufsize ) const
{
	getBytes( (unsigned char *)buf, bufsize );
}

bool String::concat( const String &s )
{
	return	concat( s.c_str() );
}

bool String::concat( const char *cstr )
{
	if ( cstr == nullptr )
		return	false;

	unsigned int	addlen	= (unsigned int)strlen( cstr );
	unsigned int	newlen	= _len + addlen;
	char			*newbuf	= new char[ newlen + 1 ];

	if ( newbuf == nullptr )
		panic( "error @ new, in String::concat" );

	if ( _buf != nullptr )
		memcpy( newbuf, _buf, _len );
	memcpy( newbuf + _len, cstr, addlen );
	newbuf[ newlen ]	= '\0';

	delete[] _buf;
	_buf	= newbuf;
	_len	= newlen;

	return	true;
}

bool String::concat( char c )
{
	char	tmp[ 2 ]	= { c, '\0' };
	return	concat( tmp );
}

bool String::concat( int num )         { return	concat( String( num ) ); }
bool String::concat( unsigned int num ){ return	concat( String( num ) ); }
bool String::concat( long num )        { return	concat( String( num ) ); }
bool String::concat( unsigned long num){ return	concat( String( num ) ); }
bool String::concat( float num )       { return	concat( String( num ) ); }
bool String::concat( double num )      { return	concat( String( num ) ); }

bool String::equals( const String &s ) const
{
	return	equals( s.c_str() );
}

bool String::equals( const char *cstr ) const
{
	if ( cstr == nullptr )
		return	_len == 0;
	return	strcmp( c_str(), cstr ) == 0;
}

bool String::equalsIgnoreCase( const String &s ) const
{
	const char	*a	= c_str();
	const char	*b	= s.c_str();

	while ( *a && *b )
	{
		if ( tolower( (unsigned char)*a ) != tolower( (unsigned char)*b ) )
			return	false;
		a++;
		b++;
	}

	return	*a == *b;
}

int String::compareTo( const String &s ) const
{
	return	strcmp( c_str(), s.c_str() );
}

char String::charAt( unsigned int index ) const
{
	return	( index < _len ) ? _buf[ index ] : '\0';
}

void String::setCharAt( unsigned int index, char c )
{
	if ( index < _len )
		_buf[ index ]	= c;
}

char String::operator[]( unsigned int index ) const
{
	return	charAt( index );
}

char& String::operator[]( unsigned int index )
{
	static char	dummy	= '\0';
	return	( index < _len ) ? _buf[ index ] : dummy;
}

bool String::startsWith( const String &s ) const
{
	return	startsWith( s, 0 );
}

bool String::startsWith( const String &s, unsigned int offset ) const
{
	if ( offset > _len || s._len > ( _len - offset ) )
		return	false;
	return	strncmp( c_str() + offset, s.c_str(), s._len ) == 0;
}

bool String::endsWith( const String &s ) const
{
	if ( s._len > _len )
		return	false;
	return	strcmp( c_str() + ( _len - s._len ), s.c_str() ) == 0;
}

int String::indexOf( char ch, unsigned int fromIndex ) const
{
	if ( fromIndex >= _len )
		return	-1;

	const char	*p	= strchr( c_str() + fromIndex, ch );
	return	p ? (int)( p - c_str() ) : -1;
}

int String::indexOf( const String &s, unsigned int fromIndex ) const
{
	if ( fromIndex >= _len )
		return	( s._len == 0 ) ? (int)_len : -1;

	const char	*p	= strstr( c_str() + fromIndex, s.c_str() );
	return	p ? (int)( p - c_str() ) : -1;
}

int String::lastIndexOf( char ch ) const
{
	const char	*p	= strrchr( c_str(), ch );
	return	p ? (int)( p - c_str() ) : -1;
}

int String::lastIndexOf( const String &s ) const
{
	if ( s._len == 0 || s._len > _len )
		return	-1;

	for ( int i = (int)( _len - s._len ); i >= 0; i-- )
	{
		if ( strncmp( c_str() + i, s.c_str(), s._len ) == 0 )
			return	i;
	}

	return	-1;
}

String String::substring( unsigned int beginIndex ) const
{
	return	substring( beginIndex, _len );
}

String String::substring( unsigned int beginIndex, unsigned int endIndex ) const
{
	if ( beginIndex > _len )
		beginIndex	= _len;
	if ( endIndex > _len )
		endIndex	= _len;
	if ( beginIndex >= endIndex )
		return	String();

	String	result;
	result._alloc_copy( c_str() + beginIndex, endIndex - beginIndex );
	return	result;
}

void String::replace( char find, char rep )
{
	for ( unsigned int i = 0; i < _len; i++ )
	{
		if ( _buf[ i ] == find )
			_buf[ i ]	= rep;
	}
}

void String::replace( const String &find, const String &rep )
{
	if ( find._len == 0 )
		return;

	String	result;
	unsigned int	i	= 0;

	while ( i < _len )
	{
		if ( _len - i >= find._len && strncmp( c_str() + i, find.c_str(), find._len ) == 0 )
		{
			result.concat( rep );
			i	+= find._len;
		}
		else
		{
			result.concat( _buf[ i ] );
			i++;
		}
	}

	*this	= result;
}

void String::remove( unsigned int index )
{
	remove( index, _len );
}

void String::remove( unsigned int index, unsigned int count )
{
	if ( index >= _len )
		return;

	unsigned int	end	= index + count;
	if ( end > _len || end < index )
		end	= _len;

	String	result	= substring( 0, index );
	result.concat( substring( end ) );
	*this	= result;
}

void String::toUpperCase( void )
{
	for ( unsigned int i = 0; i < _len; i++ )
		_buf[ i ]	= (char)toupper( (unsigned char)_buf[ i ] );
}

void String::toLowerCase( void )
{
	for ( unsigned int i = 0; i < _len; i++ )
		_buf[ i ]	= (char)tolower( (unsigned char)_buf[ i ] );
}

void String::trim( void )
{
	if ( _len == 0 )
		return;

	unsigned int	start	= 0;
	unsigned int	end		= _len;

	while ( start < end && isspace( (unsigned char)_buf[ start ] ) )
		start++;
	while ( end > start && isspace( (unsigned char)_buf[ end - 1 ] ) )
		end--;

	*this	= substring( start, end );
}

long String::toInt( void ) const
{
	return	strtol( c_str(), nullptr, 10 );
}

float String::toFloat( void ) const
{
	return	strtof( c_str(), nullptr );
}

double String::toDouble( void ) const
{
	return	strtod( c_str(), nullptr );
}

String operator+( String lhs, const String &rhs ) { lhs += rhs; return lhs; }
String operator+( String lhs, const char *rhs )    { lhs += rhs; return lhs; }
String operator+( const char *lhs, const String &rhs ) { String s( lhs ); s += rhs; return s; }
String operator+( String lhs, char rhs )           { lhs += rhs; return lhs; }
