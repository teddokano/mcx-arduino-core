/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#ifndef ARDUINO_BUSINOUT_H
#define ARDUINO_BUSINOUT_H

#include "io.h"
#include <stdint.h>

/** BusInOut class
 *	
 *  @class BusInOut
 *  
 *	This class can be inherited. 
 *	When the "operator=" need to be used in inherited class, 
 *	use "using BusInOut::operator=;" in that class. 
 */

class BusInOut {
public:
	enum	direction {
		INPUT	= DigitalInOut::INPUT, 
		OUTPUT	= DigitalInOut::OUTPUT
	};
	
	/** To define non-connected pin, use BusInOut::nc */
	constexpr static uint8_t	nc	= 0xFF;

	/** Create a BusInOut instance with specified pins
	 *
	 * @param bit<n> pin number to connect bus bit<n> (0-13, nc)
	 *
	 * @note define pins from lower to upper. No need to define upper bits if those are not be used 
	 */
	BusInOut(	uint8_t bit0 = nc,
				uint8_t bit1 = nc,
				uint8_t bit2 = nc,
				uint8_t bit3 = nc,
				uint8_t bit4 = nc,
				uint8_t bit5 = nc,
				uint8_t bit6 = nc,
				uint8_t bit7 = nc
			 );

	/** Destractor */
	virtual ~BusInOut();

	/** Configure BusInOut IO direction
	 *
	 * @param conf INPUT or OUTPUT (defined in Arduino.h)
	 */
	virtual void		config( int conf );

	/** Output a value to BusInOut pins
	 *
	 * @param v value to be set to pins
	 */
	virtual void		value( uint8_t v );
	
	/** Input a value from BusInOut pins
	 *
	 * @return value read from pins
	 */
	virtual uint8_t		value( void );
	
	
	/** A short hand for setting pins
	 */
	BusInOut&	operator=( uint8_t v );
	BusInOut&	operator=( BusInOut& rhs );

	/** A short hand for reading pins
	 */
	operator	int();
	
private:
	DigitalInOut*	_bits[ 8 ];
	uint8_t			_width;
	uint8_t			_mode;
};

class BusIn : public BusInOut {
public:
	using BusInOut::operator=;
	
	/** Create a BusIn instance with specified pins
	 *
	 * @param bit<n> pin number to connect bus bit<n> (0-13, nc)
	 *
	 * @note define pins from lower to upper. No need to define upper bits if those are not be used 
	 */
	BusIn(	uint8_t bit0 = nc,
				uint8_t bit1 = nc,
				uint8_t bit2 = nc,
				uint8_t bit3 = nc,
				uint8_t bit4 = nc,
				uint8_t bit5 = nc,
				uint8_t bit6 = nc,
				uint8_t bit7 = nc
			 );
	virtual ~BusIn();
	
	/** Configure BusIn IO direction
	 *
	 * @note call of this method will be error when compiling since this class only support INPUT
	 */
	virtual void		config( int conf );
};


class BusOut : public BusInOut {
public:
	using BusInOut::operator=;
	
	/** Create a BusOut instance with specified pins
	 *
	 * @param bit<n> pin number to connect bus bit<n> (0-13, nc)
	 *
	 * @note define pins from lower to upper. No need to define upper bits if those are not be used 
	 */
	BusOut(	uint8_t bit0 = nc,
				uint8_t bit1 = nc,
				uint8_t bit2 = nc,
				uint8_t bit3 = nc,
				uint8_t bit4 = nc,
				uint8_t bit5 = nc,
				uint8_t bit6 = nc,
				uint8_t bit7 = nc
			 );
	virtual ~BusOut();

	/** Configure BusIn IO direction
	 *
	 * @note call of this method will be error when compiling since this class only support INPUT
	 */
	virtual void		config( int conf );
};


#endif //	ARDUINO_BUSINOUT_H
