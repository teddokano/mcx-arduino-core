/** NXP Analog Front End class library for MCX
 *
 *  @author  Tedd OKANO
 *
 *  Copyright: 2026 Tedd OKANO
 *  Released under the MIT license
 */

#include "AFE_NXP.h"

#ifndef ARDUINO_AFE_NAFE33352_DRIVER_H
#define ARDUINO_AFE_NAFE33352_DRIVER_H

class NAFE33352_Base : public AFE_base
{
public:
	using					ch_setting_t	= uint16_t[ 4 ];
	static constexpr double	on_board_shunt_resister	= 50.00;
	
	/** Constructor to create a NAFE33352_Base instance */
	NAFE33352_Base( SPI& spi, bool spi_addr, bool highspeed_variant, int nINT, int DRDY, int SYN, int nRESET, int SYNCDAC );

	/** Destractor */
	virtual ~NAFE33352_Base();
	
	
private:
	/** Send data
	 * 
	 * @param data pointer to data buffer
	 * @param size data size
	 */
	virtual void txrx( uint8_t *data, int size );

	DigitalOut	*chip_select_pin;

public:	
	/** Set system-level config registers */
	virtual void boot( void );

	/** Issue RESET command */
	virtual void reset( bool hardware_reset = false );
	
	/** Configure logical channel
	 *
	 * @param ch logical channel number (0 ~ 15)
	 * @param cc0	16bit value to be set AI_CONFIG0 register (0x20)
	 * @param cc1	16bit value to be set AI_CONFIG1 register (0x21)
	 * @param cc2	16bit value to be set AI_CONFIG2 register (0x22)
	 * @param dummy	dummy variable to keep compatibility over AFE devices
	 */
	virtual void open_logical_channel( int ch, uint16_t cc0, uint16_t cc1, uint16_t cc2, uint16_t dummy );

	/** Configure logical channel
	 *
	 * @param ch logical channel number (0 ~ 15)
	 * @param cc array for AI_CONFIG0, AI_CONFIG1, AI_CONFIG2 and dummy values
	 */
	virtual void open_logical_channel( int ch, const uint16_t (&cc)[ 4 ] );

	/** LogicalChannel sub-class in NAFE33352_Base class */
	class LogicalChannel : public LogicalChannel_Base
	{
	public:
		LogicalChannel();
		virtual ~LogicalChannel();
		
		/** Configure logical channel
		 *
		 * @param cc array for AI_CONFIG0, AI_CONFIG1 and AI_CONFIG2 rgister values
		 */
		void	configure( const uint16_t (&cc)[ 3 ] );

		/** Configure logical channel
		 *
		 * @param cc0	16bit value to be set AI_CONFIG0 register (0x20)
		 * @param cc1	16bit value to be set AI_CONFIG1 register (0x21)
		 * @param cc2	16bit value to be set AI_CONFIG2 register (0x22)
		 */
		void	configure( uint16_t cc0, uint16_t cc1 = 0x0000, uint16_t cc2 = 0x0000 );
	};
	
	/** 16 LogicalChannel instance array */
	LogicalChannel	logical_channel[ 16 ];

	/** DAC sub-class in NAFE33352_Base class */
	class DAC
	{
	public:

		/** DAC default configuration register value set selector */
		enum class ModeSelect : uint16_t {
			OFF		= 0,
			VOLTAGE,
			CURRENT,
			CURRENT_RECAL,
			DAC_OFF_VOLTAGE_IN,
			DAC_OFF_CURRENT_IN
		};

		DAC();
		virtual ~DAC();
		
		/** Configure logical channel
		 *
		 * @param cc array for AIO_CONFIG, AO_CAL_COEF, AIO_PROT_CFG, AO_SLR_CTRL, AWG_PER and AO_SYSCFG rgister values
		 */
		void	configure( const uint16_t (&cc)[ 6 ] );

		/** Configure logical channel
		 *
		 * @param cc0	16bit value to be set AIO_CONFIG register (0x20)
		 * @param cc1	16bit value to be set AO_CAL_COEF register (0x21)
		 * @param cc2	16bit value to be set AIO_PROT_CFG register (0x22)
		 * @param cc3	16bit value to be set AO_SLR_CTRL register (0x23)
		 * @param cc4	16bit value to be set AWG_PER register (0x24)
		 * @param cc5	16bit value to be set AO_SYSCFG register (0x25)
		 */
		void	configure( uint16_t cc0, uint16_t cc1, uint16_t cc2, uint16_t cc3, uint16_t cc4, uint16_t cc5 );
		
		/** Configure logical channel
		 *
		 * @param mode	ModeSelect selector
		 * @param full_scale_range	Full scale range modifier. If it meeds to be +/-2.5V, define this variable 2.5. If it is +/-10mA, define it as 0.01. 
		 */		
		void	configure( ModeSelect mode, double full_scale_range = 0.00 );
		
		/** Configure logical channel
		 *
		 * @param full_scale_range	Full scale range modifier. If it meeds to be +/-2.5V, define this variable 2.5. If it is +/-10mA, define it as 0.01. 
		 */		
		void 	configure( double full_scale_range );
		
		/** Set DAC output
		 *
		 * @param value	set value in Volt or Ampere
		 */		
		void	output( double value );
		
		/** Set DAC output
		 *
		 *	shorthand to set DAC output
		 */				
		DAC&	operator=( double value );
		
		/** pointer to NAFE33352_Base based instance */
		NAFE33352_Base	*afe_ptr;
	private:
		ModeSelect		output_mode;
		double			full_scale;
	};
	
	DAC	dac;
	
private:	
	double 	calc_delay( int ch );
	void 	channel_info_update( uint16_t value );
	
public:
	void	open_dac_output( const uint16_t (&cc)[ 6 ] );


	/** Logical channel disable
	 *
	 * @param ch logical channel number (0 ~ 15)
	 */
	virtual void close_logical_channel( int ch );

	/** All logical channel disable
	 */
	virtual void close_logical_channel( void );

	/** Logical channel enable
	 *
	 * @param ch logical channel number (0 ~ 15)
	 */
	void	enable_logical_channel( int ch );

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

	/** Read ADC for all channel
	 *
	 * @param data_ptr pointer to array to store ADC data
	 */
	virtual void	read( volt_t *data );

	/** Read ADC for all channel
	 *
	 * @param data_vctr vector object to store ADC data
	 */
	virtual void	read( std::vector<volt_t>& data_vctr );

	/** Convert raw output to volt
	 *
	 * @param ch logical channel number to select its gain coefficient
	 * @param value ADC read value
	 */
	inline double raw2v( int ch, raw_t value )
	{
		if ( mux_setting[ ch ] == ISNS )
			return	value * coeff_V[ ch ] / on_board_shunt_resister;			
		else if ( mux_setting[ ch ] == BG )
			return	value - (double)(1 << 24) * 20.00 * 2.50 / (12.50 * (double)(1 << 24)) -1.50;
		else
			return	value * coeff_V[ ch ];
	}
	
	/** DAC output
	 *
	 * @param data_vctr vector object to store ADC data
	 */
	virtual void	dac_out( double vi, double full_scale, uint8_t bit_length );
	int32_t			dac_code( double a, double full_scale, uint8_t bit_length );

	constexpr static double	pga_gain[]	= { 1.00, 16.00 };

	enum IN_SEL : uint8_t {
		VCM_VCM		= 0,
		AIP_AIN,
		AIP_VSNS,
		GPIO0_GPIO1,
		AIP,
		AIN,
		ISNS,
		VSNS,
		TIA,
		GPIO0_VCM,
		VCM_GPIO1,
		REF_BYP__VCM,
		VCM__REF_BYP,
		BG,
		VADD,
		LDO,
		VHDD,
		VHSS,
		DAC_REF
	};
	
	enum GainPGA : uint8_t {
		G_PGA_x_1_0,
		G_PGA_x16_0,
	};
	
	enum class Register16 : uint16_t {
		CRC_CONF_REGS		= 0x20,
		CRC_COEF_REGS		= 0x21,
		CRC_TRIM_REGS		= 0x22,
		CRC_TRIM_REF		= 0x3F, 
		GPI_DATA			= 0x23,
		GPO_ENABLE			= 0x24,
		GPIO_FUNCTION		= 0x25,
		GPI_ENABLE			= 0x26,
		GPI_EDGE_POS		= 0x27,
		GPI_EDGE_NEG		= 0x28,
		GPO_DATA			= 0x29,
		SYS_CONFIG			= 0x2A,
		SYS_STATUS			= 0x2B,
		CK_SRC_SEL_CONFIG	= 0x30,
		GLOBAL_ALARM_ENABLE	= 0x2C,
		GLOBAL_ALARM_INT	= 0x2D,
		DIE_TEMP			= 0x2E,
		TEMP_THRS			= 0x2F,
		PN2					= 0x40,
		PN1					= 0x41,
		PN0_REV				= 0x42,
		
		AI_CONFIG0			= 0x1000 | 0x20,
		AI_CONFIG1			= 0x1000 | 0x21,
		AI_CONFIG2			= 0x1000 | 0x22,
		AI_MULTI_CH_EN		= 0x1000 | 0x23,
		AI_SYSCFG			= 0x1000 | 0x24,
		AI_STATUS			= 0x1000 | 0x25,
		AI_STATUS_OVR		= 0x1000 | 0x26,
		AI_STATUS_UDR		= 0x1000 | 0x27,
		
		AIO_CONFIG			= 0x1C00 | 0x20,
		AO_CAL_COEF			= 0x1C00 | 0x21,
		AIO_PROT_CFG		= 0x1C00 | 0x22,
		AO_SLR_CTRL			= 0x1C00 | 0x23,
		AWG_PER				= 0x1C00 | 0x24,
		AO_SYSCFG			= 0x1C00 | 0x25,
		AIO_STATUS			= 0x1C00 | 0x26
	};

	enum class Register24 : uint16_t {
		GAIN_COEF0			= 0x50,
		GAIN_COEF1,
		GAIN_COEF2,
		GAIN_COEF3,
		GAIN_COEF4,
		GAIN_COEF5,
		GAIN_COEF6,
		GAIN_COEF7,
		OFFSET_COEF0		= 0x58,
		OFFSET_COEF1,
		OFFSET_COEF2,
		OFFSET_COEF3,
		OFFSET_COEF4,
		OFFSET_COEF5,
		OFFSET_COEF6,
		OFFSET_COEF7,
		EXTRA_CAL_COEF0		= 0x60,
		EXTRA_CAL_COEF1,
		EXTRA_CAL_COEF2,
		EXTRA_CAL_COEF3,
		EXTRA_CAL_COEF4,
		EXTRA_CAL_COEF5,
		EXTRA_CAL_COEF6,
		EXTRA_CAL_COEF7,
		SERIAL1				= 0x43,
		SERIAL0,
		
		AI_DATA0			= 0x1000 | 0x30,
		AI_DATA1,
		AI_DATA2,
		AI_DATA3,
		AI_DATA4,
		AI_DATA5,
		AI_DATA6,
		AI_DATA7,
		AI_CH_OVR_THR_0		= 0x1000 | 0x38,
		AI_CH_OVR_THR_1,
		AI_CH_OVR_THR_2,
		AI_CH_OVR_THR_3,
		AI_CH_OVR_THR_4,
		AI_CH_OVR_THR_5,
		AI_CH_OVR_THR_6,
		AI_CH_OVR_THR_7,
		AI_CH_UDR_THR_0		= 0x1000 | 0x40,
		AI_CH_UDR_THR_1,
		AI_CH_UDR_THR_2,
		AI_CH_UDR_THR_3,
		AI_CH_UDR_THR_4,
		AI_CH_UDR_THR_5,
		AI_CH_UDR_THR_6,
		AI_CH_UDR_THR_7,
		
		AO_DATA				= 0x1C00 | 0x28,
		AO_OC_POS_LIMIT,
		AO_OC_NEG_LIMIT,
		AWG_AMP_MAX,
		AWG_AMP_MIN
	};

	enum Command : uint16_t {
		CMD_CLEAR_ALARM		= 0x12,
		CMD_RESET			= 0x14,
		CMD_CLEAR_REG		= 0x15,
		CMD_RELOAD			= 0x16,
		CMD_CALC_CRC_CONFIG	= 0x2006,
		CMD_CALC_CRC_COEF	= 0x2007,
		CMD_CALC_CRC_FAC	= 0x2008,
		
		CMD_CH0				= 0x1000,
		CMD_CH1				= 0x1001,
		CMD_CH2				= 0x1002,
		CMD_CH3				= 0x1003,
		CMD_CH4				= 0x1004,
		CMD_CH5				= 0x1005,
		CMD_CH6				= 0x1006,
		CMD_CH7				= 0x1007,
		CMD_ADC_ABORT		= 0x1010,
		CMD_END				= 0x1012,
		CMD_CLEAR_DATA		= 0x1013,
		CMD_SS				= 0x3000,
		CMD_SC				= 0x3001,
		CMD_MM				= 0x3002,
		CMD_MC				= 0x3003,
		CMD_MS				= 0x3004,
		CMD_BURST_DATA		= 0x3005,
		
		CMD_WGEN_STOP		= 0x1C00,
		CMD_WGEN_START		= 0x1C01,
		CMD_CLEAR_DAC0		= 0x1C02,
		CMD_AO_ABORT		= 0x1C03,
		CMD_AO_ABORT_HIZ	= 0x1C04,
		CMD_CISW_ABORT		= 0x1C05,
		CMD_CISW_ABORT_HIZ	= 0x1C06,
	};

	using	RegisterVariant	= std::variant<Register16, Register24>;
	using	RegVct			= std::vector<RegisterVariant>;
	
	/** Command
	 *
	 * @param com "Command" type or uint16_t value
	 */
	virtual void		command( uint16_t com );

	/** Write register
	 *
	 *	Writes register. Register width is selected by reg type (Register16 or Register24)
	 * @param reg register specified by Register16 member
	 */
	virtual void		reg( Register16 r, uint16_t value );

	/** Write register
	 *
	 *	Writes register. Register width is selected by reg type (Register16 or Register24)
	 * @param reg register specified by Register24 member
	 */
	virtual void		reg( Register24 r, uint32_t value );

	/** Read register
	 *
	 *	Reads register. Register width is selected by reg type (Register16 or Register24)
	 * @param reg register specified by Register16 member
	 * @return readout value
	 */
	virtual uint16_t	reg( Register16 r );

	/** Read register
	 *
	 *	Reads register. Register width is selected by reg type (Register16 or Register24)
	 * @param reg register specified by Register24 member
	 * @return readout value
	 */
	virtual uint32_t	reg( Register24 r );

	/** Register bit operation
	 *
	 *	Overwrite bits in a register
	 * @param reg register specified by Register16 or Register24 member
	 * @param mask mask bits
	 * @param value value to overwrite
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

	/** Read part number
	 *
	 * @return part number read from PN2, PN1 and PN0_REV registers
	 */
	uint64_t	part_number( void );

	/** Read revision number
	 *
	 * @return PN0_REV register value & 0xF
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
};

class NAFE33352 : public NAFE33352_Base
{
public:	
	/** Constructor to create a NAFE33352 instance */
	NAFE33352( SPI& spi, bool spi_addr = 0, bool highspeed_variant = false, int nINT = D7, int DRDY = D4, int SYN = D2, int nRESET = D5, int SYNCDAC = D3 );

	/** Destractor */
	virtual ~NAFE33352();
};

class NAFE33352_UIOM : public NAFE33352_Base
{
public:	
	/** Constructor to create a NAFE33352 instance */
	NAFE33352_UIOM( SPI& spi, bool spi_addr = 0, bool highspeed_variant = false, int nINT = D7, int DRDY = D4, int SYN = D2, int nRESET = DISABLED_PIN, int SYNCDAC = D3 );

	/** Destractor */
	virtual ~NAFE33352_UIOM();
};


inline NAFE33352_Base::Register16 operator+( NAFE33352_Base::Register16 rn, int n )
{
	return static_cast<NAFE33352_Base::Register16>( static_cast<uint16_t>( rn ) + n );
}

inline NAFE33352_Base::Register16 operator+( int n, NAFE33352_Base::Register16 rn )
{
	return static_cast<NAFE33352_Base::Register16>( n + static_cast<uint16_t>( rn ) );
}

inline NAFE33352_Base::Register24 operator+( NAFE33352_Base::Register24 rn, int n )
{
	return static_cast<NAFE33352_Base::Register24>( static_cast<uint16_t>( rn ) + n );
}

inline NAFE33352_Base::Register24 operator+( int n, NAFE33352_Base::Register24 rn )
{
	return static_cast<NAFE33352_Base::Register24>( n + static_cast<uint16_t>( rn ) );
}

#endif // !ARDUINO_AFE_NAFE33352_DRIVER_H
