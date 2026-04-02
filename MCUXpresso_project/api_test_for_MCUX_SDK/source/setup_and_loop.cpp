/*
 * Since this Arduino variant is including "MCUXpresso SDK" layer to enable hardware access,
 * user can use those APIs.
 * This is a sample of the using GPIO instead of calling digitalWrite().
 */

#include "arduino.h"

void setup( void )
{
    Serial.begin( 115200 );
    Serial.println( "Hello, world!" );

    pinMode( LED_BUILTIN, OUTPUT );
}

void loop( void )
{
    static int count = 0;

    //  on MCX-A153, the LED_BUILTIN is mapped on P3_13
    //	The P3_13 becomes port=GPIO3 and pin=13
    GPIO_PinWrite( GPIO3, 13, count++ & 0x1 );

    delay( 100 );
}
