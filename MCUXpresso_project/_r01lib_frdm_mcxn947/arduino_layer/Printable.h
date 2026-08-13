/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_PRINTABLE_H
#define R01LIB_ARDUINO_PRINTABLE_H

#include	<stddef.h>

class Print;

/**
 * @brief Interface for user-defined types that can format themselves for
 *        Print::print()/println() (matches real Arduino's Printable.h).
 */
class Printable
{
public:
	virtual size_t	printTo( Print &p ) const = 0;
};

#endif // !R01LIB_ARDUINO_PRINTABLE_H
