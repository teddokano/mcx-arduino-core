/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"r01lib.h"
#include	"Arduino.h"
#include	"arduino_timer_hooks.h"
#include	"mcx_watchdog.h"

extern "C" {
#include	"fsl_clock.h"
}

void	(* volatile systick_hook)( uint32_t ms )	= nullptr;
void	(* volatile systick_ticker_hook)( uint32_t ms )	= nullptr;

int		main() __attribute__((weak));

//	Defined by the sketch or not at all; when not, the weak reference
//	stays null and the check below costs one comparison per loop().
void	serialEvent( void ) __attribute__((weak));
#if !defined( CPU_MCXC444VLH )
void	serialEvent1( void ) __attribute__((weak));
#endif
#if defined( CPU_MCXA156VLL )
void	serialEvent2( void ) __attribute__((weak));
#endif

namespace {
void	millis_init( void );
}

int main( void )
{
	//	millis() and micros() count from here, before setup(), as on AVR
	//	(the core started them at their first call before v0.11.0)
	millis_init();

	setup();

	while ( true )
	{
		loop();

		//	Same as AVR's serialEventRun(): once per loop(), and only while
		//	there is something to read
		if ( serialEvent && Serial.available() )
			serialEvent();
#if !defined( CPU_MCXC444VLH )	//	no Serial1 there
		if ( serialEvent1 && Serial1.available() )
			serialEvent1();
#endif
#if defined( CPU_MCXA156VLL )
		if ( serialEvent2 && Serial2.available() )
			serialEvent2();
#endif
	}

	return 0;
}

/*
 *  millis() / micros()
 *
 *  SysTick counts the core clock down from one millisecond's worth, and its
 *  interrupt counts the milliseconds (32 bits, ~49 days, as on AVR).
 *  micros() adds how far SysTick has counted into the millisecond, and
 *  delay() and delayMicroseconds() count it down as well. The same on all
 *  five boards: until 0.14.0 the Cortex-M33 boards counted the part of the
 *  millisecond with the DWT's cycle counter (CYCCNT), which belongs to the
 *  debugger: a debugger letting go of a running sketch stops it (it clears
 *  DEMCR.TRCENA), and one that writes it back makes millis() jump by tens
 *  of seconds. SysTick isn't the debugger's, and stands still, as the core
 *  does, at a breakpoint.
 *  Ticks taken more than a millisecond late, with interrupts off for that
 *  long, are lost, as on AVR: the cycle counter brought them back.
 */

namespace {

volatile uint32_t	ms_ticks		= 0;
uint32_t			cycles_per_us	= 1;
uint32_t			cycles_per_ms	= 1000;
bool				millis_ready	= false;

void millis_init( void )
{
	uint32_t	core_hz	= CLOCK_GetCoreSysClkFreq();

	cycles_per_us	= core_hz / 1000000u;
	cycles_per_ms	= core_hz / 1000u;

	millis_ready	= true;

	SysTick_Config( cycles_per_ms );
}

}	// namespace

// wait_us() goes through double and 64-bit arithmetic: about 45us on a
// Cortex-M0+ before it starts to wait, and about 14us too long on the
// FRDM-MCXA153 and FRDM-MCXA156. Count SysTick's own down-counter instead,
// which runs with interrupts off too
void delayMicroseconds( unsigned int us )
{
	if ( !millis_ready )
		millis_init();

	uint32_t	load	= SysTick->LOAD + 1u;
	uint32_t	target	= us * cycles_per_us;
	uint32_t	elapsed	= 0;
	uint32_t	prev	= SysTick->VAL;

	while ( elapsed < target )
	{
		uint32_t	now	= SysTick->VAL;

		elapsed	+= ( prev >= now ) ? ( prev - now ) : ( prev + load - now );
		prev	= now;
	}
}

// Not wait() either: on the Cortex-M0+, SDK_DelayAtLeastUs() counts its own
// loop, so every interrupt taken during it added its time on top
// (delay( 500 ) took 501 ms, and 514 ms under a 10 kHz TimerOne), and on
// the Cortex-M33 it counts the debugger's cycle counter (see above).
// Count SysTick's down-counter, as delayMicroseconds() does, a whole
// millisecond at a time so that a long delay doesn't overflow. Only an
// interrupt running for over 1 ms between two reads would go uncounted
void delay( unsigned long ms )
{
	if ( !millis_ready )
		millis_init();

	uint32_t	load	= SysTick->LOAD + 1u;
	uint32_t	elapsed	= 0;
	uint32_t	prev	= SysTick->VAL;

	while ( ms > 0 )
	{
		uint32_t	now	= SysTick->VAL;

		elapsed	+= ( prev >= now ) ? ( prev - now ) : ( prev + load - now );
		prev	= now;

		while ( ( ms > 0 ) && ( elapsed >= cycles_per_ms ) )
		{
			elapsed	-= cycles_per_ms;
			ms--;
		}
	}
}

extern "C" void SysTick_Handler( void )
{
	// One millisecond, however late this handler runs (SysTick has the
	// lowest priority, and waits behind MCXEthernet's tick, for one):
	// SysTick reloads by itself, so the next millisecond has already
	// started on time, and micros() reads it from SysTick->VAL
	ms_ticks	= ms_ticks + 1;
#if defined( CPU_MCXC444VLH )
	pwm_tpm_service( ms_ticks * 1000u );	// micros() at this tick
#endif

	void	( *hook )( uint32_t )	= systick_hook;

	if ( hook )
		hook( 1 );

	hook	= systick_ticker_hook;

	if ( hook )
		hook( 1 );

	//	Defined only if the sketch includes <avr/wdt.h> or WDT.h (mcx_watchdog.h)
	if ( mcx_sketch_wdt_event )
		mcx_sketch_wdt_event( MCX_WDT_TICK );
}

unsigned long millis( void )
{
	if ( !millis_ready )
		millis_init();

	return	(unsigned long)ms_ticks;
}

unsigned long micros( void )
{
	if ( !millis_ready )
		millis_init();

	// How far SysTick has counted down in this millisecond gives the rest.
	// A tick pending but not yet taken (interrupts are off here, or the
	// handler waits behind another) means SysTick has wrapped since the last
	// count: one more millisecond, and SysTick->VAL read again, since the
	// first read may have come just before the wrap. Before 0.14.0 a tick
	// pending counted only while VAL was in the top half, and micros() read
	// a millisecond short once the handler was over half a millisecond late
	uint32_t	ms;
	uint32_t	val;
	uint32_t	load	= SysTick->LOAD;
	uint32_t	primask	= __get_PRIMASK();	// called with interrupts off, leave them off

	__disable_irq();
	ms	= ms_ticks;
	val	= SysTick->VAL;
	if ( SCB->ICSR & SCB_ICSR_PENDSTSET_Msk )
	{
		ms++;
		val	= SysTick->VAL;
	}
	__set_PRIMASK( primask );

	uint32_t	delta_us	= ( load - val ) / cycles_per_us;

	return	(unsigned long)( (uint64_t)ms * 1000u + delta_us );
}
