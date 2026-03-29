/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include	"fsl_port.h"
#include	"fsl_gpio.h"
#include	"fsl_common.h"
#include	"fsl_debug_console.h"

#include	"irq.h"

#ifdef	CPU_MCXN947VDF
void GPIO00_IRQHandler( void )
{	irq_handler( 0 );
}

void GPIO10_IRQHandler( void )
{	irq_handler( 1 );
}

void GPIO20_IRQHandler( void )
{	irq_handler( 2 );
}

void GPIO30_IRQHandler( void )
{	irq_handler( 3 );
}

void GPIO40_IRQHandler( void )
{	irq_handler( 4 );
}

void GPIO50_IRQHandler( void )
{	irq_handler( 5 );
}

#else // CPU_MCXN947VDF
void GPIO0_IRQHandler( void )
{	irq_handler( 0 );
}

void GPIO1_IRQHandler( void )
{	irq_handler( 1 );
}

void GPIO2_IRQHandler( void )
{	irq_handler( 2 );
}

void GPIO3_IRQHandler( void )
{
	irq_handler( 3 );
}
#endif // CPU_MCXN947VDF


