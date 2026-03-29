/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include	"rtc/RTC_NXP.h"

void warning( void )
{
//	Serial.println( "** NO HARDWARE SUPPORT AVAILABLE FOR PCF85063TP **" );	
}

PCF85063TP::PCF85063TP( I2C& interface, uint8_t i2c_address ) : PCF85063A( interface, i2c_address )
{
}

PCF85063TP::~PCF85063TP()
{	
}
