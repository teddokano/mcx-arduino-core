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
#include "fsl_lpi2c.h"
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
#else
	#error Not supported CPU
#endif


I2C::I2C( int sda, int scl, bool no_hw ) : Obj( true ), _sda( sda ), _scl( scl ), err_cb( nullptr )
{
	if ( no_hw )
		return;
	
#ifdef	CPU_MCXN947VDF
	if ( (sda == I2C_SDA) && (scl == I2C_SCL) )
		;
	else
		panic( "FRDM-MCXN947 only support I2C_SDA(D18)/I2C_SCL(D19) pins for I2C" );
	
	constexpr int	mux_setting	= 2;
	unit_base	= EXAMPLE_I2C_MASTER;
	
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
#else
	#error Not supported CPU
#endif

	LPI2C_MasterGetDefaultConfig( &masterConfig );
	LPI2C_MasterInit( unit_base, &masterConfig, LPI2C_MASTER_CLOCK_FREQUENCY );
	
//	frequency( I2C_FREQ );
	
	_scl.pin_mux( mux_setting );
	_sda.pin_mux( mux_setting );
	
	err_callback( err_handling );
}

I2C::~I2C()
{
	LPI2C_MasterDeinit( unit_base );
}

void I2C::frequency( uint32_t frequency )
{
	LPI2C_MasterSetBaudRate( unit_base, LPI2C_MASTER_CLOCK_FREQUENCY, frequency );
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

		if ( LPI2C_MasterGetStatusFlags( unit_base ) & kLPI2C_MasterNackDetectFlag )
		{
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

		reVal = LPI2C_MasterStop( unit_base );
		if ( reVal != kStatus_Success )
		{
			return reVal;
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

		reVal = LPI2C_MasterStop( unit_base );
		if ( reVal != kStatus_Success )
		{
			return reVal;
		}
	}
	return reVal;
}

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
	if ( kStatus_LPI2C_Nak == error )
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
	for ( uint8_t i = 0; i <= last; i++ )
		result[i]	= ping( i );
}

void I2C::scan( uint8_t start, uint8_t last )
{
	bool	result[ 128 ];

	scan( start, last, result );

	printf( "\r\nI2C scan result (in range of 0x00 ~ 0x%02X)\r\n   ", last );
	for ( uint8_t x = 0; x < 16; x++ )
		printf( " x%01X", x );
	
	for ( uint8_t i = 0; i < last; i++ )
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
