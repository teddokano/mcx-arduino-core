/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include "r01lib.h"

extern "C" {
#include	<string.h>
#include	"fsl_i3c.h"
}

#include	"i3c.h"

#define	IBI_PAYLOAD_BUFFER_SIZE		10

#ifdef	CPU_MCXN947VDF
	#define EXAMPLE_MASTER            	I3C1
	#define I3C_MASTER_CLOCK_FREQUENCY	CLOCK_GetI3cClkFreq(1)
#elif	CPU_MCXN236VDF
	#define EXAMPLE_MASTER            	I3C1
	#define I3C_MASTER_CLOCK_FREQUENCY	CLOCK_GetI3cClkFreq(1)
#elif	CPU_MCXA156VLL
	#define EXAMPLE_MASTER				I3C0
	#define I3C_MASTER_CLOCK_FREQUENCY	CLOCK_GetI3CFClkFreq()
#elif	CPU_MCXA153VLH
	#define EXAMPLE_MASTER				I3C0
	#define I3C_MASTER_CLOCK_FREQUENCY	CLOCK_GetI3CFClkFreq()
#else
	#error Target CPU is not supported
#endif

uint8_t					g_ibiBuff[ IBI_PAYLOAD_BUFFER_SIZE ];
static uint8_t			g_ibiUserBuff[ IBI_PAYLOAD_BUFFER_SIZE ];
static uint8_t			g_ibiUserBuffUsed	= 0;
static volatile bool	g_ibiWonFlag		= false;
static uint8_t 			g_ibiAddress;

i3c_master_handle_t		g_i3c_m_handle;
volatile bool			g_masterCompletionFlag;
volatile status_t		g_completionStatus;

i3c_func_ptr			g_ibi_callback	= NULL;

//I3C::I3C( int sda, int scl )
I3C::I3C( int sda, int scl, uint32_t i2c_freq, uint32_t i3c_od_freq, uint32_t i3c_pp_freq )
	: I2C( sda, scl, true )
{
#ifdef	CPU_MCXN947VDF
	if ( (sda == I3C_SDA) && (scl == I3C_SCL) )
		;
	else
		panic( "FRDM-MCXN947 only support I3C_SDA/I3C_SCL pins for I3C" );
#elif	CPU_MCXN236VDF
	if ( (sda == I3C_SDA) && (scl == I3C_SCL) )
		;
	else
		panic( "FRDM-MCXN236 only support I3C_SDA/I3C_SCL pins for I3C" );
#elif	CPU_MCXA156VLL
	if ( (sda == I3C_SDA) && (scl == I3C_SCL) )
		;
	else if ( (sda == I2C_SDA) && (scl == I2C_SCL) )
		;
	else
		panic( "FRDM-MCXA153 supports I3C_SDA/I3C_SCL or I2C_SDA(D18)/I2C_SCL(D19) pins for I3C" );
#elif 	CPU_MCXA153VLH
	if ( (sda == I3C_SDA) && (scl == I3C_SCL) )
		;
	else if ( (sda == I2C_SDA) && (scl == I2C_SCL) )
		;
	else
		panic( "FRDM-MCXA153 supports I3C_SDA/I3C_SCL or I2C_SDA(D18)/I2C_SCL(D19) pins for I3C" );
#else
	#error Target CPU is not supported
#endif // CPU_MCXN947VDF
	
	I3C_MasterGetDefaultConfig( &masterConfig );

	masterConfig.baudRate_Hz.i2cBaud          = i2c_freq    ? i2c_freq    : I2C::FREQ;
	masterConfig.baudRate_Hz.i3cOpenDrainBaud = i3c_od_freq ? i3c_od_freq : OD_FREQ;
	masterConfig.baudRate_Hz.i3cPushPullBaud  = i3c_pp_freq ? i3c_pp_freq : PP_FREQ;
	masterConfig.enableOpenDrainStop          = false;
	masterConfig.disableTimeout               = true;
	
	bus_type	= kI3C_TypeI3CSdr;
	
	I3C_MasterInit( EXAMPLE_MASTER, &masterConfig, I3C_MASTER_CLOCK_FREQUENCY );

	/* Create I3C handle. */
	I3C_MasterTransferCreateHandle( EXAMPLE_MASTER, &g_i3c_m_handle, &masterCallback, NULL );

	first_broadcast	= true;
	
	DigitalInOut	_scl( sda );
	DigitalInOut	_sda( scl );
	
	_scl.pin_mux( kPORT_MuxAlt10 );
	_sda.pin_mux( kPORT_MuxAlt10 );
}

I3C::~I3C(){
	I3C_MasterDeinit( EXAMPLE_MASTER );
}

void I3C::frequency( uint32_t i2c_freq, uint32_t i3c_od_freq, uint32_t i3c_pp_freq )
{
	i3c_baudrate_hz_t	baudRate_Hz;
	
	baudRate_Hz.i2cBaud				= i2c_freq    ? i2c_freq    : masterConfig.baudRate_Hz.i2cBaud;
	baudRate_Hz.i3cOpenDrainBaud	= i3c_od_freq ? i3c_od_freq : masterConfig.baudRate_Hz.i3cOpenDrainBaud;
	baudRate_Hz.i3cPushPullBaud  	= i3c_pp_freq ? i3c_pp_freq : masterConfig.baudRate_Hz.i3cPushPullBaud;

	I3C_MasterSetBaudRate( EXAMPLE_MASTER, &baudRate_Hz, I3C_MASTER_CLOCK_FREQUENCY );
}

void I3C::frequency( void )
{
	I3C_MasterSetBaudRate( EXAMPLE_MASTER, &(masterConfig.baudRate_Hz), I3C_MASTER_CLOCK_FREQUENCY );
}

void I3C::mode( MODE mode )
{
	bus_type	= (i3c_bus_type_t)mode;
}

status_t I3C::write( uint8_t targ, const uint8_t *dp, int length, bool stop )
{
	return xfer( kI3C_Write, bus_type, targ, (uint8_t *)dp, length, stop );
}

status_t I3C::read( uint8_t targ, uint8_t *dp, int length, bool stop )
{
	return xfer( kI3C_Read, bus_type, targ, dp, length, stop );
}

#ifdef	CUSTOM_REGISTAR_XFER
status_t I3C::reg_write( uint8_t targ, uint8_t reg, const uint8_t *dp, int length, bool stop )
{
	return reg_xfer( kI3C_Write, bus_type, targ, reg, 1, (uint8_t *)dp, length );
}

status_t I3C::reg_read( uint8_t targ, uint8_t reg, uint8_t *dp, int length, bool stop )
{
	return reg_xfer( kI3C_Read, bus_type, targ, reg, 1, dp, length );
}

status_t I3C::reg_xfer( i3c_direction_t dir, i3c_bus_type_t type, uint8_t targ, uint8_t reg, uint8_t reg_length, uint8_t *dp, int length, bool stop )
{
	i3c_master_transfer_t masterXfer = {0};
	
	masterXfer.slaveAddress		= targ;
	masterXfer.subaddress   	= reg;
	masterXfer.subaddressSize	= reg_length;
	masterXfer.data        		= dp;
	masterXfer.dataSize			= length;
	masterXfer.direction		= dir;
	masterXfer.busType			= type;
	masterXfer.flags			= stop ? kI3C_TransferDefaultFlag : kI3C_TransferNoStopFlag;
	
	return I3C_MasterTransferBlocking( EXAMPLE_MASTER, &masterXfer );
}

status_t I3C::xfer( i3c_direction_t dir, i3c_bus_type_t type, uint8_t targ, uint8_t *dp, int length, bool stop )
{
	return reg_xfer( dir, bus_type, targ, 0, 0, dp, length, stop );
}
#else
status_t I3C::xfer( i3c_direction_t dir, i3c_bus_type_t type, uint8_t targ, uint8_t *dp, int length, bool stop )
{
	i3c_master_transfer_t masterXfer = {0};
	
	masterXfer.slaveAddress = targ;
	masterXfer.data         = dp;
	masterXfer.dataSize     = length;
	masterXfer.direction    = dir;
	masterXfer.busType      = type;
	masterXfer.flags        = stop ? kI3C_TransferDefaultFlag : kI3C_TransferNoStopFlag;
	
	return I3C_MasterTransferBlocking( EXAMPLE_MASTER, &masterXfer );
}
#endif	// CUSTOM_REGISTAR_XFER

void I3C::set_IBI_callback( i3c_func_ptr fp )
{
	g_ibi_callback	= fp;
}


status_t I3C::ccc_broadcast( uint8_t ccc, const uint8_t *dp, uint8_t length, bool first_time )
{
	uint8_t		bp[ REG_RW_BUFFER_SIZE ];
	status_t	r_code;
	
	bp[ 0 ]	= ccc;
	memcpy( (uint8_t *)bp + 1, (uint8_t *)dp, length );
	
	if ( first_time || first_broadcast )
	{
		first_broadcast	= false;
		
		frequency( 0, 2000000, 2000000 );	//	I2C_freq = default, I3C_OD_freq = 2MHz, I3C_PP_freq = 2MHz
		r_code	= write( BROADCAST_ADDR, bp, length + 1 );
		frequency();	//	revert to default frequency
	}
	else
	{
		r_code	= write( BROADCAST_ADDR, bp, length + 1 );
	}
	return r_code;
}

status_t I3C::ccc_set( uint8_t ccc, uint8_t addr, uint8_t data )
{
	status_t r	= write( BROADCAST_ADDR, &ccc, 1, NO_STOP );

	if ( kStatus_Success != r )
		return r;
	
	return write( addr, &data, 1 );
}

status_t I3C::ccc_get( uint8_t ccc, uint8_t addr, uint8_t *dp, uint8_t length )
{
	status_t r	= write( BROADCAST_ADDR, &ccc, 1, NO_STOP );

	if ( kStatus_Success != r )
		return r;
	
	return read( addr, dp, length );
}

uint8_t I3C::check_IBI( void )
{
	if ( !g_ibiWonFlag )
		return 0;

	g_ibiWonFlag	= false;
	
	return g_ibiAddress;
}

void I3C::master_ibi_callback( I3C_Type *base, i3c_master_handle_t *handle, i3c_ibi_type_t ibiType, i3c_ibi_state_t ibiState )
{
	g_ibiWonFlag	= true;
	g_ibiAddress	= handle->ibiAddress;
	
	switch ( ibiType )
	{
		case kI3C_IbiNormal:
			if ( ibiState == kI3C_IbiDataBuffNeed )
			{
				handle->ibiBuff = g_ibiBuff;
			}
			else
			{
				memcpy( g_ibiUserBuff, (void *)handle->ibiBuff, handle->ibiPayloadSize );
				g_ibiUserBuffUsed = handle->ibiPayloadSize;
			}
			break;

		default:
			assert(false);
			break;
	}
	
	if ( g_ibi_callback )
		g_ibi_callback();
}

void I3C::master_callback( I3C_Type *base, i3c_master_handle_t *handle, status_t status, void *userData )
{
	if (status == kStatus_Success)
		g_masterCompletionFlag = true;

	g_completionStatus = status;
}

const i3c_master_transfer_callback_t	I3C::masterCallback = {
	.slave2Master		= NULL, 
	.ibiCallback		= master_ibi_callback,
	.transferComplete	= master_callback
};


int I3C::DAA( const uint8_t *address_list, uint8_t count, i3c_device_info_t** device_list )
{
	I3C_MasterProcessDAA( EXAMPLE_MASTER, (uint8_t *)address_list, count );

	uint8_t	devCount;
	*device_list = I3C_MasterGetDeviceListAfterDAA( EXAMPLE_MASTER, &devCount );

	return devCount;
}

