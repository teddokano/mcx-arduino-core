/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  GPIO IRQ handlers — placed in cores/arduino/ so they are compiled into
 *  core.a with --whole-archive, ensuring the weak symbols in
 *  startup_mcxa153.cpp are properly overridden.
 */

#include "fsl_port.h"
#include "fsl_gpio.h"
#include "fsl_common.h"

/* irq_handler() is defined in lib_r01lib_frdm_mcxa153.a (InterruptIn.cpp) */
void irq_handler( int num );

#ifdef CPU_MCXN947VDF

void GPIO00_IRQHandler( void ) { irq_handler( 0 ); }
void GPIO10_IRQHandler( void ) { irq_handler( 1 ); }
void GPIO20_IRQHandler( void ) { irq_handler( 2 ); }
void GPIO30_IRQHandler( void ) { irq_handler( 3 ); }
void GPIO40_IRQHandler( void ) { irq_handler( 4 ); }
void GPIO50_IRQHandler( void ) { irq_handler( 5 ); }

#elif defined( CPU_MCXC444VLH )

void PORTA_DriverIRQHandler( void )     { irq_handler( 0 ); }
void PORTC_PORTD_DriverIRQHandler( void ) { irq_handler( 1 ); }

#else /* MCXA153, MCXA156, MCXN236, ... */

void GPIO0_IRQHandler( void ) { irq_handler( 0 ); }
void GPIO1_IRQHandler( void ) { irq_handler( 1 ); }
void GPIO2_IRQHandler( void ) { irq_handler( 2 ); }
void GPIO3_IRQHandler( void ) { irq_handler( 3 ); }

#endif
