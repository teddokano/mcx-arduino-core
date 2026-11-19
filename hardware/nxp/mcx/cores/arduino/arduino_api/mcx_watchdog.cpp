/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

/*
 *  The watchdog: see mcx_watchdog.h. Nothing here is linked into a sketch
 *  that doesn't include a watchdog header: the core reaches it only through
 *  mcx_sketch_wdt_event(), which those headers define.
 */

#include	"r01lib.h"
#include	"Arduino.h"
#include	"mcx_watchdog.h"

extern "C" {
#include	"fsl_clock.h"
}

#if defined( CPU_MCXC444VLH )
//	From startup_mcxc444.cpp: a mark hw_reset_now() leaves in RAM that the
//	reset doesn't clear, and whether this boot found it there
extern "C" volatile unsigned int	mcx_wdt_reset_magic;
extern "C" volatile unsigned int	mcx_wdt_reset_at_boot;
#endif

namespace {

enum State : uint8_t
{
	WDT_OFF,		// never enabled
	WDT_ON,			// enabled: resets the board unless fed in time
	WDT_DISABLED,	// mcx_wdt_disable(): the core feeds it
};

volatile State		s_state		= WDT_OFF;
volatile uint32_t	s_timeout	= 0;	// ms
volatile uint32_t	s_last		= 0;	// millis() at the last feed

//	Saves and restores PRIMASK: a feed is a two-write sequence, and one from
//	the SysTick interrupt landing between the two writes of another would
//	break it (on the WWDT, a broken sequence is a feed error: a reset)
class irq_off
{
public:
	irq_off()	: primask( __get_PRIMASK() )	{ __disable_irq(); }
	~irq_off()	{ __set_PRIMASK( primask ); }
private:
	uint32_t	primask;
};

#if defined( CPU_MCXC444VLH )

constexpr uint32_t	WDT_RESET_MAGIC	= 0x57445452;	// "WDTR"

void hw_feed( void )
{
	irq_off	off;

	SIM->SRVCOP	= 0x55;
	SIM->SRVCOP	= 0xAA;
}

void hw_reset_now( void )
{
	mcx_wdt_reset_magic	= WDT_RESET_MAGIC;
	NVIC_SystemReset();
}

#else	// the WWDT

constexpr uint32_t	TICKS_PER_MS	= 250;			// 1MHz, divided by 4 by the WWDT
constexpr uint32_t	TC_MIN			= 0xFF;			// what the WWDT takes at least
constexpr uint32_t	TC_MAX			= 0xFFFFFF;		// 24 bits: about 67 seconds

void hw_feed( void )
{
	irq_off	off;

	WWDT0->FEED	= 0xAA;
	WWDT0->FEED	= 0x55;
}

void clock_on( void )
{
#if defined( CPU_MCXA153VLH ) || defined( CPU_MCXA156VLL )
	CLOCK_SetClockDiv( kCLOCK_DivWWDT0, 1U );
	CLOCK_EnableClock( kCLOCK_GateWWDT0 );
#elif defined( CPU_MCXN947VDF ) || defined( CPU_MCXN236VDF )
	SYSCON->CLOCK_CTRL	|= SYSCON_CLOCK_CTRL_FRO1MHZ_ENA_MASK | SYSCON_CLOCK_CTRL_FRO1MHZ_CLK_ENA_MASK;
	CLOCK_SetClkDiv( kCLOCK_DivWdt0Clk, 1U );
	CLOCK_EnableClock( kCLOCK_Wwdt0 );
#else
#error "mcx_watchdog.cpp: say how this board's WWDT is clocked"
#endif
}

//	A new TC (or MOD) takes effect 3 WDCLK cycles (3us) after it is written,
//	so the feed that loads it waits that long first
void set_tc( uint32_t ticks )
{
	WWDT0->TC	= ticks;
	delayMicroseconds( 4 );
}

#endif

}	// namespace

extern "C" bool mcx_wdt_enable( uint32_t ms )
{
	if ( ( ms == 0 ) || ( ms > MCX_WDT_MAX_MS ) )
		return	false;

#if !defined( CPU_MCXC444VLH )
	if ( s_state == WDT_OFF )
		clock_on();

	uint32_t	ticks	= ms * TICKS_PER_MS;

	set_tc( ( ticks < TC_MIN ) ? TC_MIN : ticks );
	WWDT0->MOD	= WWDT_MOD_WDEN_MASK | WWDT_MOD_WDRESET_MASK;	// for good: only a reset clears them
#endif

	s_timeout	= ms;
	s_last		= millis();
	s_state		= WDT_ON;
	hw_feed();
	return	true;
}

extern "C" void mcx_wdt_reset( void )
{
	if ( s_state == WDT_OFF )
		return;

	s_last	= millis();
#if !defined( CPU_MCXC444VLH )
	hw_feed();	// on FRDM-MCXC444 the SysTick interrupt feeds the COP: s_last is the feed
#endif
}

extern "C" void mcx_wdt_disable( void )
{
	if ( s_state != WDT_ON )
		return;

#if !defined( CPU_MCXC444VLH )
	set_tc( TC_MAX );	// the longest time, for the core's feeds from here on
#endif
	s_state	= WDT_DISABLED;
	s_last	= millis();
	hw_feed();
}

extern "C" uint32_t mcx_wdt_timeout( void )
{
	return	( s_state == WDT_ON ) ? s_timeout : 0;
}

extern "C" uint32_t mcx_wdt_remaining( void )
{
	if ( s_state != WDT_ON )
		return	0;

#if defined( CPU_MCXC444VLH )
	uint32_t	elapsed	= millis() - s_last;

	return	( elapsed < s_timeout ) ? s_timeout - elapsed : 0;
#else
	return	WWDT0->TV / TICKS_PER_MS;
#endif
}

extern "C" void mcx_wdt_event( int event )
{
	switch ( event )
	{
	case MCX_WDT_TICK:
#if defined( CPU_MCXC444VLH )
		//	The COP runs from reset (the startup code left it on), so it is
		//	fed in every state, until the time asked for has passed
		if ( ( s_state == WDT_ON ) && ( millis() - s_last >= s_timeout ) )
			hw_reset_now();
		hw_feed();
#else
		//	Disabled: the WWDT is at its longest time, so once a second is enough
		if ( ( s_state == WDT_DISABLED ) && ( millis() - s_last >= 1000 ) )
		{
			s_last	= millis();
			hw_feed();
		}
#endif
		break;

	case MCX_WDT_PANIC_ENTER:
		if ( s_state == WDT_ON )
		{
			s_last	= millis();	// on FRDM-MCXC444, the time starts again for the message
			hw_feed();
		}
		break;

	case MCX_WDT_PANIC_LOOP:
		//	Enabled: no feeds, so that the watchdog resets the board. From a
		//	HardFault, where the SysTick interrupt doesn't run, that is the
		//	WWDT, or on FRDM-MCXC444 the COP within 1024ms
#if defined( CPU_MCXC444VLH )
		if ( s_state != WDT_ON )
			hw_feed();
#else
		if ( s_state == WDT_DISABLED )
			hw_feed();
#endif
		break;

	default:
		break;
	}
}

extern "C" volatile uint8_t *mcx_reset_flags( void )
{
	static volatile uint8_t	flags	= 0;
	static bool				read	= false;

	if ( !read )
	{
		uint8_t	f	= 0;
#if defined( CPU_MCXC444VLH )
		uint32_t	s0	= RCM->SRS0;
		uint32_t	s1	= RCM->SRS1;

		if ( ( s0 & RCM_SRS0_WDOG_MASK ) || ( mcx_wdt_reset_at_boot && ( s1 & RCM_SRS1_SW_MASK ) ) )
			f	|= 1u << MCX_RESET_WDRF;
		if ( s0 & RCM_SRS0_POR_MASK )
			f	|= 1u << MCX_RESET_PORF;
		if ( s0 & RCM_SRS0_PIN_MASK )
			f	|= 1u << MCX_RESET_EXTRF;
		if ( ( s0 & RCM_SRS0_LVD_MASK ) && !( s0 & RCM_SRS0_POR_MASK ) )	// a POR sets LVD too
			f	|= 1u << MCX_RESET_BORF;
#else
#if defined( CPU_MCXA153VLH ) || defined( CPU_MCXA156VLL )
		uint32_t	srs	= CMC->SRS;
#elif defined( CPU_MCXN947VDF ) || defined( CPU_MCXN236VDF )
		uint32_t	srs	= CMC0->SRS;
#else
#error "mcx_watchdog.cpp: say where this board's reset status is"
#endif
		if ( srs & CMC_SRS_WWDT0_MASK )
			f	|= 1u << MCX_RESET_WDRF;
		if ( srs & CMC_SRS_POR_MASK )
			f	|= 1u << MCX_RESET_PORF;
		if ( srs & CMC_SRS_PIN_MASK )
			f	|= 1u << MCX_RESET_EXTRF;
		if ( ( srs & CMC_SRS_VD_MASK ) && !( srs & CMC_SRS_POR_MASK ) )
			f	|= 1u << MCX_RESET_BORF;
#endif
		flags	= f;
		read	= true;
	}

	return	&flags;
}
