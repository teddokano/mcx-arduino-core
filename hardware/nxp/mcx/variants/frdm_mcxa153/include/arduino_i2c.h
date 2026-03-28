/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_I2C_H
#define R01LIB_ARDUINO_I2C_H

#include	"i2c.h"

class WireClass
{
public:
	WireClass();

	void	begin( void );
	void	beginTransmission( uint8_t addr );
	void	write( uint8_t data );
	uint8_t	endTransmission( bool stop = true );
	uint8_t	requestFrom( uint8_t addr, uint8_t length, bool stop = true );
	int		read( void );
	int		available( void );

private:
	I2C*		_i2c;
	uint8_t		_addr;
	uint8_t		_tx_buf[ 256 ];
	int			_tx_len;
	uint8_t		_rx_buf[ 256 ];
	int			_rx_len;
	int			_rx_pos;
};

extern WireClass	Wire;

#endif // !R01LIB_ARDUINO_I2C_H
