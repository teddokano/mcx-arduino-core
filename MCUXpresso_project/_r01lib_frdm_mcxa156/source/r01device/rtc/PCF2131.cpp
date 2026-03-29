/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include	"rtc/RTC_NXP.h"

PCF2131::PCF2131( I2C& interface, uint8_t i2c_address )
	: intfp( new I2C_device( interface, i2c_address ) )
{
}

PCF2131::PCF2131( SPI& interface )
	: intfp( new SPI_for_RTC( interface ) )
{
}

PCF2131::~PCF2131()
{
}

void PCF2131::begin( void )
{
	int_clear();
}


bool PCF2131::oscillator_stop( void )
{
	return intfp->reg_r( Seconds ) & 0x80;
}

time_t PCF2131::rtc_time()
{
	struct tm	now_tm;

	uint8_t		bf[ 8 ];
	
	intfp->reg_r( _100th_Seconds, bf, sizeof( bf ) );
	
	now_tm.tm_sec	= bcd2dec( bf[ 1 ] );
	now_tm.tm_min	= bcd2dec( bf[ 2 ] );
	now_tm.tm_hour	= bcd2dec( bf[ 3 ] );
	now_tm.tm_mday	= bcd2dec( bf[ 4 ] );
	now_tm.tm_mon	= bcd2dec( bf[ 6 ] ) - 1;
	now_tm.tm_year	= bcd2dec( bf[ 7 ] ) + 100;
	now_tm.tm_isdst	= 0;

   return mktime(&now_tm);
}

void PCF2131::set( struct tm* now_tmp )
{
	time_t		now_time;
	struct tm*	cnv_tmp;

	uint8_t		bf[ 8 ];
	
	bf[ 0 ]	= 0;
	bf[ 1 ]	= dec2bcd( now_tmp->tm_sec  );
	bf[ 2 ]	= dec2bcd( now_tmp->tm_min  );
	bf[ 3 ]	= dec2bcd( now_tmp->tm_hour );
	bf[ 4 ]	= dec2bcd( now_tmp->tm_mday );
	bf[ 6 ]	= dec2bcd( now_tmp->tm_mon + 1 );
	bf[ 7 ]	= dec2bcd( now_tmp->tm_year - 100 );

	now_time	= mktime( now_tmp );
	cnv_tmp		= localtime( &now_time );
	bf[ 5 ]		= dec2bcd( cnv_tmp->tm_wday);
	
	intfp->bit_op8( Control_1, ~0x28, 0x20 );
	intfp->bit_op8( SR_Reset,  (uint8_t)(~0x80), 0x80 );

	intfp->reg_w( _100th_Seconds, bf, sizeof( bf ) );

	intfp->bit_op8( Control_1, ~0x20, 0x00 );
}

void PCF2131::alarm( alarm_setting digit, int val )
{
	alarm( digit, val, 0 );
}

void PCF2131::alarm( alarm_setting digit, int val, int int_sel )
{
	int	v = (val == 0x80) ? 0x80 : dec2bcd( val );
	intfp->reg_w( (uint8_t)Second_alarm + (uint8_t)digit, v );
	intfp->bit_op8( int_mask_reg[ int_sel ][ 0 ], ~0x04, 0x00 );
	intfp->bit_op8( Control_2, ~0x02, 0x02 );
}

void PCF2131::alarm_clear( void )
{
	intfp->bit_op8( Control_2, ~0x10, 0x00 );	
}

void PCF2131::alarm_disable( void )
{
	intfp->bit_op8( Control_2, ~0x02, 0x00 );
}

void PCF2131::timestamp( int num, timestanp_setting ts_setting, int int_sel )
{
	const int r_ofst	= 7;
	const int fst		= ts_setting ? 0x80 : 0x00;
	
	num	-=1;

	uint8_t	reg	= Timestp_ctl1 + num * r_ofst;

	intfp->bit_op8( reg, (uint8_t)(~0x80), fst );
	intfp->bit_op8( int_mask_reg[ int_sel ][ 1 ], ~(0x1 << (3 - num)), (0x0 << (3 - num)) );

	intfp->bit_op8( Control_5, ~(0x1 << (7 - num)), (0x1 << (7 - num)) );
}

time_t PCF2131::timestamp( int num )
{
	const int r_ofst	= 7;

	num	-=1;

	uint8_t	reg	= Timestp_ctl1 + num * r_ofst;
	uint8_t	v[ 7 ];
	
	intfp->reg_r( reg, v, sizeof( v ) );
	
	struct tm	ts_tm;

	ts_tm.tm_sec	= bcd2dec( v[ 1 ] );
	ts_tm.tm_min	= bcd2dec( v[ 2 ] );
	ts_tm.tm_hour	= bcd2dec( v[ 3 ] );
	ts_tm.tm_mday	= bcd2dec( v[ 4 ] );
	ts_tm.tm_mon	= bcd2dec( v[ 5 ] ) - 1;
	ts_tm.tm_year	= bcd2dec( v[ 6 ] ) + 100;
	ts_tm.tm_isdst	= 0;

   return mktime(&ts_tm);

}

uint8_t PCF2131::int_clear( void )
{
	uint8_t dummy[ 3 ];
	int_clear( dummy );
	
	return 0; // dummy
}

uint8_t PCF2131::int_clear( uint8_t* rv )
{
	intfp->reg_r( Control_2, rv, 3 );

	if ( rv[ 0 ] & 0x90 )	// if interrupt flag set in Control_2
		intfp->reg_w( Control_2, rv[ 0 ] & ~((rv[ 0 ] & 0x90) | 0x49) );	// datasheet 7.11.5
	
	if ( rv[ 1 ] & 0x08 )	// if interrupt flag set in Control_3
		intfp->reg_w( Control_3, rv[ 1 ] & ~(0x08) );
	
	if ( rv[ 2 ] & 0xF0 )	// if interrupt flag set in Control_4
		intfp->reg_w( Control_4, rv[ 2 ] & ~(rv[ 2 ] & 0xF0) );

	return 0; // dummy
}

void PCF2131::periodic_interrupt_enable( periodic_int_select sel, int int_sel )
{
	if ( !sel ) {
		intfp->bit_op8( Control_1, ~0x03, 0x00 );
		intfp->bit_op8( int_mask_reg[ int_sel ][ 0 ], ~0x30, 0x30 );
		return;
	}
	
	uint8_t v	= (sel == EVERY_MINUTE) ? 0x02 : 0x01;

	v=3;
	intfp->bit_op8( Control_1, ~0x03, v );
	intfp->bit_op8( int_mask_reg[ int_sel ][ 0 ], ~0x30, ~(v << 4) );
}
