#include "arduino.h"

void setup( void )
{
    Serial.begin( 115200 );
    Serial.println( "Hello, world!" );

    pinMode( LED_BUILTIN, OUTPUT );
}

#define	GPIO_PORT3_CLEAR	*((volatile uint32_t *)0x40105044)
#define	GPIO_PORT3_SET		*((volatile uint32_t *)0x40105048)
#define	BIT13				((uint32_t)0x1 << 13)

void loop( void )
{
    P3_n_SET	= BIT13;
    delay( 100 );

    P3_n_CLEAR	= BIT13;
    delay( 100 );
}
