/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include "test_LM75B.h"

/* LM75B class ******************************************/

test_LM75B::test_LM75B( I2C& interface, const uint8_t i2c_address ) : I2C_device( interface, i2c_address )
{
}

test_LM75B::~test_LM75B()
{
}

float test_LM75B::read()
{
	return read_r16( Temp ) / 256.0;
}

void test_LM75B::thresholds( const float v0, const float v1 )
{
	float higher	= (v0 < v1) ? v1 : v0;
	float lower		= (v0 < v1) ? v0 : v1;
	
	write_r16( Tos,   ((uint16_t)(higher * 256.0)) & 0xFF80 );
	write_r16( Thyst, ((uint16_t)(lower  * 256.0)) & 0xFF80 );
}

void test_LM75B::os_mode( const mode flag )
{
	uint8_t	v;
	
	v	 = read_r8( Conf );
	v	&= ~0x02;
	v	|= flag << 1;
	
	write_r8( Conf, v );
}

