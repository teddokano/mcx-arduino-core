/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  Arduino-compatible Print class -- a hardware-independent abstract base
 *  for anything bytes can be written to. This is an original, from-scratch
 *  implementation matching the Arduino Print API surface, not a port of
 *  ArduinoCore-avr/ArduinoCore-API's Print.cpp (which is LGPL 2.1).
 *
 *  A derived class only needs to implement write(uint8_t) (and optionally
 *  the bulk write(const uint8_t*, size_t) for efficiency) to get every
 *  print()/println() overload below for free -- matching how real Arduino
 *  libraries expect to inherit from Print.
 */

#ifndef R01LIB_ARDUINO_PRINT_H
#define R01LIB_ARDUINO_PRINT_H

#include	<stdint.h>
#include	<stddef.h>
#include	<string>
#include	<string_view>

#include	"arduino_string.h"
#include	"Printable.h"

/** @name Number base constants for print()/println()'s base parameter and String's numeric constructors */
///@{
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2
///@}

class Print
{
public:
	virtual ~Print() {}

	/** Write one byte. The only method a derived class must implement --
	 *  every print()/println() overload below is built on this (and the
	 *  optional bulk override just below it, for efficiency).
	 * @param c byte to write
	 * @return 1 on success, 0 on failure
	 */
	virtual size_t	write( uint8_t c ) = 0;

	/** Write a buffer. Default implementation just calls write(uint8_t) in
	 *  a loop; override for a more efficient bulk path.
	 * @param buffer bytes to write
	 * @param size number of bytes
	 * @return number of bytes actually written
	 */
	virtual size_t	write( const uint8_t *buffer, size_t size );

	/** @param buffer bytes to write @param size number of bytes @return number of bytes actually written */
	size_t	write( const char *buffer, size_t size );
	/** @param str NUL-terminated string to write @return number of bytes actually written */
	size_t	write( const char *str );

	/** @return free space in the underlying write buffer, in bytes (0 = no headroom / not applicable) */
	virtual int		availableForWrite( void ) { return 0; }

	/** @return the error code last passed to setWriteError() (protected, called by derived classes), or 0 if none */
	int		getWriteError( void ) { return _write_error; }
	/** Reset the write-error state to 0. */
	void	clearWriteError( void ) { setWriteError( 0 ); }

	/** @name print() family
	 *  Format and write, without a trailing newline.
	 *  @return number of bytes written
	 */
	///@{
	size_t	print( const char *s );
	size_t	print( char c );
	size_t	print( const std::string &s );
	size_t	print( std::string_view s );
	size_t	print( const String &s );
	/** @param pstr an F()-wrapped flash-string literal */
	size_t	print( const __FlashStringHelper *pstr );
	/** @param p any Printable-derived object; calls p.printTo(*this) */
	size_t	print( const Printable &p );
	/** @param n value to format @param base DEC/HEX/OCT/BIN or any radix 2..36 */
	size_t	print( int n, int base = DEC );
	/** @param n value to format @param base DEC/HEX/OCT/BIN or any radix 2..36 */
	size_t	print( unsigned int n, int base = DEC );
	/** @param n value to format @param base DEC/HEX/OCT/BIN or any radix 2..36 */
	size_t	print( long n, int base = DEC );
	/** @param n value to format @param base DEC/HEX/OCT/BIN or any radix 2..36 */
	size_t	print( unsigned long n, int base = DEC );
	/** @param n value to format @param base DEC/HEX/OCT/BIN or any radix 2..36 */
	size_t	print( long long n, int base = DEC );
	/** @param n value to format @param base DEC/HEX/OCT/BIN or any radix 2..36 */
	size_t	print( unsigned long long n, int base = DEC );
	/** @param n value to format @param digits digits after the decimal point */
	size_t	print( double n, int digits = 2 );
	///@}

	/** @name println() family
	 *  Same as the corresponding print() overload, followed by "\\r\\n".
	 *  @return number of bytes written, including the trailing "\\r\\n"
	 */
	///@{
	/** Write just "\\r\\n". */
	size_t	println( void );
	size_t	println( const char *s );
	size_t	println( char c );
	size_t	println( const std::string &s );
	size_t	println( std::string_view s );
	size_t	println( const String &s );
	size_t	println( const __FlashStringHelper *pstr );
	size_t	println( const Printable &p );
	size_t	println( int n, int base = DEC );
	size_t	println( unsigned int n, int base = DEC );
	size_t	println( long n, int base = DEC );
	size_t	println( unsigned long n, int base = DEC );
	size_t	println( long long n, int base = DEC );
	size_t	println( unsigned long long n, int base = DEC );
	size_t	println( double n, int digits = 2 );
	///@}

protected:
	/** Record a write-error code, for derived classes that track write
	 *  failures (e.g. libraries implementing SD's File/SdFile).
	 * @param err error code to record (default 1); 0 clears it
	 */
	void	setWriteError( int err = 1 ) { _write_error = err; }

private:
	size_t	_print_num( long n, int base );
	size_t	_print_unum( unsigned long n, int base );
	size_t	_print_num64( long long n, int base );
	size_t	_print_unum64( unsigned long long n, int base );
	size_t	_print_double( double val, int digits );

	int		_write_error	= 0;
};

#endif // !R01LIB_ARDUINO_PRINT_H
