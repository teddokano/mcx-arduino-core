/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include	"led/LEDDriver.h"

/* PCA9957 class ******************************************/
PCA9957::PCA9957( SPI& interface ) : 
	PCA995x_SPI( interface, 24, PCA9957::PWM0, PCA9957::IREF0, PCA9957::IREFALL, access_ref )
{
}

PCA9957::~PCA9957()
{
}

void PCA9957::init( float current )
{
	uint8_t	init[]	= { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };

	write_r8( MODE2,  0x18 );
	write_r8( PWMALL, 0x00 );
	reg_w( LEDOUT0, init, sizeof( init ) );
	
	irefall( (uint8_t)(current * 255.0) );
	
}

constexpr uint8_t PCA9957::access_ref[];
