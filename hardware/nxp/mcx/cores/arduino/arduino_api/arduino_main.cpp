/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"r01lib.h"
#include	"Arduino.h"
#include	"arduino_timer_hooks.h"

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

#if !defined( __ARM_ARCH_6M__ )	// Cortex-M0+: below, after millis_init()
void delay( unsigned long ms )
{
	wait( (float)ms / 1000.0f );
}

void delayMicroseconds( unsigned int us )
{
	wait_us( us );
}
#endif

/*
 *  millis() / micros()
 *
 *  DWT->CYCCNT already runs free (enabled by SDK_DelayAtLeastUs() for wait()),
 *  but as a bare 32bit cycle counter it wraps every ~45s at 96MHz — too short
 *  to use directly. SysTick is set to a 1ms tick to extend the range to the
 *  full 32bit millisecond count (~49 days, matching classic Arduino millis()).
 *  DWT->CYCCNT is kept only for sub-millisecond resolution inside micros().
 */

namespace {

volatile uint32_t	ms_ticks		= 0;
volatile uint32_t	ms_tick_dwt		= 0;
uint32_t			cycles_per_us	= 1;
uint32_t			cycles_per_ms	= 1000;
bool				millis_ready	= false;

void millis_init( void )
{
	uint32_t	core_hz	= CLOCK_GetCoreSysClkFreq();

	cycles_per_us	= core_hz / 1000000u;
	cycles_per_ms	= core_hz / 1000u;

#if !defined( __ARM_ARCH_6M__ )	// Cortex-M0+ has no cycle counter: micros() reads SysTick there
	CoreDebug->DEMCR	|= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CTRL			|= DWT_CTRL_CYCCNTENA_Msk;

	// DWT->CYCCNT has been free-running since boot (wait()/delay() enabled
	// it). Snapshot "now" as the baseline so micros() doesn't report the
	// raw since-boot cycle count before the first SysTick tick has fired.
	// Taken just before SysTick starts, so each tick comes a few cycles
	// after a whole number of milliseconds from it (SysTick_Handler())
	ms_tick_dwt		= DWT->CYCCNT;
#endif

	millis_ready	= true;

	SysTick_Config( cycles_per_ms );
}

}	// namespace

#if defined( __ARM_ARCH_6M__ )
// wait_us() goes through double and 64-bit arithmetic, done in software on
// a Cortex-M0+: about 45us before it starts to wait. Count SysTick's own
// down-counter instead, which runs with interrupts off too
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

// wait() has no cycle counter to read here either: SDK_DelayAtLeastUs()
// counts its own loop, so every interrupt taken during it added its time on
// top (delay( 500 ) took 501 ms, and 514 ms under a 10 kHz TimerOne).
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
#endif

extern "C" void SysTick_Handler( void )
{
#if !defined( __ARM_ARCH_6M__ )
	// The whole milliseconds since the last tick, by the cycle counter. This
	// handler can run late (SysTick has the lowest priority, and waits behind
	// MCXEthernet's tick, for one), and the time it runs mustn't become the
	// start of the millisecond, or micros() reads short until the next tick.
	// Ticks missed while interrupts were off for over 1 ms come back, and a
	// tick that comes in while this one counts it adds 0 when it runs
	uint32_t	n	= ( DWT->CYCCNT - ms_tick_dwt ) / cycles_per_ms;

	ms_ticks	= ms_ticks + n;
	ms_tick_dwt	= ms_tick_dwt + n * cycles_per_ms;
#else
	uint32_t	n	= 1;

	ms_ticks	= ms_ticks + 1;
#endif
#if defined( CPU_MCXC444VLH )
	pwm_tpm_service( ms_ticks * 1000u );	// micros() at this tick
#endif

	void	( *hook )( uint32_t )	= systick_hook;

	if ( hook && n )
		hook( n );

	hook	= systick_ticker_hook;

	if ( hook && n )
		hook( n );
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

#if defined( __ARM_ARCH_6M__ )
	// No cycle counter on a Cortex-M0+: how far SysTick has counted down in
	// this millisecond gives the rest. A tick pending but not yet taken
	// (interrupts are off here) means the count has wrapped: one more ms.
	uint32_t	ms;
	uint32_t	val;
	uint32_t	load	= SysTick->LOAD;
	uint32_t	primask	= __get_PRIMASK();	// called with interrupts off, leave them off

	__disable_irq();
	ms	= ms_ticks;
	val	= SysTick->VAL;
	if ( ( SCB->ICSR & SCB_ICSR_PENDSTSET_Msk ) && val > load / 2 )
		ms++;
	__set_PRIMASK( primask );

	uint32_t	delta_us	= ( load - val ) / cycles_per_us;
#else
	uint32_t	ms;
	uint32_t	base;
	uint32_t	primask	= __get_PRIMASK();	// called with interrupts off, leave them off

	__disable_irq();
	ms		= ms_ticks;
	base	= ms_tick_dwt;
	__set_PRIMASK( primask );

	uint32_t	delta_us	= (uint32_t)( DWT->CYCCNT - base ) / cycles_per_us;
#endif

	return	(unsigned long)( (uint64_t)ms * 1000u + delta_us );
}
