/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

extern "C" {
#include "fsl_common.h"
#include "fsl_debug_console.h"
}

#include	"Ticker.h"

ticker_callback_fp_t	fp;

void _ticker_callback( void )
{
	if ( fp )
		fp();
}

//Timer::Timer( int type, uint32_t period, const std::function<void()>& callback )
Ticker::Ticker()
	: utick_type( UTICK0 )
{
	fp	= nullptr;
}

Ticker::~Ticker() {}

void Ticker::attach( ticker_callback_fp_t callback, float sec )
{
	fp	= callback;
	UTICK_SetTick( utick_type, kUTICK_Repeat, (uint32_t)(sec * 1000000.0) - 1, _ticker_callback );
}
