/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_I2C_H
#define R01LIB_ARDUINO_I2C_H

class TwoWire
{
public:
	TwoWire( int sda_pin, int scl_pin );
	
	void	begin( int baud = 100000 );
	void	beginTransmission( const uint8_t address );
	size_t	write( uint8_t data );
	size_t	write( const uint8_t *data, size_t length );
	uint8_t	endTransmission( bool stop = true );
	uint8_t	requestFrom( const uint8_t address, const size_t length, bool stop = true );
	int		available( void );
	int		read( void );

private:
	const int	_sda;
	const int	_scl;
	I2C			*i2c;
	uint8_t		targ_addr;
	int			baudrate;
	uint8_t		data_buf[ 128 ];
	size_t		data_buf_index;
	size_t		read_size;
};

extern TwoWire	Wire;

#endif // !R01LIB_ARDUINO_I2C_H
