/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_MCU_H
#define R01LIB_MCU_H

#include "r01lib.h"

/** Chip-level bring-up: boot pins, boot clocks, peripheral clock
 *  attach/enable, and (per-CPU) debug console init. Called exactly once,
 *  automatically, by Obj's constructor the first time any r01lib
 *  peripheral object is created -- not normally called directly.
 */
void	init_mcu( void );

/** Busy-wait for at least the given duration.
 * @param delayTime_sec delay in seconds (fractional; e.g. 0.001 = 1ms)
 */
void	wait( double delayTime_sec );

/** Busy-wait for at least the given duration.
 * @param milloseconds delay in milliseconds
 */
void	wait_ms( unsigned int milloseconds );

/** Busy-wait for at least the given duration.
 * @param microseconds delay in microseconds
 */
void	wait_us( unsigned int microseconds );

/** Report a fatal error and hang.
 *
 *  Prints @p s, then blinks the on-board RGB LEDs in SOS Morse code
 *  ("... --- ...") forever. Never returns.
 *
 * @param s error message to print before entering the blink loop
 */
void 	panic( const char *s );

#if defined( CPU_MCXN236VDF )
/** Set LP_FLEXCOMM2 back to running its LPI2C and its LPUART at once.
 *
 *  On FRDM-MCXN236, FlexComm2 is Wire1's LPI2C2 (FC2_P0/P1, P4_0/P4_1) and
 *  Serial1's LPUART2 (FC2_P2/P3, D1/D0). The LPUART reaches P2/P3 only in
 *  that combined mode, and every SDK init on this FlexComm --
 *  LPUART_Init(), LPI2C_MasterInit(), LPI2C_SlaveInit() -- selects its own
 *  peripheral alone, which cuts the other one off. So each of them is
 *  followed by this. Other instances are left alone.
 *
 * @param instance the LP_FLEXCOMM instance just initialized
 */
void	flexcomm_keep_shared( uint32_t instance );
#endif


#endif // R01LIB_MCU_H
