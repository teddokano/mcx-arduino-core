/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_H
#define R01LIB_ARDUINO_H

#include	<math.h>
#include	<cstdlib>

#include	"r01lib.h"
#include	"arduino_string.h"
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
#define	PI			3.1415926535897932384626433832795
#define	HALF_PI		1.5707963267948966192313216916398
#define	TWO_PI		6.283185307179586476925286766559
#define	DEG_TO_RAD	0.017453292519943295769236907684886
#define	RAD_TO_DEG	57.295779513082320876798154814105
#define	EULER		2.718281828459045235360287471352

#define	radians( deg )	( (deg) * DEG_TO_RAD )
#define	degrees( rad )	( (rad) * RAD_TO_DEG )

// Misc constants (matches UNO R3/R4's Arduino.h)
#define	LSBFIRST	0
#define	MSBFIRST	1
#define	SERIAL		0x0
#define	DISPLAY		0x1

// Type aliases (matches UNO R3/R4's Arduino.h)
typedef	bool		boolean;
typedef	uint8_t		byte;
typedef	uint16_t	word;

// min()/max() as templates rather than macros — avoids double-evaluation
// and doesn't shadow std::min/std::max (matches UNO R4's ArduinoCore-API)
template <class T, class L>
auto min( const T& a, const L& b ) -> decltype( (b < a) ? b : a )
{
	return	(b < a) ? b : a;
}

template <class T, class L>
auto max( const T& a, const L& b ) -> decltype( (b < a) ? b : a )
{
	return	(a < b) ? b : a;
}

#ifdef	abs
#undef	abs
#endif
#define	abs( x )				( (x) > 0 ? (x) : -(x) )
#define	constrain( amt, low, high )	( (amt) < (low) ? (low) : ( (amt) > (high) ? (high) : (amt) ) )
#define	sq( x )					( (x) * (x) )

#define	lowByte( w )	( (uint8_t)( (w) & 0xff ) )
#define	highByte( w )	( (uint8_t)( (w) >> 8 ) )

#define	bitRead( value, bit )				( ( (value) >> (bit) ) & 0x01 )
#define	bitSet( value, bit )				( (value) |= (1UL << (bit)) )
#define	bitClear( value, bit )				( (value) &= ~(1UL << (bit)) )
#define	bitToggle( value, bit )				( (value) ^= (1UL << (bit)) )
#define	bitWrite( value, bit, bitvalue )	( (bitvalue) ? bitSet( (value), (bit) ) : bitClear( (value), (bit) ) )
#define	bit( b )		( 1UL << (b) )

// interrupts()/noInterrupts() — direct Cortex-M PRIMASK control, no header
// dependency beyond the compiler's own inline-asm support
#define	interrupts()	__asm volatile ( "cpsie i" ::: "memory" )
#define	noInterrupts()	__asm volatile ( "cpsid i" ::: "memory" )

// map() — pure arithmetic, matches UNO R3/R4's long map(long,long,long,long,long)
inline long map( long x, long in_min, long in_max, long out_min, long out_max )
{
	return	(x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// random()/randomSeed() — thin wrapper over newlib's rand()/srand(), matches
// UNO R3/R4's long random(long) / long random(long,long) / void randomSeed(unsigned long)
inline void randomSeed( unsigned long seed )
{
	if ( seed != 0 )
		srand( (unsigned int)seed );
}

inline long random( long max )
{
	if ( max <= 0 )
		return	0;

	return	rand() % max;
}

inline long random( long min, long max )
{
	if ( min >= max )
		return	min;

	return	random( max - min ) + min;
}

void	setup( void );
void	loop( void );
void	delay( unsigned long ms );
void	delayMicroseconds( unsigned int us );
unsigned long	millis( void );
unsigned long	micros( void );

#endif // !R01LIB_ARDUINO_H
