/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
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
#include	"r01lib_spi.h"
#include	"mcu.h"
#include	"pin_registry.h"

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

	// chip_select's own DigitalInOut constructor (initializer list, above)
	// already self-registered it as a generic "GPIO" pin owner. Undo that
	// here -- see the non-C444 constructor's matching comment for why: CS
	// is deliberately left under the sketch's own pinMode()/digitalWrite()
	// control, so this member exists only for internal bookkeeping, not to
	// compete with the sketch for pin ownership.
	pin_registry_forget( &chip_select );

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

	{
		uint8_t	spi_pins[ 3 ]	= { (uint8_t)mosi, (uint8_t)miso, (uint8_t)sclk };
		pin_registry_note( this, "SPI", spi_pins, 3, mux_setting );
	}

	chip_select			= true;
	manual_cs_control	= false;
}

SPI::~SPI()
{
	pin_registry_forget( this );
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

	if ( !manual_cs_control )
		chip_select	= false;
	
	status	= SPI_MasterTransferBlocking( unit_base, &masterXfer );

	if ( !manual_cs_control )
		chip_select	= true;

	return status;
}

uint8_t SPI::transfer_byte( uint8_t out )
{
	uint8_t	in;

	write( &out, &in, 1 );

	return in;
}

DigitalOut* SPI::cs_manual_control( bool flag )
{
	chip_select	= true;

	return &chip_select;
}

#else	//	CPU_MCXC444VLH

#define TRANSFER_SIZE     64U     /*! Transfer dataSize */
#define TRANSFER_BAUDRATE 500000U /*! Transfer baudrate - 500k */

#ifdef	CPU_MCXN947VDF
	#define EXAMPLE_LPSPI_MASTER_BASEADDR (LPSPI1)
	#define EXAMPLE_LPSPI_MASTER_PCS_FOR_INIT     (kLPSPI_Pcs0)
	#define EXAMPLE_LPSPI_MASTER_PCS_FOR_TRANSFER (kLPSPI_MasterPcs0)
	#define LPSPI_MASTER_CLK_FREQ CLOCK_GetLPFlexCommClkFreq(1u)

	// MikroBus header (MB_MOSI/MB_MISO/MB_SCK/MB_CS, P3_20/22/21/23) ->
	// FlexComm6 -> LPSPI6, Alt3 (confirmed against Zephyr's silicon-accurate
	// pinctrl header, uniform across all 4 pins).
	#define EXAMPLE_LPSPI_MB_BASEADDR (LPSPI6)
	#define EXAMPLE_LPSPI_MB_PCS_FOR_INIT     (kLPSPI_Pcs0)
	#define EXAMPLE_LPSPI_MB_PCS_FOR_TRANSFER (kLPSPI_MasterPcs0)
	#define LPSPI_MB_CLK_FREQ CLOCK_GetLPFlexCommClkFreq(6u)
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

	// MikroBus header (MB_MOSI/MB_MISO/MB_SCK/MB_CS, P1_0/P1_2/P1_1/P1_3) ->
	// LPSPI0, Alt2 (confirmed against Zephyr's silicon-accurate pinctrl
	// header; same Alt2 as the Arduino-header SPI pins above, on the
	// unrelated LPSPI1 instance -- this chip only has these two LPSPI
	// peripherals total).
	#define EXAMPLE_LPSPI_MB_BASEADDR         (LPSPI0)
	#define EXAMPLE_LPSPI_MB_PCS_FOR_INIT     (kLPSPI_Pcs0)
	#define EXAMPLE_LPSPI_MB_PCS_FOR_TRANSFER (kLPSPI_MasterPcs0)
	#define LPSPI_MB_CLK_FREQ                 (CLOCK_GetLpspiClkFreq(0))
#else
	#error Not supported CPU
#endif

SPI::SPI( int mosi, int miso, int sclk, int cs ) : Obj( true ), chip_select( cs, 1 )
{
	uint8_t	mux_setting	= 2;   // overridden below for pin-sets needing a different ALT (e.g. N947's MikroBus header)

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
#elif	CPU_MCXN947VDF
	if ( (mosi == MB_MOSI) && (miso == MB_MISO) && (sclk == MB_SCK) && (cs == MB_CS) )
	{
		unit_base			= EXAMPLE_LPSPI_MB_BASEADDR;
		master_clk_freq		= LPSPI_MB_CLK_FREQ;
		master_pcs_for_init	= EXAMPLE_LPSPI_MB_PCS_FOR_INIT;
		master_pcs_4_xfer	= EXAMPLE_LPSPI_MB_PCS_FOR_TRANSFER;
		mux_setting			= 3;
		RESET_ReleasePeripheralReset( kFC6_RST_SHIFT_RSTn );
	}
	else if ( (mosi == ARD_MOSI) && (miso == ARD_MISO) && (sclk == ARD_SCK) && (cs == ARD_CS) )
	{
		unit_base			= EXAMPLE_LPSPI_MASTER_BASEADDR;
		master_clk_freq		= LPSPI_MASTER_CLK_FREQ;
		master_pcs_for_init	= EXAMPLE_LPSPI_MASTER_PCS_FOR_INIT;
		master_pcs_4_xfer	= EXAMPLE_LPSPI_MASTER_PCS_FOR_TRANSFER;
	}
	else
	{
		panic( "FRDM-MCXN947 supports SPI on Arduino pins (D10-D13) or MikroBus (MB_MOSI/MB_MISO/MB_SCK/MB_CS)" );
	}
#elif	CPU_MCXA153VLH
	if ( (mosi == MB_MOSI) && (miso == MB_MISO) && (sclk == MB_SCK) && (cs == MB_CS) )
	{
		unit_base			= EXAMPLE_LPSPI_MB_BASEADDR;
		master_clk_freq		= LPSPI_MB_CLK_FREQ;
		master_pcs_for_init	= EXAMPLE_LPSPI_MB_PCS_FOR_INIT;
		master_pcs_4_xfer	= EXAMPLE_LPSPI_MB_PCS_FOR_TRANSFER;
		RESET_ReleasePeripheralReset( kLPSPI0_RST_SHIFT_RSTn );
	}
	else if ( (mosi == ARD_MOSI) && (miso == ARD_MISO) && (sclk == ARD_SCK) && (cs == ARD_CS) )
	{
		unit_base			= EXAMPLE_LPSPI_MASTER_BASEADDR;
		master_clk_freq		= LPSPI_MASTER_CLK_FREQ;
		master_pcs_for_init	= EXAMPLE_LPSPI_MASTER_PCS_FOR_INIT;
		master_pcs_4_xfer	= EXAMPLE_LPSPI_MASTER_PCS_FOR_TRANSFER;
		RESET_ReleasePeripheralReset( kLPSPI1_RST_SHIFT_RSTn );
	}
	else
	{
		panic( "FRDM-MCXA153 supports SPI on Arduino pins (D10-D13) or MikroBus (MB_MOSI/MB_MISO/MB_SCK/MB_CS)" );
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

	DigitalInOut	_mosi( mosi );
	DigitalInOut	_miso( miso );
	DigitalInOut	_sclk( sclk );

	_mosi.pin_mux( mux_setting );
	_sclk.pin_mux( mux_setting );
	_miso.pin_mux( mux_setting );

	{
		uint8_t	spi_pins[ 3 ]	= { (uint8_t)mosi, (uint8_t)miso, (uint8_t)sclk };
		pin_registry_note( this, "SPI", spi_pins, 3, mux_setting );
	}

	//	cs defaults to manual/GPIO control, never the LPSPI hardware PCS
	//	function: one physical SPI bus is routinely shared by several
	//	devices (e.g. an LCD + SD card), each with its own independently
	//	sketch-managed CS pin via pinMode()/digitalWrite(). If this pin were
	//	hardware-PCS-driven instead, LPSPI would auto-pulse it on *every*
	//	SPI.transfer() call for *any* device sharing the bus -- not just
	//	transfers meant for the device on this pin -- corrupting whichever
	//	device thinks it's currently deselected.
	cs_manual_control( true );
	manual_cs_control	= true;

	// chip_select's own DigitalInOut constructor self-registered it as a
	// generic "GPIO" pin owner, and cs_manual_control() above just did it
	// again via DigitalInOut::pin_mux() (which keeps the registry's
	// wanted-ALT in sync -- see that method). Undo both here, now that
	// nothing else in this constructor still needs to call pin_mux() on
	// chip_select: CS is deliberately left under the sketch's own
	// pinMode()/digitalWrite() control (see the "cs defaults to manual/
	// GPIO control" comment above), so a sketch following the documented
	// `pinMode(SS,...); SPI.begin();` pattern always ends up with two
	// independent DigitalInOut objects on the same physical pin --
	// functionally harmless (both read/write the same GPIO register), but
	// reported as a false-positive CONFLICT by pin-ownership debug
	// tooling. This member exists only for cs_manual_control()'s internal
	// bookkeeping, not to compete with the sketch for pin ownership. Must
	// stay the last statement in this constructor: anything added below
	// that calls chip_select.pin_mux() again would silently re-register it.
	pin_registry_forget( &chip_select );

#pragma GCC diagnostic pop
}

SPI::~SPI()
{
	pin_registry_forget( this );
	LPSPI_Deinit( unit_base );
}

//	frequency()/mode()/bit_order() reconfigure the peripheral in place --
//	disable, poke just the registers that actually need to change, re-enable
//	-- instead of a full LPSPI_Deinit()+LPSPI_MasterInit() (which gates the
//	peripheral clock off/on and rebuilds every register, CFGR1/FIFO
//	watermarks/dummy-data included, from scratch). A sketch that interleaves
//	two devices at different clocks/modes on one bus (e.g. an LCD + an SD
//	card sharing MOSI/MISO/SCK with separate CS pins) calls these on *every*
//	beginTransaction() where settings differ from the last transaction, so
//	the old full-reinit cost was paid on every single alternation -- this is
//	the fix for issue #2.
void SPI::frequency( uint32_t frequency )
{
	masterConfig.baudRate = frequency;

	masterConfig.pcsToSckDelayInNanoSec        = 1000000000U / (masterConfig.baudRate * 2U);
	masterConfig.lastSckToPcsDelayInNanoSec    = 1000000000U / (masterConfig.baudRate * 2U);
	masterConfig.betweenTransferDelayInNanoSec = 1000000000U / (masterConfig.baudRate * 2U);

	uint32_t	tcr_prescale;

	LPSPI_Enable( unit_base, false );

	LPSPI_MasterSetBaudRate( unit_base, masterConfig.baudRate, master_clk_freq, &tcr_prescale );
	unit_base->TCR = (unit_base->TCR & ~LPSPI_TCR_PRESCALE_MASK) | LPSPI_TCR_PRESCALE( tcr_prescale );

	LPSPI_Enable( unit_base, true );

	//	delay times depend on the (now-updated) TCR prescale field, so these
	//	must come after it's written -- matching LPSPI_MasterInit()'s own
	//	ordering, which also sets them after enabling
	LPSPI_MasterSetDelayTimes( unit_base, masterConfig.pcsToSckDelayInNanoSec,        kLPSPI_PcsToSck,        master_clk_freq );
	LPSPI_MasterSetDelayTimes( unit_base, masterConfig.lastSckToPcsDelayInNanoSec,    kLPSPI_LastSckToPcs,    master_clk_freq );
	LPSPI_MasterSetDelayTimes( unit_base, masterConfig.betweenTransferDelayInNanoSec, kLPSPI_BetweenTransfer, master_clk_freq );
}

void SPI::mode( uint8_t mode )
{
	masterConfig.cpol	= (lpspi_clock_polarity_t)((mode >> 1) & 0x1);
	masterConfig.cpha	= (lpspi_clock_phase_t   )((mode >> 0) & 0x1);

	LPSPI_Enable( unit_base, false );
	unit_base->TCR = (unit_base->TCR & ~(LPSPI_TCR_CPOL_MASK | LPSPI_TCR_CPHA_MASK))
	                | LPSPI_TCR_CPOL( masterConfig.cpol ) | LPSPI_TCR_CPHA( masterConfig.cpha );
	LPSPI_Enable( unit_base, true );
}

void SPI::bit_order( uint8_t order )
{
	masterConfig.direction	= order ? kLPSPI_MsbFirst : kLPSPI_LsbFirst;

	LPSPI_Enable( unit_base, false );
	unit_base->TCR = (unit_base->TCR & ~LPSPI_TCR_LSBF_MASK) | LPSPI_TCR_LSBF( masterConfig.direction );
	LPSPI_Enable( unit_base, true );
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

uint8_t SPI::transfer_byte( uint8_t out )
{
	//	Fast path for a single scalar byte, bypassing write()/
	//	LPSPI_MasterTransferBlocking()'s per-call disable+flush-FIFO+
	//	clear-status-flags+re-enable dance -- measured at ~40us of fixed
	//	overhead per call on real hardware, versus ~2us of actual bit-
	//	clocking at 4MHz. That overhead is negligible for the occasional
	//	bulk transfer (e.g. this project's own LCD writePixels() bursts),
	//	but dominates when a caller moves data one byte at a time in a
	//	tight loop -- as the standard Arduino SD library's spiRec()/
	//	spiSend() do (measured as the actual cause of "very slow" SD-backed
	//	rendering, once the interleaved-reinit issue #2 was independently
	//	fixed and made no difference).
	//
	//	TCR's dynamic per-transfer bits (PCS/CONT/CONTC/RXMSK/TXMSK) are
	//	rewritten to a fixed one-shot/non-continuous/TX+RX-both-active
	//	state on every call -- cheap (no FIFO-empty wait needed, since the
	//	peripheral is always left idle between calls here, unlike write()'s
	//	multi-byte bursts) -- then the byte is pushed/popped directly via
	//	TDR/RDR, polling the same TDF/RDF flags the SDK's own blocking
	//	transfer waits on internally. CPOL/CPHA/PRESCALE/LSBF/FRAMESZ are
	//	untouched here -- they're the sticky fields frequency()/mode()/
	//	bit_order() already maintain.
	uint32_t	whichPcs	= (master_pcs_4_xfer & LPSPI_MASTER_PCS_MASK) >> LPSPI_MASTER_PCS_SHIFT;

	unit_base->TCR	= (unit_base->TCR & ~(LPSPI_TCR_CONT_MASK | LPSPI_TCR_CONTC_MASK | LPSPI_TCR_RXMSK_MASK |
	                                      LPSPI_TCR_TXMSK_MASK | LPSPI_TCR_PCS_MASK))
	                | LPSPI_TCR_PCS( whichPcs );

	while ( 0U == ( LPSPI_GetStatusFlags( unit_base ) & (uint32_t)kLPSPI_TxDataRequestFlag ) )
		;
	LPSPI_WriteData( unit_base, out );

	while ( 0U == ( LPSPI_GetStatusFlags( unit_base ) & (uint32_t)kLPSPI_RxDataReadyFlag ) )
		;

	return (uint8_t)LPSPI_ReadData( unit_base );
}

DigitalOut* SPI::cs_manual_control( bool flag )
{
	chip_select.pin_mux( flag ? 0 : 2 );
	chip_select	= true;
	
	return &chip_select;
}


#endif // CPU_MCXC444VLH

