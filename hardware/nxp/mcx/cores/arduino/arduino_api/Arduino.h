/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

/** @file Arduino.h
 *  Top-level include for every sketch (`#include <Arduino.h>`). Pulls in
 *  the whole Arduino-compatible API surface (r01lib.h and every
 *  arduino_*.h) plus the UNO R3/R4-compatible macros/constants/templates
 *  defined directly below.
 */

#ifndef R01LIB_ARDUINO_H
#define R01LIB_ARDUINO_H

#include	<math.h>
#include	<cstdlib>
#include	<cctype>

#include	"mcx_arduino_core_version.h"

/*
 *  PROGMEM / F() / pgm_read_*() compatibility -- no-ops on this Cortex-M
 *  target. AVR is a Harvard-architecture chip where flash and RAM are
 *  separate address spaces, so PROGMEM-tagged data needs special
 *  instructions (pgm_read_byte() etc.) to read back; on this von-Neumann
 *  Cortex-M, flash is just regular memory-mapped, readable like anything
 *  else. These are declared purely so AVR-era sketches/libraries that use
 *  them still compile -- __FlashStringHelper is forward-declared here
 *  (before arduino_string.h/arduino_serial.h) since String and SerialClass
 *  both need to know the type exists for their F()-string overloads.
 */
/** @name PROGMEM / pgm_read_*() compatibility macros
 *  No-ops / plain pointer casts on this von-Neumann target -- see the
 *  explanation above.
 */
///@{
#define	PROGMEM
#define	PGM_P				const char *
/** Identity -- string literals need no special flash-placement here. */
#define	PSTR( s )			( s )
#define	pgm_read_byte( addr )	( *(const unsigned char *)(addr) )
#define	pgm_read_word( addr )	( *(const unsigned short *)(addr) )
#define	pgm_read_dword( addr )	( *(const unsigned long *)(addr) )
#define	pgm_read_float( addr )	( *(const float *)(addr) )
#define	pgm_read_ptr( addr )	( *(const void * const *)(addr) )
///@}

class __FlashStringHelper;
/** Wrap a string literal for the String/Print/SerialClass overloads that
 *  take a `const __FlashStringHelper*` (matches classic Arduino's F()
 *  macro; here it's purely a type-tag, since there's no separate flash
 *  address space to route through).
 * @param string_literal a plain C string literal, e.g. F("hello")
 */
#define	F( string_literal )	( reinterpret_cast<const __FlashStringHelper *>( PSTR( string_literal ) ) )

#include	"r01lib.h"
#include	"arduino_string.h"
#include	"Printable.h"
#include	"Print.h"
#include	"Stream.h"
#include	"arduino_serial.h"
#include	"arduino_io.h"
#include	"arduino_analog.h"
#include	"arduino_tone.h"
#include	"arduino_i2c.h"
#include	"arduino_spi.h"

// The constants, macros, and templates below (through the min()/max()/
// bit-manipulation/map()/random() section) are original reimplementations of
// the standard Arduino API surface, written to match the interface and
// naming of ArduinoCore-avr (github.com/arduino/ArduinoCore-avr) and
// ArduinoCore-API (github.com/arduino/ArduinoCore-API) for sketch
// compatibility -- not copied from either. Both reference projects are
// LGPL 2.1; see LICENSE for this project's own MIT terms.

// Math constants (matches UNO R3/R4's Arduino.h — sketches can use these
// and <math.h> functions without an explicit #include <math.h>)
/** @name Math constants (matches UNO R3/R4's Arduino.h) */
///@{
#define	PI			3.1415926535897932384626433832795
#define	HALF_PI		1.5707963267948966192313216916398
#define	TWO_PI		6.283185307179586476925286766559
#define	DEG_TO_RAD	0.017453292519943295769236907684886
#define	RAD_TO_DEG	57.295779513082320876798154814105
#define	EULER		2.718281828459045235360287471352
///@}

/** @param deg angle in degrees @return angle in radians */
#define	radians( deg )	( (deg) * DEG_TO_RAD )
/** @param rad angle in radians @return angle in degrees */
#define	degrees( rad )	( (rad) * RAD_TO_DEG )

// Misc constants (matches UNO R3/R4's Arduino.h). LSBFIRST/MSBFIRST are
// enumerators of BitOrder, in arduino_spi.h
/** Not used anywhere in this core's own implementation -- declared purely
 *  for source compatibility with sketches/libraries that reference them
 *  (matches UNO R3/R4's Arduino.h, which likewise leaves them unused by
 *  the core itself).
 */
#define	SERIAL		0x0
#define	DISPLAY		0x1	/**< @see SERIAL */

// Clock-cycle conversion macros (matches UNO R3/R4's Arduino.h). Each board
// in this core always runs its core clock at a fixed frequency (no
// user-selectable F_CPU like AVR), supplied on the command line via
// platform.txt's compiler.defines (-DF_CPU={build.f_cpu}, from boards.txt)
// since it differs per board (e.g. 96MHz on FRDM-MCXA153, 150MHz on
// FRDM-MCXN947). Falls back to the FRDM-MCXA153 value if not supplied.
#ifndef F_CPU
#define	F_CPU	96000000UL
#endif
#define	clockCyclesPerMicrosecond()		( F_CPU / 1000000UL )
#define	clockCyclesToMicroseconds( a )	( (a) / clockCyclesPerMicrosecond() )
#define	microsecondsToClockCycles( a )	( (a) * clockCyclesPerMicrosecond() )

// Type aliases (matches UNO R3/R4's Arduino.h)
/** @name Classic Arduino type aliases */
///@{
typedef	bool		boolean;
typedef	uint8_t		byte;
typedef	uint16_t	word;
///@}

/** @param w a 16-bit value @return w (the one-argument form of word()) */
inline word makeWord( uint16_t w )           { return	w; }
/** @param h high byte @param l low byte @return (h << 8) | l */
inline word makeWord( uint8_t h, uint8_t l ) { return	(word)( ( h << 8 ) | l ); }
/** word(h, l) / word(w), as on AVR: a function-like macro, so the plain
 *  type name `word` above is left alone and only `word(` is rewritten. */
#define	word( ... )	makeWord( __VA_ARGS__ )

// min()/max() as templates rather than macros — avoids double-evaluation
// and doesn't shadow std::min/std::max (matches UNO R4's ArduinoCore-API)
/** @return the smaller of a and b (return type follows whichever operand's type "wins" via decltype) */
template <class T, class L>
auto min( const T& a, const L& b ) -> decltype( (b < a) ? b : a )
{
	return	(b < a) ? b : a;
}

/** @return the larger of a and b (return type follows whichever operand's type "wins" via decltype) */
template <class T, class L>
auto max( const T& a, const L& b ) -> decltype( (b < a) ? b : a )
{
	return	(a < b) ? b : a;
}

#ifdef	abs
#undef	abs
#endif
/** @param x value @return absolute value of x */
#define	abs( x )				( (x) > 0 ? (x) : -(x) )
/** @param amt value @param low lower bound @param high upper bound @return amt clamped to [low, high] */
#define	constrain( amt, low, high )	( (amt) < (low) ? (low) : ( (amt) > (high) ? (high) : (amt) ) )
/** @param x value @return x squared */
#define	sq( x )					( (x) * (x) )

/** @param w a 16-bit value @return its low byte */
#define	lowByte( w )	( (uint8_t)( (w) & 0xff ) )
/** @param w a 16-bit value @return its high byte */
#define	highByte( w )	( (uint8_t)( (w) >> 8 ) )

/** @name Bit-manipulation macros (matches UNO R3/R4's Arduino.h) */
///@{
/** @param value integer @param bit bit index @return the bit's value (0 or 1) */
#define	bitRead( value, bit )				( ( (value) >> (bit) ) & 0x01 )
/** Set a bit in value, in place. @param value integer lvalue @param bit bit index */
#define	bitSet( value, bit )				( (value) |= (1UL << (bit)) )
/** Clear a bit in value, in place. @param value integer lvalue @param bit bit index */
#define	bitClear( value, bit )				( (value) &= ~(1UL << (bit)) )
/** Toggle a bit in value, in place. @param value integer lvalue @param bit bit index */
#define	bitToggle( value, bit )				( (value) ^= (1UL << (bit)) )
/** Set or clear a bit in value, in place. @param value integer lvalue @param bit bit index @param bitvalue 0 or 1 */
#define	bitWrite( value, bit, bitvalue )	( (bitvalue) ? bitSet( (value), (bit) ) : bitClear( (value), (bit) ) )
/** @param b bit index @return 1UL << b */
#define	bit( b )		( 1UL << (b) )
/** avr-libc's name for bit(), still common in AVR-era code. @param b bit index @return 1UL << b */
#define	_BV( b )		( 1UL << (b) )
///@}

// interrupts()/noInterrupts() — direct Cortex-M PRIMASK control, no header
// dependency beyond the compiler's own inline-asm support
#define	interrupts()	__asm volatile ( "cpsie i" ::: "memory" )
#define	noInterrupts()	__asm volatile ( "cpsid i" ::: "memory" )

// map() — pure arithmetic, matches UNO R3/R4's long map(long,long,long,long,long)
/** Re-scale x from [in_min, in_max] to [out_min, out_max] (linear
 *  interpolation; out-of-range x extrapolates rather than clamping,
 *  matching classic Arduino's map()).
 * @return the re-scaled value
 */
inline long map( long x, long in_min, long in_max, long out_min, long out_max )
{
	return	(x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// random()/randomSeed() — thin wrapper over newlib's rand()/srand(), matches
// UNO R3/R4's long random(long) / long random(long,long) / void randomSeed(unsigned long)
/** Seed the PRNG used by random(). A seed of 0 is ignored (matches
 *  classic Arduino, which treats 0 as "don't reseed").
 * @param seed seed value
 */
inline void randomSeed( unsigned long seed )
{
	if ( seed != 0 )
		srand( (unsigned int)seed );
}

/** @param max exclusive upper bound @return a pseudo-random value in [0, max), or 0 if max <= 0 */
inline long random( long max )
{
	if ( max <= 0 )
		return	0;

	return	rand() % max;
}

/** @param min inclusive lower bound @param max exclusive upper bound @return a pseudo-random value in [min, max), or min if min >= max */
inline long random( long min, long max )
{
	if ( min >= max )
		return	min;

	return	random( max - min ) + min;
}

/** User-supplied one-time initialization, called once before the first loop(). */
void	setup( void );
/** User-supplied main loop body, called repeatedly forever after setup(). */
void	loop( void );
/** Busy-wait for at least the given number of milliseconds.
 * @param ms delay in milliseconds
 */
void	delay( unsigned long ms );
/** Busy-wait for at least the given number of microseconds.
 * @param us delay in microseconds
 */
void	delayMicroseconds( unsigned int us );
/** @return milliseconds since boot (SysTick-driven, wraps after ~49 days like classic Arduino) */
unsigned long	millis( void );
/** @return microseconds since boot (DWT cycle-count based) */
unsigned long	micros( void );

// yield() is a no-op here: there's no cooperative scheduler to hand control
// to (unlike ESP8266/ESP32/SAMD cores) -- declared purely so sketches that
// call it defensively (many libraries do, in busy-wait loops) still compile.
/** No-op on this core -- see explanation above. */
inline void yield( void ) {}

// avr-libc's number-to-string conversions, which AVR-era sketches and
// libraries call without including anything. itoa()/utoa() are newlib's own
// (same behavior as avr-libc's: a minus sign only in base 10), just
// declared here because -std=c++20 hides them in <stdlib.h>. The rest are
// in arduino_stdlib.cpp.
extern "C" {
/** @param value number @param str output buffer @param base 2..36 @return str */
char	*itoa( int value, char *str, int base );
/** @param value number @param str output buffer @param base 2..36 @return str */
char	*utoa( unsigned value, char *str, int base );
/** @param value number @param str output buffer @param base 2..36 @return str */
char	*ltoa( long value, char *str, int base );
/** @param value number @param str output buffer @param base 2..36 @return str */
char	*ultoa( unsigned long value, char *str, int base );
/** Format a double with a fixed number of decimals, rounded, as
 *  sprintf("%*.*f") would: right-aligned in width characters, or
 *  left-aligned if width is negative.
 * @param val value @param width minimum field width @param prec digits after the decimal point
 * @param str output buffer, large enough for the result @return str
 */
char	*dtostrf( double val, signed char width, unsigned char prec, char *str );
}

// String functions avr-libc's <string.h> has and newlib's hides under
// -std=c++20, since they are BSD/POSIX rather than ISO C. All are newlib's
// own, declared here as newlib declares them. Not switching to -std=gnu++20
// instead is deliberate: that also exposes names such as index() and
// y0()/y1(), and a sketch's global `int index` or `int x0, y0, x1, y1`,
// which builds on AVR, fails to build ("redeclared as different kind of
// entity").
extern "C" {
/** Copy at most size-1 chars, always terminated. @return strlen(src) */
size_t	strlcpy( char *dst, const char *src, size_t size );
/** Append, keeping the result within size, always terminated. @return the length it tried to make */
size_t	strlcat( char *dst, const char *src, size_t size );
/** @return a malloc'd copy of s, or NULL */
char	*strdup( const char *s );
/** @return a malloc'd copy of at most n chars of s, terminated, or NULL */
char	*strndup( const char *s, size_t n );
/** Reentrant strtok(); saveptr keeps the position between calls */
char	*strtok_r( char *__restrict str, const char *__restrict delim, char **__restrict saveptr );
/** @return strlen(s), but at most maxlen */
size_t	strnlen( const char *s, size_t maxlen );
/** Split *stringp at the first char in delim; empty fields are returned, unlike strtok() */
char	*strsep( char **stringp, const char *delim );
/** Copy up to n bytes, stopping after the first c. @return past the c in dst, or NULL */
void	*memccpy( void *__restrict dst, const void *__restrict src, int c, size_t n );
/** @return the first occurrence of needle in haystack, or NULL */
void	*memmem( const void *haystack, size_t hlen, const void *needle, size_t nlen );
/** Case-insensitive strstr() */
char	*strcasestr( const char *haystack, const char *needle );
}

// The <math.h> constants avr-libc defines and newlib hides under -std=c++20
// (it defines them only outside strict ISO mode). Same values as newlib's.
#ifndef M_PI
#define	M_E			2.7182818284590452354
#define	M_LOG2E		1.4426950408889634074
#define	M_LOG10E	0.43429448190325182765
#define	M_LN2		_M_LN2
#define	M_LN10		2.30258509299404568402
#define	M_PI		3.14159265358979323846
#define	M_PI_2		1.57079632679489661923
#define	M_PI_4		0.78539816339744830962
#define	M_1_PI		0.31830988618379067154
#define	M_2_PI		0.63661977236758134308
#define	M_2_SQRTPI	1.12837916709551257390
#define	M_SQRT2		1.41421356237309504880
#define	M_SQRT1_2	0.70710678118654752440
#endif

// ctype.h wrappers (matches UNO R3/R4's Arduino.h "Characters" category) —
// thin bool-returning renames of the standard <cctype> functions
/** @name Character-classification helpers (thin wrappers over &lt;cctype&gt;) */
///@{
inline bool isAlphaNumeric( int c )    { return	isalnum( c ); }
inline bool isAlpha( int c )           { return	isalpha( c ); }
/** Unlike the rest of this family, not a &lt;cctype&gt; wrapper -- newlib's own isascii() isn't used here, just a direct range check. */
inline bool isAscii( int c )           { return	(unsigned)c < 128; }
inline bool isWhitespace( int c )      { return	isspace( c ); }
inline bool isControl( int c )         { return	iscntrl( c ); }
inline bool isDigit( int c )           { return	isdigit( c ); }
inline bool isGraph( int c )           { return	isgraph( c ); }
inline bool isLowerCase( int c )       { return	islower( c ); }
inline bool isPrintable( int c )       { return	isprint( c ); }
inline bool isPunct( int c )           { return	ispunct( c ); }
inline bool isSpace( int c )           { return	isspace( c ); }
inline bool isUpperCase( int c )       { return	isupper( c ); }
inline bool isHexadecimalDigit( int c ){ return	isxdigit( c ); }
///@}

#endif // !R01LIB_ARDUINO_H
