/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

extern "C" {
#include "fsl_device_registers.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_debug_console.h"

#ifdef	CPU_MCXC444VLH
#include "fsl_spi.h"
#else
#include "fsl_lpspi.h"
#endif
}

#include	"io.h"
#include	"spi.h"
#include	"mcu.h"

#ifdef	CPU_MCXC444VLH

#define EXAMPLE_SPI_MASTER              SPI1
#define EXAMPLE_SPI_MASTER_SOURCE_CLOCK kCLOCK_BusClk
#define EXAMPLE_SPI_MASTER_CLK_FREQ     CLOCK_GetFreq( kCLOCK_BusClk )

SPI::SPI( int mosi, int miso, int sclk, int cs ) : Obj( true ), chip_select( cs )
{
	unit_base			= EXAMPLE_SPI_MASTER;
	master_clk_freq		= EXAMPLE_SPI_MASTER_CLK_FREQ;

	SPI_MasterGetDefaultConfig( &masterConfig );
	SPI_MasterInit( unit_base, &masterConfig, master_clk_freq );

	frequency( SPI_FREQ );
	mode( 0 );

	//	pin enable
	
//	DigitalInOut	_cs(   cs   );
	DigitalInOut	_mosi( mosi );
	DigitalInOut	_miso( miso );
	DigitalInOut	_sclk( sclk );

	constexpr uint8_t	mux_setting	= 2;

	_mosi.pin_mux( mux_setting );
	_sclk.pin_mux( mux_setting );
	_miso.pin_mux( mux_setting );
//	_cs.pin_mux(   mux_setting );

	chip_select	= true;
}

SPI::~SPI()
{
	SPI_Deinit( unit_base );
}

void SPI::frequency( uint32_t frequency )
{
	masterConfig.baudRate_Bps = frequency / 2;	//	This may be a problem of SDK v25.12

//	SPI_Deinit( unit_base );
	SPI_MasterInit( unit_base, &masterConfig, master_clk_freq );
}

void SPI::mode( uint8_t mode )
{
	masterConfig.polarity	= (spi_clock_polarity_t)((mode >> 1) & 0x1);
	masterConfig.phase		= (spi_clock_phase_t   )((mode >> 0) & 0x1);

//	SPI_Deinit( unit_base );
	SPI_MasterInit( unit_base, &masterConfig, master_clk_freq );
}

status_t SPI::write( uint8_t *wp, uint8_t *rp, int length )
{
	spi_transfer_t	masterXfer;
	status_t		status;

	masterXfer.txData		= wp;
	masterXfer.rxData		= rp;
	masterXfer.dataSize		= length;

	chip_select	= false;
	status	= SPI_MasterTransferBlocking( unit_base, &masterXfer );
	chip_select	= true;

	return status;
}


#else	//	CPU_MCXC444VLH

#define TRANSFER_SIZE     64U     /*! Transfer dataSize */
#define TRANSFER_BAUDRATE 500000U /*! Transfer baudrate - 500k */

#ifdef	CPU_MCXN947VDF
	#define EXAMPLE_LPSPI_MASTER_BASEADDR (LPSPI1)
	#define EXAMPLE_LPSPI_MASTER_PCS_FOR_INIT     (kLPSPI_Pcs0)
	#define EXAMPLE_LPSPI_MASTER_PCS_FOR_TRANSFER (kLPSPI_MasterPcs0)
	#define LPSPI_MASTER_CLK_FREQ CLOCK_GetLPFlexCommClkFreq(1u)
#elif	CPU_MCXN236VDF
	#define EXAMPLE_LPSPI_MASTER_BASEADDR (LPSPI3)
	#define EXAMPLE_LPSPI_MASTER_PCS_FOR_INIT     (kLPSPI_Pcs0)
	#define EXAMPLE_LPSPI_MASTER_PCS_FOR_TRANSFER (kLPSPI_MasterPcs0)
	#define LPSPI_MASTER_CLK_FREQ CLOCK_GetLPFlexCommClkFreq(3u)
#elif	CPU_MCXA156VLL
	#define EXAMPLE_LPSPI_DEALY_COUNT				0xFFFFF

	#define EXAMPLE_LPSPI_MASTER_BASEADDR0			(LPSPI0)
	#define EXAMPLE_LPSPI_MASTER_IRQN0				(LPSPI0_IRQn)
	#define LPSPI_MASTER_CLK_FREQ0					(CLOCK_GetLpspiClkFreq(0))
	#define EXAMPLE_LPSPI_MASTER_PCS_FOR_INIT0		(kLPSPI_Pcs0)
	#define EXAMPLE_LPSPI_MASTER_PCS_FOR_TRANSFER0	(kLPSPI_MasterPcs0)
	#define EXAMPLE_LPSPI_MASTER_IRQHandler			(LPSPI0_IRQHandler)

	#define EXAMPLE_LPSPI_MASTER_BASEADDR1			(LPSPI1)
	#define EXAMPLE_LPSPI_MASTER_IRQN1				(LPSPI1_IRQn)
	#define LPSPI_MASTER_CLK_FREQ1					(CLOCK_GetLpspiClkFreq(1))
	#define EXAMPLE_LPSPI_MASTER_PCS_FOR_INIT1		(kLPSPI_Pcs1)
	#define EXAMPLE_LPSPI_MASTER_PCS_FOR_TRANSFER1	(kLPSPI_MasterPcs1)
	#define EXAMPLE_LPSPI_MASTER_IRQHandler1		(LPSPI1_IRQHandler)
#elif	CPU_MCXA153VLH
	#define EXAMPLE_LPSPI_MASTER_BASEADDR         (LPSPI1)
	#define EXAMPLE_LPSPI_MASTER_IRQN             (LPSPI1_IRQn)
	#define EXAMPLE_LPSPI_DEALY_COUNT             0xFFFFF
	#define LPSPI_MASTER_CLK_FREQ                 (CLOCK_GetLpspiClkFreq(1))
	#define EXAMPLE_LPSPI_MASTER_PCS_FOR_INIT     (kLPSPI_Pcs1)
	#define EXAMPLE_LPSPI_MASTER_PCS_FOR_TRANSFER (kLPSPI_MasterPcs1)
	#define EXAMPLE_LPSPI_MASTER_IRQHandler       (LPSPI1_IRQHandler)
#else
	#error Not supported CPU
#endif

SPI::SPI( int mosi, int miso, int sclk, int cs ) : Obj( true )
{
#ifdef	CPU_MCXN947VDF
#elif	CPU_MCXN236VDF
#elif	CPU_MCXA156VLL
#elif	CPU_MCXA153VLH
	RESET_ReleasePeripheralReset( kLPSPI1_RST_SHIFT_RSTn );
#else
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"	// for master_pcs_for_init
	lpspi_which_pcs_t	master_pcs_for_init;

#ifdef	CPU_MCXA156VLL
	if ( (mosi == MB_MOSI) && (miso == MB_MISO) && (sclk == MB_SCK) && (cs == MB_CS) )
	{
		unit_base			= EXAMPLE_LPSPI_MASTER_BASEADDR0;
		master_clk_freq		= LPSPI_MASTER_CLK_FREQ0;	
		master_pcs_for_init	= EXAMPLE_LPSPI_MASTER_PCS_FOR_INIT0;
		master_pcs_4_xfer	= EXAMPLE_LPSPI_MASTER_PCS_FOR_TRANSFER0;
	}
	else if ( (mosi == ARD_MOSI) && (miso == ARD_MISO) && (sclk == ARD_SCK) && (cs == ARD_CS) )
	{
		unit_base			= EXAMPLE_LPSPI_MASTER_BASEADDR1;
		master_clk_freq		= LPSPI_MASTER_CLK_FREQ1;	
		master_pcs_for_init	= EXAMPLE_LPSPI_MASTER_PCS_FOR_INIT1;
		master_pcs_4_xfer	= EXAMPLE_LPSPI_MASTER_PCS_FOR_TRANSFER1;
	}
	else
	{
		panic( "FRDM-MCXA156 SPI on Arduino pin and MikroBus are supported. To use Arduino pins, change jumper setting (short 2-3 pins on R59 and R60) and use \"ARD_MOSI\" and \"ARD_CS\" keywords instead of D10 and D11." );
	}
#else
	unit_base			= EXAMPLE_LPSPI_MASTER_BASEADDR;
	master_clk_freq		= LPSPI_MASTER_CLK_FREQ;
	master_pcs_for_init	= EXAMPLE_LPSPI_MASTER_PCS_FOR_INIT;
	master_pcs_4_xfer	= EXAMPLE_LPSPI_MASTER_PCS_FOR_TRANSFER;
#endif
	
	LPSPI_MasterGetDefaultConfig( &masterConfig );
	
	masterConfig.whichPcs						= master_pcs_for_init;
	masterConfig.pcsToSckDelayInNanoSec			= 1000000000U / (masterConfig.baudRate * 2U);
	masterConfig.lastSckToPcsDelayInNanoSec		= 1000000000U / (masterConfig.baudRate * 2U);
	masterConfig.betweenTransferDelayInNanoSec	= 1000000000U / (masterConfig.baudRate * 2U);

	LPSPI_MasterInit( unit_base, &masterConfig, master_clk_freq );

	frequency( SPI_FREQ );
	mode( 0 );

	//	pin enable
	
	DigitalInOut	_cs(   cs   );
	DigitalInOut	_mosi( mosi );
	DigitalInOut	_miso( miso );
	DigitalInOut	_sclk( sclk );

	constexpr uint8_t	mux_setting	= 2;

	_mosi.pin_mux( mux_setting );
	_sclk.pin_mux( mux_setting );
	_miso.pin_mux( mux_setting );
	_cs.pin_mux(   mux_setting );

#pragma GCC diagnostic pop
}

SPI::~SPI()
{
	LPSPI_Deinit( unit_base );
}

void SPI::frequency( uint32_t frequency )
{
	masterConfig.baudRate = frequency;

	masterConfig.pcsToSckDelayInNanoSec        = 1000000000U / (masterConfig.baudRate * 2U);
	masterConfig.lastSckToPcsDelayInNanoSec    = 1000000000U / (masterConfig.baudRate * 2U);
	masterConfig.betweenTransferDelayInNanoSec = 1000000000U / (masterConfig.baudRate * 2U);

	LPSPI_Deinit( unit_base );
	LPSPI_MasterInit( unit_base, &masterConfig, master_clk_freq );
}

void SPI::mode( uint8_t mode )
{
	masterConfig.cpol	= (lpspi_clock_polarity_t)((mode >> 1) & 0x1);
	masterConfig.cpha	= (lpspi_clock_phase_t   )((mode >> 0) & 0x1);

	LPSPI_Deinit( unit_base );
	LPSPI_MasterInit( unit_base, &masterConfig, master_clk_freq );	
}

status_t SPI::write( uint8_t *wp, uint8_t *rp, int length )
{
	lpspi_transfer_t	masterXfer;

	masterXfer.txData		= wp;
	masterXfer.rxData		= rp;
	masterXfer.dataSize		= length;
	masterXfer.configFlags	= master_pcs_4_xfer | kLPSPI_MasterPcsContinuous | kLPSPI_MasterByteSwap;

	return LPSPI_MasterTransferBlocking( unit_base, &masterXfer );
}

#endif // CPU_MCXC444VLH
