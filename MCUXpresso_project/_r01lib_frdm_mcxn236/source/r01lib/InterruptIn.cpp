/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

extern "C" {
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "fsl_common.h"
#include "pin_mux.h"
#include "board.h"
#include "fsl_debug_console.h"

#include	"irq.h"
}

#include	"io.h"
#include	"InterruptIn.h"

#ifdef	CPU_MCXN947VDF
	#define		N_GPIO		6
	#define		GPIO_BITS	32
	static	GPIO_Type	*gpio_ptr[]	= GPIO_BASE_PTRS;
	static	IRQn_Type	irqs[]		= GPIO_IRQS;
#else // CPU_MCXN947VDF
	#define		N_GPIO		4
	#define		GPIO_BITS	32
	static	GPIO_Type	*gpio_ptr[]	= GPIO_BASE_PTRS;
	static	IRQn_Type	irqs[]		= GPIO_IRQS;
#endif //CPU_MCXN947VDF

utick_callback_t	cb_table[ N_GPIO ][ GPIO_BITS ]	= { NULL };

void irq_handler( int num )
{
	uint32_t	flags;
	flags	= GPIO_GpioGetInterruptFlags( gpio_ptr[ num ] );
	GPIO_GpioClearInterruptFlags( gpio_ptr[ num ], flags );
	
	for ( int i = 0; i < GPIO_BITS; i++ )
		if ( cb_table[ num ][ i ] && (flags & (1 << i)) )
			(cb_table[ num ][ i ])();
}


InterruptIn::InterruptIn( uint8_t pin_num )
	: DigitalIn( pin_num )
{
}

InterruptIn::~InterruptIn() {}

void InterruptIn::rise( func_ptr callback )
{
	regist( callback, kGPIO_InterruptRisingEdge );
}

void InterruptIn::fall( func_ptr callback )
{
	regist( *callback, kGPIO_InterruptFallingEdge );
}

void InterruptIn::regist( func_ptr callback, gpio_interrupt_config_t type )
{
	/* Init input switch GPIO. */
	#if (defined(FSL_FEATURE_PORT_HAS_NO_INTERRUPT) && FSL_FEATURE_PORT_HAS_NO_INTERRUPT)
		GPIO_SetPinInterruptConfig( gpio_n, gpio_pin, type );
	#else
		PORT_SetPinInterruptConfig( gpio_n, gpio_pin, type );
	#endif

	for ( int i = 0; i < N_GPIO; i++ )
	{
		if ( gpio_ptr[i] == gpio_n )
		{
			cb_table[ i ][ gpio_pin ]	= callback;
			EnableIRQ( irqs[ i ] );
			break;
		}
	}
}
