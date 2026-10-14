/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  Arduino-compatible UDP class -- the abstract base of a UDP socket
 *  (EthernetUDP, WiFiUDP, ...), so libraries such as NTP clients can take
 *  any of them as a UDP&. Original implementation; the pure virtual
 *  functions have exactly the signatures of ArduinoCore-API's UDP
 *  (LGPL 2.1, not copied), so network libraries derived from it build
 *  unchanged.
 */

#ifndef R01LIB_ARDUINO_UDP_H
#define R01LIB_ARDUINO_UDP_H

#include	"Stream.h"
#include	"IPAddress.h"

class UDP : public Stream
{
public:
	/** Start listening on a local port. @param port port @return 1 on success, 0 if no socket is free */
	virtual uint8_t	begin( uint16_t port ) = 0;
	/** Start listening on a multicast address. Fails unless a derived class supports it.
	 * @param ip multicast address @param port port @return 1 on success, 0 on failure
	 */
	virtual uint8_t	beginMulticast( IPAddress ip, uint16_t port ) { (void)ip; (void)port; return 0; }
	/** Close the socket */
	virtual void	stop() = 0;

	/** Start building a packet to send. @param ip destination @param port destination port @return 1 on success, 0 on failure */
	virtual int		beginPacket( IPAddress ip, uint16_t port ) = 0;
	/** Start building a packet to send to a host by name. @param host name to look up @param port destination port @return 1 on success, 0 on failure */
	virtual int		beginPacket( const char *host, uint16_t port ) = 0;
	/** Send the packet built since beginPacket(). @return 1 on success, 0 on failure */
	virtual int		endPacket() = 0;
	/** Add a byte to the packet being built. @param c byte @return 1 if it fit, 0 if not */
	virtual size_t	write( uint8_t c ) = 0;
	/** Add bytes to the packet being built. @param buffer bytes @param size number of bytes @return number of bytes that fit */
	virtual size_t	write( const uint8_t *buffer, size_t size ) = 0;

	/** Move to the next received packet, dropping what is left of the current one.
	 * @return size of the packet, or 0 if none has arrived */
	virtual int		parsePacket() = 0;
	/** @return bytes left to read in the current packet */
	virtual int		available() = 0;
	/** @return next byte of the current packet, or -1 if none is left */
	virtual int		read() = 0;
	/** @param buffer destination @param len its size @return number of bytes read, or -1 if none */
	virtual int		read( unsigned char *buffer, size_t len ) = 0;
	/** @param buffer destination @param len its size @return number of bytes read, or -1 if none */
	virtual int		read( char *buffer, size_t len ) = 0;
	/** @return next byte of the current packet without consuming it, or -1 if none is left */
	virtual int		peek() = 0;
	/** Finish with the current packet */
	virtual void	flush() = 0;

	/** @return address the current packet came from */
	virtual IPAddress	remoteIP() = 0;
	/** @return port the current packet came from */
	virtual uint16_t	remotePort() = 0;

protected:
	/** For derived classes: the address's raw bytes, to hand to a network chip or stack */
	uint8_t*	rawIPAddress( IPAddress &addr ) { return addr.raw_address(); }
};

namespace arduino
{
	using ::UDP;
}

#endif // !R01LIB_ARDUINO_UDP_H
