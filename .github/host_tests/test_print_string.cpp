/*
 *  Host tests of the core's Print and String: what they print, checked on
 *  the build machine with its own compiler (run.sh), in seconds and with
 *  no board. Print.cpp and arduino_string.cpp are the core's own files;
 *  only r01lib's mcu.h is replaced (stub/).
 *
 *  The expected strings are the ones the sketches under
 *  examples/Arduino_compatible_API (test_String, test_String_64bit,
 *  test_print_float_rounding, test_Serial_BIN_and_write) check on the
 *  boards. Left out: whatever depends on long being 32 bits, as it is on
 *  the boards and isn't on a 64-bit host (print(-1L, HEX) is 8 Fs there).
 */

#include	"Print.h"
#include	"arduino_string.h"

#include	<climits>
#include	<cmath>
#include	<cstdio>
#include	<string>

namespace {

int	failures	= 0;
int	checks		= 0;

struct Capture : Print
{
	std::string	s;
	size_t write( uint8_t c ) override { s += (char)c; return 1; }
	std::string take( void ) { std::string r = s; s.clear(); return r; }
};

void expect( const char *label, const std::string &got, const char *want )
{
	checks++;
	if ( got == want )
		return;
	failures++;
	std::printf( "FAIL: %s: got \"%s\", want \"%s\"\n", label, got.c_str(), want );
}

void expect_true( const char *label, bool ok )
{
	checks++;
	if ( ok )
		return;
	failures++;
	std::printf( "FAIL: %s\n", label );
}

#define	PRINTS( expr, want )	do { Capture c; c.expr; expect( #expr, c.take(), want ); } while ( 0 )
#define	STRING( expr, want )	expect( #expr, std::string( String expr .c_str() ), want )

void integers( void )
{
	PRINTS( print( 0 ), "0" );
	PRINTS( print( -42 ), "-42" );
	PRINTS( print( 255, HEX ), "FF" );
	PRINTS( print( 255, OCT ), "377" );
	PRINTS( print( 5, BIN ), "101" );
	PRINTS( print( 35, 36 ), "Z" );
	PRINTS( print( 0x41, 0 ), "A" );					// base 0: the byte itself
	PRINTS( print( 4000000000UL ), "4000000000" );
	PRINTS( println( 7 ), "7\r\n" );

	//	64 bits: the core converts them itself (newlib-nano's printf has no %lld)
	PRINTS( print( -5000000000LL ), "-5000000000" );
	PRINTS( print( LLONG_MIN ), "-9223372036854775808" );
	PRINTS( print( LLONG_MAX ), "9223372036854775807" );
	PRINTS( print( ULLONG_MAX ), "18446744073709551615" );
	PRINTS( print( 0xDEADBEEFCAFEULL, HEX ), "DEADBEEFCAFE" );
	PRINTS( print( 01234567012345ULL, OCT ), "1234567012345" );
	PRINTS( print( -1LL, HEX ), "FFFFFFFFFFFFFFFF" );	// two's complement, as on AVR
	PRINTS( print( 5LL, BIN ), "101" );
	PRINTS( println( -42LL ), "-42\r\n" );
}

void floats( void )
{
	PRINTS( print( 1.999 ), "2.00" );
	PRINTS( print( 3.14159, 4 ), "3.1416" );
	PRINTS( print( -3.14159, 3 ), "-3.142" );
	PRINTS( print( 0.125, 2 ), "0.13" );
	PRINTS( print( 9.9999, 2 ), "10.00" );
	PRINTS( print( 1.999, 0 ), "2" );
	PRINTS( print( 2.5, 0 ), "3" );
	PRINTS( print( 0.0, 2 ), "0.00" );
	PRINTS( print( -0.001, 2 ), "-0.00" );
	PRINTS( print( 123456.789, 1 ), "123456.8" );
	PRINTS( print( 4000000000.0, 0 ), "4000000000" );
	PRINTS( print( 5e9, 2 ), "ovf" );
	PRINTS( print( -5e9, 2 ), "ovf" );
	PRINTS( print( NAN, 2 ), "nan" );
	PRINTS( print( INFINITY, 2 ), "inf" );
	PRINTS( print( 0.1f, 3 ), "0.100" );
	PRINTS( println( 1.5, 0 ), "2\r\n" );
}

void strings( void )
{
	STRING( ( 1.999 ), "2.00" );
	STRING( ( 1.999f, 1 ), "2.0" );
	STRING( ( 3.14159, 4 ), "3.1416" );
	STRING( ( -2.345678, 3 ), "-2.346" );
	STRING( ( 5.0, 0 ), "5" );
	STRING( ( 1e10, 1 ), "10000000000.0" );
	STRING( ( 255, HEX ), "ff" );					// lower case, as AVR's String
	STRING( ( 5, BIN ), "101" );
	STRING( ( -5000000000LL ), "-5000000000" );
	STRING( ( LLONG_MIN ), "-9223372036854775808" );
	STRING( ( 10000000000ULL ), "10000000000" );
	STRING( ( 255LL, HEX ), "ff" );
	STRING( ( 0xDEADBEEFCAFEULL, 16 ), "deadbeefcafe" );
	STRING( ( ULLONG_MAX, 10 ), "18446744073709551615" );

	String	c	= String( "Hello" ) + String( ", " ) + "world" + String( '!' );
	expect( "concat", c.c_str(), "Hello, world!" );
	expect( "numeric concat", ( String( 42 ) + " / " + String( 3.14, 2 ) ).c_str(), "42 / 3.14" );
	expect_true( "length", c.length() == 13 );
	expect_true( "indexOf", c.indexOf( "world" ) == 7 );
	expect( "substring", c.substring( 7, 12 ).c_str(), "world" );

	String	u	= c;
	u.toUpperCase();
	expect( "toUpperCase", u.c_str(), "HELLO, WORLD!" );

	String	t	= "  trim me \t";
	t.trim();
	expect( "trim", t.c_str(), "trim me" );

	String	r	= c;
	r.replace( "world", "there" );
	expect( "replace", r.c_str(), "Hello, there!" );

	expect_true( "toInt", String( "12345" ).toInt() == 12345 );
	expect_true( "toFloat", fabsf( String( "3.5" ).toFloat() - 3.5f ) < 1e-6f );

	String	v	= "value=";
	v	+= 10000000000ULL;
	expect( "+= unsigned long long", v.c_str(), "value=10000000000" );

	String	w;
	w.concat( -5000000000LL );
	expect( "concat(long long)", w.c_str(), "-5000000000" );

	String	f	= "t=";
	f	+= 21.996f;
	expect( "+= float", f.c_str(), "t=22.00" );
}

}	// namespace

int main( void )
{
	integers();
	floats();
	strings();
	std::printf( "%d checks, %d failed\n", checks, failures );
	return failures ? 1 : 0;
}
