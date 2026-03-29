/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include	"fsl_common.h"
#include	"fsl_gpio.h"
#include	"fsl_port.h"
#include	"pin_mux.h"
#include	"mcu.h"
#include	"io.h"

#define	DISABLED_GPIO		-1
#define	DISABLED_PIN		0xFF

GPIO_Type*	gpio_type[]	= GPIO_BASE_PTRS;
PORT_Type*	port_type[]	= PORT_BASE_PTRS;

typedef	struct	_gpio_pin {
	int			base;
	uint32_t	pin;
} gpio_pin;

#ifdef	CPU_MCXN947VDF
static gpio_pin pins[]	= {
	{ DISABLED_GPIO, DISABLED_PIN },
	{ 0,  0 },
	{ 0,  1 },
	{ 0,  2 },
	{ 0,  3 },
	{ 0,  4 },
	{ 0,  5 },
	{ 0,  6 },
	{ 0,  7 },
	{ 0,  8 },
	{ 0,  9 },
	{ 0, 10 },
	{ 0, 11 },
	{ 0, 12 },
	{ 0, 13 },
	{ 0, 14 },
	{ 0, 15 },
	{ 0, 16 },
	{ 0, 17 },
	{ 0, 18 },
	{ 0, 19 },
	{ 0, 20 },
	{ 0, 21 },
	{ 0, 22 },
	{ 0, 23 },
	{ 0, 24 },
	{ 0, 25 },
	{ 0, 26 },
	{ 0, 27 },
	{ 0, 28 },
	{ 0, 29 },
	{ 0, 30 },
	{ 0, 31 },
	{ 1,  0 },
	{ 1,  1 },
	{ 1,  2 },
	{ 1,  3 },
	{ 1,  4 },
	{ 1,  5 },
	{ 1,  6 },
	{ 1,  7 },
	{ 1,  8 },
	{ 1,  9 },
	{ 1, 10 },
	{ 1, 11 },
	{ 1, 12 },
	{ 1, 13 },
	{ 1, 14 },
	{ 1, 15 },
	{ 1, 16 },
	{ 1, 17 },
	{ 1, 18 },
	{ 1, 19 },
	{ 1, 20 },
	{ 1, 21 },
	{ 1, 22 },
	{ 1, 23 },
	{ 1, 30 },
	{ 1, 31 },
	{ 2,  0 },
	{ 2,  1 },
	{ 2,  2 },
	{ 2,  3 },
	{ 2,  4 },
	{ 2,  5 },
	{ 2,  6 },
	{ 2,  7 },
	{ 2,  8 },
	{ 2,  9 },
	{ 2, 10 },
	{ 2, 11 },
	{ 3,  0 },
	{ 3,  1 },
	{ 3,  2 },
	{ 3,  3 },
	{ 3,  4 },
	{ 3,  5 },
	{ 3,  6 },
	{ 3,  7 },
	{ 3,  8 },
	{ 3,  9 },
	{ 3, 10 },
	{ 3, 11 },
	{ 3, 12 },
	{ 3, 13 },
	{ 3, 14 },
	{ 3, 15 },
	{ 3, 16 },
	{ 3, 17 },
	{ 3, 18 },
	{ 3, 19 },
	{ 3, 20 },
	{ 3, 21 },
	{ 3, 22 },
	{ 3, 23 },
	{ 4,  0 },
	{ 4,  1 },
	{ 4,  2 },
	{ 4,  3 },
	{ 4,  4 },
	{ 4,  5 },
	{ 4,  6 },
	{ 4,  7 },
	{ 4, 12 },
	{ 4, 13 },
	{ 4, 14 },
	{ 4, 15 },
	{ 4, 16 },
	{ 4, 17 },
	{ 4, 18 },
	{ 4, 19 },
	{ 4, 20 },
	{ 4, 21 },
	{ 4, 22 },
	{ 4, 23 },
	{ 5,  0 },
	{ 5,  1 },
	{ 5,  2 },
	{ 5,  3 },
	{ 5,  4 },
	{ 5,  5 },
	{ 5,  6 },
	{ 5,  7 },
	{ 5,  8 },
	{ 5,  9 },
};

#elif	CPU_MCXN236VDF

static gpio_pin pins[]	= {
	{ DISABLED_GPIO, DISABLED_PIN },
	{ 0,  0 },
	{ 0,  1 },
	{ 0,  2 },
	{ 0,  3 },
	{ 0,  4 },
	{ 0,  5 },
	{ 0,  6 },
	{ 0,  7 },
	{ 0, 14 },
	{ 0, 15 },
	{ 0, 16 },
	{ 0, 17 },
	{ 0, 18 },
	{ 0, 19 },
	{ 0, 20 },
	{ 0, 21 },
	{ 0, 22 },
	{ 0, 23 },
	{ 0, 24 },
	{ 0, 25 },
	{ 0, 26 },
	{ 0, 27 },
	{ 0, 28 },
	{ 0, 29 },
	{ 1,  0 },
	{ 1,  1 },
	{ 1,  2 },
	{ 1,  3 },
	{ 1,  4 },
	{ 1,  5 },
	{ 1,  6 },
	{ 1,  7 },
	{ 1,  8 },
	{ 1,  9 },
	{ 1, 10 },
	{ 1, 11 },
	{ 1, 12 },
	{ 1, 13 },
	{ 1, 14 },
	{ 1, 15 },
	{ 1, 16 },
	{ 1, 17 },
	{ 1, 18 },
	{ 1, 19 },
	{ 1, 30 },
	{ 1, 31 },
	{ 2,  0 },
	{ 2,  1 },
	{ 2,  2 },
	{ 2,  3 },
	{ 2,  4 },
	{ 2,  5 },
	{ 2,  6 },
	{ 2,  7 },
	{ 2,  8 },
	{ 2,  9 },
	{ 2, 10 },
	{ 2, 11 },
	{ 3,  0 },
	{ 3,  1 },
	{ 3,  2 },
	{ 3,  6 },
	{ 3,  7 },
	{ 3,  8 },
	{ 3,  9 },
	{ 3, 10 },
	{ 3, 11 },
	{ 3, 12 },
	{ 3, 13 },
	{ 3, 14 },
	{ 3, 15 },
	{ 3, 16 },
	{ 3, 17 },
	{ 3, 18 },
	{ 3, 20 },
	{ 3, 21 },
	{ 3, 22 },
	{ 3, 23 },
	{ 4,  0 },
	{ 4,  1 },
	{ 4,  2 },
	{ 4,  3 },
	{ 4,  4 },
	{ 4,  5 },
	{ 4,  6 },
	{ 4,  7 },
	{ 4, 12 },
	{ 4, 13 },
	{ 4, 14 },
	{ 4, 15 },
	{ 4, 16 },
	{ 4, 17 },
	{ 4, 18 },
	{ 4, 19 },
	{ 4, 20 },
	{ 4, 21 },
	{ 4, 22 },
	{ 4, 23 },
	{ 5,  0 },
	{ 5,  1 },
	{ 5,  2 },
	{ 5,  3 },
	{ 5,  4 },
	{ 5,  5 },
	{ 5,  6 },
	{ 5,  7 },
};

#elif	CPU_MCXA156VLL

static gpio_pin pins[]	= {
	{ DISABLED_GPIO, DISABLED_PIN },
	{ 0,  0 },
	{ 0,  1 },
	{ 0,  2 },
	{ 0,  3 },
	{ 0,  6 },
	{ 0, 16 },
	{ 0, 17 },
	{ 0, 18 },
	{ 0, 19 },
	{ 0, 20 },
	{ 0, 21 },
	{ 0, 22 },
	{ 0, 23 },
	{ 1,  0 },
	{ 1,  1 },
	{ 1,  2 },
	{ 1,  3 },
	{ 1,  4 },
	{ 1,  5 },
	{ 1,  6 },
	{ 1,  7 },
	{ 1,  8 },
	{ 1,  9 },
	{ 1, 10 },
	{ 1, 11 },
	{ 1, 12 },
	{ 1, 13 },
	{ 1, 14 },
	{ 1, 15 },
	{ 1, 29 },
	{ 1, 30 },
	{ 1, 31 },
	{ 2,  0 },
	{ 2,  1 },
	{ 2,  2 },
	{ 2,  3 },
	{ 2,  4 },
	{ 2,  5 },
	{ 2,  6 },
	{ 2,  7 },
	{ 2, 10 },
	{ 2, 11 },
	{ 2, 12 },
	{ 2, 13 },
	{ 2, 15 },
	{ 2, 16 },
	{ 2, 17 },
	{ 2, 19 },
	{ 2, 20 },
	{ 2, 21 },
	{ 2, 23 },
	{ 3,  0 },
	{ 3,  1 },
	{ 3,  6 },
	{ 3,  7 },
	{ 3,  8 },
	{ 3,  9 },
	{ 3, 10 },
	{ 3, 11 },
	{ 3, 12 },
	{ 3, 13 },
	{ 3, 14 },
	{ 3, 15 },
	{ 3, 16 },
	{ 3, 17 },
	{ 3, 18 },
	{ 3, 19 },
	{ 3, 20 },
	{ 3, 21 },
	{ 3, 22 },
	{ 3, 27 },
	{ 3, 28 },
	{ 3, 29 },
	{ 3, 30 },
	{ 3, 31 },
	{ 4,  2 },
	{ 4,  3 },
	{ 4,  4 },
	{ 4,  5 },
	{ 4,  6 },
	{ 4,  7 },
};

#elif	CPU_MCXA153VLH

static gpio_pin pins[]	= {
	{ DISABLED_GPIO, DISABLED_PIN },
	{ 0,  0 },
	{ 0,  1 },
	{ 0,  2 },
	{ 0,  3 },
	{ 0,  6 },
	{ 0, 16 },
	{ 0, 17 },
	{ 1,  0 },
	{ 1,  1 },
	{ 1,  2 },
	{ 1,  3 },
	{ 1,  4 },
	{ 1,  5 },
	{ 1,  6 },
	{ 1,  7 },
	{ 1,  8 },
	{ 1,  9 },
	{ 1, 10 },
	{ 1, 11 },
	{ 1, 12 },
	{ 1, 13 },
	{ 1, 29 },
	{ 1, 30 },
	{ 1, 31 },
	{ 2,  0 },
	{ 2,  1 },
	{ 2,  2 },
	{ 2,  3 },
	{ 2,  4 },
	{ 2,  5 },
	{ 2,  6 },
	{ 2,  7 },
	{ 2, 12 },
	{ 2, 13 },
	{ 2, 16 },
	{ 3,  0 },
	{ 3,  1 },
	{ 3,  6 },
	{ 3,  7 },
	{ 3,  8 },
	{ 3,  9 },
	{ 3, 10 },
	{ 3, 11 },
	{ 3, 12 },
	{ 3, 13 },
	{ 3, 14 },
	{ 3, 15 },
	{ 3, 27 },
	{ 3, 28 },
	{ 3, 29 },
	{ 3, 30 },
	{ 3, 31 },
};

#elif	CPU_MCXC444VLH

static gpio_pin pins[]	= {
	{ DISABLED_GPIO, DISABLED_PIN },
	{ 0,  0 },
	{ 0,  1 },
	{ 0,  2 },
	{ 0,  3 },
	{ 0,  4 },
	{ 0,  5 },
	{ 0, 12 },
	{ 0, 13 },
	{ 0, 14 },
	{ 0, 15 },
	{ 0, 16 },
	{ 1,  0 },
	{ 1,  1 },
	{ 1,  2 },
	{ 1,  3 },
	{ 1, 16 },
	{ 1, 17 },
	{ 1, 18 },
	{ 1, 19 },
	{ 2,  0 },
	{ 2,  1 },
	{ 2,  2 },
	{ 2,  3 },
	{ 2,  4 },
	{ 2,  5 },
	{ 2,  6 },
	{ 2,  7 },
	{ 2, 20 },
	{ 2, 21 },
	{ 2, 22 },
	{ 2, 23 },
	{ 3,  0 },
	{ 3,  1 },
	{ 3,  2 },
	{ 3,  3 },
	{ 3,  4 },
	{ 3,  5 },
	{ 3,  6 },
	{ 3,  7 },
	{ 4,  0 },
	{ 4,  1 },
	{ 4, 20 },
	{ 4, 21 },
	{ 4, 22 },
	{ 4, 23 },
	{ 4, 24 },
	{ 4, 25 },
	{ 4, 29 },
	{ 4, 30 },
	{ 4, 31 },
};
#else
#error Target CPU is not supported
#endif // CPU_MCXN947VDF

extern "C" {
#include "fsl_debug_console.h"
}


DigitalInOut::DigitalInOut( uint8_t pin_num, bool direction, bool v, int pin_mode )
	: Obj( true ), _pn( pin_num ), _dir( direction ), _value( v )
{
	if ( -1 == pins[ _pn ].base )
		return;
	
	gpio_pin_config_t led_config = { (gpio_pin_direction_t)direction, _value };
	
	gpio_n		= gpio_type[ pins[ _pn ].base ];
	port_n		= port_type[ pins[ _pn ].base ];
	gpio_pin	= pins[ _pn ].pin;
	
	GPIO_PinInit( gpio_n, gpio_pin, &led_config );
	mode( pin_mode );
	
	value( (bool)_value );
}

DigitalInOut::~DigitalInOut(){}

void DigitalInOut::value( bool value )
{
	if ( -1 == pins[ _pn ].base )
		return;

	if ( kGPIO_DigitalOutput == _dir )
		GPIO_PinWrite( gpio_n, gpio_pin, value );
	
	_value	= value;
}

bool DigitalInOut::value( void )
{
	if ( -1 == pins[ _pn ].base )
		return 0;

	if ( kGPIO_DigitalInput == _dir )
		return GPIO_PinRead( gpio_n, gpio_pin );
	else
		return _value;
}

void DigitalInOut::output( void )
{
	_dir	= OUTPUT;
	direction( _dir );
}

void DigitalInOut::input( void )
{
	_dir	= INPUT;
	direction( _dir );
}

void DigitalInOut::direction( bool dir )
{
	gpio_pin_config_t led_config = { (gpio_pin_direction_t)dir, _value };
	GPIO_PinInit( gpio_n, gpio_pin, &led_config );
}

void DigitalInOut::pin_mux( int mux )
{
	PORT_SetPinMux( port_n, gpio_pin, (port_mux_t)mux );
}

void DigitalInOut::mode( int pin_mode )
{
	PORT_SetPinPullUpDown( port_n, gpio_pin, (pin_mode & (PullUp | PullDown)) ? 1 : 0, (pin_mode & PullUp) ? 1 : 0);
	PORT_SetPinOpenDrain( port_n, gpio_pin, (pin_mode & OpenDrain) ? 1 : 0 );
}

uint32_t DigitalInOut::mode( void )
{
	return PORT_GetPinMode( port_n, gpio_pin );
}

DigitalInOut& DigitalInOut::operator=( bool v )
{
	value( v );
	return *this;
}

DigitalInOut& DigitalInOut::operator=( DigitalInOut& )
{
	return *this;
}

DigitalInOut::operator bool()
{
	return value();
}

DigitalOut::DigitalOut( uint8_t pin_num, bool value, int pin_mode )
	: DigitalInOut( pin_num, kGPIO_DigitalOutput, value, pin_mode )
{
}

DigitalOut::~DigitalOut() {}

DigitalIn::DigitalIn( uint8_t pin_num, int pin_mode )
	: DigitalInOut( pin_num, kGPIO_DigitalInput, 0, pin_mode )
{
}

DigitalIn::~DigitalIn() {}

