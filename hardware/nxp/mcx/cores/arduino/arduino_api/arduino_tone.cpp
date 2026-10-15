/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  tone()/noTone() toggle an arbitrary GPIO pin from a periodic timer
 *  interrupt (square wave, 2 toggles per cycle) so any digital pin can be
 *  used, matching classic Arduino tone() — unlike analogWrite() which is
 *  limited to the PWM pins. Only 1 tone can be active at a time (same
 *  limitation as most Arduino cores).
 *
 *  The timer is CTIMER0's match 0 on the MCX A/N chips, and PIT channel 0
 *  on FRDM-MCXC444, which has no CTIMER. Only timer_init(), timer_start()
 *  and timer_stop() differ.
 */

#include	"r01lib.h"
#include	"Arduino.h"

extern "C" {
#if defined( CPU_MCXC444VLH )
#include	"fsl_pit.h"
#else
#include	"fsl_ctimer.h"
#endif
#include	"fsl_common.h"
#include	"fsl_clock.h"
}

namespace {

constexpr uint32_t	TONE_INDEFINITE	= 0xFFFFFFFFu;

int				tone_pin_num	= -1;
DigitalInOut	*tone_pin		= nullptr;
bool			tone_state		= false;
uint32_t		toggles_left	= 0;
bool			timer_ready		= false;

#if defined( CPU_MCXC444VLH )

void timer_init( void )
{
	pit_config_t	cfg;

	PIT_GetDefaultConfig( &cfg );
	PIT_Init( PIT, &cfg );	// also turns the PIT's clock on

	timer_ready	= true;
}

void timer_stop( void )
{
	PIT_StopTimer( PIT, kPIT_Chnl_0 );
}

// One interrupt every half period. The PIT counts the bus clock.
void timer_start( unsigned int frequency )
{
	uint32_t	ticks	= CLOCK_GetFreq( kCLOCK_BusClk ) / ( 2u * frequency );

	PIT_SetTimerPeriod( PIT, kPIT_Chnl_0, ticks ? ticks : 1u );
	PIT_ClearStatusFlags( PIT, kPIT_Chnl_0, kPIT_TimerFlag );
	PIT_EnableInterrupts( PIT, kPIT_Chnl_0, kPIT_TimerInterruptEnable );
	EnableIRQ( PIT_IRQn );
	PIT_StartTimer( PIT, kPIT_Chnl_0 );	// loads the period afresh
}

#else

void timer_init( void )
{
	/*
	 *  CTIMER0's clock mux is unattached out of reset (CLOCK_GetCTimerClkFreq()
	 *  returns 0 until a source is attached), so CTIMER_Init() alone (clock
	 *  gate + reset only) leaves the peripheral clockless — match never fires.
	 *  Attach FRO12M, same fixed/always-available source AnalogIn.cpp uses.
	 *
	 *  N947 note: the divider symbol is named differently here
	 *  (`kCLOCK_DivCtimer0Clk`, mixed case + "Clk" suffix) than on A153
	 *  (`kCLOCK_DivCTIMER0`, all-caps) -- confirmed via NXP's own
	 *  FRDM-MCXN947 CTIMER driver example, which also uses CLOCK_SetClkDiv
	 *  (not CLOCK_SetClockDiv -- also a different function name on this
	 *  chip's SDK).
	 */
	CLOCK_AttachClk( kFRO12M_to_CTIMER0 );
#if defined( CPU_MCXN947VDF ) || defined( CPU_MCXN236VDF )
	CLOCK_SetClkDiv( kCLOCK_DivCtimer0Clk, 1u );
#elif defined( CPU_MCXA153VLH ) || defined( CPU_MCXA156VLL )
	CLOCK_SetClockDiv( kCLOCK_DivCTIMER0, 1u );
#else
#error "arduino_tone.cpp: set CTIMER0's clock divider for this chip"
#endif

	ctimer_config_t	cfg;

	CTIMER_GetDefaultConfig( &cfg );
	CTIMER_Init( CTIMER0, &cfg );

	timer_ready	= true;
}

void timer_stop( void )
{
	CTIMER_StopTimer( CTIMER0 );
}

void timer_start( unsigned int frequency )
{
	uint32_t	timer_clk	= CLOCK_GetCTimerClkFreq( 0 );
	uint32_t	match		= ( timer_clk / ( 2u * frequency ) ) - 1u;

	ctimer_match_config_t	match_cfg	= {};
	match_cfg.matchValue			= match;
	match_cfg.enableCounterReset	= true;
	match_cfg.enableCounterStop		= false;
	match_cfg.outControl			= kCTIMER_Output_NoAction;
	match_cfg.outPinInitState		= false;
	match_cfg.enableInterrupt		= true;

	CTIMER_SetupMatch( CTIMER0, kCTIMER_Match_0, &match_cfg );
	EnableIRQ( CTIMER0_IRQn );

	/*
	 *  StopTimer() only clears CEN — it leaves TC wherever it was. If the new
	 *  match value is smaller than the stale TC, TC won't hit it until it
	 *  wraps all the way around (~6 min @ 12MHz), so the tone never starts.
	 *  Reset TC/PR to 0 before (re)starting.
	 */
	CTIMER_Reset( CTIMER0 );
	CTIMER_StartTimer( CTIMER0 );
}

#endif

// Called from the timer interrupt, every half period
void tone_tick( void )
{
	if ( tone_pin == nullptr )
		return;

	if ( toggles_left != TONE_INDEFINITE )
	{
		if ( toggles_left == 0 )
		{
			timer_stop();
			*tone_pin	= false;
			return;
		}

		toggles_left--;
	}

	tone_state	= !tone_state;
	*tone_pin	= tone_state;
}

}	// namespace

#if defined( CPU_MCXC444VLH )

extern "C" void PIT_IRQHandler( void )
{
	uint32_t	flags	= PIT_GetStatusFlags( PIT, kPIT_Chnl_0 );

	PIT_ClearStatusFlags( PIT, kPIT_Chnl_0, flags );

	if ( flags & kPIT_TimerFlag )
		tone_tick();
}

#else

extern "C" void CTIMER0_IRQHandler( void )
{
	uint32_t	flags	= CTIMER_GetStatusFlags( CTIMER0 );

	CTIMER_ClearStatusFlags( CTIMER0, flags );

	if ( flags & kCTIMER_Match0Flag )
		tone_tick();
}

#endif

void tone( int pin_num, unsigned int frequency, unsigned long duration )
{
#ifdef	ARDUINO_PIN_RENUMBERING
	pin_num	= arduino_pin_by_number[ pin_num ];
#endif

	if ( ( pin_num < 0 ) || ( frequency == 0 ) )
		return;

	if ( !timer_ready )
		timer_init();

	timer_stop();

	if ( tone_pin_num != pin_num )
	{
		delete tone_pin;
		tone_pin		= new DigitalInOut( (uint8_t)pin_num, DigitalInOut::OUTPUT );
		tone_pin_num	= pin_num;
	}

	tone_state	= false;
	*tone_pin	= tone_state;

	toggles_left	= ( duration == 0 )
					? TONE_INDEFINITE
					: (uint32_t)( ( (uint64_t)2u * frequency * duration ) / 1000u );

	timer_start( frequency );
}

void noTone( int pin_num )
{
#ifdef	ARDUINO_PIN_RENUMBERING
	pin_num	= arduino_pin_by_number[ pin_num ];
#endif

	if ( pin_num != tone_pin_num )
		return;

	if ( timer_ready )
		timer_stop();

	if ( tone_pin != nullptr )
		*tone_pin	= false;
}
