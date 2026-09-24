/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  avr-libc's ltoa()/ultoa()/dtostrf(), which newlib doesn't have. Written
 *  to match avr-libc's documented behavior, not copied from it or from
 *  ArduinoCore-API. itoa()/utoa() come from newlib itself; see Arduino.h.
 */

#include	<stdio.h>
#include	"Arduino.h"

// long and int are the same width here, so newlib's int versions already
// give avr-libc's long ones exactly.
static_assert( sizeof( long ) == sizeof( int ), "ltoa()/ultoa() assume a 32-bit long" );

extern "C" char *ltoa( long value, char *str, int base )
{
	return	itoa( (int)value, str, base );
}

extern "C" char *ultoa( unsigned long value, char *str, int base )
{
	return	utoa( (unsigned)value, str, base );
}

// platform.txt links with -u _printf_float, so snprintf()'s %f is the full
// one: rounded, and with width/alignment, as avr-libc's dtostrf() is. No
// length limit is passed on, same as avr-libc -- the caller sizes str.
extern "C" char *dtostrf( double val, signed char width, unsigned char prec, char *str )
{
	sprintf( str, "%*.*f", (int)width, (int)prec, val );
	return	str;
}
