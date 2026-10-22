/*
 *  MCXEthernet: EthernetClient, and the TCP connections it and EthernetServer share
 *
 *  lwIP's callbacks run in the CTIMER2 interrupt and only change a slot's state and
 *  its received data; the sketch side reads them with the Lock held.
 */

#include	"mcx_eth_internal.h"

namespace mcx_eth
{

TcpSlot	tcp_slots[ MAX_SOCK_NUM ];

static err_t on_recv( void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err )
{
	(void)pcb;
	(void)err;
	TcpSlot	*s	= (TcpSlot *)arg;

	if ( !p )
	{
		s->state	= TCP_PEER_CLOSED;
		return ERR_OK;
	}

	//  copy out of the driver's receive buffer, so data the sketch hasn't read doesn't hold it
	struct pbuf	*q	= pbuf_clone( PBUF_RAW, PBUF_RAM, p );
	if ( !q )
		return ERR_MEM;	//  lwIP keeps p and offers it again
	pbuf_free( p );

	if ( s->rx )
		pbuf_cat( s->rx, q );
	else
		s->rx	= q;
	return ERR_OK;
}

//  lwIP has freed the pcb (reset, or aborted after too many retries)
static void on_err( void *arg, err_t err )
{
	(void)err;
	TcpSlot	*s	= (TcpSlot *)arg;

	s->pcb		= NULL;
	s->state	= TCP_CLOSED;
}

static err_t on_connected( void *arg, struct tcp_pcb *pcb, err_t err )
{
	(void)pcb;
	(void)err;
	( (TcpSlot *)arg )->state	= TCP_CONNECTED;
	return ERR_OK;
}

int tcp_slot_take( struct tcp_pcb *pcb, TcpState state, uint16_t server_port )
{
	for ( int i = 0; i < MAX_SOCK_NUM; i++ )
	{
		TcpSlot	*s	= &tcp_slots[ i ];
		if ( s->state != TCP_FREE )
			continue;

		s->pcb			= pcb;
		s->rx			= NULL;
		s->state		= state;
		s->server_port	= server_port;
		s->remote_ip	= pcb->remote_ip;
		s->remote_port	= pcb->remote_port;
		s->local_port	= pcb->local_port;

		tcp_arg( pcb, s );
		tcp_recv( pcb, on_recv );
		tcp_err( pcb, on_err );
		tcp_nagle_disable( pcb );	//  send each write() at once, as a W5x00 does
		return i;
	}
	return -1;
}

void tcp_slot_release( int sock )
{
	TcpSlot	*s	= &tcp_slots[ sock ];

	if ( s->pcb )
	{
		//  take unread data off the window first: lwIP answers a close with data
		//  the sketch hasn't read with a reset, where a W5x00 closes gracefully
		if ( s->rx )
			tcp_recved( s->pcb, s->rx->tot_len );

		tcp_arg( s->pcb, NULL );
		tcp_recv( s->pcb, NULL );
		tcp_err( s->pcb, NULL );
		if ( s->state == TCP_CONNECTING || tcp_close( s->pcb ) != ERR_OK )
			tcp_abort( s->pcb );
		s->pcb	= NULL;
	}
	if ( s->rx )
	{
		pbuf_free( s->rx );
		s->rx	= NULL;
	}
	s->state	= TCP_FREE;
}

}	//  namespace mcx_eth

using namespace mcx_eth;

//  the W5x00's socket status codes, which the official library's status() returns
uint8_t EthernetClient::status()
{
	if ( _sock < 0 )
		return 0x00;	//  CLOSED

	switch ( tcp_slots[ _sock ].state )
	{
		case TCP_CONNECTING:	return 0x15;	//  SYNSENT
		case TCP_CONNECTED:		return 0x17;	//  ESTABLISHED
		case TCP_PEER_CLOSED:	return 0x1C;	//  CLOSE_WAIT
		default:				return 0x00;
	}
}

int EthernetClient::connect( const char *host, uint16_t port )
{
	IPAddress	ip;

	if ( !Ethernet.hostByName( host, ip ) )
		return -2;	//  as the official library: INVALID_SERVER
	return connect( ip, port );
}

int EthernetClient::connect( IPAddress ip, uint16_t port )
{
	if ( !started() || (uint32_t)ip == 0 || (uint32_t)ip == 0xFFFFFFFF )
		return 0;

	if ( _sock >= 0 )
		stop();

	{
		Lock			lock;
		struct tcp_pcb	*pcb	= tcp_new();
		if ( !pcb )
			return 0;

		ip_addr_t	a	= to_lwip( ip );
		_sock	= tcp_slot_take( pcb, TCP_CONNECTING, 0 );
		if ( _sock < 0 )
		{
			tcp_abort( pcb );
			return 0;
		}

		if ( tcp_connect( pcb, &a, port, on_connected ) != ERR_OK )
		{
			tcp_slot_release( _sock );
			_sock	= -1;
			return 0;
		}
		tcp_slots[ _sock ].remote_ip	= a;
		tcp_slots[ _sock ].remote_port	= port;
		tcp_slots[ _sock ].local_port	= pcb->local_port;
	}

	uint32_t	start	= millis();
	while ( tcp_slots[ _sock ].state == TCP_CONNECTING && millis() - start < _timeout )
		idle();

	if ( tcp_slots[ _sock ].state == TCP_CONNECTED )
		return 1;

	Lock	lock;
	tcp_slot_release( _sock );
	_sock	= -1;
	return 0;
}

int EthernetClient::availableForWrite()
{
	if ( _sock < 0 )
		return 0;

	Lock	lock;
	TcpSlot	*s	= &tcp_slots[ _sock ];
	return ( s->pcb && ( s->state == TCP_CONNECTED || s->state == TCP_PEER_CLOSED ) ) ? tcp_sndbuf( s->pcb ) : 0;
}

size_t EthernetClient::write( uint8_t b )
{
	return write( &b, 1 );
}

size_t EthernetClient::write( const uint8_t *buf, size_t size )
{
	if ( _sock < 0 )
	{
		setWriteError();
		return 0;
	}

	//  as the official library, wait for room as long as the connection is open, with no
	//  time limit; the peer having sent its FIN (CLOSE_WAIT) still takes data
	size_t	sent	= 0;

	while ( sent < size )
	{
		size_t	n	= 0;
		{
			Lock	lock;
			TcpSlot	*s	= &tcp_slots[ _sock ];
			if ( !s->pcb || ( s->state != TCP_CONNECTED && s->state != TCP_PEER_CLOSED ) )
				break;

			n	= min( (size_t)tcp_sndbuf( s->pcb ), size - sent );
			n	= min( n, (size_t)0xFFFF );
			if ( n && tcp_write( s->pcb, buf + sent, n, TCP_WRITE_FLAG_COPY ) != ERR_OK )
				n	= 0;	//  out of segments: wait for ACKs as for a full buffer
			tcp_output( s->pcb );
		}

		if ( n )
			sent	+= n;
		else
			idle();
	}

	if ( sent < size )
		setWriteError();
	return sent;
}

int EthernetClient::available()
{
	if ( _sock < 0 )
		return 0;

	Lock	lock;
	struct pbuf	*rx	= tcp_slots[ _sock ].rx;
	return rx ? rx->tot_len : 0;
}

int EthernetClient::read()
{
	uint8_t	b;
	return read( &b, 1 ) == 1 ? b : -1;
}

int EthernetClient::read( uint8_t *buf, size_t size )
{
	if ( _sock < 0 )
		return -1;

	Lock	lock;
	TcpSlot	*s	= &tcp_slots[ _sock ];
	if ( !s->rx )
		return -1;

	uint16_t	n	= pbuf_copy_partial( s->rx, buf, (uint16_t)min( size, (size_t)s->rx->tot_len ), 0 );
	s->rx	= pbuf_free_header( s->rx, n );
	if ( s->pcb )
		tcp_recved( s->pcb, n );
	return n;
}

int EthernetClient::peek()
{
	if ( _sock < 0 )
		return -1;

	Lock	lock;
	struct pbuf	*rx	= tcp_slots[ _sock ].rx;
	return rx ? pbuf_get_at( rx, 0 ) : -1;
}

void EthernetClient::flush()
{
	if ( _sock < 0 )
		return;

	uint32_t	start	= millis();
	while ( millis() - start < _timeout )
	{
		{
			Lock	lock;
			struct tcp_pcb	*pcb	= tcp_slots[ _sock ].pcb;
			if ( !pcb || ( !pcb->unsent && !pcb->unacked ) )
				return;
		}
		idle();
	}
}

void EthernetClient::stop()
{
	if ( _sock < 0 )
		return;

	Lock	lock;
	tcp_slot_release( _sock );
	_sock	= -1;
}

uint8_t EthernetClient::connected()
{
	if ( _sock < 0 )
		return 0;

	Lock	lock;
	TcpSlot	*s	= &tcp_slots[ _sock ];
	return s->state == TCP_CONNECTED || ( s->rx && s->rx->tot_len );
}

EthernetClient::operator bool()
{
	return _sock >= 0 && tcp_slots[ _sock ].state != TCP_FREE;
}

uint16_t EthernetClient::localPort()
{
	return _sock < 0 ? 0 : tcp_slots[ _sock ].local_port;
}

IPAddress EthernetClient::remoteIP()
{
	return _sock < 0 ? IPAddress() : from_lwip( &tcp_slots[ _sock ].remote_ip );
}

uint16_t EthernetClient::remotePort()
{
	return _sock < 0 ? 0 : tcp_slots[ _sock ].remote_port;
}
