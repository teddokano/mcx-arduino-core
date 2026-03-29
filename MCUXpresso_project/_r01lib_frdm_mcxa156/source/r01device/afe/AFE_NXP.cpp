/** NXP Analog Front End class library for MCX
 *
 *  @author  Tedd OKANO
 *
 *  Copyright: 2023 - 2025 Tedd OKANO
 *  Released under the MIT license
 */

#include	"AFE_NXP.h"
#include	"r01lib.h"
#include	<math.h>

using enum	NAFE13388_Base::Register16;
using enum	NAFE13388_Base::Register24;
using enum	NAFE13388_UIM::Command;

double	AFE_base::delay_accuracy	= 1.1;

/* AFE_base class ******************************************/

AFE_base::AFE_base( SPI& spi, bool spi_addr, bool hsv, int nINT, int DRDY, int SYN, int nRESET ) : 
	SPI_for_AFE( spi, spi_addr ), highspeed_variant( hsv ), pin_nINT( nINT ), pin_DRDY( DRDY ), pin_SYN( SYN ), pin_nRESET( nRESET, 1 ), enabled_channels( 0 )
{
}

AFE_base::~AFE_base()
{
}

void AFE_base::init( void )
{
	pin_DRDY.rise( DRDY_cb );
	
	drdy_flag		= false;
	set_DRDY_callback( [this](void){ default_drdy_cb(); } );
}

void AFE_base::begin( void )
{
	reset();
	boot();
	init();
}

void AFE_base::set_DRDY_callback( callback_fp_t func )
{
	cbf_DRDY		= func;
}

void AFE_base::DRDY_cb( void )
{
	if ( cbf_DRDY )
		cbf_DRDY();
}

void AFE_base::default_drdy_cb( void )
{
	drdy_count++;
	drdy_flag		= true;
}

int32_t AFE_base::start_and_read( int ch )
{
	double	wait_time	= cbf_DRDY ? -1.0 : ch_delay[ ch ] * delay_accuracy;
	
	start( ch );
	wait_conversion_complete( wait_time );
	
	return read( ch );
};

#ifdef	NON_TEMPLATE_VERSION_FOR_START_AND_READ
void AFE_base::start_and_read( raw_t* data )
{
	double	wait_time	= cbf_DRDY ? -1.0 : total_delay * delay_accuracy;
	
	start();
	wait_conversion_complete( wait_time );
	
	read( data );
};

void AFE_base::start_and_read( std::vector<raw_t>& data )
{
	double	wait_time	= cbf_DRDY ? -1.0 : total_delay * delay_accuracy;
	
	start();
	wait_conversion_complete( wait_time );
	
	read( data );
};
#else
template void AFE_base::start_and_read( raw_t* data );
template void AFE_base::start_and_read( std::vector<raw_t>& data );
#endif

int AFE_base::bit_count( uint32_t value )
{
	constexpr int	bit_length	= 32;
	int				count	= 0;
		
	for ( int i = 0; i < bit_length; i++ ) {
		if ( value & (0x1 << i) )
			count++;
	}
	
	return count;
}

int AFE_base::wait_conversion_complete( double delay )
{
	if ( 0 < delay )
	{
		wait( delay * delay_accuracy );
		return	0;
	}

	auto	timeout_count	= timeout_limit;

	while ( !drdy_flag && --timeout_count )
		;

	drdy_flag	= false;
	
	if ( !timeout_count )
	{
		printf( "DRDY signal wait timeout\r\n" );
		return	-1;
	}
	return	0;
}

void AFE_base::use_DRDY_trigger( bool use )
{
	if ( use )
		set_DRDY_callback( [this](void){ default_drdy_cb(); } );
	else
		set_DRDY_callback( nullptr );
}


AFE_base::callback_fp_t	AFE_base::cbf_DRDY		= nullptr;

/* NAFE13388_Base class ******************************************/

NAFE13388_Base::NAFE13388_Base( SPI& spi, bool spi_addr, bool hsv, int nINT, int DRDY, int SYN, int nRESET ) 
	: AFE_base( spi, spi_addr, hsv, nINT, DRDY, SYN, nRESET )
{
}

NAFE13388_Base::~NAFE13388_Base()
{
}

void NAFE13388_Base::boot( void )
{
	command( CMD_ABORT ); 
	wait( 0.001 );
	
	DRDY_by_sequencer_done( true );
}

void NAFE13388_Base::reset( bool hardware_reset )
{
	if ( hardware_reset )
	{
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
		if ( reg( SYS_STATUS0 ) & CHIP_READY )
			return;
	}
	
	panic( "NAFE13388 couldn't get ready. Check power supply or pin conections\r\n" );
}

void NAFE13388_Base::open_logical_channel( int ch, const uint16_t (&cc)[ 4 ] )
{	
	command( ch );
	
	for ( auto i = 0; i < 4; i++ )
		reg( CH_CONFIG0 + i, cc[ i ] );
	
	const uint16_t	setbit	= 0x1 << ch;
	const uint16_t	bits	= bit_op( CH_CONFIG4, ~setbit, setbit );
	
	if ( cc[ 0 ] & 0x0010 )
		coeff_uV[ ch ]	= ((10.0 / (double)(1L << 24)) / pga_gain[ (cc[ 0 ] >> 5) & 0x7 ]) * 1e6;
	else
		coeff_uV[ ch ]	= (4.0 / (double)(1L << 24)) * 1e6;
	
	ch_delay[ ch ]		= calc_delay( ch );
	channel_info_update( bits );
}

void NAFE13388_Base::channel_info_update( uint16_t value )
{
	constexpr auto	bit_length	= 16;
	enabled_channels			= 0;
	total_delay					= 0.00;
		
	for ( auto i = 0; i < bit_length; i++ )
	{
		if ( value & (0x1 << i) )
		{
			enabled_channels++;
			total_delay	+= ch_delay[ i ];
		}
	}
}

double NAFE13388_Base::calc_delay( int ch )
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
	
	command( ch );

	uint16_t ch_config1	= reg( CH_CONFIG1 );
	uint16_t ch_config2	= reg( CH_CONFIG2 );
	
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
	
	if ( !adc_normal_setting  )
		base_freq	/= (adc_sinc + 1);
	
	if ( ch_chop )
		base_freq	/= 2;
	
#if 0
	printf( "base_freq = %lf\r\n", base_freq );
	printf( "delay_setting = %lf\r\n", delay_setting  );
	printf( "channel delay = %lf\r\n", (1 / base_freq) + delay_setting  );
#endif
	
	return (1 / base_freq) + delay_setting;
}


void NAFE13388_Base::open_logical_channel( int ch, uint16_t cc0, uint16_t cc1, uint16_t cc2, uint16_t cc3 )
{	
	const ch_setting_t	tmp_ch_config	= { cc0, cc1, cc2, cc3 };
	open_logical_channel( ch, tmp_ch_config );
}

void NAFE13388_Base::close_logical_channel( int ch )
{	
	const uint16_t	clearingbit	= 0x1 << ch;
	const uint16_t	bits		= bit_op( CH_CONFIG4, ~clearingbit, ~clearingbit );

	channel_info_update( bits );
}

void NAFE13388_Base::close_logical_channel( void )
{	
	reg( CH_CONFIG4, 0x0000 );
	channel_info_update( 0x0000 );
}

void NAFE13388_Base::start( int ch )
{
	command( ch     );
	command( CMD_SS );
}

void NAFE13388_Base::start( void )
{
	command( CMD_MM );
}

void NAFE13388_Base::start_continuous_conversion( void )
{
	command( CMD_MC );
}

void NAFE13388_Base::DRDY_by_sequencer_done( bool flag )
{
	bit_op( SYS_CONFIG0, ~0x0010, flag ? 0x0010 : 0x00 );	
}

int32_t NAFE13388_Base::read( int ch )
{
	return reg( CH_DATA0 + ch );
}

void NAFE13388_Base::read( raw_t *data )
{
	burst( (uint32_t *)data, enabled_channels );
}

void NAFE13388_Base::read( std::vector<raw_t>& data_vctr )
{
	raw_t	data[ 16 ];
	
	burst( (uint32_t *)data, enabled_channels );
	std::copy( data, data + enabled_channels, data_vctr.begin() );
}

void NAFE13388_Base::command( uint16_t com )
{
	write_r16( com );
}

void NAFE13388_Base::reg( Register16 r, uint16_t value )
{
	write_r16( static_cast<uint16_t>( r ), value );
}

void NAFE13388_Base::reg( Register24 r, uint32_t value )
{
	write_r24( static_cast<uint16_t>( r ), value );
}

uint16_t NAFE13388_Base::reg( Register16 r )
{
	return read_r16( static_cast<uint16_t>( r ) );
}

uint32_t NAFE13388_Base::reg( Register24 r )
{
	return read_r24( static_cast<uint16_t>( r ) );
}

uint32_t NAFE13388_Base::part_number( void )
{
	return (static_cast<uint32_t>( reg( PN2 ) ) << 16) | reg( PN1 );
}

uint8_t NAFE13388_Base::revision_number( void )
{
	return reg( PN0 ) & 0xF;
}

uint64_t NAFE13388_Base::serial_number( void )
{
	uint64_t	serial_number;

	serial_number	  = reg( SERIAL1 );
	serial_number	<<=  24;
	return serial_number | reg( SERIAL0 );
}
			
float NAFE13388_Base::temperature( void )
{
	return reg( DIE_TEMP ) / 64.0;
}

void NAFE13388_Base::gain_offset_coeff( const ref_points &ref )
{
	constexpr double	pga1x_voltage		= 5.0;
	constexpr int		adc_resolution		= 24;
	constexpr double	pga_gain_setting	= 0.2;

	constexpr double	fullscale_voltage	= pga1x_voltage / pga_gain_setting;

	double	fullscale_data		= pow( 2, (adc_resolution - 1) );
	double	ref_data_span		= ref.high.data		- ref.low.data;
	double	ref_voltage_span	= ref.high.voltage	- ref.low.voltage;
	
	double	dv_slope			= ref_data_span / ref_voltage_span;
	double	custom_gain			= dv_slope * (fullscale_voltage / fullscale_data);
	double	custom_offset		= (dv_slope * ref.low.voltage - ref.low.data) / custom_gain;
	
	int32_t	gain_coeff_cal		= reg( GAIN_COEFF0   + ref.cal_index );
	int32_t	offsset_coeff_cal	= reg( OFFSET_COEFF0 + ref.cal_index );
	int32_t	gain_coeff_new		= round( gain_coeff_cal * custom_gain );
	int32_t	offset_coeff_new	= round( custom_offset - offsset_coeff_cal );

#if 0
	printf( "ref_point_high = %8ld @%6.3lf\r\n", ref.high.data, ref.high.voltage );
	printf( "ref_point_low  = %8ld @%6.3lf\r\n", ref.low.data,  ref.low.voltage  );
	printf( "gain_coeff_new   = %8ld\r\n", gain_coeff_new   );
	printf( "offset_coeff_new = %8ld\r\n", offset_coeff_new );
#endif
	
	reg( GAIN_COEFF0   + ref.coeff_index, gain_coeff_new   );
	reg( OFFSET_COEFF0 + ref.coeff_index, offset_coeff_new );
}

int NAFE13388_Base::self_calibrate( int pga_gain_index, int channel_selection, int input_select, double reference_source_voltage, bool use_positive_side )
{
	constexpr	auto	low_gain_index	= 2;
	auto				channel_in_use	= false;
	ch_setting_t		tmp_ch_config;
	int					gain_index		= static_cast<int>( pga_gain_index );
	
	//	logical channel selection to perform the self-calibration
	//	if the chennel in-use, save channel setting to temporal memory
	
	if ( reg( CH_CONFIG4 ) & (0x1 << channel_selection) )
	{
		channel_in_use	= true;
		
		command( channel_selection );

		for ( auto i = 0; i < 4; i++ )
			tmp_ch_config[ i ]	= reg( CH_CONFIG0 + i );
	}
	
	//	if user doesn't specify the channel and voltage, use REFH or REFL
	
	if ( !input_select )
	{
		bool	low_gain	= (gain_index <= low_gain_index);

		input_select				= low_gain ? 0x5 : 0x6;
		reference_source_voltage	= (reg( low_gain ? OPT_COEF1 : OPT_COEF2 ) * 5.00) / (double)(1 << 24);

#if 1
		printf( "==== self-calibration for PGA gain setting: x%3.1lf\r\n", pga_gain[ gain_index ] );
		printf( "gain = %s\r\n", low_gain ? "low" : "high" );
		printf( "REF%s = %10.8lfV\r\n", low_gain ? "H" : "L", reference_source_voltage );
#endif
	}
	
	//	logical channel settings
	//	Total 3 settings are prepared to measure reference_voltage, internal-GND and AICOM
	
	const uint16_t		REF_GND		= 0x0011  | (gain_index << 5);
	const uint16_t		REF_V		= (input_select << (use_positive_side ? 12 : 8)) | REF_GND;
	const uint16_t		REF_COM		= 0x7700 | REF_GND;
	const uint16_t		ch_config1	= (gain_index << 12) | 0x00E4;
	constexpr uint16_t	ch_config2	= 0x8480;
	constexpr uint16_t	ch_config3	= 0x0000;

	const ch_setting_t	refh	= { REF_V,   ch_config1, ch_config2,           ch_config3 };
	const ch_setting_t	refg	= { REF_GND, ch_config1, ch_config2,           ch_config3 };
	const ch_setting_t	refc	= { REF_COM, ch_config1, ch_config2 & ~0x0080, ch_config3 };	//CH_CHOP:off

	//	forcing to set unity-gain and zero-offset

	constexpr raw_t	default_gain_coeff_value	= 0x1 << 22;
	constexpr raw_t	default_offset_coeff_value	= 0;

	reg( GAIN_COEFF0   + gain_index, default_gain_coeff_value   );
	reg( OFFSET_COEFF0 + gain_index, default_offset_coeff_value );
	
	//	measure the logical channel with those different 3 settings
	
	open_logical_channel( channel_selection, refh );	
	raw_t	data_REF	= start_and_read( channel_selection );

	open_logical_channel( channel_selection, refg );
	raw_t	data_GND	= start_and_read( channel_selection );

	open_logical_channel( channel_selection, refc );
	raw_t	data_COM	= start_and_read( channel_selection );

	//	calculation
	
	const double	fullscale_voltage	= 5.00 / pga_gain[ gain_index ];
	const double	calibrated_gain		= (double)(0x1 << 23) * (reference_source_voltage / fullscale_voltage) / (double)(data_REF - data_GND);

#if 0
	printf( "data_REF = %8ld (%lfV)\r\n",  data_REF, raw2v(  channel_selection, data_REF ) );
	printf( "data_GND = %8ld (%lfmV)\r\n", data_GND, raw2mv( channel_selection, data_GND ) );
	printf( "data_COM = %8ld (%lfmV)\r\n", data_COM, raw2mv( channel_selection, data_COM ) );
	printf( "gain adjustment = %8lf (%lfdB)\r\n\r\n", calibrated_gain, 20 * log10( calibrated_gain ) );
#endif
	
	if ( !( (0.95 < calibrated_gain) && (calibrated_gain < 1.05) ) )
		return CalibrationError::GainError;
	
	const double	offset_mv	= raw2mv( channel_selection, data_COM );
	
	if ( !( (-10.0 < offset_mv) && (offset_mv < 10.0) ) )
		return CalibrationError::OffsetError;
	
	//	setting registers: GAIN_COEFF[n] and OFFSET_COEFF[n]
	
	reg( GAIN_COEFF0   + gain_index, (uint32_t)(default_gain_coeff_value * calibrated_gain) );
	reg( OFFSET_COEFF0 + gain_index, default_offset_coeff_value + data_COM );

	//	if the channel was in-use, revert the setting
	
	if ( channel_in_use )
		open_logical_channel( channel_selection, tmp_ch_config );
	else
		close_logical_channel( channel_selection );

	return CalibrationError::NoError;
}

void NAFE13388_Base::blink_leds( void )
{
}



/* NAFE13388 class ******************************************/

NAFE13388::NAFE13388( SPI& spi, bool spi_addr, bool hsv, int nINT, int DRDY, int SYN, int nRESET ) 
	: NAFE13388_Base( spi, spi_addr, hsv, nINT, DRDY, SYN, nRESET )
{
}

NAFE13388::~NAFE13388()
{
}

/* NAFE13388_UIM class ******************************************/

NAFE13388_UIM::NAFE13388_UIM( SPI& spi, bool spi_addr, bool hsv, int nINT, int DRDY, int SYN, int nRESET ) 
	: NAFE13388_Base( spi, spi_addr, hsv, nINT, DRDY, SYN, nRESET )
{
}

NAFE13388_UIM::~NAFE13388_UIM()
{
}

void NAFE13388_UIM::blink_leds( void )
{
	std::vector<uint16_t>	pattern	= {
			0x8000, 0x0040, 0x0100, 0x0080, 0x0200, 0x0400, 0x0800, 0x1000,
			0x2000, 0x4000, 0x2000, 0x1000, 0x0800, 0x0400, 0x0200, 0x0080,
			0x0100, 0x0040,
	};
	reg( GPIO_CONFIG0, 0xFFC0 );
	reg( GPIO_CONFIG1, 0xFFC0 );
	reg( GPIO_CONFIG2, 0x0000 );

	for ( auto i = 0; i < 2; i++ )
		for_each( pattern.begin(), pattern.end(), [ this ]( auto p ){ reg( GPO_DATA, p ); wait( 0.02 ); } );

	pattern.resize( 10 );
	uint16_t	pv	= 0;

	for ( auto i = 0; i < 4; i++ )
		for_each(
			pattern.begin(),
			pattern.end(),
			[ &pv, i, this ]( auto p ){
				pv	= (i % 2) ? pv & ~p : pv | p;
				reg( GPO_DATA, pv ); wait( 0.02 );
			}
		);
}

//double	NAFE13388::coeff_uV[ 16 ];
