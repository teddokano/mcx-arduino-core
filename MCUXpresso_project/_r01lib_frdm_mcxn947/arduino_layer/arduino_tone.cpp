/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  tone()/noTone() toggle an arbitrary GPIO pin from a CTIMER0 match
 *  interrupt (square wave, 2 toggles per cycle) so any digital pin can be
 *  used, matching classic Arduino tone() — unlike analogWrite() which is
 *  limited to the FlexPWM0-backed PWM0-PWM5 pins. Only 1 tone can be
 *  active at a time (same limitation as most Arduino cores).
 */

#include	"r01lib.h"
#include	"Arduino.h"

extern "C" {
#include	"fsl_ctimer.h"
#include	"fsl_common.h"
#include	"fsl_clock.h"
}

namespace {

constexpr uint32_t	TONE_INDEFINITE	= 0xFFFFFFFFu;

int				tone_pin_num	= -1;
DigitalInOut	*tone_pin		= nullptr;
bool			tone_state		= false;
uint32_t		toggles_left	= 0;
bool			ctimer_ready	= false;

void ctimer_init( void )
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
#if defined( CPU_MCXN947VDF )
	CLOCK_SetClkDiv( kCLOCK_DivCtimer0Clk, 1u );
#else
	CLOCK_SetClockDiv( kCLOCK_DivCTIMER0, 1u );
#endif

	ctimer_config_t	cfg;

	CTIMER_GetDefaultConfig( &cfg );
	CTIMER_Init( CTIMER0, &cfg );

	ctimer_ready	= true;
}

}	// namespace

extern "C" void CTIMER0_IRQHandler( void )
{
	uint32_t	flags	= CTIMER_GetStatusFlags( CTIMER0 );

	CTIMER_ClearStatusFlags( CTIMER0, flags );

	if ( !( flags & kCTIMER_Match0Flag ) || ( tone_pin == nullptr ) )
		return;

	if ( toggles_left != TONE_INDEFINITE )
	{
		if ( toggles_left == 0 )
		{
			CTIMER_StopTimer( CTIMER0 );
			*tone_pin	= false;
			return;
		}

		toggles_left--;
	}

	tone_state	= !tone_state;
	*tone_pin	= tone_state;
}

void tone( int pin_num, unsigned int frequency, unsigned long duration )
{
#ifdef	ARDUINO_PIN_RENUMBERING
	pin_num	= arduino_pin_by_number[ pin_num ];
#endif

	if ( ( pin_num < 0 ) || ( frequency == 0 ) )
		return;

	if ( !ctimer_ready )
		ctimer_init();

	CTIMER_StopTimer( CTIMER0 );

	if ( tone_pin_num != pin_num )
	{
		delete tone_pin;
		tone_pin		= new DigitalInOut( (uint8_t)pin_num, DigitalInOut::OUTPUT );
		tone_pin_num	= pin_num;
	}

	tone_state	= false;
	*tone_pin	= tone_state;

	uint32_t	timer_clk	= CLOCK_GetCTimerClkFreq( 0 );
	uint32_t	match		= ( timer_clk / ( 2u * frequency ) ) - 1u;

	toggles_left	= ( duration == 0 )
					? TONE_INDEFINITE
					: (uint32_t)( ( (uint64_t)2u * frequency * duration ) / 1000u );

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

void noTone( int pin_num )
{
#ifdef	ARDUINO_PIN_RENUMBERING
	pin_num	= arduino_pin_by_number[ pin_num ];
#endif

	if ( pin_num != tone_pin_num )
		return;

	CTIMER_StopTimer( CTIMER0 );

	if ( tone_pin != nullptr )
		*tone_pin	= false;
}
