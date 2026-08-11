/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"r01lib.h"
#include	"arduino.h"

extern "C" {
#include	"fsl_clock.h"
}

int		main() __attribute__((weak));

int main( void )
{
	setup();

	while ( true )
		loop();

	return 0;
}

void delay( unsigned long ms )
{
	wait( (float)ms / 1000.0f );
}

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
bool				millis_ready	= false;

void millis_init( void )
{
	uint32_t	core_hz	= CLOCK_GetCoreSysClkFreq();

	cycles_per_us	= core_hz / 1000000u;

	CoreDebug->DEMCR	|= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CTRL			|= DWT_CTRL_CYCCNTENA_Msk;

	// DWT->CYCCNT has been free-running since boot (wait()/delay() enabled
	// it). Snapshot "now" as the baseline so micros() doesn't report the
	// raw since-boot cycle count before the first SysTick tick has fired.
	ms_tick_dwt		= DWT->CYCCNT;

	millis_ready	= true;

	SysTick_Config( core_hz / 1000u );
}

}	// namespace

extern "C" void SysTick_Handler( void )
{
	ms_ticks	= ms_ticks + 1;
	ms_tick_dwt	= DWT->CYCCNT;
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

	uint32_t	ms;
	uint32_t	base;

	__disable_irq();
	ms		= ms_ticks;
	base	= ms_tick_dwt;
	__enable_irq();

	uint32_t	delta_us	= (uint32_t)( DWT->CYCCNT - base ) / cycles_per_us;

	return	(unsigned long)( (uint64_t)ms * 1000u + delta_us );
}
