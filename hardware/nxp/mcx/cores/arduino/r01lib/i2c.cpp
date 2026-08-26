/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

extern "C" {
#include <stdio.h>
#include <string.h>
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_debug_console.h"

#ifdef	CPU_MCXC444VLH
#include "fsl_i2c.h"
#else
#include "fsl_lpi2c.h"
#endif

#include "fsl_port.h"
}

#include	"i2c.h"
#include	"mcu.h"

#ifdef	CPU_MCXN947VDF
	#define EXAMPLE_I2C_MASTER_BASE			(LPI2C2_BASE)
	#define LPI2C_MASTER_CLOCK_FREQUENCY 	CLOCK_GetLPFlexCommClkFreq( 2u )
	#define EXAMPLE_I2C_MASTER				((LPI2C_Type *)EXAMPLE_I2C_MASTER_BASE)
#elif	CPU_MCXN236VDF
	#define EXAMPLE_I2C_MASTER_BASE			(LPI2C2_BASE)
	#define LPI2C_MASTER_CLOCK_FREQUENCY	CLOCK_GetLPFlexCommClkFreq( 2u )
	#define EXAMPLE_I2C_MASTER				((LPI2C_Type *)EXAMPLE_I2C_MASTER_BASE)
#elif	CPU_MCXA156VLL
	#define LPI2C_MASTER_CLOCK_FREQUENCY	CLOCK_GetLpi2cClkFreq( 0u )
#elif	CPU_MCXA153VLH
	#define EXAMPLE_I2C_MASTER_BASE			LPI2C0
	#define LPI2C_MASTER_CLOCK_FREQUENCY	CLOCK_GetLpi2cClkFreq()
	#define EXAMPLE_I2C_MASTER				((LPI2C_Type *)EXAMPLE_I2C_MASTER_BASE)
#elif	CPU_MCXC444VLH
	#define I2C_MASTER_CLK_SRC				I2C0_CLK_SRC
	#define I2C_MASTER_CLOCK_FREQUENCY      CLOCK_GetFreq(I2C0_CLK_SRC)
	#define EXAMPLE_I2C_MASTER_BASEADDR		I2C1
#else
	#error Not supported CPU
#endif


I2C::I2C( int sda, int scl, bool no_hw ) : Obj( true ), _sda( sda ), _scl( scl ), err_cb( nullptr ), _no_hw( no_hw )
{
	if ( no_hw )
		return;
	
#ifdef	CPU_MCXN947VDF
	int	mux_setting;

	if ( (sda == I2C_SDA) && (scl == I2C_SCL) )
	{
		mux_setting	= 2;
		unit_base	= EXAMPLE_I2C_MASTER;
	}
	else if ( (sda == MB_SDA) && (scl == MB_SCL) )
	{
		// MikroBus header (P1_0/P1_1) -> FlexComm3 -> LPI2C3, Alt2 (confirmed
		// against Zephyr's silicon-accurate pinctrl header, not derived by
		// pin_mux.c position-counting -- see PwmOut.h's ALT-derivation
		// postmortem for why that method alone isn't trusted anymore).
		mux_setting	= 2;
		unit_base	= LPI2C3;
		RESET_ReleasePeripheralReset( kFC3_RST_SHIFT_RSTn );
	}
	else
		panic( "FRDM-MCXN947 supports I2C_SDA(D18)/I2C_SCL(D19) or MB_SDA/MB_SCL pins for I2C" );

#elif	CPU_MCXN236VDF
	if ( (sda == A4) && (scl == A5) )
		;
	else if ( (sda == MB_SDA) && (scl == MB_SCL) )
		;
	else
		panic( "FRDM-MCXN236 only support I2C_SDA(D18)/I2C_SCL(D19) pins for I2C" );
	
	constexpr int	mux_setting	= 2;
	unit_base	= EXAMPLE_I2C_MASTER;
	
#elif	CPU_MCXA156VLL
	int	mux_setting	= kPORT_MuxAlt2;

	if ( (sda == I3C_SDA) && (scl == I3C_SCL) )
	{
		mux_setting	= kPORT_MuxAlt2;
		unit_base	= LPI2C0;
		RESET_ReleasePeripheralReset( kLPI2C0_RST_SHIFT_RSTn );
	}
	else if ( (sda == I2C_SDA) && (scl == I2C_SCL) )
	{
		mux_setting	= kPORT_MuxAlt2;
		unit_base	= LPI2C0;
		RESET_ReleasePeripheralReset( kLPI2C0_RST_SHIFT_RSTn );
	}
	else if ( (sda == MB_SDA) && (scl == MB_SCL) )
	{
		mux_setting	= kPORT_MuxAlt2;
		unit_base	= LPI2C3;
		RESET_ReleasePeripheralReset( kLPI2C3_RST_SHIFT_RSTn );
	}
	else if ( (sda == MB_MOSI) && (scl == MB_SCK) )
	{
		mux_setting	= kPORT_MuxAlt3;
		unit_base	= LPI2C1;
		RESET_ReleasePeripheralReset( kLPI2C1_RST_SHIFT_RSTn );
	}
	else if ( (sda == A4) && (scl == A5) )
	{
		mux_setting	= kPORT_MuxAlt2;
		unit_base	= LPI2C1;
		RESET_ReleasePeripheralReset( kLPI2C1_RST_SHIFT_RSTn );
	}
	else
		panic( "FRDM-MCXA156 supports I3C_SDA/I3C_SCL, I2C_SDA(D18)/I2C_SCL(D19), MB_SDA/MB_SCL or MB_MOSI/MB_SCK pins for I2C" );

#elif	CPU_MCXA153VLH
	if ( (sda == I3C_SDA) && (scl == I3C_SCL) )
		;
	else if ( (sda == I2C_SDA) && (scl == I2C_SCL) )
		;
	else if ( (sda == MB_SDA) && (scl == MB_SCL) )
		;
	else if ( (sda == MB_MOSI) && (scl == MB_SCK) )
		;
	else
		panic( "FRDM-MCXA153 supports I3C_SDA/I3C_SCL, I2C_SDA(D18)/I2C_SCL(D19), MB_SDA/MB_SCL or MB_MOSI/MB_SCK pins for I2C" );

	constexpr int	mux_setting	= kPORT_MuxAlt3;
	unit_base	= EXAMPLE_I2C_MASTER;
	
	RESET_ReleasePeripheralReset( kLPI2C0_RST_SHIFT_RSTn );
	
#elif	CPU_MCXC444VLH
	int	mux_setting	= kPORT_MuxAlt2;

	if ( (sda == D18) && (scl == D19) )
		mux_setting	= kPORT_MuxAlt6;
	else if ( (sda == A4) && (scl == A5) )
		mux_setting	= kPORT_MuxAlt2;
	else
		panic( "FRDM-MCXA153 supports I3C_SDA/I3C_SCL, I2C_SDA(D18)/I2C_SCL(D19), MB_SDA/MB_SCL or MB_MOSI/MB_SCK pins for I2C" );

	unit_base	= I2C1;
	repeated_start_required_flag	= false;
#else
	#error Not supported CPU
#endif


#if	CPU_MCXC444VLH
	I2C_MasterGetDefaultConfig( &masterConfig );
	I2C_MasterInit( unit_base, &masterConfig, I2C_MASTER_CLOCK_FREQUENCY );
#else
	LPI2C_MasterGetDefaultConfig( &masterConfig );
	LPI2C_MasterInit( unit_base, &masterConfig, LPI2C_MASTER_CLOCK_FREQUENCY );
#endif
	
//	frequency( I2C_FREQ );
	
	_scl.pin_mux( mux_setting );
	_sda.pin_mux( mux_setting );
	
	err_callback( err_handling );
}

I2C::~I2C()
{
	/*
	 *  no_hw=true (the I3C constructor's delegation path) skips hardware
	 *  init entirely, leaving unit_base uninitialized -- deinit-ing it here
	 *  would dereference garbage. I3C::~I3C() already deinits its own
	 *  peripheral (I3C_MasterDeinit), so there's nothing for this base
	 *  destructor to clean up in that case.
	 */
	if ( _no_hw )
		return;

#if	CPU_MCXC444VLH
	I2C_MasterDeinit( unit_base );
#else
	LPI2C_MasterDeinit( unit_base );
#endif
}

void I2C::frequency( uint32_t frequency )
{
#if	CPU_MCXC444VLH
	I2C_MasterSetBaudRate( unit_base, I2C_MASTER_CLOCK_FREQUENCY, frequency );
#else
	/*	Clear the glitch filters before handing over to the SDK.
	 *
	 *	LPI2C_MasterSetBaudRate() only ever raises FILTSDA, never lowers it:
	 *	the block that writes it is guarded by
	 *	  (sourceClock / baudRate / 20) > (divider + 2)
	 *	which is false for fast baud rates, so switching *down* in period
	 *	(i.e. up in speed) skips the write entirely and leaves whatever a
	 *	previous, slower setting left behind.
	 *
	 *	Worked example on this hardware (12MHz source), all three values
	 *	confirmed by reading MCFGR2 back on a real board:
	 *	  begin() @100kHz -> 12e6/100e3/20 = 6  > 3  -> FILTSDA = 6-1-2  = 3
	 *	  setClock(10kHz) -> 12e6/10e3/20  = 60 > 18 -> FILTSDA = 60-16-2 = 42,
	 *	                     truncated to the 4-bit field as 42 & 0xF = 10
	 *	  setClock(400kHz)-> 12e6/400e3/20 = 1  > 3? no -> skipped, stays 10
	 *
	 *	FILTSDA 10 at 12MHz is ~833ns of SDA filtering, against a 2.5us bit
	 *	time at 400kHz. On a logic analyzer that showed up as the address
	 *	phase being cut short after three SCL pulses and restarted with a
	 *	repeated START, over and over -- and, before the STOP handling above
	 *	was fixed, as an outright hang. It only ever reproduced when a fast
	 *	rate followed a slow one, which is exactly what this guard predicts.
	 *
	 *	Zeroing both filter fields first makes each call recompute from a
	 *	clean slate: the SDK then writes the right value when it wants one,
	 *	and leaving it at 0 when it skips the block is the intended "no
	 *	extra filtering needed at this speed" result rather than stale state.
	 *
	 *	MCFGR2 is only writable with the module disabled -- the SDK writes it
	 *	inside its own disable/restore window for the same reason -- so do
	 *	the same here, and hand the module back in the state it was found in
	 *	so LPI2C_MasterSetBaudRate()'s own save/restore still sees the truth.
	 */
	bool	was_enabled	= ( unit_base->MCR & LPI2C_MCR_MEN_MASK ) != 0U;

	LPI2C_MasterEnable( unit_base, false );
	unit_base->MCFGR2	&= ~( LPI2C_MCFGR2_FILTSDA_MASK | LPI2C_MCFGR2_FILTSCL_MASK );
	LPI2C_MasterEnable( unit_base, was_enabled );

	LPI2C_MasterSetBaudRate( unit_base, LPI2C_MASTER_CLOCK_FREQUENCY, frequency );
#endif
}

void I2C::pullup( bool enable )
{
	int	flag	= enable ? DigitalInOut::PullUp : DigitalInOut::PullNone;
	
	_scl.mode( flag );
	_sda.mode( flag );
}

status_t I2C::write( uint8_t address, const uint8_t *dp, int length, bool stop )
{
	status_t	r;
	
	if ( (r = write_core( address, dp, length, stop )) )
		if ( err_cb )
			err_cb( r, address );
	
	return r;
}

status_t I2C::read( uint8_t address, uint8_t *dp, int length, bool stop )
{
	status_t	r;
	
	if ( (r = read_core( address, dp, length, stop )) )
		if ( err_cb )
			err_cb( r, address );

	return r;
}

#if	CPU_MCXC444VLH
status_t I2C::write_core( uint8_t address, const uint8_t *dp, int length, bool stop )
{
	i2c_master_transfer_t	masterXfer;
	
	memset( &masterXfer, 0, sizeof( masterXfer ) );

	masterXfer.slaveAddress   = address;
	masterXfer.direction      = kI2C_Write;
	masterXfer.subaddress     = 0;
	masterXfer.subaddressSize = 0;
	masterXfer.data           = const_cast<uint8_t *>( dp );
	masterXfer.dataSize       = length;
	masterXfer.flags          = kI2C_TransferDefaultFlag;

	masterXfer.flags	|= !stop						? kI2C_TransferNoStopFlag			: 0x0;
	masterXfer.flags	|= repeated_start_required_flag	? kI2C_TransferRepeatedStartFlag	: 0x0;

	repeated_start_required_flag	= !stop;

	return I2C_MasterTransferBlocking( unit_base, &masterXfer );
}

status_t I2C::read_core( uint8_t address, uint8_t *dp, int length, bool stop )
{
	i2c_master_transfer_t	masterXfer;
	
	memset( &masterXfer, 0, sizeof( masterXfer ) );

	masterXfer.slaveAddress   = address;
	masterXfer.direction      = kI2C_Read;
	masterXfer.subaddress     = 0;
	masterXfer.subaddressSize = 0;
	masterXfer.data           = dp;
	masterXfer.dataSize       = length;
	masterXfer.flags          = kI2C_TransferDefaultFlag;

	masterXfer.flags	|= !stop						? kI2C_TransferNoStopFlag			: 0x0;
	masterXfer.flags	|= repeated_start_required_flag	? kI2C_TransferRepeatedStartFlag	: 0x0;

	repeated_start_required_flag	= !stop;

	return I2C_MasterTransferBlocking( unit_base, &masterXfer );
}
#else
/*	Emit a STOP, working around LPI2C_MasterStop()'s own error handling.
 *
 *	LPI2C_MasterStop() opens with LPI2C_MasterWaitForTxReady(), which runs
 *	LPI2C_MasterCheckAndClearError(). When the target NAKed, that check
 *	consumes the NAK and makes STOP return kStatus_LPI2C_Nak *before* it
 *	ever writes the stop command -- so the transaction is never terminated
 *	on the bus.
 *
 *	This is not a rare corner: writing one byte to an absent address, the
 *	NAK is not yet visible at either of the earlier checks in write_core()
 *	(LPI2C_MasterStart() only queues the address, the FIFO-drain loop only
 *	waits for it to leave the FIFO, and LPI2C_MasterSend() can still place
 *	the data byte before the bus has answered), so LPI2C_MasterStop() is
 *	where it consistently surfaces. Measured on real hardware: 1500 of 1500
 *	probe transfers, with a logic analyzer showing each transaction's STOP
 *	missing and only appearing once the *next* transaction started.
 *
 *	CheckAndClearError has cleared the flag by the time it returns, so a
 *	second call goes through and actually emits the STOP. Only retried for
 *	NAK: the other error flags (arbitration lost, pin-low timeout) mean the
 *	bus itself is in trouble, and LPI2C_MasterStop() has no timeout of its
 *	own to fall back on (I2C_RETRY_TIMES is 0), so retrying those could
 *	block forever. The caller still gets the original status.
 */
static status_t stop_with_nak_recovery( LPI2C_Type *unit_base )
{
	status_t	r	= LPI2C_MasterStop( unit_base );

	if ( r == kStatus_LPI2C_Nak )
		LPI2C_MasterStop( unit_base );

	return r;
}

status_t I2C::write_core( uint8_t address, const uint8_t *dp, int length, bool stop )
{
	status_t reVal        = kStatus_Fail;
	size_t txCount        = 0xFFU;
	
	if ( kStatus_Success == (reVal	= LPI2C_MasterStart( unit_base, address, kLPI2C_Write)) )
	{
		LPI2C_MasterGetFifoCounts( unit_base, NULL, &txCount );
		while ( txCount )
		{
			LPI2C_MasterGetFifoCounts( unit_base, NULL, &txCount );
		}

		uint32_t	status	= LPI2C_MasterGetStatusFlags( unit_base );

		if ( status & kLPI2C_MasterNackDetectFlag )
		{
			//	Address-phase NAK, on the occasions it is already visible
			//	here (on a slow enough SCL it can be). Clear the error first:
			//	leaving the flag set would make the STOP below bail out without
			//	emitting anything -- see stop_with_nak_recovery() above, which
			//	is the same trap reached from the other direction.
			(void)LPI2C_MasterCheckAndClearError( unit_base, status );
			LPI2C_MasterStop( unit_base );

			return kStatus_LPI2C_Nak;
		}

		reVal = LPI2C_MasterSend( unit_base, (uint8_t *)dp, length );
		if (reVal != kStatus_Success)
		{
			if ( reVal == kStatus_LPI2C_Nak )
			{
				LPI2C_MasterStop( unit_base );
			}
			return reVal;
		}

		if ( stop )
		{
			reVal = stop_with_nak_recovery( unit_base );
			if ( reVal != kStatus_Success )
			{
				return reVal;
			}
		}
	}
	return reVal;
}

status_t I2C::read_core( uint8_t address, uint8_t *dp, int length, bool stop )
{
	status_t reVal        = kStatus_Fail;
		
	if ( kStatus_Success == (reVal = LPI2C_MasterRepeatedStart( unit_base, address, kLPI2C_Read )) )
	{
		reVal = LPI2C_MasterReceive( unit_base,  (uint8_t *)dp, length );
		if ( reVal != kStatus_Success )
		{
			if ( reVal == kStatus_LPI2C_Nak )
			{
				LPI2C_MasterStop( unit_base );
			}
			return reVal;
		}

		if ( stop )
		{
			reVal = stop_with_nak_recovery( unit_base );
			if ( reVal != kStatus_Success )
			{
				return reVal;
			}
		}
	}
	return reVal;
}
#endif


status_t I2C::reg_write( uint8_t targ, uint8_t reg, const uint8_t *dp, int length )
{
	uint8_t	bp[ REG_RW_BUFFER_SIZE ];
	
	bp[ 0 ]	= reg;
	memcpy( (uint8_t *)bp + 1, (uint8_t *)dp, length );

	last_status	= write( targ, bp, length + 1 );
	
	return last_status;
}

status_t I2C::reg_write( uint8_t targ, uint8_t reg, uint8_t data )
{
	uint8_t	bp[ 2 ];
	
	bp[ 0 ]	= reg;
	bp[ 1 ]	= data;

	return write( targ, bp, 2 );
}

status_t I2C::reg_read( uint8_t targ, uint8_t reg, uint8_t *dp, int length )
{
	last_status	= write( targ, &reg, sizeof( reg ), NO_STOP );
	
	if ( kStatus_Success != last_status )
		return last_status;
	
	return read( targ, dp, length );
}

uint8_t I2C::reg_read( uint8_t targ, uint8_t reg )
{
	last_status	= write( targ, reg, NO_STOP );
	return read( targ );
}

status_t I2C::write( uint8_t targ, uint8_t data, bool stop )
{
	return write( targ, &data, sizeof( data ), stop );
}

uint8_t I2C::read( uint8_t targ, bool stop )
{
	uint8_t	data;

	last_status	= read( targ, &data, sizeof( data ), stop );

	return data;
}

I2C::err_cb_ptr I2C::err_callback( err_cb_ptr callback )
{
	err_cb_ptr	previous_cb	= err_cb;
	err_cb	= callback;

	return previous_cb;
}

void I2C::err_handling( status_t error, uint8_t address )
{
#if	CPU_MCXC444VLH
#define	NAK_FLAG	kStatus_I2C_Nak
#else
#define	NAK_FLAG	kStatus_LPI2C_Nak
#endif
	if ( NAK_FLAG == error )
		printf( "NACK from target: 0x%02X\r\n", address );
	else
		printf( "error 0x%04lX @transfer on 0x%02X\r\n", error, address );
}

bool I2C::ping( uint8_t addr )
{
	uint8_t	dummy	= 0;
	return !write_core( addr, &dummy, 0 );
}

void I2C::scan( uint8_t start, uint8_t last, bool *result )
{
	// result[] is always indexed by the full address (0..last), even
	// though only [start, last] is actually pinged -- addresses below
	// start are marked "not found" rather than left uninitialized, so
	// scan(start, last)'s display loop below can print the whole table.
	for ( uint8_t i = 0; i <= last; i++ )
		result[i]	= ( i >= start ) ? ping( i ) : false;
}

void I2C::scan( uint8_t start, uint8_t last )
{
	bool	result[ 128 ];

	scan( start, last, result );

	printf( "\r\nI2C scan result (in range of 0x%02X ~ 0x%02X)\r\n   ", start, last );
	for ( uint8_t x = 0; x < 16; x++ )
		printf( " x%01X", x );

	// <= last, not < last -- last is the last address to include (see the
	// scan(uint8_t last) overload's doc comment), so excluding it here
	// silently dropped the requested upper-bound address from the table.
	for ( uint8_t i = 0; i <= last; i++ )
	{
		if ( !( i % 16) )
			printf( "\r\n%01Xx:", i / 16 );

		if ( result[ i ] )
			printf( " %02X", i );
		else
			printf( " --" );
	}
	printf( "\r\n\r\n" );
}

void I2C::scan( uint8_t last )
{
	scan( 0, last );
}

status_t I2C::ccc_set( uint8_t ccc, uint8_t addr, uint8_t data )
{
	return kStatus_Success;
}

status_t I2C::ccc_get( uint8_t ccc, uint8_t addr, uint8_t *dp, uint8_t length )
{
	memset( dp, 0, length );
	return kStatus_Success;
}
