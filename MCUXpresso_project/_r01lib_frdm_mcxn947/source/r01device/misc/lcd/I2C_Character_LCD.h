/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef ARDUINO_I2C_CHARACTER_LCD
#define ARDUINO_I2C_CHARACTER_LCD

#include "r01lib.h"
#include "I2C_device.h"
#include <cstdint>
#include <string>
#include <vector>

static constexpr uint8_t	MAX_PRINTF_LENGTH = 128;

class I2C_Character_LCD : public I2C_device
{
public:
	I2C_Character_LCD( I2C& i2c, uint8_t address, const std::vector<uint8_t>& line_select, size_t width = 16, uint8_t command_byte = 0x00, uint8_t data_byte = 0x40 );
	virtual	~I2C_Character_LCD() = default;

	void	send( bool data_flag, uint8_t value );
	void	command( uint8_t command );
	void	data( uint8_t data );
	void	clear( void );
	void	putc( char c );
	void	puts( const char* str, size_t line_num = 0 );
	//void	printf( const char* format, ... );
	void	printf( uint8_t line_num, const char* format, ... );

protected:
	size_t	lines;
	uint8_t	command_byte;
	uint8_t	data_byte;
	size_t	width;
	std::vector<uint8_t> line_select;
};

// Device-specific derived classes
class AE_AQM0802 : public I2C_Character_LCD {
public:
	static constexpr uint8_t DEFAULT_ADDR = (0x7C >> 1);
	AE_AQM0802( I2C& i2c );
};

class ACM2004D_FLW_FBW_IIC : public I2C_Character_LCD {
public:
	static constexpr uint8_t DEFAULT_ADDR = (0x7E >> 1);
	ACM2004D_FLW_FBW_IIC( I2C& i2c );
};

class ACM1602NI_FLW_FBW : public I2C_Character_LCD {
public:
	static constexpr uint8_t DEFAULT_ADDR = (0xA0 >> 1);
	ACM1602NI_FLW_FBW( I2C& i2c );
};

class AQM0802 : public AE_AQM0802 {
public:
	AQM0802( I2C& i2c );
};

class ACM2004 : public ACM2004D_FLW_FBW_IIC {
public:
	ACM2004( I2C& i2c );
};

class ACM1602 : public ACM1602NI_FLW_FBW {
public:
	ACM1602( I2C& i2c );
};


#endif // !ARDUINO_I2C_CHARACTER_LCD
