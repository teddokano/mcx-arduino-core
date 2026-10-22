/*
 *  MCXEthernet internals shared by its .cpp files
 */

#ifndef MCX_ETH_INTERNAL_H
#define MCX_ETH_INTERNAL_H

#include	"MCXEthernet.h"

extern "C" {
#include	"lwip/tcp.h"
#include	"lwip/udp.h"
#include	"lwip/pbuf.h"
#include	"lwip/ip_addr.h"
}

namespace mcx_eth
{

/** Masks the CTIMER2 interrupt that runs lwIP, for the sketch's calls into lwIP. Nests */
class Lock
{
public:
	Lock();
	~Lock();
};

/** Wait 1 ms with lwIP running */
void	idle();

/** The interface is up (begin() has run) */
bool	started();

enum TcpState : uint8_t
{
	TCP_FREE,
	TCP_CONNECTING,
	TCP_CONNECTED,
	TCP_PEER_CLOSED,	//  the peer sent FIN; data may still wait to be read
	TCP_CLOSED,			//  reset or aborted; lwIP has freed the pcb
};

struct TcpSlot
{
	struct tcp_pcb		*pcb;
	struct pbuf			*rx;			//  received, not yet read
	volatile TcpState	state;
	uint16_t			server_port;	//  the listening port it came in on, until accept() hands it out; 0 otherwise
	ip_addr_t			remote_ip;
	uint16_t			remote_port;
	uint16_t			local_port;
};

extern TcpSlot	tcp_slots[ MAX_SOCK_NUM ];

/** @return a free slot, set up for pcb, or -1. Call with the Lock held or from lwIP */
int		tcp_slot_take( struct tcp_pcb *pcb, TcpState state, uint16_t server_port );

/** Close the slot's connection and free it. Call with the Lock held */
void	tcp_slot_release( int sock );

/** @return the address in lwIP's form */
inline ip_addr_t	to_lwip( const IPAddress &ip )
{
	ip_addr_t	a;
	ip_addr_set_ip4_u32( &a, (uint32_t)ip );
	return a;
}

inline IPAddress	from_lwip( const ip_addr_t *a )
{
	return IPAddress( (uint32_t)ip4_addr_get_u32( ip_2_ip4( a ) ) );
}

}	//  namespace mcx_eth

#endif
