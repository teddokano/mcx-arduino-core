/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

/** LED operation library for Arduino
 *	
 *	Abstraction class for each single LED. 
 *	Enabes to handle an LED as an instance, to set PWM by assign
 *	
 *  @class  LED
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#ifndef ARDUINO_LED_NXP_ARD_H
#define ARDUINO_LED_NXP_ARD_H

#include	"led/LEDDriver.h"

class LED {
public:
  LED( LEDDriver& leddp, int ch );
  float operator=(float v);
private:
  LEDDriver&	devp;
  int			channel;
};

class _LED : public LED {
public:
	using LED::operator=;
	_LED( LEDDriver& leddp, int ch );
};

#endif //	ARDUINO_LED_NXP_ARD_H
