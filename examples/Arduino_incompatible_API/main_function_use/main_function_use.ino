/*
 * Since this Arduino variant is defining a main() as a WEAK symbpl in main,
 * user can write his/her own main() function.
 * This is a sample of the using main().
 */

#include "arduino.h"

int main()
{
    Serial.begin( 115200 );
    Serial.println( "Hello, world!" );
    pinMode( LED_BUILTIN, OUTPUT );

    static int count = 0;

    while ( true )
	{
	    digitalWrite( LED_BUILTIN, count & 0x1 );
	    count++;
	    delay( 100 );
	}

	return 0;
}
