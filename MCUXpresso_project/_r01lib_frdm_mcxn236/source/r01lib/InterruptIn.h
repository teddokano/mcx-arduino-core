/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

/** InterruptIn class
 *	
 *  @class InterruptIn
 *
 *	A class for demonstrating GPIO pin interrupt
 */

#ifndef R01LIB_INTERRUPTIN_H
#define R01LIB_INTERRUPTIN_H

extern "C" {
#include	"fsl_utick.h"
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

	/** Destractor for InterruptIn
	 */
	virtual ~InterruptIn();
	
	/** Register callback function which is called by rising edge
	 *
	 * @param callback pointer to callback fuction
	 */
	virtual void	rise( func_ptr callback );

	/** Register callback function which is called by falling edge
	 *
	 * @param callback pointer to callback fuction
	 */
	virtual void	fall( func_ptr callback );

private:
	void	regist( func_ptr callback, gpio_interrupt_config_t type );
};

#endif // R01LIB_INTERRUPTIN_H
