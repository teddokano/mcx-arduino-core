/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include "TempSensor.h"

/* TempSensor class ******************************************/

TempSensor::TempSensor( I2C& interface, uint8_t i2c_address ) : I2C_device( interface, i2c_address ){}
TempSensor::~TempSensor(){}

template<> 
int16_t TempSensor::read( void )
{
	return read_Temp_register();
};

template<>
float TempSensor::read( void )
{
	return read<int16_t>() / 256.0;
};

float TempSensor::temp()
{
	return read<float>();
}

TempSensor::operator float()
{
	return temp();
}


/* LM75B class ******************************************/

LM75B::LM75B( I2C& interface, uint8_t i2c_address ) : TempSensor( interface, i2c_address ){}
LM75B::~LM75B(){}

int16_t LM75B::read_Temp_register( void )
{
	return (int16_t)read_r16( Temp );
}

void LM75B::thresholds( float v0, float v1 )
{
	float higher	= (v0 < v1) ? v1 : v0;
	float lower		= (v0 < v1) ? v0 : v1;
	
	write_r16( Tos,   ((uint16_t)(higher * 256.0)) & 0xFF80 );
	write_r16( Thyst, ((uint16_t)(lower  * 256.0)) & 0xFF80 );
}

void LM75B::os_mode( mode flag )
{
	bit_op8( Conf, ~0x02, flag << 1 );
}

/* PCT2075 class ******************************************/
PCT2075::PCT2075( I2C& interface, uint8_t i2c_address ) : LM75B( interface, i2c_address ){}
PCT2075::~PCT2075(){}

/* P3T1755 class ******************************************/

P3T1755::P3T1755( I2C& interface, uint8_t i2c_address ) : LM75B( interface, i2c_address ){}
P3T1755::~P3T1755(){}

void P3T1755::thresholds( float v0, float v1 )
{
	float higher	= (v0 < v1) ? v1 : v0;
	float lower		= (v0 < v1) ? v0 : v1;
	
	write_r16( T_HIGH, ((uint16_t)(higher * 256.0)) & 0xFFF0 );
	write_r16( T_LOW,  ((uint16_t)(lower  * 256.0)) & 0xFFF0 );
}

/* P3T1085 class ******************************************/

P3T1085::P3T1085( I2C& interface, uint8_t i2c_address ) : P3T1755( interface, i2c_address ){}
P3T1085::~P3T1085(){}
void P3T1085::os_mode( mode flag )
{
	bit_op16( Conf, ~0x0400, flag << 10 );
}

bool P3T1085::clear( void )
{
	return (read_r16( Conf ) & 0x1000) ? true : false;
}

/* P3T1035 class ******************************************/

P3T1035::P3T1035( I2C& interface, uint8_t i2c_address ) : P3T1755( interface, i2c_address ){}
P3T1035::~P3T1035(){}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void P3T1035::os_mode( mode flag )
{
	//	Do nothing since this device doesn't have "Thermostat Mode"
}
#pragma GCC diagnostic pop

/* P3T2030 class ******************************************/

P3T2030::P3T2030( I2C& interface, uint8_t i2c_address ) : P3T1035( interface, i2c_address ){}
P3T2030::~P3T2030(){}
