/*
 * Since this Arduino variant is including "r01lib" layer as its base class,
 * user can use r01lib APIs.
 * One thing that user should care is some definitions like pin name r01lib/soure/io.h is
 * over-ridden by arduino_io.h. The pins should be referenced like P3_13.
 * This is a sample of the using DigitalOut class.
 */

#include "arduino.h"

DigitalOut	led( P3_13 );

void setup( void )
{
    Serial.begin( 115200 );
    Serial.println( "Hello, world!" );
}

void loop( void )
{
    static int count = 0;

    led	= count++ & 0x1;

    delay( 100 );
}
