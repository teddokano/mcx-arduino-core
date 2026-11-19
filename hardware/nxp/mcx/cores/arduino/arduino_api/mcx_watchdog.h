/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

/** @file mcx_watchdog.h
 *  The watchdog behind <avr/wdt.h> and the bundled WDT library (UNO R4's
 *  interface). Internal: sketches use one of those two.
 *
 *  - FRDM-MCXA153, FRDM-MCXA156, FRDM-MCXN947, FRDM-MCXN236: the WWDT,
 *    clocked at 1MHz (a 250kHz count), set to the time asked for. It can't
 *    be stopped once started (its WDEN and WDRESET only clear on a reset),
 *    so after mcx_wdt_disable() the core's 1ms SysTick interrupt feeds it.
 *  - FRDM-MCXC444: the COP, whose one-time setting (SIM_COPC) is left at
 *    its reset value, 1024ms on the 1kHz LPO: the startup code turns the
 *    COP off unless the sketch includes a watchdog header (see
 *    mcx_sketch_wdt_event()). The SysTick interrupt feeds it, and resets
 *    the board itself (NVIC_SystemReset()) once the time asked for has
 *    passed since the last mcx_wdt_reset(), so the reset comes within 1ms
 *    of it. The COP only resets the board when interrupts stop (a hang
 *    with them off, a HardFault), within 1024ms.
 *
 *  While enabled, panic() and a HardFault (which ends in panic()) end in a
 *  watchdog reset, after the message is sent; after mcx_wdt_disable()
 *  they stop as they do without a watchdog.
 *
 *  The WWDT and the COP both stop counting while a debugger halts the CPU.
 */

#ifndef MCX_WATCHDOG_H
#define MCX_WATCHDOG_H

#include	<stdint.h>
#include	<stdbool.h>

/** The longest time mcx_wdt_enable() takes: the WWDT's 24-bit count at
 *  250kHz, on every board for the same range everywhere. */
#define	MCX_WDT_MAX_MS		67108u

/** Bits of mcx_reset_flags(), as AVR's MCUSR. */
#define	MCX_RESET_PORF		0	///< power-on reset
#define	MCX_RESET_EXTRF		1	///< the reset pin (the RESET button, the debugger)
#define	MCX_RESET_BORF		2	///< low-voltage (brown-out) reset
#define	MCX_RESET_WDRF		3	///< watchdog reset

#ifdef __cplusplus
extern "C" {
#endif

/** Start the watchdog, or change its time.
 * @param ms the time, 1 to MCX_WDT_MAX_MS milliseconds
 * @return false, changing nothing, for a time out of that range */
bool		mcx_wdt_enable( uint32_t ms );

/** Feed the watchdog: its time starts again. */
void		mcx_wdt_reset( void );

/** Stop it resetting the board. The hardware keeps running (it can't be
 *  stopped), fed by the core from here on. */
void		mcx_wdt_disable( void );

/** @return the time set by mcx_wdt_enable(), in milliseconds; 0 before it
 *  or after mcx_wdt_disable() */
uint32_t	mcx_wdt_timeout( void );

/** @return milliseconds left before the watchdog resets the board; 0 while
 *  it isn't enabled */
uint32_t	mcx_wdt_remaining( void );

/** The cause of the last reset, as AVR's MCUSR: the MCX_RESET_* bits, read
 *  from the chip's reset status at the first call. Writable, as MCUSR is
 *  (`MCUSR = 0;`), but only the copy changes. Unlike AVR's, it holds the
 *  last reset only: the flags don't add up over resets until cleared. */
volatile uint8_t	*mcx_reset_flags( void );

/** What the core tells the watchdog. */
enum
{
	MCX_WDT_TICK		= 0,	///< from the 1ms SysTick interrupt
	MCX_WDT_PANIC_ENTER	= 1,	///< panic() starts: one feed, so its message goes out
	MCX_WDT_PANIC_LOOP	= 2,	///< panic() goes on: fed only after mcx_wdt_disable()
};

/** The watchdog's handling of the core's events. */
void		mcx_wdt_event( int event );

/** Defined, weak, by mcx_watchdog_used.h, which <avr/wdt.h> and WDT.h
 *  include: the core (the SysTick interrupt, panic(), and FRDM-MCXC444's
 *  startup code) calls the watchdog through it only when it is defined,
 *  so a sketch without a watchdog links none of it, and on FRDM-MCXC444
 *  has its COP turned off at startup as before. */
void		mcx_sketch_wdt_event( int event ) __attribute__((weak));

#ifdef __cplusplus
}
#endif

#endif	// MCX_WATCHDOG_H
