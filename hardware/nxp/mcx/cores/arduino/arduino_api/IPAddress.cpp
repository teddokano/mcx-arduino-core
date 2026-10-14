/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  Arduino-compatible IPAddress -- original implementation, see IPAddress.h.
 */

#include	<string.h>

#include	"IPAddress.h"
#include	"Print.h"

const IPAddress	IN6ADDR_ANY( IPv6 );
const IPAddress	INADDR_NONE( 0, 0, 0, 0 );

IPAddress::IPAddress( uint32_t address ) : IPAddress( IPv4 )
{
	*this	= address;
}

IPAddress::IPAddress( const uint8_t *address ) : IPAddress( IPv4, address )
{
}

IPAddress::IPAddress( IPType ip_type, const uint8_t *address ) : IPAddress( ip_type )
{
	memcpy( raw(), address, ip_type == IPv4 ? 4 : 16 );
}

IPAddress::IPAddress( const char *address ) : IPAddress( IPv4 )
{
	fromString( address );
}

void IPAddress::set_v4( const uint8_t *octets )
{
	memset( _bytes, 0, sizeof( _bytes ) );
	memcpy( &_bytes[ V4_OFFSET ], octets, 4 );
	_type	= IPv4;
}

bool IPAddress::fromString( const char *address )
{
	uint8_t	parsed[ 16 ];

	if ( parse_v4( address, parsed ) )
	{
		set_v4( parsed );
		return	true;
	}

	if ( parse_v6( address, parsed ) )
	{
		memcpy( _bytes, parsed, sizeof( _bytes ) );
		_type	= IPv6;
		return	true;
	}

	return	false;
}

// Exactly four decimal fields of 1-3 digits, each 0-255, separated by dots
bool IPAddress::parse_v4( const char *s, uint8_t *out )
{
	for ( int field = 0; field < 4; field++ )
	{
		if ( field > 0 && *s++ != '.' )
			return	false;

		int	value	= 0;
		int	digits	= 0;

		while ( '0' <= *s && *s <= '9' )
		{
			if ( ++digits > 3 )
				return	false;

			value	= value * 10 + ( *s++ - '0' );
		}

		if ( digits == 0 || value > 255 )
			return	false;

		out[ field ]	= (uint8_t)value;
	}

	return	*s == '\0';
}

static int hex_value( char c )
{
	if ( '0' <= c && c <= '9' )
		return	c - '0';
	if ( 'a' <= c && c <= 'f' )
		return	c - 'a' + 10;
	if ( 'A' <= c && c <= 'F' )
		return	c - 'A' + 10;

	return	-1;
}

// Groups of 1-4 hex digits separated by ':', eight of them, or fewer with
// one "::" standing for the zero groups that are left out (at least one)
bool IPAddress::parse_v6( const char *s, uint8_t *out )
{
	uint16_t	groups[ 8 ];
	int			count	= 0;	// groups read so far
	int			gap		= -1;	// where "::" was, as a group index

	if ( s[ 0 ] == ':' )
	{
		if ( s[ 1 ] != ':' )
			return	false;

		gap	 = 0;
		s	+= 2;
	}

	while ( *s != '\0' )
	{
		if ( count == 8 )
			return	false;

		int	value	= 0;
		int	digits	= 0;
		int	h;

		while ( ( h = hex_value( *s ) ) >= 0 )
		{
			if ( ++digits > 4 )
				return	false;

			value	= value * 16 + h;
			s++;
		}

		if ( digits == 0 )
			return	false;

		groups[ count++ ]	= (uint16_t)value;

		if ( *s == '\0' )
			break;
		if ( *s++ != ':' )
			return	false;

		if ( *s == ':' )
		{
			if ( gap >= 0 )
				return	false;

			gap	= count;
			s++;
		}
		else if ( *s == '\0' )
		{
			return	false;	// a single ':' at the end
		}
	}

	if ( gap < 0 ? count != 8 : count > 7 )
		return	false;

	int	zeros	= 8 - count;
	int	g		= 0;

	for ( int i = 0; i < 8; i++ )
	{
		uint16_t	v	= ( gap <= i && i < gap + zeros ) ? 0 : groups[ g++ ];
		out[ i * 2 ]		= v >> 8;
		out[ i * 2 + 1 ]	= v & 0xFF;
	}

	return	true;
}

IPAddress::operator uint32_t() const
{
	if ( _type != IPv4 )
		return	0;

	uint32_t	value;
	memcpy( &value, &_bytes[ V4_OFFSET ], sizeof( value ) );
	return	value;
}

bool IPAddress::operator==( const IPAddress &addr ) const
{
	return	_type == addr._type && memcmp( _bytes, addr._bytes, sizeof( _bytes ) ) == 0;
}

bool IPAddress::operator==( const uint8_t *addr ) const
{
	return	_type == IPv4 && memcmp( &_bytes[ V4_OFFSET ], addr, 4 ) == 0;
}

IPAddress& IPAddress::operator=( const uint8_t *address )
{
	set_v4( address );
	return	*this;
}

IPAddress& IPAddress::operator=( uint32_t address )
{
	uint8_t	octets[ 4 ];

	memcpy( octets, &address, sizeof( octets ) );
	set_v4( octets );
	return	*this;
}

IPAddress& IPAddress::operator=( const char *address )
{
	fromString( address );
	return	*this;
}

// Writes value in decimal or lower-case hex without leading zeros, returns
// where the next character goes
static char *put_number( char *t, unsigned value, unsigned base )
{
	char	digits[ 5 ];
	int		n	= 0;

	do
	{
		digits[ n++ ]	= "0123456789abcdef"[ value % base ];
		value			/= base;
	}
	while ( value );

	while ( n )
		*t++	= digits[ --n ];

	return	t;
}

// Writes the text and its NUL into text (TEXT_SIZE bytes), returns its length
size_t IPAddress::format( char *text ) const
{
	char	*t	= text;

	if ( _type == IPv4 )
	{
		for ( int i = 0; i < 4; i++ )
		{
			if ( i > 0 )
				*t++	= '.';

			t	= put_number( t, _bytes[ V4_OFFSET + i ], 10 );
		}

		*t	= '\0';
		return	t - text;
	}

	// The longest run of zero groups, the first one if two are as long.
	// A single zero group is written as "0", not "::" (RFC 5952 4.2.2).
	int	run_start	= -1;
	int	run_length	= 1;

	for ( int i = 0; i < 8; )
	{
		int	n	= 0;

		while ( i + n < 8 && _bytes[ ( i + n ) * 2 ] == 0 && _bytes[ ( i + n ) * 2 + 1 ] == 0 )
			n++;

		if ( n > run_length )
		{
			run_start	= i;
			run_length	= n;
		}

		i	+= n ? n : 1;
	}

	for ( int i = 0; i < 8; i++ )
	{
		if ( i == run_start )
		{
			*t++	= ':';
			*t++	= ':';
			i		+= run_length - 1;
			continue;
		}

		if ( i > 0 && i != run_start + run_length )
			*t++	= ':';

		t	= put_number( t, ( _bytes[ i * 2 ] << 8 ) | _bytes[ i * 2 + 1 ], 16 );
	}

	*t	= '\0';
	return	t - text;
}

size_t IPAddress::printTo( Print &p ) const
{
	char	text[ TEXT_SIZE ];
	size_t	length	= format( text );

	return	p.write( text, length );
}

String IPAddress::toString() const
{
	char	text[ TEXT_SIZE ];

	format( text );
	return	String( text );
}
