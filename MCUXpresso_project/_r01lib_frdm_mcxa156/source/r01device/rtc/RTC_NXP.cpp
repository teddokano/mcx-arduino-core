/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include	"rtc/RTC_NXP.h"

RTC_NXP::RTC_NXP()
{
}

RTC_NXP::~RTC_NXP()
{
}

time_t RTC_NXP::time( time_t* tp )
{
	time_t	t	= rtc_time();
	if ( tp )
		*tp	= t;
	return t;
}

uint8_t	RTC_NXP::bcd2dec( uint8_t v )
{
	return (v >> 4) * 10 + (v & 0x0F);
}

uint8_t	RTC_NXP::dec2bcd( uint8_t v )
{
	return ((v / 10) << 4) + (v % 10);
}

/*
ForFutureExtention::ForFutureExtention(){}
ForFutureExtention::~ForFutureExtention(){}
void ForFutureExtention::begin( void ){}
void ForFutureExtention::set( struct tm* now_tm ){}
bool ForFutureExtention::oscillator_stop( void ){}
void ForFutureExtention::alarm( alarm_setting digit, int val ){}
void ForFutureExtention::alarm_clear( void ){}
void ForFutureExtention::alarm_disable( void ){}
uint8_t ForFutureExtention::int_clear( void ){}
time_t ForFutureExtention::rtc_time( void ){}
*/

