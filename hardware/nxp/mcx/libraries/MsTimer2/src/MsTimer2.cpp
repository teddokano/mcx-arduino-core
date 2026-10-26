/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  Original implementation of the MsTimer2 library's interface on the
 *  core's SysTick interrupt; none of its source (LGPL 2.1 or later) is copied.
 */

#include	"MsTimer2.h"
#include	"arduino_timer_hooks.h"

unsigned long			MsTimer2::msecs			= 1;
void					(*MsTimer2::func)()		= nullptr;
volatile unsigned long	MsTimer2::count			= 0;
volatile char			MsTimer2::overflowing	= 0;

static void tick( uint32_t ms )
{
	while ( ms-- )
		MsTimer2::_overflow();
}

void MsTimer2::set( unsigned long ms, void (*f)() )
{
	stop();

	msecs	= ms ? ms : 1;
	func	= f;
}

void MsTimer2::start()
{
	count		= 0;
	overflowing	= 0;

	systick_hook	= tick;
}

void MsTimer2::stop()
{
	systick_hook	= nullptr;
}

void MsTimer2::_overflow()
{
	count	= count + 1;

	//	Taking the period off the count, rather than clearing it, keeps
	//	the calls on time after func has run longer than a period.
	//	overflowing keeps func from running inside itself
	if ( ( count >= msecs ) && !overflowing )
	{
		overflowing	= 1;
		count		= count - msecs;

		if ( func )
			func();

		overflowing	= 0;
	}
}
