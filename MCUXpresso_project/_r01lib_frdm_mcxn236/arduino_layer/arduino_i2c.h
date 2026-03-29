/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_I2C_H
#define R01LIB_ARDUINO_I2C_H

class WireClass
{
public:
	void	begin( int baud = 100000 );
	void	beginTransmission( const uint8_t address );
	size_t	write( uint8_t data );
	size_t	write( const uint8_t *data, size_t length );
	uint8_t	endTransmission( bool stop = false );
	uint8_t	requestFrom( const uint8_t address, const size_t length, bool stop = false );
	uint8_t	available( void );
	uint8_t	read( void );
	
private:
	uint8_t	targ_addr;
	int		baudrate;
	uint8_t	data_buf[ 128 ];
	size_t	data_buf_index;
	size_t	read_size;
};

extern WireClass	Wire;

#endif // !R01LIB_ARDUINO_I2C_H
