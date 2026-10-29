/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  mcxTicker: calls a function every given time, or once after it, from an
 *  interrupt, with any number of objects. The interface is that of the
 *  Ticker in the ESP32 and ESP8266 cores (attach(), attach_ms(), once(),
 *  once_ms(), detach(), active()), written from scratch; a sketch written
 *  for it moves here by changing `#include <Ticker.h>` to
 *  `#include <mcxTicker.h>` and the class name `Ticker` to `mcxTicker`.
 *  The name differs because other Arduino libraries have a class named
 *  Ticker too.
 *
 *  It runs on the core's 1 ms SysTick interrupt (millis()'s), so it takes
 *  no timer and no pin, and times are in whole milliseconds.
 */

#ifndef MCX_TICKER_H
#define MCX_TICKER_H

#include	<Arduino.h>
#include	<string.h>
#include	<type_traits>

/** Calls a function every given time, or once after it.
 *
 *  Each object calls its own function: make as many as needed. The
 *  functions run in the SysTick interrupt, at the lowest priority: other
 *  interrupts, Serial's among them, still run while one does, and
 *  millis() reads the time of this tick. Keep them short, and don't wait
 *  in them for time to pass. A function may attach() or detach() any
 *  mcxTicker, its own included.
 *
 *  Times are rounded to whole milliseconds, at least 1. The first call
 *  comes at least the given time after attach(); after that, calls are a
 *  period apart. A call that would come while interrupts are off for more
 *  than a period is skipped, and the calls keep their timing. Objects due
 *  in the same millisecond run in the reverse order of their attach().
 */
class mcxTicker
{
public:
	/** A function with no argument */
	typedef void	(*callback_t)( void );

	/** TArg as it is: an argument of this type is not used to find TArg,
	 *  which comes from the function alone, so that toggle( int ) takes RED */
	template<typename T> struct same { typedef T type; };

	mcxTicker();
	/** detach()es, so an object can go out of scope while it runs */
	~mcxTicker();

	/** Calls a function every given number of seconds, until detach().
	 *  On an mcxTicker that runs, changes its function and time.
	 * @param seconds period [s], to the nearest millisecond
	 * @param callback function to call
	 */
	void	attach( float seconds, callback_t callback );
	/** The same, with the period in milliseconds.
	 * @param milliseconds period [ms]
	 * @param callback function to call
	 */
	void	attach_ms( uint32_t milliseconds, callback_t callback );

	/** Calls a function with an argument every given number of seconds.
	 * @param seconds period [s], to the nearest millisecond
	 * @param callback function to call
	 * @param arg its argument: a pointer, or a value no larger than one
	 */
	template<typename TArg>
	void	attach( float seconds, void (*callback)( TArg ), typename same<TArg>::type arg )
	{
		start( ms_of( seconds ), true, callback, arg );
	}
	/** The same, with the period in milliseconds.
	 * @param milliseconds period [ms]
	 * @param callback function to call
	 * @param arg its argument: a pointer, or a value no larger than one
	 */
	template<typename TArg>
	void	attach_ms( uint32_t milliseconds, void (*callback)( TArg ), typename same<TArg>::type arg )
	{
		start( milliseconds, true, callback, arg );
	}

	/** Calls a function once, the given number of seconds from now.
	 * @param seconds delay [s], to the nearest millisecond
	 * @param callback function to call
	 */
	void	once( float seconds, callback_t callback );
	/** The same, with the delay in milliseconds.
	 * @param milliseconds delay [ms]
	 * @param callback function to call
	 */
	void	once_ms( uint32_t milliseconds, callback_t callback );

	/** Calls a function with an argument once, the given number of
	 *  seconds from now.
	 * @param seconds delay [s], to the nearest millisecond
	 * @param callback function to call
	 * @param arg its argument: a pointer, or a value no larger than one
	 */
	template<typename TArg>
	void	once( float seconds, void (*callback)( TArg ), typename same<TArg>::type arg )
	{
		start( ms_of( seconds ), false, callback, arg );
	}
	/** The same, with the delay in milliseconds.
	 * @param milliseconds delay [ms]
	 * @param callback function to call
	 * @param arg its argument: a pointer, or a value no larger than one
	 */
	template<typename TArg>
	void	once_ms( uint32_t milliseconds, void (*callback)( TArg ), typename same<TArg>::type arg )
	{
		start( milliseconds, false, callback, arg );
	}

	/** Stops the calls. Nothing happens on an mcxTicker that doesn't run. */
	void	detach();

	/** @return true from attach() or once() until detach(), or until a
	 *  once() function has been called
	 */
	bool	active() const;

private:
	typedef void	(*thunk_t)( mcxTicker *t );

	static uint32_t	ms_of( float seconds );
	static void		call_plain( mcxTicker *t );
	static void		tick( uint32_t ms );

	template<typename TArg>
	static void call_with_arg( mcxTicker *t )
	{
		TArg	a;

		memcpy( &a, t->arg.b, sizeof( TArg ) );
		reinterpret_cast<void (*)( TArg )>( t->func )( a );
	}

	void	start( uint32_t ms, bool repeat, callback_t callback );

	template<typename TArg>
	void start( uint32_t ms, bool repeat, void (*callback)( TArg ), typename same<TArg>::type arg )
	{
		static_assert( sizeof( TArg ) <= sizeof( arg_t ), "mcxTicker: the argument must be no larger than a pointer" );
		static_assert( std::is_trivially_copyable<TArg>::value, "mcxTicker: the argument must be a plain value or a pointer" );

		arg_t	bits	= {};

		memcpy( bits.b, &arg, sizeof( TArg ) );
		link( ms, repeat, call_with_arg<TArg>, reinterpret_cast<callback_t>( callback ), bits );
	}

	struct arg_t { alignas( void * ) unsigned char b[ sizeof( void * ) ]; };

	void	link( uint32_t ms, bool repeat, thunk_t thunk, callback_t callback, const arg_t &bits );
	void	unlink();

	static mcxTicker	*head;
	static uint32_t		serial;		// counts ticks, so that a tick handles each object once

	mcxTicker			*next		= nullptr;
	thunk_t				thunk		= nullptr;
	callback_t			func		= nullptr;
	arg_t				arg			= {};
	uint32_t			period		= 0;
	uint32_t			left		= 0;
	uint32_t			seen		= 0;
	bool				repeat		= false;
	volatile bool		linked		= false;
};

#endif	// MCX_TICKER_H
