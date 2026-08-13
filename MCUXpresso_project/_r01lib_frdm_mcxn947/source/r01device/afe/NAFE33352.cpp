/** NXP Analog Front End class library for MCX
 *
 *  @author  Tedd OKANO
 *
 *  Copyright: 2026 Tedd OKANO
 *  Released under the MIT license
 */

#include	"NAFE33352.h"
#include	"r01lib.h"
#include	<math.h>

using enum	NAFE33352_Base::Register16;
using enum	NAFE33352_Base::Register24;
using enum	NAFE33352_Base::Command;


NAFE33352_Base::LogicalChannel::LogicalChannel() : LogicalChannel_Base()
{
}

NAFE33352_Base::LogicalChannel::~LogicalChannel()
{
	disable();
}

void NAFE33352_Base::LogicalChannel::configure( const uint16_t (&cc)[ 3 ] )
{
	const uint16_t	tmp_ch_config[ 4 ]	= { cc[ 0 ], cc[ 1 ], cc[ 2 ], 0 };
	afe_ptr->open_logical_channel( ch_number, tmp_ch_config );
}

void NAFE33352_Base::LogicalChannel::configure( uint16_t cc0, uint16_t cc1, uint16_t cc2 )
{
	const uint16_t	tmp_ch_config[ 4 ]	= { cc0, cc1, cc2, 0 };
	afe_ptr->open_logical_channel( ch_number, tmp_ch_config );
}

NAFE33352_Base::DAC::DAC()
{
}

NAFE33352_Base::DAC::~DAC()
{
}

void NAFE33352_Base::DAC::configure( uint16_t cc0, uint16_t cc1, uint16_t cc2, uint16_t cc3, uint16_t cc4, uint16_t cc5 )
{
	const uint16_t	tmp_ch_config[ 6 ]	= { cc0, cc1, cc2, cc3, cc4, cc5 };
	afe_ptr->open_dac_output( tmp_ch_config );
}

void NAFE33352_Base::DAC::configure( const uint16_t (&cc)[ 6 ] )
{
	afe_ptr->open_dac_output( cc );
}

void NAFE33352_Base::DAC::configure( ModeSelect mode, double full_scale_range )
{
	uint16_t	default_dac_setting[ 6 ]	= { 0x0000, 0x1000, 0x87FF, 0x0000, 0xE7FF, 0x0C00 };
	
	output_mode	= mode;
	
	switch ( output_mode )
	{
		case NAFE33352_Base::DAC::ModeSelect::OFF:
			full_scale					= 12.50;
			default_dac_setting[ 0 ]   |= 0x0002;
			break;
		case NAFE33352_Base::DAC::ModeSelect::VOLTAGE:
			full_scale					= 12.50;
			default_dac_setting[ 0 ]   |= 0x6040;
			break;
		case NAFE33352_Base::DAC::ModeSelect::CURRENT:
			full_scale					= 0.025 / 0.95;
			default_dac_setting[ 0 ]   |= 0x6061;
			default_dac_setting[ 1 ]   |= 0x1000;
			break;
		case NAFE33352_Base::DAC::ModeSelect::CURRENT_RECAL:
			full_scale					= 0.025;
			default_dac_setting[ 0 ]   |= 0x6061;
			default_dac_setting[ 1 ]   |= 0x1000;
			break;
		case NAFE33352_Base::DAC::ModeSelect::DAC_OFF_VOLTAGE_IN:
			full_scale					= 0.025;
			default_dac_setting[ 0 ]   |= 0x6000;
			break;
		case NAFE33352_Base::DAC::ModeSelect::DAC_OFF_CURRENT_IN:
			full_scale					= 0.025;
			default_dac_setting[ 0 ]   |= 0x7000;
			break;
	}

	if ( 0.0 < full_scale_range )	//	overwrite fullscale range if it is specified
		full_scale	= full_scale_range;
	
	configure( default_dac_setting );
}

void NAFE33352_Base::DAC::configure( double full_scale_range )
{
	full_scale	= full_scale_range;
}

void NAFE33352_Base::DAC::output( double value )
{
	afe_ptr->dac_out( value, full_scale, 18 );
}

NAFE33352_Base::DAC& NAFE33352_Base::DAC::operator=( double value )
{
	output( value );
	return	*this;
}



/* NAFE33352_Base class ******************************************/

NAFE33352_Base::NAFE33352_Base( SPI& spi, bool spi_addr, bool hsv, int nINT, int DRDY, int SYN, int nRESET, int SYNCDAC )
	: AFE_base( spi, spi_addr, hsv, nINT, DRDY, SYN, nRESET, SYNCDAC )
{
	for ( auto i = 0; i < 16; i++ )
	{
		logical_channel[ i ].afe_ptr	= this;
		logical_channel[ i ].ch_number	= i;
	}
	
	dac.afe_ptr	= this;

	chip_select_pin	= spi.cs_manual_control( true );
}

NAFE33352_Base::~NAFE33352_Base()
{
}


void NAFE33352_Base::txrx( uint8_t *data, int size )
{
	*chip_select_pin	= false;
	SPI_for_AFE::txrx( data, size );
	wait_us( 4 );
	*chip_select_pin	= true;
}



void NAFE33352_Base::boot( void )
{
	command( CMD_ADC_ABORT );
	command( CMD_AO_ABORT );
	wait( 0.001 );
	
	DRDY_by_sequencer_done( true );
	
	reg( SYS_CONFIG,        0x0000 );
	reg( CK_SRC_SEL_CONFIG, 0x0000 );

	reg( AI_SYSCFG,         0x0800 );


}

void NAFE33352_Base::reset( bool hardware_reset )
{
	if ( hardware_reset )
	{
		printf( "warning: UIOM doesn't have hardware RESET pin on the board. This reset will be ignored\r\n" );
		pin_nRESET	= 0;
		wait( 0.001 );
		pin_nRESET	= 1;
	}
	else
	{
		command( CMD_RESET ); 
	}
	
	constexpr uint16_t	CHIP_READY	= 1 << 13;
	constexpr auto		RETRY		= 10;
	
	for ( auto i = 0; i < RETRY; i++ )
	{
		wait( 0.003 );
		if ( reg( SYS_STATUS ) & CHIP_READY )
			return;
	}
	
	panic( "NAFE33352 couldn't get ready. Check power supply or pin conections\r\n" );
}

void NAFE33352_Base::open_dac_output( const uint16_t (&cc)[ 6 ] )
{
	for ( auto i = 0; i < 6; i++ )
		reg( AIO_CONFIG + i, cc[ i ] );
}


void NAFE33352_Base::open_logical_channel( int ch, const uint16_t (&cc)[ 4 ] )
{	
	static bool			pga_enabled	= false;
	constexpr double	pow2_24		= (double)(1 << 24);
	double				coeff		= 0.00;
	bool				pga_on		= false;
	
	mux_setting[ ch ]	= (cc[ 0 ] >> 3) & 0x1F;
	
	command( CMD_CH0 + ch );

	switch ( mux_setting[ ch ] )
	{
		case VCM_VCM:
		case GPIO0_GPIO1:
			coeff	= (20.00 * 2.50) / (12.5 * pow2_24);
			break;
		case AIP_AIN:
			coeff	= (20.00 * 2.50) / ((cc[ 0 ] & 0x0100 ? 16.00 : 1.00) * pow2_24);
			pga_on	= true;
			break;
		case AIP_VSNS:
		case VSNS:
			coeff	= (20.00 * 2.50) / pow2_24;
			pga_on	= true;
			break;
		case AIP:
		case AIN:
			coeff	= (20.00 * 2.50) / ((cc[ 0 ] & 0x0100 ? 16.00 : 1.00) * pow2_24);
			pga_on	= true;
			break;
		case ISNS:
			coeff	= (20.00 * 2.50) / (3.7989 * pow2_24);
			break;
		case TIA:
			coeff	= (20.00 * 2.50) / (2.50 * pow2_24);
			break;
		case GPIO0_VCM:
		case VCM_GPIO1:
		case REF_BYP__VCM:
		case LDO:
		case DAC_REF:
			coeff	= (20.00 * 2.50) / (12.5 * pow2_24) + 1.50;
			break;
		case VCM__REF_BYP:
			coeff	= (20.00 * 2.50) / (12.5 * pow2_24) - 1.50;
			break;
		case BG:
			//coeff	= (20.00 * 2.50) / (12.5 * pow2_24) - 1.50;
			break;
		case VADD:
			coeff	= (2.00 * 20.00 * 2.50) / (12.5 * pow2_24) - 1.50;
			break;
		case VHDD:
		case VHSS:
			coeff	= (40.00 * 20.00 * 2.50) / (12.5 * pow2_24);
			break;
	}
	
	coeff_V[ ch ]		= coeff;
	
	if ( pga_on && !pga_enabled )
	{
		reg( AI_SYSCFG, 0x0800 );
		pga_enabled	= true;
	}
	
	for ( auto i = 0; i < 3; i++ )
		reg( AI_CONFIG0 + i, cc[ i ] );
	
	enable_logical_channel( ch );
	
	ch_delay[ ch ]		= calc_delay( ch );
}

void NAFE33352_Base::channel_info_update( uint16_t value )
{
	constexpr auto	bit_length	= 16;
	enabled_channels			= 0;
	total_delay					= 0.00;
	
	memset( sequence_order, 0, 16 );
		
	for ( auto i = 0; i < bit_length; i++ )
	{
		if ( value & (0x1 << i) )
		{
			sequence_order[ enabled_channels ]	= i;
			enabled_channels++;
			total_delay	+= ch_delay[ i ];
		}
	}

#if 0
	for ( auto i = 0; i < bit_length; i++ )
		printf( " %x", sequence_order[ i ] );
	printf( "\r\n" );
#endif
}

double NAFE33352_Base::calc_delay( int ch )
{
	constexpr static double	data_rates[]	= {	   288000, 192000, 144000, 96000, 72000, 48000, 36000, 24000, 
													18000,  12000,   9000,  6000,  4500,  3000,  2250,  1125, 
													 562.5,    400,    300,   200,   100,    60,    50,    30, 
														25,     20,     15,    10,   7.5, 						};
	constexpr static uint16_t	delays[]	= {		0,   2,   4,   6,   8,  10,   12,  14, 
												   16,  18,  20,  28,  38,  40,   42,  56, 
												   64,  76,  90, 128, 154, 178, 204, 224, 
												  256, 358, 512, 716, 
												  1024, 1664, 3276, 7680, 19200, 23040, };
	
	command( CMD_CH0 + ch );

	uint16_t ch_config1	= reg( AI_CONFIG1 );
	uint16_t ch_config2	= reg( AI_CONFIG2 );
	
	uint8_t		adc_data_rate		= (ch_config1 >>  3) & 0x001F;
	uint8_t		adc_sinc			= (ch_config1 >>  0) & 0x0007;
	uint8_t		ch_delay			= (ch_config2 >> 10) & 0x003F;
	bool		adc_normal_setting	= (ch_config2 >>  9) & 0x0001;
	bool		ch_chop				= (ch_config2 >>  7) & 0x0001;
	
	double		base_freq			= data_rates[ adc_data_rate ];
	double		delay_setting		= delays[ ch_delay ] / 4608000.00;
	
	if ( highspeed_variant )
	{
		base_freq		*= 2.00;
		delay_setting	/= 2.00;		
	}
	
	if ( (28 < adc_data_rate) || (4 < adc_sinc) || ((adc_data_rate < 12) && (adc_sinc)) )
		return 0.00;
	
	if ( !adc_normal_setting )
		base_freq	/= (adc_sinc + 1);
	
	if ( ch_chop )
		base_freq	/= 2;
	
#if  0
	printf( "adc_data_rate = %d\r\n", adc_data_rate );
	printf( "base_freq = %lf\r\n", base_freq );
	printf( "delay_setting = %lf\r\n", delay_setting  );
	printf( "channel delay = %lf\r\n", (1 / base_freq) + delay_setting  );
#endif
	
	return (1 / base_freq) + delay_setting;
}


void NAFE33352_Base::open_logical_channel( int ch, uint16_t cc0, uint16_t cc1, uint16_t cc2, uint16_t dummy )
{	
	const ch_setting_t	tmp_ch_config	= { cc0, cc1, cc2 };
	open_logical_channel( ch, tmp_ch_config );
}

void NAFE33352_Base::enable_logical_channel( int ch )
{	
	const uint16_t	setbit	= 0x1 << (ch + 8);
	const uint16_t	bits	= bit_op( AI_MULTI_CH_EN, ~setbit, setbit );

	channel_info_update( bits >> 8 );
}

void NAFE33352_Base::close_logical_channel( int ch )
{	
	const uint16_t	clearingbit	= 0x1 << (ch + 8);
	const uint16_t	bits		= bit_op( AI_MULTI_CH_EN, ~clearingbit, ~clearingbit );

	channel_info_update( bits >> 8 );
}

void NAFE33352_Base::close_logical_channel( void )
{	
	reg( AI_MULTI_CH_EN, 0x0000 );
	channel_info_update( 0x0000 );
}

void NAFE33352_Base::start( int ch )
{
	command( CMD_CH0 + ch );
	command( CMD_SS );
}

void NAFE33352_Base::start( void )
{
	command( CMD_MM );
}

void NAFE33352_Base::start_continuous_conversion( void )
{
	command( CMD_MC );
}

void NAFE33352_Base::DRDY_by_sequencer_done( bool flag )
{
	bit_op( AI_SYSCFG, ~0x0100, flag ? 0x0100 : 0x0000 );	
}

int32_t NAFE33352_Base::read( int ch )
{
	return reg( AI_DATA0 + ch );
}

void NAFE33352_Base::read( raw_t *data )
{
	burst( (uint32_t *)data, enabled_channels );
}

void NAFE33352_Base::read( std::vector<raw_t>& data_vctr )
{
	raw_t	raw_data[ 16 ];
	
	read( raw_data );
	std::copy( raw_data, raw_data + enabled_channels, data_vctr.begin() );
}

void NAFE33352_Base::read( volt_t *data )
{
	raw_t	raw_data[ 16 ];
	
	read( raw_data );
	
	for ( auto i = 0; i < enabled_channels; i++ )
		data[ i ]	= raw2uv( sequence_order[ i ], raw_data[ i ] );
}

void NAFE33352_Base::read( std::vector<volt_t>& data_vctr )
{
	raw_t	raw_data[ 16 ];
	
	read( raw_data );
	
	for ( auto i = 0; i < enabled_channels; i++ )
		data_vctr[ i ]	= raw2uv( sequence_order[ i ], raw_data[ i ] );
}

void NAFE33352_Base::dac_out( double vi, double full_scale, uint8_t bit_length )
{
	reg( AO_DATA, dac_code( vi, full_scale, bit_length ) );
}

int32_t NAFE33352_Base::dac_code( double a, double full_scale, uint8_t bit_length )
{
	int32_t	fsv	= (1L << (bit_length - 1));

	int32_t	v	= (int32_t)((double)fsv * -a / full_scale);

	v	= v < -fsv ? -fsv : v;
	v	= v >  (fsv - 1) ?  (fsv - 1) : v;

	return	v << (24 - bit_length);
}

void NAFE33352_Base::command( uint16_t com )
{
	write_r16( com );
}

void NAFE33352_Base::reg( Register16 r, uint16_t value )
{
	write_r16( static_cast<uint16_t>( r ), value );
}

void NAFE33352_Base::reg( Register24 r, uint32_t value )
{
	write_r24( static_cast<uint16_t>( r ), value );
}

uint16_t NAFE33352_Base::reg( Register16 r )
{
	return read_r16( static_cast<uint16_t>( r ) );
}

uint32_t NAFE33352_Base::reg( Register24 r )
{
	return read_r24( static_cast<uint16_t>( r ) );
}

uint64_t NAFE33352_Base::part_number( void )
{
	return (static_cast<uint64_t>( reg( PN2 ) ) << (16 + 8)) | static_cast<uint64_t>( reg( PN1 ) ) << 8 | reg( PN0_REV ) >> 8;
}

uint8_t NAFE33352_Base::revision_number( void )
{
	return reg( PN0_REV ) & 0xF;
}

uint64_t NAFE33352_Base::serial_number( void )
{
	uint64_t	serial_number;

	serial_number	  = reg( SERIAL1 );
	serial_number	<<=  24;
	return serial_number | reg( SERIAL0 );
}
			
float NAFE33352_Base::temperature( void )
{
	return ((int16_t)reg( DIE_TEMP )) / 64.0;
}

/* NAFE33352 class ******************************************/

NAFE33352::NAFE33352( SPI& spi, bool spi_addr, bool hsv, int nINT, int DRDY, int SYN, int nRESET, int SYNCDAC )
	: NAFE33352_Base( spi, spi_addr, hsv, nINT, DRDY, SYN, nRESET, SYNCDAC )
{
}

NAFE33352::~NAFE33352()
{
}

/* NAFE33352 class ******************************************/

NAFE33352_UIOM::NAFE33352_UIOM( SPI& spi, bool spi_addr, bool hsv, int nINT, int DRDY, int SYN, int nRESET, int SYNCDAC )
	: NAFE33352_Base( spi, spi_addr, hsv, nINT, DRDY, SYN, nRESET, SYNCDAC )
{
}

NAFE33352_UIOM::~NAFE33352_UIOM()
{
}
