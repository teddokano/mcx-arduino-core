/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

/** InterruptIn class
 *
 *  @class InterruptIn
 *
 *	A class for GPIO pin interrupt handling
 */

#ifndef R01LIB_INTERRUPTIN_H
#define R01LIB_INTERRUPTIN_H

extern "C" {
#ifndef	CPU_MCXC444VLH
#include	"fsl_utick.h"
#endif
}

#include	"io.h"

typedef	void (*func_ptr)( void );

class InterruptIn : public DigitalIn
{	
public:
	
	/** Create an InterruptIn instance with specified pins
	 *
	 * @param pin_num pin number to get the interrupt
	 */
	InterruptIn( uint8_t pin_num );

	/** Destructor for InterruptIn
	 */
	virtual ~InterruptIn();
	
	/** Register callback function which is called by rising edge
	 *
	 * @param callback pointer to callback function
	 */
	virtual void	rise( func_ptr callback );

	/** Register callback function which is called by falling edge
	 *
	 * @param callback pointer to callback function
	 */
	virtual void	fall( func_ptr callback );

	/** Register callback function which is called continuously while the
	 *  pin reads logic low (level-triggered, matches classic Arduino's
	 *  attachInterrupt(pin, isr, LOW))
	 *
	 * @param callback pointer to callback function
	 */
	virtual void	low( func_ptr callback );

	/** Disable the interrupt and clear the registered callback
	 *
	 *  Reverses rise()/fall() -- disables the pin's interrupt/DMA request
	 *  in hardware and removes its entry from the IRQ dispatch table.
	 */
	virtual void	disable( void );

private:
#if (defined(FSL_FEATURE_PORT_HAS_NO_INTERRUPT) && FSL_FEATURE_PORT_HAS_NO_INTERRUPT)
	void	regist( func_ptr callback, gpio_interrupt_config_t type );
#else
	void	regist( func_ptr callback, port_interrupt_t type );
#endif
};

#endif // R01LIB_INTERRUPTIN_H
