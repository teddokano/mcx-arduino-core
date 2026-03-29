/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include "led/LEDDriver.h"

/* LEDDriver class ******************************************/

LEDDriver::LEDDriver( uint8_t n_ch, uint8_t PWM_r, uint8_t oe ) :
	n_channel( n_ch ), reg_PWM( PWM_r ), oe_pin( oe ), bp( NULL )
{
}

LEDDriver::~LEDDriver()
{
	if ( bp )
		delete[]	bp;
}

void LEDDriver::pwm( uint8_t ch, float value )
{
	if ( bp ) {
		bp[ ch ]	= (uint8_t)(value * 255.0);
	}
	else {
		reg_access( reg_PWM + ch, (uint8_t)(value * 255.0) );		
	}
}

void LEDDriver::pwm( float* values )
{
	if ( bp ) {
		for ( int i = 0; i < n_channel; i++ )
			bp[ i ]	= (uint8_t)(values[ i ] * 255.0);
	}
	else {
		uint8_t	v[ n_channel ];
		for ( int i = 0; i < n_channel; i++ )
			v[ i ]	= (uint8_t)(values[ i ] * 255.0);

		reg_access( 0x80 | reg_PWM, v, n_channel );
	}
}

void LEDDriver::flush( void )
{
	if ( bp )
		reg_access( 0x80 | reg_PWM, bp, n_channel );
}

void LEDDriver::buffer_enable( bool flag )
{
	if ( bp ) {
		delete[]	bp;
		bp	= NULL;		
	}
	
	if ( flag ) {
		bp	= new uint8_t[ n_channel ];	//	no error check done but execution will be aboerted in NEWLIB when the allocation failed
		for ( int i = 0; i < n_channel; i++ )
			bp[ i ]	= 0x00;
	}
	
}


/* PCA995x class ******************************************/

PCA995x::PCA995x( uint8_t n_ch, uint8_t PWM_r, uint8_t IREF_r, uint8_t IREFALL_r, const uint8_t* ar, uint8_t oe ) : 
	LEDDriver( n_ch, PWM_r, oe ), reg_IREF( IREF_r ), reg_IREFALL( IREFALL_r ), arp( ar )
{
	//  do nothing.
	//  leave it in default state.
}

PCA995x::~PCA995x()
{
}

void PCA995x::begin( float current, board env, bool bflag )
{
	init( current );
	
	if ( env ) {
		DigitalOut( oe_pin , 0 );
	}
	
	buffer_enable( bflag );
}

void PCA995x::irefall( uint8_t iref )
{
	reg_access( reg_IREFALL, iref );
}



/* PCA995x_I2C class ******************************************/

PCA995x_I2C::PCA995x_I2C( I2C& interface, uint8_t i2c_address, uint8_t n_ch, uint8_t PWM_r, uint8_t IREF_r, uint8_t IREFALL_r, const uint8_t* ar, uint8_t oe ) : 
	PCA995x( n_ch, PWM_r, IREF_r, IREFALL_r, ar, oe ), 
	I2C_device( interface, i2c_address )
{
}

PCA995x_I2C::~PCA995x_I2C()
{
}

void PCA995x_I2C::reg_access( uint8_t reg, uint8_t val )
{
	write_r8( reg, val );
}

void PCA995x_I2C::reg_access( uint8_t reg, uint8_t *vp, int len )
{
	reg_w( 0x80 | reg, vp, len );
}

uint8_t PCA995x_I2C::reg_access( uint8_t reg )
{
	return read_r8( reg );
}

void PCA995x_I2C::reg_access_r( uint8_t reg, uint8_t *vp, int len )
{
	reg_r( 0x80 | reg, vp, len );
}



/* PCA995x_SPI class ******************************************/

PCA995x_SPI::PCA995x_SPI( SPI& interface, uint8_t n_ch, uint8_t PWM_r, uint8_t IREF_r, uint8_t IREFALL_r, const uint8_t* ar, uint8_t oe ) : 
	PCA995x( n_ch, PWM_r, IREF_r, IREFALL_r, ar, oe ),
	spi( interface )
{
	spi_setting	= 1000000;
}

PCA995x_SPI::~PCA995x_SPI()
{
}

void PCA995x_SPI::txrx( uint8_t *data, int size )
{
	uint8_t	r_data[ 128 ];
	
	spi.write( data, r_data, size );
	memcpy( data, r_data, size );
}

void PCA995x_SPI::reg_access( uint8_t reg, uint8_t val )
{
	uint8_t data[]	= { (uint8_t)(reg << 1), val };

	txrx( data, sizeof( data ) );
}

void PCA995x_SPI::reg_access( uint8_t reg, uint8_t *vp, int len )
{
	uint8_t data[ len * 2 ];
	
	for ( int i = 0; i < len; i++ ) {
		data[ i * 2 + 0 ]	= (reg << 1) + (i * 2);
		data[ i * 2 + 1 ]	= vp[ i ];
	}
	
	for ( int i = 0; i < len * 2; i += 2 )
		txrx( data + i, 2 );
}

uint8_t PCA995x_SPI::reg_access( uint8_t reg )
{
	uint8_t data0[ 2 ]	= { (uint8_t)((reg << 1) | 0x01), 0xFF };
	uint8_t	data1[ 2 ]	= { 0xFF, 0xFF };
	
	txrx( data0, sizeof( data0 ) );
	txrx( data1, sizeof( data1 ) );

	return data1[ 1 ];
}

void PCA995x_SPI::reg_access_r( uint8_t reg, uint8_t *vp, int len )
{
	for ( int i = 0; i < len; i++ ) {
		*vp++	= reg_access( reg + i );
	}
}

void PCA995x_SPI::reg_w( uint8_t reg, uint8_t val )
{
	reg_access( reg, val );
}

void PCA995x_SPI::reg_w( uint8_t reg, uint8_t *vp, int len )
{
	reg_access( reg, vp, len );
}

uint8_t PCA995x_SPI::reg_r( uint8_t reg )
{
	return reg_access( reg );
}

void PCA995x_SPI::reg_r( uint8_t reg, uint8_t *vp, int len )
{
	reg_access_r( reg, vp, len );
}

void PCA995x_SPI::write_r8( uint8_t reg, uint8_t val )
{
	reg_w( reg, val );
}

uint8_t PCA995x_SPI::read_r8( uint8_t reg )
{
	return reg_r( reg );
}

void PCA995x_SPI::irefall( uint8_t iref )
{
	write_r8( reg_IREFALL, iref );
}

void PCA995x_SPI::pwm( uint8_t ch, float value )
{
	write_r8( reg_PWM + ch, (uint8_t)(value * 255.0) );
}

void PCA995x_SPI::pwm( float* values )
{
	uint8_t	v[ n_channel ];
	for ( int i = 0; i < n_channel; i++ )
		v[ i ]	= (uint8_t)(values[ i ] * 255.0);

	reg_w( reg_PWM, v, n_channel );
}
