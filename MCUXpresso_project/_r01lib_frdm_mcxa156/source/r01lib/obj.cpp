/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include	"obj.h"
#include	"mcu.h"

bool Obj::init_done	= false;


Obj::Obj( bool done )
{
	if ( !init_done )
	{
		init_mcu();
#if 0
		if ( true )
			printf( "init from inherited constructor\r\n" );
		else
			printf( "init by \"__attribute__((constructor(0)))\"\r\n" );
#endif
	}

	init_done	= true;
}

Obj::~Obj() {}
