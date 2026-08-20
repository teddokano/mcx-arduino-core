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

// Misc constants (matches UNO R3/R4's Arduino.h)
/** SPI/shiftOut()/shiftIn() bit-order constants. */
#define	LSBFIRST	0
#define	MSBFIRST	1	/**< @see LSBFIRST */
/** Not used anywhere in this core's own implementation -- declared purely
 *  for source compatibility with sketches/libraries that reference them
 *  (matches UNO R3/R4's Arduino.h, which likewise leaves them unused by
 *  the core itself).
 */
#define	SERIAL		0x0
#define	DISPLAY		0x1	/**< @see SERIAL */

/*
 *  Real Arduino declares LSBFIRST/MSBFIRST as enumerators of an actual
 *  `BitOrder` enum type (api/Common.h), so any type named BitOrder is
 *  automatically usable. This project defines LSBFIRST/MSBFIRST as plain
 *  #define macros instead (established earlier -- see arduino_spi.h's
 *  `enum endian`), so a real `enum BitOrder { LSBFIRST, MSBFIRST }` can't
 *  be declared here too: the preprocessor would substitute those tokens
 *  away before the compiler ever saw them as enumerator names. BitOrder
 *  is provided as a plain integer typedef instead, just so the TYPE NAME
 *  exists for libraries (e.g. Adafruit BusIO's `typedef BitOrder
 *  BusIOBitOrder;`) that expect the core to declare it.
 */
typedef	uint8_t	BitOrder;

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
