#include "arduino.h"

void setup()
{
    pinMode( RED, OUTPUT );
}

void loop()
{
    digitalWrite( RED, LOW );
    delay( 500 );
    digitalWrite( RED, HIGH );
    delay( 500 );
}
