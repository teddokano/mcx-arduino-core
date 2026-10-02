/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_IRQ_H
#define	R01LIB_IRQ_H

/** Common GPIO interrupt dispatcher.
 *
 *  Called by each per-bank GPIOn_IRQHandler/PORTx_DriverIRQHandler defined
 *  in irq.c (the actual vector table entries, force-linked via
 *  --whole-archive so they override the startup file's weak defaults).
 *  Clears the bank's pending interrupt flags, then calls every registered
 *  per-pin callback (InterruptIn::rise()/fall()/low()) whose flag is set.
 *
 * @param num GPIO bank/port index (chip-specific numbering; see irq.c's
 *            gpio_ptr[]/ports[] tables)
 */
void irq_handler( int num );

#endif // R01LIB_IRQ_H
