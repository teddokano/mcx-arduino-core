/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  Arduino-compatible String class. This is an original, from-scratch
 *  implementation matching the Arduino String API surface -- it is not a
 *  port of ArduinoCore-avr/ArduinoCore-API's WString (which is LGPL 2.1),
 *  so no third-party license notice is needed here.
 */

#ifndef R01LIB_ARDUINO_STRING_H
#define R01LIB_ARDUINO_STRING_H

#include	<string>
#include	<cstdint>
#include	<cstddef>

class String
{
public:
	String( void );
	String( const char *cstr );
	String( const String &s );
	String( String &&s ) noexcept;
	String( char c );
	String( const std::string &s );
	explicit String( int value, unsigned char base = 10 );
	explicit String( unsigned int value, unsigned char base = 10 );
	explicit String( long value, unsigned char base = 10 );
	explicit String( unsigned long value, unsigned char base = 10 );
	explicit String( long long value, unsigned char base = 10 );
	explicit String( unsigned long long value, unsigned char base = 10 );
	explicit String( float value, unsigned char decimalPlaces = 2 );
	explicit String( double value, unsigned char decimalPlaces = 2 );
	~String();

	String&	operator=( const String &rhs );
	String&	operator=( String &&rhs ) noexcept;
	String&	operator=( const char *cstr );

	unsigned int	length( void ) const	{ return _len; }
	bool			isEmpty( void ) const	{ return _len == 0; }
	const char*		c_str( void ) const		{ return _buf ? _buf : ""; }

	/*
	 *  reserve() is a no-op here: this implementation always allocates each
	 *  buffer as an exact fit on concat/assign (see _alloc_copy()), so
	 *  there's no separate "capacity" ahead of "length" to pre-grow. Kept
	 *  for source compatibility only -- always returns true.
	 */
	bool	reserve( unsigned int size );

	void	getBytes( unsigned char *buf, unsigned int bufsize ) const;
	void	toCharArray( char *buf, unsigned int bufsize ) const;

	bool	concat( const String &s );
	bool	concat( const char *cstr );
	bool	concat( char c );
	bool	concat( int num );
	bool	concat( unsigned int num );
	bool	concat( long num );
	bool	concat( unsigned long num );
	bool	concat( long long num );
	bool	concat( unsigned long long num );
	bool	concat( float num );
	bool	concat( double num );

	String&	operator+=( const String &s )     { concat( s ); return *this; }
	String&	operator+=( const char *cstr )    { concat( cstr ); return *this; }
	String&	operator+=( char c )               { concat( c ); return *this; }
	String&	operator+=( int num )              { concat( num ); return *this; }
	String&	operator+=( unsigned int num )     { concat( num ); return *this; }
	String&	operator+=( long num )             { concat( num ); return *this; }
	String&	operator+=( unsigned long num )    { concat( num ); return *this; }
	String&	operator+=( long long num )        { concat( num ); return *this; }
	String&	operator+=( unsigned long long num ){ concat( num ); return *this; }
	String&	operator+=( float num )            { concat( num ); return *this; }
	String&	operator+=( double num )           { concat( num ); return *this; }

	bool	equals( const String &s ) const;
	bool	equals( const char *cstr ) const;
	bool	equalsIgnoreCase( const String &s ) const;
	int		compareTo( const String &s ) const;

	bool	operator==( const String &rhs ) const	{ return equals( rhs ); }
	bool	operator==( const char *cstr ) const	{ return equals( cstr ); }
	bool	operator!=( const String &rhs ) const	{ return !equals( rhs ); }
	bool	operator!=( const char *cstr ) const	{ return !equals( cstr ); }
	bool	operator<( const String &rhs ) const	{ return compareTo( rhs ) < 0; }
	bool	operator>( const String &rhs ) const	{ return compareTo( rhs ) > 0; }
	bool	operator<=( const String &rhs ) const	{ return compareTo( rhs ) <= 0; }
	bool	operator>=( const String &rhs ) const	{ return compareTo( rhs ) >= 0; }

	char	charAt( unsigned int index ) const;
	void	setCharAt( unsigned int index, char c );
	char	operator[]( unsigned int index ) const;
	char&	operator[]( unsigned int index );

	bool	startsWith( const String &s ) const;
	bool	startsWith( const String &s, unsigned int offset ) const;
	bool	endsWith( const String &s ) const;

	int		indexOf( char ch, unsigned int fromIndex = 0 ) const;
	int		indexOf( const String &s, unsigned int fromIndex = 0 ) const;
	int		lastIndexOf( char ch ) const;
	int		lastIndexOf( const String &s ) const;

	String	substring( unsigned int beginIndex ) const;
	String	substring( unsigned int beginIndex, unsigned int endIndex ) const;

	void	replace( char find, char rep );
	void	replace( const String &find, const String &rep );
	void	remove( unsigned int index );
	void	remove( unsigned int index, unsigned int count );

	void	toUpperCase( void );
	void	toLowerCase( void );
	void	trim( void );

	long	toInt( void ) const;
	float	toFloat( void ) const;
	double	toDouble( void ) const;

private:
	char			*_buf;
	unsigned int	_len;

	void	_init( void );
	bool	_alloc_copy( const char *src, unsigned int len );
};

String	operator+( String lhs, const String &rhs );
String	operator+( String lhs, const char *rhs );
String	operator+( const char *lhs, const String &rhs );
String	operator+( String lhs, char rhs );

#endif // !R01LIB_ARDUINO_STRING_H
