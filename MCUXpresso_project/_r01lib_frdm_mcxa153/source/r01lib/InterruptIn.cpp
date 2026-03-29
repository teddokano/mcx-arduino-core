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
#include	"mcu.h"

#ifdef	CPU_MCXN947VDF
	#define		N_GPIO		6
	#define		GPIO_BITS	32
	static	GPIO_Type	*gpio_ptr[]	= GPIO_BASE_PTRS;
	static	IRQn_Type	irqs[]		= GPIO_IRQS;

#elif	CPU_MCXC444VLH
	#define		N_GPIO		3
	#define		GPIO_BITS	24
	static	GPIO_Type	*gpio_ptr[]	= { GPIOA, GPIOC, GPIOD };
	
#else // CPU_MCXN947VDF
	#define		N_GPIO		4
	#define		GPIO_BITS	32
	static	GPIO_Type	*gpio_ptr[]	= GPIO_BASE_PTRS;
	static	IRQn_Type	irqs[]		= GPIO_IRQS;
#endif //CPU_MCXN947VDF

#if (defined(FSL_FEATURE_PORT_HAS_NO_INTERRUPT) && FSL_FEATURE_PORT_HAS_NO_INTERRUPT)
#define	kRisingEdge		kGPIO_InterruptRisingEdge
#define	kFallingEdge	kGPIO_InterruptFallingEdge
#else
#define	kRisingEdge		kPORT_InterruptRisingEdge
#define	kFallingEdge	kPORT_InterruptFallingEdge
typedef void (*utick_callback_t)(void);
#endif


utick_callback_t	cb_table[ N_GPIO ][ GPIO_BITS ]	= { NULL };

#if (defined(FSL_FEATURE_PORT_HAS_NO_INTERRUPT) && FSL_FEATURE_PORT_HAS_NO_INTERRUPT)

void irq_handler( int num )
{
	uint32_t	flags;
	flags	= GPIO_GpioGetInterruptFlags( gpio_ptr[ num ] );
	GPIO_GpioClearInterruptFlags( gpio_ptr[ num ], flags );
	
	for ( int i = 0; i < GPIO_BITS; i++ )
		if ( cb_table[ num ][ i ] && (flags & (1 << i)) )
			(cb_table[ num ][ i ])();
	
	SDK_ISR_EXIT_BARRIER;
}

#else

void irq_handler( int num )
{
	PORT_Type	*ports[]	= { PORTA, PORTC, PORTD };
	GPIO_Type	*gpios[]	= { GPIOA, GPIOC, GPIOD };
	uint32_t	flags;
	
	for ( int i = num; i < 3; i++ )
	{
		if ( (flags	= ports[ i ]->ISFR) )
		{
			GPIO_PortClearInterruptFlags( gpios[ i ], flags );

			for ( int b = 0; b < GPIO_BITS; b++ )
				if ( cb_table[ i ][ b ] && flags & (1 << b) )
					(cb_table[ i ][ b ])();			
		}
	}
	SDK_ISR_EXIT_BARRIER;
}

#endif



InterruptIn::InterruptIn( uint8_t pin_num )
	: DigitalIn( pin_num )
{
}

InterruptIn::~InterruptIn() {}

void InterruptIn::rise( func_ptr callback )
{
	regist( callback, kRisingEdge );
}

void InterruptIn::fall( func_ptr callback )
{
	regist( *callback, kFallingEdge );
}

#if (defined(FSL_FEATURE_PORT_HAS_NO_INTERRUPT) && FSL_FEATURE_PORT_HAS_NO_INTERRUPT)
void InterruptIn::regist( func_ptr callback, gpio_interrupt_config_t type )
#else
void InterruptIn::regist( func_ptr callback, port_interrupt_t type )
#endif
{
#if (defined(FSL_FEATURE_PORT_HAS_NO_INTERRUPT) && FSL_FEATURE_PORT_HAS_NO_INTERRUPT)
	GPIO_SetPinInterruptConfig( gpio_n, gpio_pin, type );
	
	for ( int i = 0; i < N_GPIO; i++ )
	{
		if ( gpio_ptr[i] == gpio_n )
		{
			cb_table[ i ][ gpio_pin ]	= callback;
			EnableIRQ( irqs[ i ] );
			break;
		}
	}
#else
	int	irqn[]	= PORT_IRQS;
	
	uint32_t ofst	= ((uint32_t)gpio_n - (uint32_t)GPIOA);
	uint32_t step	= ((uint32_t)GPIOB - (uint32_t)GPIOA);
	uint32_t idx	= ofst / step;
	
	if ( irqn[ idx ] == NotAvail_IRQn )
	{
		panic( "error on interrupt registering: not supported on this port" );
	}
	
	PORT_SetPinInterruptConfig( port_n, gpio_pin, type );
	
	idx	= (idx == 0) ? idx : (idx - 1);	//	skip portB
	
	for ( int i = 0; i < N_GPIO; i++ )
	{
		if ( gpio_ptr[i] == gpio_n )
		{
			cb_table[ i ][ gpio_pin ]	= callback;
			EnableIRQ( (idx == 0) ? PORTA_IRQn : PORTC_PORTD_IRQn );
			break;
		}
	}
	
#endif

}
