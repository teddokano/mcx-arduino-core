/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  Arduino-compatible Server class -- the abstract base of a listening TCP
 *  server (EthernetServer, WiFiServer, ...). Writing to it sends to every
 *  connected client. Original implementation with the same interface as
 *  ArduinoCore-API's Server (LGPL 2.1, not copied).
 */

#ifndef R01LIB_ARDUINO_SERVER_H
#define R01LIB_ARDUINO_SERVER_H

#include	"Print.h"

class Server : public Print
{
public:
	/** Start listening */
	virtual void	begin() = 0;
};

namespace arduino
{
	using ::Server;
}

#endif // !R01LIB_ARDUINO_SERVER_H
