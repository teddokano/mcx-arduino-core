/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#ifndef R01LIB_TICKER_H
#define R01LIB_TICKER_H

#ifndef	CPU_MCXC444VLH

#include <functional>

extern "C" {
#include	"fsl_utick.h"
}

#include	<functional>

using	ticker_callback_fp_t	= std::function<void(void)>;

/** Ticker class
 *	
 *  @class Ticker
 *
 *	A class for demonstrating SPI bus
 *
 * @note only 1 ticker can be used in this version
 */
class Ticker
{	
public:
	enum{
		repeat	= kUTICK_Repeat
	};
	
	/** Create a Ticker instance */
	Ticker();
	
	/** Destractor to freeing SPI resource */
	virtual ~Ticker();
	
	/** Register callback function
	 *
	 * @param callback callback function
	 * @param sec periodic cycle to call the callback
	 */
	virtual void	attach( ticker_callback_fp_t callback, float sec );

private:
	UTICK_Type	*utick_type;
};

#endif // !CPU_MCXC444VLH

#endif // R01LIB_TICKER_H
