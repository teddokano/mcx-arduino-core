/** NXP Analog Front End class library for MCX
 *
 *  @class   NAFE13388
 *  @author  Tedd OKANO
 *
 *  Copyright: 2023 - 2025 Tedd OKANO
 *  Released under the MIT license
 *
 *  A simple class library for NXP Analog Front End: NAFE13388
 *
 *  Example:
 *  @code
 *  #include	"r01lib.h"
 *  #include	"afe/NAFE13388_UIM.h"
 *  
 *  SPI				spi( D11, D12, D13, D10 );	//	MOSI, MISO, SCLK, CS
 *  NAFE13388_UIM	afe( spi );
 *  
 *  int main( void )
 *  {
 *  	printf( "***** Hello, NAFE13388 UIM board! *****\r\n" );
 *  
 *  	spi.frequency( 1'000'000 );
 *  	spi.mode( 1 );
 *  
 *  	afe.begin();
 *  
 *  	afe.open_logical_channel( 0, 0x1710, 0x00A4, 0xBC00, 0x0000 );
 *  	afe.open_logical_channel( 1, 0x2710, 0x00A4, 0xBC00, 0x0000 );
 *  
 *  	while ( true )
 *  	{
 *  		for ( auto ch = 0; ch < 2; ch++ )
 *  		{
 *  			int32_t	data	= afe.start_and_read( ch );
 *  			printf( "channel %2d : %8ld (%lfuV),", ch, data, afe.raw2uv( ch, data ) );
 *  		}
 *  		printf( "\r\n" );
 *  	}
 *  }
 *  @endcode
 */

#ifndef ARDUINO_AFE_DRIVER_H
#define ARDUINO_AFE_DRIVER_H

#include	<stdint.h>
#include	"r01lib.h"
#include	"SPI_for_AFE.h"
#include	<cmath>
#include	<vector>
#include	<variant>
#include	<algorithm>

#define		NON_TEMPLATE_VERSION_FOR_START_AND_READ

class AFE_base : public SPI_for_AFE
{
public:
	/** ADC readout types */
	using raw_t								= int32_t;
	using microvolt_t						= double;

	/** Constructor to create a AFE_base instance */
	AFE_base( SPI& spi, bool spi_addr, bool highspeed_variant, int nINT, int DRDY, int SYN, int nRESET );

	/** Destractor */
	virtual ~AFE_base();
	
	/** Begin the device operation
	 *
	 *	NAFE13388 initialization. It does following steps
	 *	(1) Call reset()
	 *	(2) Call boot()
	 */
	virtual void begin( void );

	/** Set system-level config registers */
	virtual void boot( void )	= 0;

	/** Issue RESET command */
	virtual void reset( bool hardware_reset = false )	= 0;
	
	/** set callback function when DRDY comes */
	using	callback_fp_t	= std::function<void(void)>;
	virtual void set_DRDY_callback( callback_fp_t fnc );
	
	/** Configure logical channel
	 *
	 * @param ch logical channel number (0 ~ 15)
	 * @param cc0	16bit value to be set CH_CONFIG0 register (0x20)
	 * @param cc1	16bit value to be set CH_CONFIG1 register (0x21)
	 * @param cc2	16bit value to be set CH_CONFIG2 register (0x22)
	 * @param cc3	16bit value to be set CH_CONFIG3 register (0x23)
	 */
	virtual void open_logical_channel( int ch, uint16_t cc0, uint16_t cc1, uint16_t cc2, uint16_t cc3 )	= 0;

	/** Configure logical channel
	 *
	 * @param ch logical channel number (0 ~ 15)
	 * @param cc array for CH_CONFIG0, CH_CONFIG1, CH_CONFIG2 and CH_CONFIG3 values
	 */
	virtual void open_logical_channel( int ch, const uint16_t (&cc)[ 4 ] )	= 0;

	/** Logical channel disable
	 *
	 * @param ch logical channel number (0 ~ 15)
	 */
	virtual void close_logical_channel( int ch )		= 0;

	/** All logical channel disable
	 */
	virtual void close_logical_channel( void )			= 0;

	/** Start ADC
	 *
	 * @param ch logical channel number (0 ~ 15)
	 */
	virtual void start( int ch )						= 0;

	/** Start ADC on all logical channel
	 */
	virtual void start( void )							= 0;

	/** Start continuous AD conversion
	 */
	virtual void start_continuous_conversion( void )	= 0;

	/** DRDY event select
	 *
	 * @param set true for DRDY by sequencer is done
	 */	
	virtual void DRDY_by_sequencer_done( bool flag = true )	= 0;

	/** Read ADC for single channel
	 *
	 * @param ch logical channel number (0 ~ 15)
	 */
	virtual raw_t	read( int ch )							= 0;

	/** Read ADC for all channel
	 *
	 * @param data_ptr pointer to array to store ADC data
	 */
	virtual void	read( raw_t *data_ptr )					= 0;

	/** Read ADC for all channel
	 *
	 * @param data_vctr vector object to store ADC data
	 */
	virtual void	read( std::vector<raw_t>& data_vctr )	= 0;

	/** Start and read ADC for single  channel
	 *
	 * @param ch logical channel number (0 ~ 15)
	 */
	virtual raw_t	start_and_read( int ch );
	
#ifdef	NON_TEMPLATE_VERSION_FOR_START_AND_READ

	/** Start and read ADC for all channel
	 *
	 * @param data_ptr pointer to array to store ADC data
	 */
	virtual void	start_and_read( raw_t *data_ptr );

	/** Start and read ADC for all channel
	 *
	 * @param data_vctr vector object to store ADC data
	 */
	virtual void	start_and_read( std::vector<raw_t>& data_vctr );
#else
	template<typename T>
	inline void start_and_read( T data )
	{
		double	wait_time	= cbf_DRDY ? -1.0 : total_delay * delay_accuracy;
		
		start();
		wait_conversion_complete( wait_time );
		
		read( data );
	};
#endif
	
	/** Convert raw output to micro-volt
	 *
	 * @param ch logical channel number to select its gain coefficient
	 * @param value ADC read value
	 */
	inline double raw2uv( int ch, raw_t value )
	{
		return value * coeff_uV[ ch ];
	}
	
	/** Convert raw output to milli-volt
	 *
	 * @param ch logical channel number to select its gain coefficient
	 * @param value ADC read value
	 */
	inline double raw2mv( int ch, raw_t value )
	{
		return value * coeff_uV[ ch ] * 1e-3;
	}
	
	/** Convert raw output to volt
	 *
	 * @param ch logical channel number to select its gain coefficient
	 * @param value ADC read value
	 */
	inline double raw2v( int ch, raw_t value )
	{
		return value * coeff_uV[ ch ] * 1e-6;
	}
	
	/** Coefficient to convert from ADC read value to micro-volt
	 *
	 * @param ch logical channel number
	 */
	inline double coeff_mV( int ch )
	{
		return coeff_uV[ ch ];
	}
	
	/** Caliculated delay from logical channel setting (for single channel)
	 *
	 * @param ch logical channel number
	 */
	inline double drdy_delay( int ch )
	{
		return ch_delay[ ch ];
	}

	/** Caliculated delay from logical channel setting (for all channels)
	 */
	inline double drdy_delay( void )
	{
		return total_delay;
	}

	/** Number of enabled logical channels */
	inline int enabled_logical_channels( void )
	{
		return enabled_channels;
	}
	
	/** Switch to use DRDY to start ADC result reading
	 *
	 * @param use true (default) to use DRDY. if false, caliculated delay is used to start reading. 
	 */
	void	use_DRDY_trigger( bool use = true );

protected:
	bool			highspeed_variant;
	InterruptIn		pin_nINT;
	InterruptIn		pin_DRDY;
	DigitalOut		pin_SYN;
	DigitalOut		pin_nRESET;

	int 			bit_count( uint32_t value );

	/** Number of enabled logical channels */
	int				enabled_channels;
	
	/** Coefficient to convert from ADC read value to micro-volt */
	double			coeff_uV[ 16 ];

	/** Channel delay */
	double			ch_delay[ 16 ];
	double			total_delay;
	static double	delay_accuracy;
	

	uint32_t		drdy_count;
	volatile bool	drdy_flag;

	constexpr static uint32_t	timeout_limit	= 100000000;

	static callback_fp_t	cbf_DRDY;

	virtual void			init( void );
	void					default_drdy_cb( void );
	
	static void				DRDY_cb( void );
	int						wait_conversion_complete( double delay = -1.0 );

};

class NAFE13388_Base : public AFE_base
{
public:
	using	ch_setting_t	= uint16_t[ 4 ];

	typedef struct	_reference_point	{
		double	voltage;
		int32_t	data;
	} reference_point;

	typedef struct	_ref_points	{
		int				coeff_index;
		reference_point	high;
		reference_point	low;
		int				cal_index;
	} ref_points;
	
	/** Constructor to create a AFE_base instance */
	NAFE13388_Base( SPI& spi, bool spi_addr, bool highspeed_variant, int nINT, int DRDY, int SYN, int nRESET );

	/** Destractor */
	virtual ~NAFE13388_Base();
	
	/** Set system-level config registers */
	virtual void boot( void );

	/** Issue RESET command */
	virtual void reset( bool hardware_reset = false );
	
	/** Configure logical channel
	 *
	 * @param ch logical channel number (0 ~ 15)
	 * @param cc0	16bit value to be set CH_CONFIG0 register (0x20)
	 * @param cc1	16bit value to be set CH_CONFIG1 register (0x21)
	 * @param cc2	16bit value to be set CH_CONFIG2 register (0x22)
	 * @param cc3	16bit value to be set CH_CONFIG3 register (0x23)
	 */
	virtual void open_logical_channel( int ch, uint16_t cc0, uint16_t cc1, uint16_t cc2, uint16_t cc3 );

	/** Configure logical channel
	 *
	 * @param ch logical channel number (0 ~ 15)
	 * @param cc array for CH_CONFIG0, CH_CONFIG1, CH_CONFIG2 and CH_CONFIG3 values
	 */
	virtual void open_logical_channel( int ch, const uint16_t (&cc)[ 4 ] );

private:	
	double 	calc_delay( int ch );
	void 	channel_info_update( uint16_t value );

public:
	/** Logical channel disable
	 *
	 * @param ch logical channel number (0 ~ 15)
	 */
	virtual void close_logical_channel( int ch );

	/** All logical channel disable
	 */
	virtual void close_logical_channel( void );

	/** Start ADC
	 *
	 * @param ch logical channel number (0 ~ 15)
	 */
	virtual void start( int ch );

	/** Start ADC on all logical channel
	 */
	virtual void start( void );

	/** Start continuous AD conversion
	 */
	virtual void start_continuous_conversion();

	/** DRDY event select
	 *
	 * @param set true for DRDY by sequencer is done
	 */	
	virtual void DRDY_by_sequencer_done( bool flag = true );
	
	/** Read ADC for single channel
	 *
	 * @param ch logical channel number (0 ~ 15)
	 */
	virtual raw_t	read( int ch );

	/** Read ADC for all channel
	 *
	 * @param data_ptr pointer to array to store ADC data
	 */
	virtual void	read( raw_t *data );

	/** Read ADC for all channel
	 *
	 * @param data_vctr vector object to store ADC data
	 */
	virtual void	read( std::vector<raw_t>& data_vctr );
	
	constexpr static double	pga_gain[]	= { 0.2, 0.4, 0.8, 1, 2, 4, 8, 16 };

	enum GainPGA : uint8_t {
		G_PGA_x_0_2	= 0,
		G_PGA_x_0_4,
		G_PGA_x_0_8,
		G_PGA_x_1_0,
		G_PGA_x_2_0,
		G_PGA_x_4_0,
		G_PGA_x_8_0,
		G_PGA_x16_0,
	};
	
	enum class Register16 : uint16_t {
		CH_CONFIG0				= 0x20,
		CH_CONFIG1				= 0x21,
		CH_CONFIG2				= 0x22,
		CH_CONFIG3				= 0x23,
		CH_CONFIG4				= 0x24,
		CRC_CONF_REGS			= 0x25,
		CRC_COEF_REGS			= 0x26,
		CRC_TRIM_REGS			= 0x27,
		GPI_DATA				= 0x29,
		GPIO_CONFIG0			= 0x2A,
		GPIO_CONFIG1			= 0x2B,
		GPIO_CONFIG2			= 0x2C,
		GPI_EDGE_POS			= 0x2D,
		GPI_EDGE_NEG			= 0x2E,
		GPO_DATA				= 0x2F,
		SYS_CONFIG0				= 0x30,
		SYS_STATUS0				= 0x31,
		GLOBAL_ALARM_ENABLE		= 0x32,
		GLOBAL_ALARM_INTERRUPT	= 0x33,
		DIE_TEMP				= 0x34,
		CH_STATUS0				= 0x35,
		CH_STATUS1				= 0x36,
		THRS_TEMP				= 0x37,
		PN2						= 0x7C,
		PN1						= 0x7D,
		PN0						= 0x7E,
		CRC_TRIM_INT			= 0x7F,
	};

	enum class Register24 : uint16_t {
		CH_DATA0		= 0x40,
		CH_DATA1		= 0x41,
		CH_DATA2		= 0x42,
		CH_DATA3		= 0x43,
		CH_DATA4		= 0x44,
		CH_DATA5		= 0x45,
		CH_DATA6		= 0x46,
		CH_DATA7		= 0x47,
		CH_DATA8		= 0x48,
		CH_DATA9		= 0x4A,
		CH_DATA10		= 0x4B,
		CH_DATA11		= 0x4C,
		CH_DATA13		= 0x4D,
		CH_DATA14		= 0x4E,
		CH_DATA15		= 0x4F,
		CH_CONFIG5_0	= 0x50,
		CH_CONFIG5_1	= 0x51,
		CH_CONFIG5_2	= 0x52,
		CH_CONFIG5_3	= 0x53,
		CH_CONFIG5_4	= 0x54,
		CH_CONFIG5_5	= 0x55,
		CH_CONFIG5_6	= 0x56,
		CH_CONFIG5_7	= 0x57,
		CH_CONFIG5_8	= 0x58,
		CH_CONFIG5_9	= 0x59,
		CH_CONFIG5_10	= 0x5A,
		CH_CONFIG5_11	= 0x5B,
		CH_CONFIG5_12	= 0x5C,
		CH_CONFIG5_13	= 0x5D,
		CH_CONFIG5_14	= 0x5E,
		CH_CONFIG5_15	= 0x5F,
		CH_CONFIG6_0	= 0x60,
		CH_CONFIG6_1	= 0x61,
		CH_CONFIG6_2	= 0x62,
		CH_CONFIG6_3	= 0x63,
		CH_CONFIG6_4	= 0x64,
		CH_CONFIG6_5	= 0x65,
		CH_CONFIG6_6	= 0x66,
		CH_CONFIG6_7	= 0x67,
		CH_CONFIG6_8	= 0x68,
		CH_CONFIG6_9	= 0x69,
		CH_CONFIG6_10	= 0x6A,
		CH_CONFIG6_11	= 0x6B,
		CH_CONFIG6_12	= 0x6C,
		CH_CONFIG6_13	= 0x6D,
		CH_CONFIG6_14	= 0x6E,
		CH_CONFIG6_15	= 0x6F,
		GAIN_COEFF0		= 0x80,
		GAIN_COEFF1		= 0x81,
		GAIN_COEFF2		= 0x82,
		GAIN_COEFF3		= 0x83,
		GAIN_COEFF4		= 0x84,
		GAIN_COEFF5		= 0x85,
		GAIN_COEFF6		= 0x86,
		GAIN_COEFF7		= 0x87,
		GAIN_COEFF8		= 0x88,
		GAIN_COEFF9		= 0x89,
		GAIN_COEFF10	= 0x8A,
		GAIN_COEFF11	= 0x8B,
		GAIN_COEFF12	= 0x8C,
		GAIN_COEFF13	= 0x8D,
		GAIN_COEFF14	= 0x8E,
		GAIN_COEFF15	= 0x8F,
		OFFSET_COEFF0	= 0x90,
		OFFSET_COEFF1	= 0x91,
		OFFSET_COEFF2	= 0x92,
		OFFSET_COEFF3	= 0x93,
		OFFSET_COEFF4	= 0x94,
		OFFSET_COEFF5	= 0x95,
		OFFSET_COEFF6	= 0x96,
		OFFSET_COEFF7	= 0x97,
		OFFSET_COEFF8	= 0x98,
		OFFSET_COEFF9	= 0x99,
		OFFSET_COEFF10	= 0x9A,
		OFFSET_COEFF11	= 0x9B,
		OFFSET_COEFF12	= 0x9C,
		OFFSET_COEFF13	= 0x9D,
		OFFSET_COEFF14	= 0x9E,
		OFFSET_COEFF15	= 0x9F,
		OPT_COEF0		= 0xA0,
		OPT_COEF1		= 0xA1,
		OPT_COEF2		= 0xA2,
		OPT_COEF3		= 0xA3,
		OPT_COEF4		= 0xA4,
		OPT_COEF5		= 0xA5,
		OPT_COEF6		= 0xA6,
		OPT_COEF7		= 0xA7,
		OPT_COEF8		= 0xA8,
		OPT_COEF9		= 0xA9,
		OPT_COEF10		= 0xAA,
		OPT_COEF11		= 0xAB,
		OPT_COEF12		= 0xAC,
		OPT_COEF13		= 0xAD,
		SERIAL1			= 0xAE,
		SERIAL0			= 0xAF,
	};

	enum Command : uint16_t {
		CMD_CH0 			= 0x0000,
		CMD_CH1 			= 0x0001,
		CMD_CH2 			= 0x0002,
		CMD_CH3 			= 0x0003,
		CMD_CH4 			= 0x0004,
		CMD_CH5 			= 0x0005,
		CMD_CH6 			= 0x0006,
		CMD_CH7 			= 0x0007,
		CMD_CH8 			= 0x0008,
		CMD_CH9 			= 0x0009,
		CMD_CH10 			= 0x000A,
		CMD_CH11 			= 0x000B,
		CMD_CH12 			= 0x000C,
		CMD_CH13 			= 0x000D,
		CMD_CH14 			= 0x000E,
		CMD_CH15 			= 0x000F,
		CMD_ABORT 			= 0x0010,
		CMD_END				= 0x0011,
		CMD_CLEAR_ALARM		= 0x0012,
		CMD_CLEAR_DATA		= 0x0013,
		CMD_RESET			= 0x0014,
		CMD_CLEAR_REG		= 0x0015,
		CMD_RELOAD			= 0x0016,
		TBD					= 0x0017,
		CMD_SS				= 0x2000,
		CMD_SC				= 0x2001,
		CMD_MM				= 0x2002,
		CMD_MC				= 0x2003,
		CMD_MS				= 0x2004,
		CMD_BURST_DATA		= 0x2005,
		CMD_CALC_CRC_CONFG	= 0x2006,
		CMD_CALC_CRC_COEF	= 0x2007,
		CMD_CALC_CRC_FAC	= 0x2008,
	};

	using	RegisterVariant	= std::variant<Register16, Register24>;
	using	RegVct			= std::vector<RegisterVariant>;
	
	/** Command
	 *	
	 * @param com "Comand" type or uint16_t value
	 */
	virtual void		command( uint16_t com );

	/** Write register
	 *
	 *	Writes register. Register width is selected by reg type (Register16 ot Register24)
	 * @param reg register specified by Register16 member
	 */
	virtual void		reg( Register16 r, uint16_t value );

	/** Write register
	 *
	 *	Writes register. Register width is selected by reg type (Register16 ot Register24)
	 * @param reg register specified by Register24 member
	 */
	virtual void		reg( Register24 r, uint32_t value );

	/** Read register
	 *
	 *	Reads register. Register width is selected by reg type (Register16 ot Register24)
	 * @param reg register specified by Register16 member
	 * @return readout value
	 */
	virtual uint16_t	reg( Register16 r );

	/** Read register
	 *
	 *	Reads register. Register width is selected by reg type (Register16 ot Register24)
	 * @param reg register specified by Register24 member
	 * @return readout value
	 */
	virtual uint32_t	reg( Register24 r );
	
	/** Register bit operation
	 *
	 *	overwrite bits i a register
	 * @param reg register specified by Register16 or Register24 member
	 * @param mask mask bits
	 * @param reg value to over write
	 */
	template<typename T>
	uint32_t	bit_op( T rg, uint32_t mask, uint32_t value )
	{
		uint32_t v	= reg( rg );

		v	&= mask;
		v	|= value & ~mask;

		reg( rg, v );
		
		return v;
	}
	
	/** Read part_number
	 *
	 * @return 0x13388B40 
	 */
	uint32_t	part_number( void );

	/** Read rivision number
	 *
	 * @return PN0 register value & 0xF
	 */
	uint8_t	revision_number( void );

	/** Read serial number
	 *
	 * @return serial number
	 */
	uint64_t	serial_number( void );

	/** Die temperature
	 *
	 * @return die temperature in celsius
	 */
	float	temperature( void );
	
	/** Gain and offset coefficient customization
	 *
	 *	Sets gain and offset coefficients with given target ADC read-out values at two reference voltaeg points
	 * @param ref struct to define the target coefficient index and two reference poins and reference pre-calibrated coeffs
	 */
	void	gain_offset_coeff( const ref_points &ref );

	enum CalibrationError : int {
		NoError		=  0,
		GainError	= -1,
		OffsetError	= -2,
	};
	
	/** On-board calibration with specified input and voltage
	 *
	 *	Updates coefficients at pga_gain_index
	 *	
	 * @param pga_gain_index			PGA gain index to measure and update the coefficients
	 * @param channel_selection			Logical channel number for calibration use
	 * @param reference_source_voltage	Reference voltage. This is not required if internal reference is used
	 * @param input_select				Physical input channel selection. It will use internal voltage reference if this value is 0
	 * @param use_positive_side			Physical input channel selection AnP or AnN
	 * @return CalibrationError 		Error code
	 */
	int		self_calibrate( int pga_gain_index, int channel_selection = 15, int input_select = 0, double reference_source_voltage = 0, bool use_positive_side = true );

	/** Blinks LEDs on GPIO pins */
	void blink_leds( void );
};

class NAFE13388 : public NAFE13388_Base
{
public:	
	/** Constructor to create a NAFE13388 instance */
	NAFE13388( SPI& spi, bool spi_addr = 0, bool highspeed_variant = false, int nINT = D2, int DRDY = D3, int SYN = D5, int nRESET = D6 );

	/** Destractor */
	virtual ~NAFE13388();
};

class NAFE13388_UIM : public NAFE13388_Base
{
public:	
	/** Constructor to create a NAFE13388 instance */
	NAFE13388_UIM( SPI& spi, bool spi_addr = 0, bool highspeed_variant = false, int nINT = D3, int DRDY = D4, int SYN = D6, int nRESET = D7 );

	/** Destractor */
	virtual ~NAFE13388_UIM();

	void blink_leds( void );
};

inline NAFE13388_Base::Register16 operator+( NAFE13388_Base::Register16 rn, int n )
{
    return static_cast<NAFE13388_Base::Register16>( static_cast<uint16_t>( rn ) + n );
}

inline NAFE13388_Base::Register16 operator+( int n, NAFE13388_Base::Register16 rn )
{
    return static_cast<NAFE13388_Base::Register16>( n + static_cast<uint16_t>( rn ) );
}

inline NAFE13388_Base::Register24 operator+( NAFE13388_Base::Register24 rn, int n )
{
    return static_cast<NAFE13388_Base::Register24>( static_cast<uint16_t>( rn ) + n );
}

inline NAFE13388_Base::Register24 operator+( int n, NAFE13388_Base::Register24 rn )
{
    return static_cast<NAFE13388_Base::Register24>( n + static_cast<uint16_t>( rn ) );
}

#endif //	ARDUINO_AFE_DRIVER_H
