/** MCXEthernet: the on-board Ethernet of FRDM-MCXN947, with the official Ethernet library's interface
 *
 *  The classes and the global object have the official library's names (Ethernet,
 *  EthernetClient, EthernetServer, EthernetUDP), so a sketch written for it moves over
 *  by changing `#include <Ethernet.h>` to `#include <MCXEthernet.h>`. The header has its
 *  own name so that the official library, for W5x00 shields, stays usable on every
 *  board; the two can't be included in one sketch.
 *
 *  TCP/IP is lwIP on the chip's ENET and the board's LAN8741A PHY. lwIP runs in a 1 ms
 *  CTIMER2 interrupt, so connections are served while the sketch is busy or in delay(),
 *  as they are by a W5x00. Every call here into lwIP masks that interrupt only.
 */

#ifndef MCX_ETHERNET_H
#define MCX_ETHERNET_H

#if !defined( CPU_MCXN947VDF_cm33_core0 )
#error "MCXEthernet: this library is for FRDM-MCXN947 only (the other boards have no Ethernet)"
#endif

#include	<Arduino.h>
#include	"Client.h"
#include	"Server.h"
#include	"Udp.h"
#include	"IPAddress.h"

/** Connections and UDP sockets that can be open at once */
#define MAX_SOCK_NUM	8

/** As in the official library: a buffer size sketches use for small UDP packets */
#define UDP_TX_PACKET_MAX_SIZE	24

/** Link state, as in the official library */
enum EthernetLinkStatus
{
	Unknown,
	LinkON,
	LinkOFF
};

/** The hardware found. The W5x00 values are kept so that sketches written for the official library build */
enum EthernetHardwareStatus
{
	EthernetNoHardware,
	EthernetW5100,
	EthernetW5200,
	EthernetW5500,
	EthernetMCXENET,	///< FRDM-MCXN947's ENET with its LAN8741A PHY
};

class EthernetUDP;
class EthernetClient;
class EthernetServer;

/** The network interface. Use the global `Ethernet` */
class EthernetClass
{
public:
	/** Start with DHCP.
	 *  @param mac MAC address (6 bytes)
	 *  @param timeout ms to wait for the link and an address
	 *  @param responseTimeout kept for the official library's signature; lwIP times its own DHCP retries
	 *  @return 1 if an address was leased, 0 if not
	 */
	static int	begin( uint8_t *mac, unsigned long timeout = 60000, unsigned long responseTimeout = 4000 );

	/** Start with DHCP and a MAC address made from the chip's unique ID (a locally administered address).
	 *  Not in the official library, which always takes a MAC address
	 *  @param timeout ms to wait for the link and an address
	 *  @return 1 if an address was leased, 0 if not
	 */
	static int	begin( unsigned long timeout = 60000 );

	/** Start with a fixed address. As in the official library, the DNS server and the gateway
	 *  default to the address with its last byte 1, and the subnet mask to 255.255.255.0
	 */
	static void	begin( uint8_t *mac, IPAddress ip );
	static void	begin( uint8_t *mac, IPAddress ip, IPAddress dns );
	static void	begin( uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway );
	static void	begin( uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway, IPAddress subnet );

	/** Kept for the official library's signature: there is no chip-select pin to set */
	static void	init( uint8_t sspin = 10 ) { (void)sspin; }

	/** lwIP renews and rebinds the DHCP lease by itself.
	 *  @return 0 (nothing happened), as the official library returns when the lease needs nothing
	 */
	static int	maintain();

	static EthernetLinkStatus		linkStatus();
	static EthernetHardwareStatus	hardwareStatus();

	/** @param mac_address receives the MAC address in use (6 bytes) */
	static void	MACAddress( uint8_t *mac_address );

	static IPAddress	localIP();
	static IPAddress	subnetMask();
	static IPAddress	gatewayIP();
	static IPAddress	dnsServerIP();

	/** Takes effect at the next begin() */
	void	setMACAddress( const uint8_t *mac_address );
	void	setLocalIP( const IPAddress local_ip );
	void	setSubnetMask( const IPAddress subnet );
	void	setGatewayIP( const IPAddress gateway );
	void	setDnsServerIP( const IPAddress dns_server );

	/** Kept for the official library's signature: lwIP times TCP retransmissions itself */
	void	setRetransmissionTimeout( uint16_t milliseconds ) { (void)milliseconds; }
	void	setRetransmissionCount( uint8_t num ) { (void)num; }

	/** Look up a host name with the DNS server.
	 *  @return 1 and the address in `result`, or 0 if it couldn't be found
	 */
	static int	hostByName( const char *hostname, IPAddress &result );

	friend class EthernetClient;
	friend class EthernetServer;
	friend class EthernetUDP;
};

extern EthernetClass	Ethernet;

/** For checking the library: the longest time the 1 ms lwIP interrupt has taken, in microseconds. Not in the official library */
uint32_t	mcx_ethernet_tick_max_us();

/** A UDP socket */
class EthernetUDP : public UDP
{
public:
	EthernetUDP();
	virtual ~EthernetUDP();

	virtual uint8_t	begin( uint16_t port ) override;
	virtual uint8_t	beginMulticast( IPAddress ip, uint16_t port ) override;
	virtual void	stop() override;

	virtual int		beginPacket( IPAddress ip, uint16_t port ) override;
	virtual int		beginPacket( const char *host, uint16_t port ) override;
	virtual int		endPacket() override;
	virtual size_t	write( uint8_t c ) override;
	virtual size_t	write( const uint8_t *buffer, size_t size ) override;
	using Print::write;

	virtual int		parsePacket() override;
	virtual int		available() override;
	virtual int		read() override;
	virtual int		read( unsigned char *buffer, size_t len ) override;
	virtual int		read( char *buffer, size_t len ) override { return read( (unsigned char *)buffer, len ); }
	virtual int		peek() override;
	virtual void	flush() override;

	virtual IPAddress	remoteIP() override;
	virtual uint16_t	remotePort() override;
	uint16_t			localPort();

private:
	int		_sock;			//  index of the socket's state, -1 if closed
	void	*_tx;			//  pbuf being built since beginPacket()
	size_t	_tx_len;
	IPAddress	_tx_ip;
	uint16_t	_tx_port;
};

/** A TCP connection */
class EthernetClient : public Client
{
public:
	EthernetClient() : _sock( -1 ), _timeout( 1000 ) {}
	explicit EthernetClient( int sock ) : _sock( sock ), _timeout( 1000 ) {}

	/** @return 1 while the connection is open (as in the official library, a socket state) */
	uint8_t	status();

	virtual int		connect( IPAddress ip, uint16_t port ) override;
	virtual int		connect( const char *host, uint16_t port ) override;
	virtual int		availableForWrite() override;
	virtual size_t	write( uint8_t b ) override;
	virtual size_t	write( const uint8_t *buf, size_t size ) override;
	using Print::write;
	virtual int		available() override;
	virtual int		read() override;
	virtual int		read( uint8_t *buf, size_t size ) override;
	virtual int		peek() override;
	virtual void	flush() override;
	virtual void	stop() override;
	virtual uint8_t	connected() override;
	virtual operator bool() override;

	virtual bool	operator==( const bool value ) { return bool() == value; }
	virtual bool	operator!=( const bool value ) { return bool() != value; }
	virtual bool	operator==( const EthernetClient &rhs ) { return _sock == rhs._sock && _sock >= 0; }
	virtual bool	operator!=( const EthernetClient &rhs ) { return !this->operator==( rhs ); }

	uint16_t	localPort();
	IPAddress	remoteIP();
	uint16_t	remotePort();

	/** @param timeout ms that connect() and flush() wait. As in the official library, write() waits for room as long as the connection is open */
	void	setConnectionTimeout( uint16_t timeout ) { _timeout = timeout; }

	/** @return index of the connection's state, or MAX_SOCK_NUM if none */
	uint8_t	getSocketNumber() const { return _sock < 0 ? MAX_SOCK_NUM : _sock; }

	friend class EthernetServer;

private:
	int			_sock;
	uint16_t	_timeout;
};

/** A TCP server: listens on a port and hands out the connections made to it */
class EthernetServer : public Server
{
public:
	EthernetServer( uint16_t port ) : _port( port ), _listening( false ) {}

	/** @return a connected client that has data to read, or a client that is false */
	EthernetClient	available();

	/** @return a connection not handed out by accept() before, or a client that is false */
	EthernetClient	accept();

	virtual void	begin() override;
	virtual size_t	write( uint8_t b ) override;
	virtual size_t	write( const uint8_t *buf, size_t size ) override;
	using Print::write;
	virtual operator bool();

private:
	uint16_t	_port;
	bool		_listening;
};

#endif // MCX_ETHERNET_H
