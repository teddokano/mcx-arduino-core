/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include	"gpio/PORT.h"

PORT::PORT( GPIO_base& gpio, int port_num, uint8_t _mask )
	: dev( gpio ), pn( port_num ), mask( _mask )
{
}

void PORT::config( uint8_t config )
{
	dev.config( pn, config );
}

void PORT::set( access_word wd, uint8_t value )
{
	dev.write_port( wd, value, pn );
}

PORT& PORT::operator=( uint8_t v )
{
	dev.output( pn, v );
	return *this;
}

PORT& PORT::operator=( PORT& )
{
	return *this;
}

PORT::operator int()
{
	return dev.input( pn );
}

GPIO_PORT::GPIO_PORT( GPIO_base& gpio, int port_num, uint8_t _mask ) : PORT( gpio, port_num, _mask ){}
