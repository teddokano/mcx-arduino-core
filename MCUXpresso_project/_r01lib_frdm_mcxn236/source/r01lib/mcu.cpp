/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

extern "C" {
#include "peripherals.h"
#include "fsl_common.h"
#include "fsl_debug_console.h"
#include "fsl_i3c.h"
#include "fsl_lpi2c.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"

#include "fsl_utick.h"
#include "fsl_clock.h"
#include "fsl_reset.h"
}

#include "mcu.h"
#include "obj.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wprio-ctor-dtor"
__attribute__((constructor(0)))
void start_mcu()
{
	Obj	o( false );
}

//__attribute__((constructor(1)))
void start_message()
{
	PRINTF("\r\n***  MCU initialized properly  ***\r\n");
}
#pragma GCC diagnostic pop


void init_mcu( void )
{
#ifdef	CPU_MCXN947VDF
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom4Clk, 1);
	CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

	/* Attach PLL0 clock to I3C, 150MHz / 6 = 25MHz. */
	CLOCK_SetClkDiv(kCLOCK_DivI3c1FClk, 6U);
	CLOCK_AttachClk(kPLL0_to_I3C1FCLK);

	/* I2C */
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom2Clk, 1u);
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);

	/* SPI */
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom1Clk, 1u);
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM1);

	SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_FRO1MHZ_ENA_MASK;	//	UTICK

	CLOCK_EnableClock( kCLOCK_Gpio0 );
	CLOCK_EnableClock( kCLOCK_Gpio1 );
	CLOCK_EnableClock( kCLOCK_Gpio2 );
	CLOCK_EnableClock( kCLOCK_Gpio3 );
	CLOCK_EnableClock( kCLOCK_Gpio4 );

	/* Init board hardware. */
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
	#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
		/* Init FSL debug console. */
		BOARD_InitDebugConsole();
	#endif

#elif	CPU_MCXN236VDF
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom4Clk, 1);
	CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

	/* Attach PLL0 clock to I3C, 150MHz / 12 = 12.5MHz. */
	CLOCK_SetClkDiv(kCLOCK_DivI3c1FClk, 12U);
	CLOCK_AttachClk(kPLL0_to_I3C1FCLK);

	/* I2C */
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom2Clk, 1u);
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);

	/* SPI */
	CLOCK_SetClkDiv(kCLOCK_DivFlexcom3Clk, 1u);
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM3);

	SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_FRO1MHZ_ENA_MASK;	//	UTICK

	CLOCK_EnableClock( kCLOCK_Gpio0 );
	CLOCK_EnableClock( kCLOCK_Gpio1 );
	CLOCK_EnableClock( kCLOCK_Gpio2 );
	CLOCK_EnableClock( kCLOCK_Gpio3 );
	CLOCK_EnableClock( kCLOCK_Gpio4 );

	/* Init board hardware. */
#if 1
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
#else
	BOARD_InitPins();
	BOARD_BootClockFRO12M();
	BOARD_InitPeripherals();
#endif
	
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
		/* Init FSL debug console. */
		BOARD_InitDebugConsole();
	#endif


#elif	CPU_MCXA156VLL
	
	RESET_ReleasePeripheralReset( kLPUART0_RST_SHIFT_RSTn);
	RESET_ReleasePeripheralReset( kPORT0_RST_SHIFT_RSTn );
	RESET_ReleasePeripheralReset( kPORT1_RST_SHIFT_RSTn );
	RESET_ReleasePeripheralReset( kGPIO1_RST_SHIFT_RSTn );
	
	/* Attach peripheral clock */
	CLOCK_SetClockDiv( kCLOCK_DivI3C0_FCLK, 4U );
	CLOCK_AttachClk( kFRO_HF_DIV_to_I3C0FCLK );

	/* I2C */
	CLOCK_SetClockDiv( kCLOCK_DivLPI2C0, 1u );
	CLOCK_SetClockDiv( kCLOCK_DivLPI2C1, 1u );
	CLOCK_SetClockDiv( kCLOCK_DivLPI2C3, 1u );
	CLOCK_AttachClk( kFRO12M_to_LPI2C0 );
	CLOCK_AttachClk( kFRO12M_to_LPI2C1 );
	CLOCK_AttachClk( kFRO12M_to_LPI2C3 );

	/* SPI */
	CLOCK_SetClockDiv( kCLOCK_DivLPSPI0, 1u );
	CLOCK_AttachClk( kFRO12M_to_LPSPI0 );
	CLOCK_SetClockDiv( kCLOCK_DivLPSPI1, 1u );
	CLOCK_AttachClk( kFRO12M_to_LPSPI1 );

	CLOCK_EnableClock( kCLOCK_GateGPIO0 );
	CLOCK_EnableClock( kCLOCK_GateGPIO1 );
	CLOCK_EnableClock( kCLOCK_GateGPIO2 );
	CLOCK_EnableClock( kCLOCK_GateGPIO3 );
	CLOCK_EnableClock( kCLOCK_GateGPIO4 );

	RESET_PeripheralReset( kUTICK0_RST_SHIFT_RSTn );
	
	BOARD_InitPins();
	BOARD_InitBootClocks();
	BOARD_InitDebugConsole();


#elif	CPU_MCXA153VLH
	/* Attach clock to I3C 24MHZ */
	CLOCK_SetClockDiv( kCLOCK_DivI3C0_FCLK, 2U );
	CLOCK_AttachClk( kFRO_HF_DIV_to_I3C0FCLK );

	/* I2C */
	CLOCK_SetClockDiv(kCLOCK_DivLPI2C0, 1u);
	CLOCK_AttachClk(kFRO12M_to_LPI2C0);

	/* SPI */
	CLOCK_SetClockDiv(kCLOCK_DivLPSPI1, 1u);
	CLOCK_AttachClk(kFRO12M_to_LPSPI1);

	CLOCK_EnableClock( kCLOCK_GateGPIO0 );
	CLOCK_EnableClock( kCLOCK_GateGPIO1 );
	CLOCK_EnableClock( kCLOCK_GateGPIO2 );
	CLOCK_EnableClock( kCLOCK_GateGPIO3 );

	RESET_PeripheralReset( kUTICK0_RST_SHIFT_RSTn );
	
	BOARD_InitPins();
	BOARD_InitBootClocks();
	BOARD_InitDebugConsole();

#else
	#error Not supported CPU
	
#endif

	UTICK_Init( UTICK0 );
}

void wait( double delayTime_sec )
{
	SDK_DelayAtLeastUs( (uint32_t)(delayTime_sec * 1000000.0), CLOCK_GetCoreSysClkFreq() );
}

void wait_ms( unsigned int milloseconds )
{
	wait( (double)milloseconds * 1e-3 );
}

void wait_us( unsigned int microseconds )
{
	wait( (double)microseconds * 1e-6 );
}

void panic( const char *s )
{
	PRINTF( "error: %s", s );

	typedef struct			{ int on; int off; }	single_code_t;
	static single_code_t	code[]	= { { 1, 1 }, { 1, 1 }, { 1, 3 }, { 3, 1 },  { 3, 1 }, { 3, 3 }, { 1, 1 }, { 1, 1 }, { 1, 7 } };
	DigitalOut				leds[]	= { DigitalOut( RED ), DigitalOut( GREEN ), DigitalOut( BLUE ) };
	float					duration	= 0.07;
	
	leds[ 0 ]	= 1;
	leds[ 1 ]	= 1;
	leds[ 2 ]	= 1;
	
	while ( true )
	{
		for ( unsigned long i = 0; i < sizeof( code ) / sizeof( single_code_t ); i++ )
		{
			leds[ 0 ]	= 0;
			wait( code[ i ].on  * duration );
			leds[ 0 ]	= 1;
			wait( code[ i ].off * duration );
		}
	}
}
