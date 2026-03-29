/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include	"rtc/RTC_NXP.h"

PCF85053A::PCF85053A( I2C& interface, uint8_t i2c_address ) : I2C_device( interface, i2c_address ){}
PCF85053A::~PCF85053A(){}

void PCF85053A::begin( void ){}

void PCF85053A::set( struct tm* now_tmp )
{
	time_t		now_time;
	struct tm*	cnv_tmp;
	uint8_t		bf[ 10 ];
	
	bf[ 0 ]	= dec2bcd( now_tmp->tm_sec  );
	bf[ 1 ]	= 0xFF;	//	Seconds_alarm clear
	bf[ 2 ]	= dec2bcd( now_tmp->tm_min  );
	bf[ 3 ]	= 0xFF;	//	Minutes_alarm clear
	bf[ 4 ]	= dec2bcd( now_tmp->tm_hour );
	bf[ 5 ]	= 0xFF;	//	Hours_alarm clear
	bf[ 7 ]	= dec2bcd( now_tmp->tm_mday );
	bf[ 8 ]	= dec2bcd( now_tmp->tm_mon + 1 );
	bf[ 9 ]	= dec2bcd( now_tmp->tm_year - 100 );

	now_time	= mktime( now_tmp );
	cnv_tmp		= localtime( &now_time );
	bf[ 6 ]		= dec2bcd( cnv_tmp->tm_wday );
	
	reg_w( Seconds, bf, sizeof( bf ) );
}

bool PCF85053A::oscillator_stop( void )
{
	return reg_r( Status_Register ) & 0x40;
	bit_op8( Status_Register, ~0x40, 0x40 );
}

void PCF85053A::alarm( alarm_setting digit, int val )
{
	reg_w( alarm_offsets[ digit ], dec2bcd( val ) );
	bit_op8( Control_Register, ~0x08, 0x08 );
}

void PCF85053A::alarm_clear( void )
{
	reg_w( alarm_offsets[ SECOND ], 0xFF );
	reg_w( alarm_offsets[ MINUTE ], 0xFF );
	reg_w( alarm_offsets[ HOUR   ], 0xFF );
}

void PCF85053A::alarm_disable( void )
{
	bit_op8( Control_Register, ~0x08, 0x00 );
}

uint8_t PCF85053A::int_clear( void )
{
	uint8_t	status	= reg_r( Status_Register );
	
	reg_w( Status_Register, ~(status & 0xF0) );
	return status;
}

time_t PCF85053A::rtc_time( void )
{
	struct tm	now_tm;
	uint8_t		bf[ 10 ];
	
	reg_r( Seconds, bf, sizeof( bf ) );
	
	now_tm.tm_sec	= bcd2dec( bf[ 0 ] );
	now_tm.tm_min	= bcd2dec( bf[ 2 ] );
	now_tm.tm_hour	= bcd2dec( bf[ 4 ] );
	now_tm.tm_mday	= bcd2dec( bf[ 7 ] );
	now_tm.tm_mon	= bcd2dec( bf[ 8 ] ) - 1;
	now_tm.tm_year	= bcd2dec( bf[ 9 ] ) + 100;
	now_tm.tm_isdst	= 0;

   return mktime( &now_tm );
}

int PCF85053A::alarm_offsets[ 3 ]	= { 1, 3, 5 };
