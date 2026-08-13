/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include "I2C_Character_LCD.h"
#include <algorithm>
# include <stdarg.h>

I2C_Character_LCD::I2C_Character_LCD( I2C& i2c_, uint8_t addr, const std::vector<uint8_t>& line_select_, size_t width_, uint8_t command_byte_, uint8_t data_byte_ )
	: I2C_device( i2c_, addr ), lines(line_select_.size()), command_byte(command_byte_), data_byte(data_byte_), width(width_), line_select(line_select_)
{
}

void I2C_Character_LCD::send( bool data_flag, uint8_t value )
{
	reg_w( data_flag ? data_byte : command_byte, value );
}

void I2C_Character_LCD::command( uint8_t command )
{
	send( false, command );
	wait_us(27);
	wait_ms(1);
}

void I2C_Character_LCD::data( uint8_t d )
{
	send( true, d );
	wait_ms(1);
}

void I2C_Character_LCD::clear()
{
	command(0x01);
}

void I2C_Character_LCD::putc( char c )
{
	data(static_cast<uint8_t>(c));
}

void I2C_Character_LCD::puts( const char *str, size_t line_num )
{
	char	c;
	
	if (lines == 0)
		return;
	
	size_t line	= line_num % lines;
	
	command( 0x80 | ( line_select[ line ] ) );
	
	while ( (c	= *str++) )
		putc( c );
}

void I2C_Character_LCD::printf( uint8_t line_num, const char *format, ... )
{
	char	s[ MAX_PRINTF_LENGTH ];
	va_list	args;

	va_start( args, format );
	vsnprintf( s, MAX_PRINTF_LENGTH, format, args );
	va_end( args );
	
	puts( s, line_num );
}


// AE_AQM0802
AE_AQM0802::AE_AQM0802( I2C& i2c )
	: I2C_Character_LCD( i2c, DEFAULT_ADDR, std::vector<uint8_t>{ 0x00, 0x40 }, 8 )
{
    // init sequence for 3.3V operation from original
    const std::vector<std::vector<uint8_t>>	init_commands = { { 0x38, 0x39, 0x14, 0x70, 0x56, 0x6C }, { 0x38, 0x0C, 0x01 } };
	
    for ( const auto& seq : init_commands )
	{
        for (auto v : seq) command( v );
        wait_ms(200);
    }
    wait_ms(200);
}

// ACM2004D
ACM2004D_FLW_FBW_IIC::ACM2004D_FLW_FBW_IIC( I2C& i2c )
	: I2C_Character_LCD( i2c, DEFAULT_ADDR, std::vector<uint8_t>{ 0x00, 0x40, 0x14, 0x54 }, 20 )
{
    const std::vector<uint8_t>	init_commands = { 0x38, 0x01, 0x02, 0x0C };
    
	for (auto v : init_commands)
	{
		command(v);
		wait_ms(20);
	}
    wait_ms(200);
}

// ACM1602
ACM1602NI_FLW_FBW::ACM1602NI_FLW_FBW( I2C& i2c )
	: I2C_Character_LCD( i2c, DEFAULT_ADDR, std::vector<uint8_t>{ 0x00, 0xC0 }, 16, 0x00, 0x80 )
{
    const std::vector<uint8_t>	init_commands = { 0x01, 0x38, 0x0F, 0x06 };
    
	for (auto v : init_commands)
	{
		command(v);
		wait_ms(1);
	}
}

AQM0802::AQM0802( I2C& i2c ) : AE_AQM0802( i2c ){}
ACM2004::ACM2004( I2C& i2c ) : ACM2004D_FLW_FBW_IIC( i2c ){}
ACM1602::ACM1602( I2C& i2c ) : ACM1602NI_FLW_FBW( i2c ){}
