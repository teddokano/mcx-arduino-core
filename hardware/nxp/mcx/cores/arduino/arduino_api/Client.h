/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  Arduino-compatible Client class -- the abstract base of a TCP-style
 *  connection (EthernetClient, WiFiClient, ...), so libraries such as HTTP
 *  or MQTT clients can take any of them as a Client&. Original
 *  implementation; the pure virtual functions have exactly the signatures
 *  of ArduinoCore-API's Client (LGPL 2.1, not copied), so network libraries
 *  derived from it build unchanged.
 */

#ifndef R01LIB_ARDUINO_CLIENT_H
#define R01LIB_ARDUINO_CLIENT_H

#include	"Stream.h"
#include	"IPAddress.h"

class Client : public Stream
{
public:
	/** Connect to a host. @param ip address @param port port @return 1 on success, 0 or negative on failure */
	virtual int		connect( IPAddress ip, uint16_t port ) = 0;
	/** Connect to a host by name. @param host name to look up @param port port @return 1 on success, 0 or negative on failure */
	virtual int		connect( const char *host, uint16_t port ) = 0;
	/** @param c byte to send @return 1 if it was accepted, 0 if not */
	virtual size_t	write( uint8_t c ) = 0;
	/** @param buf bytes to send @param size number of bytes @return number of bytes accepted */
	virtual size_t	write( const uint8_t *buf, size_t size ) = 0;
	/** @return number of received bytes waiting to be read */
	virtual int		available() = 0;
	/** @return next received byte, or -1 if none */
	virtual int		read() = 0;
	/** @param buf destination @param size its size @return number of bytes read, or -1 if none */
	virtual int		read( uint8_t *buf, size_t size ) = 0;
	/** @return next received byte without consuming it, or -1 if none */
	virtual int		peek() = 0;
	/** Wait until everything written has been sent */
	virtual void	flush() = 0;
	/** Close the connection */
	virtual void	stop() = 0;
	/** @return nonzero while connected, or while received data is still waiting to be read */
	virtual uint8_t	connected() = 0;
	/** @return true if this object stands for a connection (e.g. one a server handed out) */
	virtual operator bool() = 0;

protected:
	/** For derived classes: the address's raw bytes, to hand to a network chip or stack */
	uint8_t*	rawIPAddress( IPAddress &addr ) { return addr.raw_address(); }
};

namespace arduino
{
	using ::Client;
}

#endif // !R01LIB_ARDUINO_CLIENT_H
