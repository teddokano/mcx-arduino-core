/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include "lcd/LCDDriver.h"
#include "r01lib.h"
#include <ctype.h>

/* PCA8561 class ******************************************/

PCA8561::PCA8561( I2C& interface, uint8_t i2c_address ) : I2C_device( interface, i2c_address )
{
	init();
}

void PCA8561::init( void )
{
	for ( int i = 0; i < 12; i++ )
		bf[ i ]	= 0x00;	
	
	str_pos	= 0;
	
	reg_w( PCA8561::Display_ctrl_1, 0x01 );	
}

PCA8561::~PCA8561()
{
}

void PCA8561::begin( void )
{
	reg_w( PCA8561::Display_ctrl_1, 0x01 );
}

void PCA8561::com_seg( int com, int seg, bool v )
{
	const int reg[ 4 ][ 3 ]	= {
		{ COM0_07_00, COM0_15_08, COM0_17_16 },
		{ COM1_07_00, COM1_15_08, COM1_17_16 },
		{ COM2_07_00, COM2_15_08, COM2_17_16 },
		{ COM3_07_00, COM3_15_08, COM3_17_16 }
	};
	
	bit_op8( reg[ com][ seg / 8 ], (uint8_t)(~(1 << (seg % 8))), v << (seg % 8) );
}

void PCA8561::puts( const char* s, int dly )
{
	while ( int c = *s++ ) {
		putchar( c );
		wait( (float)dly / 1000.0 );
	}
}

void PCA8561::putchar( char c )
{
	if ( ('\n' == c) || ('\r' == c)) {
		clear( true );
		return;
	}

	if (4 == str_pos) {
		for ( int i = 0; i < (4 - 1); i++ )
			str_buffer[ i ]	= str_buffer[ i + 1 ];
		
		str_buffer[ 3 ]	= c;	
	}
	else {
		str_buffer[ str_pos++ ]	= c;	
	}
	
	for ( int i = 0; i < str_pos; i++ )
		char2seg( i, str_buffer[ i ] );
	
	flush();
}

void PCA8561::flush( void )
{
	reg_w( COM0_07_00, bf, 12 );
}

void PCA8561::clear( bool no_flush )
{
	for ( int i = 0; i < 12; i++ )
		bf[ i ]	= 0x00;	
	
	str_pos	= 0;
	
	if ( !no_flush )
		flush();
}

void PCA8561::char2seg( int pos, int c )
{
	c	= toupper( c );
	
	if ( (c < 32) || (92 < c) )
		c	= 32;
	
	uint16_t	p	= char_pattern[ c - 32 ];
		
	uint8_t	c0	=  p        & 0x0F;
	uint8_t	c1	= (p >>  4) & 0x0F;
	uint8_t	c2	= (p >>  8) & 0x0F;
	uint8_t	c3	= (p >> 12) & 0x0F;
		
	bf[ pos / 2 + 0 ]	&= ~(0x0F << (4 * (pos % 2)));
	bf[ pos / 2 + 3 ]	&= ~(0x0F << (4 * (pos % 2)));
	bf[ pos / 2 + 6 ]	&= ~(0x0F << (4 * (pos % 2)));
	bf[ pos / 2 + 9 ]	&= ~(0x0F << (4 * (pos % 2)));

	bf[ pos / 2 + 0 ]	|= c0 << (4 * (pos % 2));
	bf[ pos / 2 + 3 ]	|= c1 << (4 * (pos % 2));
	bf[ pos / 2 + 6 ]	|= c2 << (4 * (pos % 2));
	bf[ pos / 2 + 9 ]	|= c3 << (4 * (pos % 2));
}

uint16_t PCA8561::char_pattern[] = {	//from ASCII code 32 ~ 92 (61 slots)
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4001, 
	0x0000, 0x0000, 0x2AA8, 0x8282, 0x8282, 0x0280, 0x4000, 0x0820, 
	0x1D74, 0x0440, 0x13C4, 0x16C4, 0x06D0, 0x1694, 0x1794, 0x0444, 
	0x17D4, 0x16D4, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x07D4, 0x17B4, 0x1114, 0x9446, 0x1394, 0x0394, 0x1714, 
	0x07D0, 0x9006, 0x1540, 0x21B0, 0x1110, 0x0578, 0x2558, 0x1554, 
	0x03D4, 0x3554, 0x23D4, 0x1694, 0x8006, 0x1550, 0x0930, 0x2D50, 
	0x2828, 0x8028, 0x1824, 0x0000, 0x2008, 
};
