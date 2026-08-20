/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"Stream.h"
#include	"Arduino.h"		// for millis(), used by the timeout-based helpers below

#include	<cstring>

int Stream::_timed_read( void )
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

int Stream::_timed_peek( void )
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

size_t Stream::readBytes( char *buffer, size_t length )
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

size_t Stream::readBytesUntil( char terminator, char *buffer, size_t length )
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

String Stream::readString( void )
{
	String	result;
	int		c;

	while ( ( c = _timed_read() ) >= 0 )
		result.concat( (char)c );

	return	result;
}

String Stream::readStringUntil( char terminator )
{
	String	result;
	int		c;

	while ( ( c = _timed_read() ) >= 0 && c != terminator )
		result.concat( (char)c );

	return	result;
}

double Stream::_parseNumber( bool allow_decimal )
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

long Stream::parseInt( void )
{
	return	(long)_parseNumber( false );
}

float Stream::parseFloat( void )
{
	return	(float)_parseNumber( true );
}

bool Stream::find( const char *target )
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

bool Stream::find( const char *target, size_t length )
{
	if ( length == 0 )
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
			if ( matched == length )
				return	true;
		}
		else
		{
			matched	= ( (char)c == target[ 0 ] ) ? 1 : 0;
		}
	}
}

bool Stream::findUntil( const char *target, const char *terminator )
{
	size_t	target_len	= strlen( target );
	size_t	term_len	= strlen( terminator );

	if ( target_len == 0 )
		return	true;

	size_t	matched		= 0;
	size_t	term_matched	= 0;

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

		if ( term_len > 0 )
		{
			if ( (char)c == terminator[ term_matched ] )
			{
				term_matched++;
				if ( term_matched == term_len )
					return	false;
			}
			else
			{
				term_matched	= ( (char)c == terminator[ 0 ] ) ? 1 : 0;
			}
		}
	}
}
