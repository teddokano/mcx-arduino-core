/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  Arduino-compatible IPAddress class -- an IPv4 or IPv6 address that the
 *  network base classes (Client, Server, UDP) and the libraries built on
 *  them pass around. Original implementation matching ArduinoCore-API's
 *  IPAddress interface (so code written for UNO R4 and other ArduinoCore-API
 *  cores builds unchanged), not a port of its IPAddress.cpp (LGPL 2.1).
 *
 *  An IPv4 address is kept in the last 4 of the 16 bytes, as an IPv4-mapped
 *  address would be, so the integer form (operator uint32_t) and the raw
 *  bytes line up with the other cores.
 */

#ifndef R01LIB_ARDUINO_IPADDRESS_H
#define R01LIB_ARDUINO_IPADDRESS_H

#include	<stdint.h>
#include	<stddef.h>

#include	"Printable.h"
#include	"arduino_string.h"

// The network libraries that read an address's raw bytes directly, as
// friends (as in ArduinoCore-API and ArduinoCore-avr)
class EthernetClass;
class DhcpClass;
class DNSClient;

/** Which kind of address an IPAddress holds */
enum IPType
{
	IPv4,
	IPv6
};

class IPAddress : public Printable
{
public:
	/** 0.0.0.0 (IPv4) */
	constexpr IPAddress() : IPAddress( IPv4 ) {}
	/** The all-zero address of the given type ("0.0.0.0" or "::") @param ip_type IPv4 or IPv6 */
	constexpr IPAddress( IPType ip_type ) : _bytes{}, _type( ip_type ) {}
	/** An IPv4 address, e.g. IPAddress( 192, 168, 0, 1 ) */
	constexpr IPAddress( uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet )
		: _bytes{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, first_octet, second_octet, third_octet, fourth_octet }, _type( IPv4 ) {}
	/** An IPv6 address from its 16 bytes, most significant first */
	constexpr IPAddress( uint8_t o1, uint8_t o2, uint8_t o3, uint8_t o4, uint8_t o5, uint8_t o6, uint8_t o7, uint8_t o8,
						 uint8_t o9, uint8_t o10, uint8_t o11, uint8_t o12, uint8_t o13, uint8_t o14, uint8_t o15, uint8_t o16 )
		: _bytes{ o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, o12, o13, o14, o15, o16 }, _type( IPv6 ) {}

	/** An IPv4 address from the 32-bit value whose bytes in memory are the
	 *  four octets in order (network byte order), as on the other cores.
	 *  On this little-endian chip, 192.168.0.1 is 0x0100A8C0.
	 * @param address the address as a 32-bit value
	 */
	IPAddress( uint32_t address );
	/** An IPv4 address from 4 bytes @param address the octets, first one first */
	IPAddress( const uint8_t *address );
	/** @param ip_type IPv4 (address points to 4 bytes) or IPv6 (16 bytes) @param address the bytes, most significant first */
	IPAddress( IPType ip_type, const uint8_t *address );
	/** Parse text, as fromString() does. Gives 0.0.0.0 if it isn't an address.
	 * @param address e.g. "192.168.0.1" or "fe80::1"
	 */
	IPAddress( const char *address );

	/** Parse "a.b.c.d" (each 0-255), or else an IPv6 address in the
	 *  colon-separated hex form, with "::" for one run of zero groups.
	 *  On failure the address is left as it was.
	 * @param address the text
	 * @return true if it was an address
	 */
	bool	fromString( const char *address );
	/** @param address the text @return true if it was an address */
	bool	fromString( const String &address ) { return fromString( address.c_str() ); }

	/** @return the IPv4 address as a 32-bit value (see the uint32_t constructor), or 0 for an IPv6 address */
	operator uint32_t() const;

	/** @return true if both the type and the address are the same */
	bool	operator==( const IPAddress &addr ) const;
	/** @return true if the type or the address differs */
	bool	operator!=( const IPAddress &addr ) const { return !( *this == addr ); }
	/** @param addr 4 bytes @return true if this is the IPv4 address made of those bytes */
	bool	operator==( const uint8_t *addr ) const;

	/** @param index 0-3 for IPv4, 0-15 for IPv6 @return that byte of the address */
	uint8_t		operator[]( int index ) const	{ return raw()[ index ]; }
	/** @param index 0-3 for IPv4, 0-15 for IPv6 @return that byte of the address, to read or change */
	uint8_t&	operator[]( int index )			{ return raw()[ index ]; }

	/** Make this the IPv4 address made of 4 bytes @param address the octets @return *this */
	IPAddress&	operator=( const uint8_t *address );
	/** Make this the IPv4 address in a 32-bit value (see the uint32_t constructor) @return *this */
	IPAddress&	operator=( uint32_t address );
	/** Parse text, as fromString() does (left as it was if it isn't an address) @return *this */
	IPAddress&	operator=( const char *address );

	/** Print "a.b.c.d", or an IPv6 address in the RFC 5952 form
	 *  (lower case, no leading zeros, the longest run of two or more zero
	 *  groups as "::").
	 * @param p where to print
	 * @return number of bytes written
	 */
	virtual size_t	printTo( Print &p ) const;
	/** @return the same text printTo() prints */
	String	toString() const;
	/** @return IPv4 or IPv6 */
	IPType	type() const { return _type; }

	friend class UDP;
	friend class Client;
	friend class Server;
	friend class ::EthernetClass;
	friend class ::DhcpClass;
	friend class ::DNSClient;

private:
	static constexpr int	V4_OFFSET	= 12;
	/** Longest text: 8 groups of 4 hex digits and 7 colons, plus the NUL */
	static constexpr size_t	TEXT_SIZE	= 40;

	/** The address's own bytes: the last 4 for IPv4, all 16 for IPv6.
	 *  Used by the friends above to hand the bytes to a network chip or stack. */
	uint8_t*		raw_address()	{ return raw(); }

	uint8_t*		raw()			{ return _type == IPv4 ? &_bytes[ V4_OFFSET ] : _bytes; }
	const uint8_t*	raw() const		{ return _type == IPv4 ? &_bytes[ V4_OFFSET ] : _bytes; }
	void			set_v4( const uint8_t *octets );
	size_t			format( char *text ) const;

	static bool		parse_v4( const char *s, uint8_t *out );
	static bool		parse_v6( const char *s, uint8_t *out );

	alignas( 4 ) uint8_t	_bytes[ 16 ];
	IPType					_type;
};

/** "::" -- the IPv6 any-address */
extern const IPAddress	IN6ADDR_ANY;
/** 0.0.0.0 -- what the network libraries return for "no address" */
extern const IPAddress	INADDR_NONE;

// Code written for ArduinoCore-API cores sometimes spells these with the
// namespace those cores put them in
namespace arduino
{
	using ::IPType;
	using ::IPv4;
	using ::IPv6;
	using ::IPAddress;
	using ::IN6ADDR_ANY;
	using ::INADDR_NONE;
}

#endif // !R01LIB_ARDUINO_IPADDRESS_H
