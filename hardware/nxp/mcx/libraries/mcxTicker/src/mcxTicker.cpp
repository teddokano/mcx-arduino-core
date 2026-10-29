/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  Original implementation of the interface of the ESP32 and ESP8266
 *  cores' Ticker, on the core's SysTick interrupt; none of their source
 *  is copied.
 */

#include	"mcxTicker.h"
#include	"arduino_timer_hooks.h"

mcxTicker	*mcxTicker::head	= nullptr;
uint32_t	mcxTicker::serial	= 0;

namespace {

uint32_t irq_save( void )
{
	uint32_t	primask	= __get_PRIMASK();

	__disable_irq();
	return	primask;
}

void irq_restore( uint32_t primask )
{
	__set_PRIMASK( primask );
}

//	Whether this runs in the SysTick interrupt: in an mcxTicker's function
bool in_systick( void )
{
	return	( __get_IPSR() & 0x1FFu ) == 15u;
}

}	// namespace

mcxTicker::mcxTicker()
{
}

mcxTicker::~mcxTicker()
{
	detach();
}

void mcxTicker::attach( float seconds, callback_t callback )
{
	start( ms_of( seconds ), true, callback );
}

void mcxTicker::attach_ms( uint32_t milliseconds, callback_t callback )
{
	start( milliseconds, true, callback );
}

void mcxTicker::once( float seconds, callback_t callback )
{
	start( ms_of( seconds ), false, callback );
}

void mcxTicker::once_ms( uint32_t milliseconds, callback_t callback )
{
	start( milliseconds, false, callback );
}

void mcxTicker::detach()
{
	uint32_t	primask	= irq_save();

	unlink();
	irq_restore( primask );
}

bool mcxTicker::active() const
{
	return	linked;
}

uint32_t mcxTicker::ms_of( float seconds )
{
	float	ms	= seconds * 1000.0f + 0.5f;

	if ( !( ms >= 1.0f ) )	// NaN too
		return	1u;
	if ( ms >= 4294967040.0f )	// the largest float below 2^32
		return	0xFFFFFFFFu;

	return	(uint32_t)ms;
}

void mcxTicker::call_plain( mcxTicker *t )
{
	t->func();
}

void mcxTicker::start( uint32_t ms, bool repeat, callback_t callback )
{
	link( ms, repeat, call_plain, callback, arg_t{} );
}

void mcxTicker::link( uint32_t ms, bool repeat, thunk_t thunk, callback_t callback, const arg_t &bits )
{
	if ( callback == nullptr )
	{
		detach();
		return;
	}

	if ( ms == 0 )
		ms	= 1;

	uint32_t	primask	= irq_save();

	this->thunk		= thunk;
	func			= callback;
	arg				= bits;
	period			= ms;
	this->repeat	= repeat;

	//	From loop(), the next tick comes in less than 1 ms, so one more
	//	makes the first call at least ms away. From an mcxTicker's function,
	//	at a tick, it is a whole ms away. seen keeps the tick that may be
	//	going on from counting this one
	left	= ( in_systick() || ms == 0xFFFFFFFFu ) ? ms : ms + 1u;
	seen	= serial;

	if ( !linked )
	{
		next	= head;
		head	= this;
		linked	= true;
	}

	if ( systick_ticker_hook == nullptr )
		systick_ticker_hook	= tick;

	irq_restore( primask );
}

//	With interrupts off
void mcxTicker::unlink()
{
	if ( !linked )
		return;

	for ( mcxTicker **p = &head; *p != nullptr; p = &(*p)->next )
	{
		if ( *p == this )
		{
			*p	= next;	// next stays as it is, so that tick() can go on past this one
			break;
		}
	}

	linked	= false;
}

void mcxTicker::tick( uint32_t ms )
{
	serial	= serial + 1;

	for ( mcxTicker *t = head; t != nullptr; )
	{
		mcxTicker	*following	= t->next;	// a function may detach or move its own object

		if ( t->linked && t->seen != serial )
		{
			t->seen	= serial;

			if ( t->left > ms )
				t->left	= t->left - ms;
			else
			{
				//	Late by more than a period (interrupts were off): the
				//	missed calls are skipped, on the same timing
				uint32_t	over	= ms - t->left;

				if ( t->repeat )
					t->left	= t->period - over % t->period;
				else
					t->unlink();

				t->thunk( t );
			}
		}

		t	= following;
	}
}
