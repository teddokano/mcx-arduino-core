/** Crash reports: what a HardFault prints.
 *
 *  A crash (a HardFault) stops the sketch, blinks the red LED in SOS, and
 *  prints a line starting with "error: HardFault:" on the Serial Monitor,
 *  repeated on every round of the blink. This sketch crashes on purpose,
 *  in the way FAULT_KIND below picks, to check that line. Change
 *  FAULT_KIND and upload again for each kind; the board stays stopped
 *  until then.
 *
 *  No wiring. What each kind should print (the addresses vary with the
 *  build):
 *
 *    1  bad memory access to 0x00000000 at PC ...        (write to address 0)
 *    2  bad memory access to 0x2F000000 at PC ...        (nothing at that address)
 *    3  call through a null or bad function pointer at PC 0x00000000
 *    4  stack overflow                                   (no PC: see below)
 *    5  bad memory access to 0x00000000 at PC ... in an interrupt
 *    6  bad memory access to 0x2F000000 at PC ...        (before setup(), so
 *                                                         nothing else is printed)
 *    7  undefined instruction at PC ...
 *    8  breakpoint without a debugger at PC ...          (with no debugger attached)
 *
 *  Before the error line, every kind but 6 prints "about to crash", which
 *  was still waiting in the transmit buffer when the crash came: the
 *  report sends what the sketch printed first.
 *
 *  The PC is where the crash happened. arm-none-eabi-addr2line -e
 *  <sketch>.elf <PC> gives the source line: here the line that FAULT_KIND
 *  picks in setup(). A stack overflow has no PC, because the processor
 *  could not save one on a stack that has no room left.
 */

#include <Arduino.h>

#define FAULT_KIND 2

volatile uint32_t sink;

//	Kind 6: a global object's constructor runs before setup()
struct CrashEarly
{
	CrashEarly()
	{
		if ( FAULT_KIND == 6 )
			sink = *(volatile uint32_t *)0x2F000000;
	}
} crash_early;

//	Kind 5: SVC_Handler is an exception handler, entered by "svc 0"
extern "C" void SVC_Handler( void )
{
	*(volatile uint32_t *)0 = 1;
}

//	Kind 4: never returns, and uses 64 bytes of stack more on every call
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winfinite-recursion"
int recurse( int n )
{
	volatile uint8_t buf[ 64 ];

	buf[ 0 ] = n;
	return recurse( n + 1 ) + buf[ 0 ];
}
#pragma GCC diagnostic pop

void setup()
{
	Serial.begin( 115200 );
	delay( 300 );
	Serial.print( "\r\ncrash kind " );
	Serial.println( FAULT_KIND );
	delay( 50 );

	//	Left in the transmit buffer: nothing waits for it to go out
	Serial.println( "about to crash" );

	switch ( FAULT_KIND )
	{
		case 1:
			*(volatile uint32_t *)0 = 1;
			break;
		case 2:
			sink = *(volatile uint32_t *)0x2F000000;
			break;
		case 3:
		{
			void ( *volatile fp )( void ) = nullptr;
			fp();
			break;
		}
		case 4:
			sink = recurse( 0 );
			break;
		case 5:
			__asm volatile ( "svc 0" );
			break;
		case 7:
			__asm volatile ( ".short 0xDE00" );	//	UDF #0
			break;
		case 8:
			__BKPT( 1 );
			break;
	}

	Serial.println( "no crash: FAULT_KIND must be 1 to 8" );
}

void loop()
{
}
