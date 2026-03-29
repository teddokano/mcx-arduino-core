/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"r01lib.h"
#include	"arduino.h"

WireClass	Wire;

WireClass::WireClass()
	: _i2c( nullptr ), _addr( 0 ), _tx_len( 0 ), _rx_len( 0 ), _rx_pos( 0 )
{
}

void WireClass::begin( void )
{
	if ( !_i2c )
		_i2c = new I2C( I2C_SDA, I2C_SCL );
}

void WireClass::beginTransmission( uint8_t addr )
{
	_addr	= addr;
	_tx_len	= 0;
}

void WireClass::write( uint8_t data )
{
	if ( _tx_len < (int)sizeof(_tx_buf) )
		_tx_buf[ _tx_len++ ] = data;
}

uint8_t WireClass::endTransmission( bool stop )
{
	if ( !_i2c ) return 4;
	// i2c.h: write( uint8_t address, const uint8_t *dp, int length, bool stop )
	int r = _i2c->write( (_addr << 1), (const uint8_t*)_tx_buf, _tx_len, !stop );
	return ( r == 0 ) ? 0 : 2;
}

uint8_t WireClass::requestFrom( uint8_t addr, uint8_t length, bool stop )
{
	if ( !_i2c ) return 0;
	_rx_pos = 0;
	_rx_len = 0;
	// i2c.h: read( uint8_t address, uint8_t *dp, int length, bool stop )
	int r = _i2c->read( (addr << 1), (uint8_t*)_rx_buf, length, !stop );
	if ( r == 0 )
		_rx_len = length;
	return _rx_len;
}

int WireClass::read( void )
{
	if ( _rx_pos < _rx_len )
		return _rx_buf[ _rx_pos++ ];
	return -1;
}

int WireClass::available( void )
{
	return _rx_len - _rx_pos;
}
