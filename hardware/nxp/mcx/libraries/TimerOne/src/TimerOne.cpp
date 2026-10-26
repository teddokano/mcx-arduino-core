/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  Original implementation of the TimerOne library's interface on CTIMER1
 *  (MCX A and N) and PIT channel 1 (FRDM-MCXC444); none of its source
 *  (CC BY 3.0) is copied.
 *
 *  As Timer1 does on AVR, the timer sets its flag at the end of every
 *  period whether a function is attached or not, and attaching only lets
 *  the flag through to the interrupt: a period that ended while nothing
 *  was attached is called for as soon as something is.
 */

#include	"TimerOne.h"
#include	"arduino_timer_hooks.h"

extern "C" {
#if defined( CPU_MCXC444VLH )
#include	"fsl_pit.h"
#else
#include	"fsl_ctimer.h"
#endif
#include	"fsl_clock.h"
}

TimerOne	Timer1;

void	(*TimerOne::isrCallback)()	= TimerOne::isrDefaultUnused;

void TimerOne::isrDefaultUnused()
{
}

namespace {

bool		ready			= false;
uint32_t	clock_hz		= 0;
uint32_t	period_ticks	= 0;

uint32_t ticks_for( unsigned long us )
{
	uint64_t	ticks	= (uint64_t)us * clock_hz / 1000000u;
	uint64_t	least	= clock_hz / 1000000u;	// 1 us

	if ( ticks < least )
		ticks	= least;
	if ( ticks > 0xFFFFFFFFu )
		ticks	= 0xFFFFFFFFu;

	return	(uint32_t)ticks;
}

void call_isr( void )
{
	TimerOne::isrCallback();
}

#if defined( CPU_MCXC444VLH )

//	The PIT counts down from LDVAL and can't be paused: stop() keeps what
//	was left, and resume() counts that first. A new LDVAL written while the
//	channel runs takes effect when it next reloads
bool		running		= false;
uint32_t	left		= 0;	// counts left when stopped, 0 for a whole period

void hw_init( void )
{
	pit_module_init();	// not PIT_Init(), which would stop tone()'s channel 0
	PIT_StopTimer( PIT, kPIT_Chnl_1 );

	clock_hz	= CLOCK_GetFreq( kCLOCK_BusClk );

	//	One interrupt for both channels, so this also moves tone()'s
	//	channel 0 to the lowest priority
	pit_channel1_hook	= call_isr;
	NVIC_SetPriority( PIT_IRQn, ( 1u << __NVIC_PRIO_BITS ) - 1 );
	EnableIRQ( PIT_IRQn );
}

void hw_run( uint32_t first )
{
	PIT_StopTimer( PIT, kPIT_Chnl_1 );
	PIT_SetTimerPeriod( PIT, kPIT_Chnl_1, first );
	PIT_StartTimer( PIT, kPIT_Chnl_1 );	// loads LDVAL
	PIT_SetTimerPeriod( PIT, kPIT_Chnl_1, period_ticks );	// for the periods after the first

	running	= true;
	left	= 0;
}

void hw_set_period( void )
{
	if ( running )
		hw_run( period_ticks );
	else
		left	= 0;
}

void hw_start( void )
{
	hw_run( period_ticks );
}

void hw_stop( void )
{
	if ( running )
		left	= PIT_GetCurrentTimerCount( PIT, kPIT_Chnl_1 ) + 1u;

	PIT_StopTimer( PIT, kPIT_Chnl_1 );
	running	= false;
}

void hw_resume( void )
{
	if ( running )
		return;

	hw_run( left ? left : period_ticks );
}

void hw_attach( void )
{
	PIT_EnableInterrupts( PIT, kPIT_Chnl_1, kPIT_TimerInterruptEnable );
}

void hw_detach( void )
{
	PIT_DisableInterrupts( PIT, kPIT_Chnl_1, kPIT_TimerInterruptEnable );
}

#else

void set_divider( uint32_t div )
{
#if defined( CPU_MCXN947VDF ) || defined( CPU_MCXN236VDF )
	CLOCK_SetClkDiv( kCLOCK_DivCtimer1Clk, div );
#elif defined( CPU_MCXA153VLH ) || defined( CPU_MCXA156VLL )
	CLOCK_SetClockDiv( kCLOCK_DivCTIMER1, div );
#else
#error "TimerOne: set CTIMER1's clock divider for this chip"
#endif
}

void hw_init( void )
{
	//	FRO_HF, which the core clock comes from too (directly on the MCX A
	//	chips, through PLL0 on the MCX N chips), so that the periods agree
	//	with millis() and micros(), as on AVR, where both come from one
	//	clock. FRO12M, which tone() uses, is a separate oscillator: 0.5%
	//	apart from the core clock was seen on FRDM-MCXA156. Divided to
	//	12 MHz (from 96 MHz on the A chips, 48 MHz on the N chips)
	CLOCK_AttachClk( kFRO_HF_to_CTIMER1 );
	set_divider( 1u );
	set_divider( CLOCK_GetCTimerClkFreq( 1 ) / 12000000u );

	ctimer_config_t	cfg;

	CTIMER_GetDefaultConfig( &cfg );
	CTIMER_Init( CTIMER1, &cfg );

	clock_hz	= CLOCK_GetCTimerClkFreq( 1 );

	ctimer_match_config_t	match	= {};
	match.matchValue			= 0xFFFFFFFFu;
	match.enableCounterReset	= true;
	match.enableInterrupt		= true;	// the flag; the interrupt itself waits for attachInterrupt()
	match.outControl			= kCTIMER_Output_NoAction;
	CTIMER_SetupMatch( CTIMER1, kCTIMER_Match_0, &match );

	NVIC_DisableIRQ( CTIMER1_IRQn );	// CTIMER_SetupMatch() turned it on
	NVIC_SetPriority( CTIMER1_IRQn, ( 1u << __NVIC_PRIO_BITS ) - 1 );
}

void hw_set_period( void )
{
	CTIMER1->MR[ kCTIMER_Match_0 ]	= period_ticks - 1u;

	//	Past the new match, the count would run on to 2^32 (about 6 min)
	//	before the next period
	if ( CTIMER1->TC >= period_ticks - 1u )
		CTIMER1->TC	= 0;
}

void hw_start( void )
{
	CTIMER_StopTimer( CTIMER1 );
	CTIMER_Reset( CTIMER1 );	// the count and the prescaler to 0
	CTIMER_StartTimer( CTIMER1 );
}

void hw_stop( void )
{
	CTIMER_StopTimer( CTIMER1 );
}

void hw_resume( void )
{
	CTIMER_StartTimer( CTIMER1 );
}

void hw_attach( void )
{
	NVIC_EnableIRQ( CTIMER1_IRQn );
}

void hw_detach( void )
{
	NVIC_DisableIRQ( CTIMER1_IRQn );
}

#endif

void ensure_ready( void )
{
	if ( !ready )
		Timer1.initialize();
}

}	// namespace

#if !defined( CPU_MCXC444VLH )

extern "C" void CTIMER1_IRQHandler( void )
{
	CTIMER_ClearStatusFlags( CTIMER1, CTIMER_GetStatusFlags( CTIMER1 ) );

	call_isr();

	SDK_ISR_EXIT_BARRIER;
}

#endif

void TimerOne::initialize( unsigned long microseconds )
{
	if ( !ready )
	{
		hw_init();
		ready	= true;
	}

	hw_stop();
	period_ticks	= ticks_for( microseconds );
	hw_set_period();
	hw_start();
}

void TimerOne::setPeriod( unsigned long microseconds )
{
	ensure_ready();

	period_ticks	= ticks_for( microseconds );
	hw_set_period();
	hw_resume();
}

void TimerOne::start()
{
	ensure_ready();
	hw_start();
}

void TimerOne::stop()
{
	ensure_ready();
	hw_stop();
}

void TimerOne::restart()
{
	start();
}

void TimerOne::resume()
{
	ensure_ready();
	hw_resume();
}

void TimerOne::attachInterrupt( void (*isr)() )
{
	ensure_ready();

	isrCallback	= isr ? isr : isrDefaultUnused;
	hw_attach();
}

void TimerOne::attachInterrupt( void (*isr)(), unsigned long microseconds )
{
	if ( microseconds > 0 )
		setPeriod( microseconds );

	attachInterrupt( isr );
}

void TimerOne::detachInterrupt()
{
	ensure_ready();
	hw_detach();
}
