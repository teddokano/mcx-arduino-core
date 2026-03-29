/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include	"rtc/RTC_NXP.h"

PCF85063_base::PCF85063_base()
{
}

PCF85063_base::~PCF85063_base()
{	
}

void PCF85063_base::begin( void )
{
}

void PCF85063_base::set( struct tm* now_tmp )
{
	time_t		now_time;
	struct tm*	cnv_tmp;

	uint8_t		bf[ 7 ];
	
	bf[ 0 ]	= dec2bcd( now_tmp->tm_sec  );
	bf[ 1 ]	= dec2bcd( now_tmp->tm_min  );
	bf[ 2 ]	= dec2bcd( now_tmp->tm_hour );
	bf[ 3 ]	= dec2bcd( now_tmp->tm_mday );
	bf[ 5 ]	= dec2bcd( now_tmp->tm_mon + 1 );
	bf[ 6 ]	= dec2bcd( now_tmp->tm_year - 100 );

	now_time	= mktime( now_tmp );
	cnv_tmp		= localtime( &now_time );
	bf[ 4 ]		= dec2bcd( cnv_tmp->tm_wday);
	
	_bit_op8( Control_1, ~0x20, 0x20 );
	_reg_w( Seconds, bf, sizeof( bf ) );
	_bit_op8( Control_1, ~0x20, 0x00 );
}

bool PCF85063_base::oscillator_stop( void )
{
	return _reg_r( Seconds ) & 0x80;
}

void PCF85063_base::alarm( alarm_setting digit, int val )
{
	int	v = (val == 0x80) ? 0x80 : dec2bcd( val );
	_reg_w( (uint8_t)Second_alarm + (uint8_t)digit, v );
	_bit_op8( Control_2, (uint8_t)(~0x80), 0x80 );
}

void PCF85063_base::alarm_clear( void )
{
	//	will be implemented later
}

void PCF85063_base::alarm_disable( void )
{
	_bit_op8( Control_2, (uint8_t)(~0x80), 0x00 );	
}

uint8_t PCF85063_base::int_clear( void )
{
	uint8_t v = _reg_r( Control_2 );
	_reg_w( Control_2, v & ~0x48 );

	return v;
}

float PCF85063_base::timer( float period )
{
	float	sf[] = { 1 / 4096.0, 1 / 64.0, 1.0, 60 };
	int		tcf;
	
	period	= ((255 * 60.0) < period) ? 255 * 60.0 : period;
	
	for ( tcf = 0; tcf < 4; tcf++ ) {
		if ( period <= sf[ tcf ] * 255 )
			break;
	}

	uint8_t	v	= (uint8_t)(period / sf[ tcf ]);

#if 0
	Serial.print("  period:");
	Serial.print( period, 10 );
	Serial.print("  tcf:");
	Serial.print( tcf );
	Serial.print("  v:");
	Serial.print( v );
	Serial.print("  v*tcf:");
	Serial.print( v * sf[tcf], 10 );
	Serial.println( "" );
#endif
	
	_reg_w( Timer_value, v );
	_reg_w( Timer_mode, tcf << 3 | 0x06 );
	
	return v * sf[tcf];
}

time_t PCF85063_base::rtc_time( void )
{
	struct tm	now_tm;

	uint8_t		bf[ 7 ];
	
	_reg_r( Seconds, bf, sizeof( bf ) );
	
	now_tm.tm_sec	= bcd2dec( bf[ 0 ] );
	now_tm.tm_min	= bcd2dec( bf[ 1 ] );
	now_tm.tm_hour	= bcd2dec( bf[ 2 ] );
	now_tm.tm_mday	= bcd2dec( bf[ 3 ] );
	now_tm.tm_mon	= bcd2dec( bf[ 5 ] ) - 1;
	now_tm.tm_year	= bcd2dec( bf[ 6 ] ) + 100;
	now_tm.tm_isdst	= 0;

   return mktime(&now_tm);
}


PCF85063A::PCF85063A( I2C& interface, uint8_t i2c_address ) : I2C_device( interface, i2c_address )
{
}

PCF85063A::~PCF85063A()
{	
}

void PCF85063A::_reg_w( uint8_t reg, uint8_t *vp, int len )
{
	reg_w( reg, vp, len );
}

void PCF85063A::_reg_r( uint8_t reg, uint8_t *vp, int len )
{
	reg_r( reg, vp, len );
}

void PCF85063A::_reg_w( uint8_t reg, uint8_t val )
{
	reg_w( reg, val );
}

uint8_t PCF85063A::_reg_r( uint8_t reg )
{
	return 	reg_r( reg );
}

void PCF85063A::_bit_op8( uint8_t reg, uint8_t mask, uint8_t val )
{
	bit_op8( reg, mask, val );
}
