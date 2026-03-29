/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include	"led/LEDDriver.h"

/* PCA9955B class ******************************************/
PCA9955B::PCA9955B( I2C& interface, uint8_t i2c_address ) : 
	PCA995x_I2C( interface, i2c_address, 16, PCA9955B::PWM0, PCA9955B::IREF0, PCA9955B::IREFALL, access_ref )
{
}

PCA9955B::~PCA9955B()
{
}

void PCA9955B::init( float current )
{
	uint8_t	init[]	= { 0xAA, 0xAA, 0xAA, 0xAA };
	reg_w( 0x80 | LEDOUT0, init, sizeof( init ) );
	write_r8( PWMALL, 0x00 );
	
	irefall( (uint8_t)(current * 255.0) );
}

constexpr uint8_t PCA9955B::access_ref[];
