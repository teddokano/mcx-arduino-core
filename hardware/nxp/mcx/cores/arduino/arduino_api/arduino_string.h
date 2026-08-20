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

// Forward declaration only, never defined -- matches AVR/ARM Arduino cores'
// convention of using this purely as an opaque pointer type to distinguish
// F()-wrapped flash-string literals at compile time (see arduino.h's F()
// macro). Declared here too (not just in arduino.h) so this header stays
// self-contained regardless of include order.
class __FlashStringHelper;

/** Arduino-compatible String class: a growable, heap-backed byte string.
 *  Independent from-scratch implementation (see file header) -- every
 *  concat/assign reallocates its buffer as an exact fit for the new
 *  content (see reserve()'s doc below), rather than pre-growing capacity.
 */
class String
{
public:
	/** Empty string. */
	String( void );
	/** Copy from a C string. */
	String( const char *cstr );
	String( const String &s );
	String( String &&s ) noexcept;
	/** Single-character string. */
	String( char c );
	String( const std::string &s );
	/** From an F()-wrapped flash-string literal. */
	String( const __FlashStringHelper *pstr );
	/** @param value integer value to format @param base numeric base (e.g. 10, 16, 2) */
	explicit String( int value, unsigned char base = 10 );
	/** @param value integer value to format @param base numeric base (e.g. 10, 16, 2) */
	explicit String( unsigned int value, unsigned char base = 10 );
	/** @param value integer value to format @param base numeric base (e.g. 10, 16, 2) */
	explicit String( long value, unsigned char base = 10 );
	/** @param value integer value to format @param base numeric base (e.g. 10, 16, 2) */
	explicit String( unsigned long value, unsigned char base = 10 );
	/** @param value integer value to format @param base numeric base (e.g. 10, 16, 2) */
	explicit String( long long value, unsigned char base = 10 );
	/** @param value integer value to format @param base numeric base (e.g. 10, 16, 2) */
	explicit String( unsigned long long value, unsigned char base = 10 );
	/** @param value floating-point value to format @param decimalPlaces digits after the decimal point */
	explicit String( float value, unsigned char decimalPlaces = 2 );
	/** @param value floating-point value to format @param decimalPlaces digits after the decimal point */
	explicit String( double value, unsigned char decimalPlaces = 2 );
	~String();

	String&	operator=( const String &rhs );
	String&	operator=( String &&rhs ) noexcept;
	String&	operator=( const char *cstr );

	/** @return length in bytes, not counting the terminating NUL */
	unsigned int	length( void ) const	{ return _len; }
	/** @return true if length() == 0 */
	bool			isEmpty( void ) const	{ return _len == 0; }
	/** @return NUL-terminated C string view of the contents; never nullptr, even when empty */
	const char*		c_str( void ) const		{ return _buf ? _buf : ""; }

	/*
	 *  reserve() is a no-op here: this implementation always allocates each
	 *  buffer as an exact fit on concat/assign (see _alloc_copy()), so
	 *  there's no separate "capacity" ahead of "length" to pre-grow. Kept
	 *  for source compatibility only -- always returns true.
	 */
	bool	reserve( unsigned int size );

	/** Copy the contents (including the terminating NUL, if it fits) into buf.
	 * @param buf destination buffer
	 * @param bufsize destination buffer size; copies at most bufsize-1 content bytes plus a NUL
	 */
	void	getBytes( unsigned char *buf, unsigned int bufsize ) const;

	/** Same as getBytes(), for a char* destination.
	 * @param buf destination buffer
	 * @param bufsize destination buffer size; copies at most bufsize-1 content bytes plus a NUL
	 */
	void	toCharArray( char *buf, unsigned int bufsize ) const;

	/** Append to this string, in place.
	 * @return true on success, false only if cstr/pstr is nullptr (the
	 *         numeric/char overloads below never fail and always return true)
	 */
	bool	concat( const String &s );
	bool	concat( const char *cstr );		/**< @return see concat(const String&) */
	bool	concat( const __FlashStringHelper *pstr );	/**< @return see concat(const String&) */
	bool	concat( char c );			/**< @return always true */
	bool	concat( int num );			/**< @return always true */
	bool	concat( unsigned int num );		/**< @return always true */
	bool	concat( long num );			/**< @return always true */
	bool	concat( unsigned long num );		/**< @return always true */
	bool	concat( long long num );		/**< @return always true */
	bool	concat( unsigned long long num );	/**< @return always true */
	bool	concat( float num );			/**< @return always true */
	bool	concat( double num );			/**< @return always true */

	String&	operator+=( const String &s )     { concat( s ); return *this; }
	String&	operator+=( const char *cstr )    { concat( cstr ); return *this; }
	String&	operator+=( const __FlashStringHelper *pstr ) { concat( pstr ); return *this; }
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
	/** @return <0, 0, or >0 as this string sorts before, equal to, or after s (byte-wise, like strcmp) */
	int		compareTo( const String &s ) const;

	bool	operator==( const String &rhs ) const	{ return equals( rhs ); }
	bool	operator==( const char *cstr ) const	{ return equals( cstr ); }
	bool	operator!=( const String &rhs ) const	{ return !equals( rhs ); }
	bool	operator!=( const char *cstr ) const	{ return !equals( cstr ); }
	bool	operator<( const String &rhs ) const	{ return compareTo( rhs ) < 0; }
	bool	operator>( const String &rhs ) const	{ return compareTo( rhs ) > 0; }
	bool	operator<=( const String &rhs ) const	{ return compareTo( rhs ) <= 0; }
	bool	operator>=( const String &rhs ) const	{ return compareTo( rhs ) >= 0; }

	/** @return the byte at index, or '\0' if index is out of range */
	char	charAt( unsigned int index ) const;
	/** Replace the byte at index, if index is in range. */
	void	setCharAt( unsigned int index, char c );
	char	operator[]( unsigned int index ) const;
	char&	operator[]( unsigned int index );

	/** @return true if this string begins with s */
	bool	startsWith( const String &s ) const;
	/** @return true if this string, starting at offset, begins with s */
	bool	startsWith( const String &s, unsigned int offset ) const;
	/** @return true if this string ends with s */
	bool	endsWith( const String &s ) const;

	/** @param ch byte to search for @param fromIndex index to start searching from @return index of the first match at/after fromIndex, or -1 if not found */
	int		indexOf( char ch, unsigned int fromIndex = 0 ) const;
	/** @param s substring to search for @param fromIndex index to start searching from @return index of the first match at/after fromIndex, or -1 if not found */
	int		indexOf( const String &s, unsigned int fromIndex = 0 ) const;
	/** @return index of the last occurrence of ch, or -1 if not found */
	int		lastIndexOf( char ch ) const;
	/** @return index of the last occurrence of s, or -1 if not found */
	int		lastIndexOf( const String &s ) const;

	/** @return the substring from beginIndex to the end of the string (clamped to length()) */
	String	substring( unsigned int beginIndex ) const;
	/** @return the substring [beginIndex, endIndex) (both clamped to length(); empty if beginIndex >= endIndex) */
	String	substring( unsigned int beginIndex, unsigned int endIndex ) const;

	/** Replace every occurrence of one byte with another, in place. */
	void	replace( char find, char rep );
	/** Replace every non-overlapping occurrence of find with rep, in place. No-op if find is empty. */
	void	replace( const String &find, const String &rep );
	/** Remove everything from index to the end of the string. */
	void	remove( unsigned int index );
	/** Remove up to count bytes starting at index (clamped to the string's actual length). */
	void	remove( unsigned int index, unsigned int count );

	void	toUpperCase( void );
	void	toLowerCase( void );
	/** Remove leading and trailing whitespace, in place. */
	void	trim( void );

	/** @return the string parsed as a base-10 integer (leading whitespace/sign allowed, like strtol), or 0 if it doesn't start with a valid number */
	long	toInt( void ) const;
	/** @return the string parsed as a floating-point number, or 0 if it doesn't start with a valid number */
	float	toFloat( void ) const;
	/** @return the string parsed as a floating-point number, or 0 if it doesn't start with a valid number */
	double	toDouble( void ) const;

private:
	char			*_buf;
	unsigned int	_len;

	void	_init( void );
	bool	_alloc_copy( const char *src, unsigned int len );
};

/** @name Free operator+ overloads
 *  Each concatenates rhs onto a copy of lhs (by value, so this always
 *  copies lhs once) and returns the result, e.g. `String s = "x=" + n;`.
 */
///@{
String	operator+( String lhs, const String &rhs );
String	operator+( String lhs, const char *rhs );
String	operator+( const char *lhs, const String &rhs );
String	operator+( String lhs, char rhs );
String	operator+( String lhs, const __FlashStringHelper *rhs );
String	operator+( String lhs, int rhs );
String	operator+( String lhs, unsigned int rhs );
String	operator+( String lhs, long rhs );
String	operator+( String lhs, unsigned long rhs );
String	operator+( String lhs, long long rhs );
String	operator+( String lhs, unsigned long long rhs );
String	operator+( String lhs, float rhs );
String	operator+( String lhs, double rhs );
///@}

#endif // !R01LIB_ARDUINO_STRING_H
