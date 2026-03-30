/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"r01lib.h"
#include	"arduino.h"

int		main() __attribute__((weak));

int main( void )
{
	setup();
	
	while ( true )
		loop();
	
	return 0;
}

void delay( unsigned long ms )
{
	wait( (float)ms / 1000.0f );
}
