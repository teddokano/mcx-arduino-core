/*
 *  MCXEthernet: EthernetServer
 *
 *  A connection made to the server takes a TCP slot in lwIP's accept callback and
 *  stays tied to the server's port until accept() hands it out, as in the official
 *  library: available() returns the tied ones that have data, and frees the ones the
 *  peer has closed with nothing left to read.
 */

#include	"mcx_eth_internal.h"

using namespace mcx_eth;

static err_t on_accept( void *arg, struct tcp_pcb *pcb, err_t err )
{
	if ( err != ERR_OK || !pcb )
		return ERR_VAL;

	uint16_t	port	= (uint16_t)(uintptr_t)arg;
	if ( tcp_slot_take( pcb, TCP_CONNECTED, port ) < 0 )
	{
		tcp_abort( pcb );
		return ERR_ABRT;
	}
	return ERR_OK;
}

void EthernetServer::begin()
{
	if ( _listening || !started() )
		return;

	Lock			lock;
	struct tcp_pcb	*pcb	= tcp_new();
	if ( !pcb )
		return;

	ip_set_option( pcb, SOF_REUSEADDR );
	if ( tcp_bind( pcb, IP_ANY_TYPE, _port ) != ERR_OK )
	{
		tcp_abort( pcb );
		return;
	}

	struct tcp_pcb	*listen	= tcp_listen( pcb );	//  frees pcb on success
	if ( !listen )
	{
		tcp_abort( pcb );
		return;
	}
	tcp_arg( listen, (void *)(uintptr_t)_port );
	tcp_accept( listen, on_accept );
	_listening	= true;
}

EthernetClient EthernetServer::available()
{
	Lock	lock;

	for ( int i = 0; i < MAX_SOCK_NUM; i++ )
	{
		TcpSlot	*s	= &tcp_slots[ i ];
		if ( s->state == TCP_FREE || s->server_port != _port )
			continue;

		bool	data	= s->rx && s->rx->tot_len;
		if ( data )
			return EthernetClient( i );
		if ( s->state == TCP_PEER_CLOSED || s->state == TCP_CLOSED )
			tcp_slot_release( i );
	}
	return EthernetClient();
}

EthernetClient EthernetServer::accept()
{
	Lock	lock;

	//  as the official library: open connections only, each once; reset ones are dropped
	for ( int i = 0; i < MAX_SOCK_NUM; i++ )
	{
		TcpSlot	*s	= &tcp_slots[ i ];
		if ( s->state == TCP_FREE || s->server_port != _port )
			continue;

		if ( s->state == TCP_CONNECTED || s->state == TCP_PEER_CLOSED )
		{
			s->server_port	= 0;
			return EthernetClient( i );
		}
		tcp_slot_release( i );
	}
	return EthernetClient();
}

size_t EthernetServer::write( uint8_t b )
{
	return write( &b, 1 );
}

//  to every connection still tied to the server
size_t EthernetServer::write( const uint8_t *buf, size_t size )
{
	for ( int i = 0; i < MAX_SOCK_NUM; i++ )
	{
		if ( tcp_slots[ i ].state == TCP_CONNECTED && tcp_slots[ i ].server_port == _port )
		{
			EthernetClient	client( i );
			client.write( buf, size );
		}
	}
	return size;
}

EthernetServer::operator bool()
{
	return _listening;
}
