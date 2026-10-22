/*
 *  MCXEthernet: EthernetUDP
 *
 *  Received packets wait in a short queue per socket, filled by lwIP's callback in the
 *  CTIMER2 interrupt; parsePacket() takes the next one. A packet being built lives in a
 *  pbuf from beginPacket() to endPacket().
 */

#include	"mcx_eth_internal.h"

extern "C" {
#include	"lwip/igmp.h"
}

using namespace mcx_eth;

static constexpr int	RX_QUEUE		= 4;	//  packets that wait per socket; more are dropped
static constexpr size_t	TX_MAX			= 1472;	//  a UDP payload in one Ethernet frame

struct UdpSlot
{
	bool				used;
	struct udp_pcb		*pcb;
	struct pbuf			*queue[ RX_QUEUE ];
	ip_addr_t			queue_ip[ RX_QUEUE ];
	uint16_t			queue_port[ RX_QUEUE ];
	volatile uint8_t	head;
	volatile uint8_t	count;
	struct pbuf			*cur;		//  the packet parsePacket() returned, what is left of it
	ip_addr_t			cur_ip;
	uint16_t			cur_port;
	bool				joined;		//  beginMulticast() joined `group`
	ip4_addr_t			group;
};

static UdpSlot	udp_slots[ MAX_SOCK_NUM ];

static void on_recv( void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port )
{
	(void)pcb;
	UdpSlot	*s	= (UdpSlot *)arg;

	//  copy out of the driver's receive buffer, so packets waiting here don't hold it
	struct pbuf	*q	= s->count < RX_QUEUE ? pbuf_clone( PBUF_RAW, PBUF_RAM, p ) : NULL;
	pbuf_free( p );
	if ( !q )
		return;

	int	i	= ( s->head + s->count ) % RX_QUEUE;
	s->queue[ i ]		= q;
	s->queue_ip[ i ]	= *addr;
	s->queue_port[ i ]	= port;
	s->count	= s->count + 1;
}

EthernetUDP::EthernetUDP() : _sock( -1 ), _tx( NULL ), _tx_len( 0 ), _tx_port( 0 )
{
}

EthernetUDP::~EthernetUDP()
{
	stop();
}

uint8_t EthernetUDP::begin( uint16_t port )
{
	if ( !started() )
		return 0;
	if ( _sock >= 0 )
		stop();

	Lock	lock;
	int		sock	= -1;
	for ( int i = 0; i < MAX_SOCK_NUM; i++ )
	{
		if ( !udp_slots[ i ].used )
		{
			sock	= i;
			break;
		}
	}
	if ( sock < 0 )
		return 0;

	struct udp_pcb	*pcb	= udp_new();
	if ( !pcb )
		return 0;

	ip_set_option( pcb, SOF_REUSEADDR );
	if ( udp_bind( pcb, IP_ANY_TYPE, port ) != ERR_OK )
	{
		udp_remove( pcb );
		return 0;
	}

	UdpSlot	*s	= &udp_slots[ sock ];
	memset( s, 0, sizeof( *s ) );
	s->used	= true;
	s->pcb	= pcb;
	udp_recv( pcb, on_recv, s );
	_sock	= sock;
	return 1;
}

uint8_t EthernetUDP::beginMulticast( IPAddress ip, uint16_t port )
{
	if ( !begin( port ) )
		return 0;

	Lock		lock;
	ip_addr_t	group	= to_lwip( ip );
	if ( igmp_joingroup( IP4_ADDR_ANY4, ip_2_ip4( &group ) ) != ERR_OK )
	{
		stop();
		return 0;
	}
	udp_slots[ _sock ].joined	= true;
	udp_slots[ _sock ].group	= *ip_2_ip4( &group );
	return 1;
}

void EthernetUDP::stop()
{
	if ( _sock < 0 )
		return;

	Lock	lock;
	UdpSlot	*s	= &udp_slots[ _sock ];

	if ( s->joined )
		igmp_leavegroup( IP4_ADDR_ANY4, &s->group );
	udp_remove( s->pcb );
	for ( int i = 0; i < s->count; i++ )
		pbuf_free( s->queue[ ( s->head + i ) % RX_QUEUE ] );
	if ( s->cur )
		pbuf_free( s->cur );
	if ( _tx )
		pbuf_free( (struct pbuf *)_tx );
	memset( s, 0, sizeof( *s ) );

	_tx		= NULL;
	_sock	= -1;
}

int EthernetUDP::beginPacket( const char *host, uint16_t port )
{
	IPAddress	ip;

	if ( !Ethernet.hostByName( host, ip ) )
		return 0;
	return beginPacket( ip, port );
}

int EthernetUDP::beginPacket( IPAddress ip, uint16_t port )
{
	if ( _sock < 0 )
		return 0;

	Lock	lock;
	if ( _tx )
		pbuf_free( (struct pbuf *)_tx );
	_tx		= pbuf_alloc( PBUF_TRANSPORT, TX_MAX, PBUF_RAM );
	_tx_len	= 0;
	_tx_ip	= ip;
	_tx_port	= port;
	return _tx != NULL;
}

size_t EthernetUDP::write( uint8_t c )
{
	return write( &c, 1 );
}

size_t EthernetUDP::write( const uint8_t *buffer, size_t size )
{
	if ( !_tx )
		return 0;

	size	= min( size, TX_MAX - _tx_len );
	memcpy( (uint8_t *)( (struct pbuf *)_tx )->payload + _tx_len, buffer, size );
	_tx_len	+= size;
	return size;
}

int EthernetUDP::endPacket()
{
	if ( _sock < 0 || !_tx )
		return 0;

	Lock		lock;
	struct pbuf	*p	= (struct pbuf *)_tx;
	ip_addr_t	a	= to_lwip( _tx_ip );

	pbuf_realloc( p, (u16_t)_tx_len );
	err_t	err	= udp_sendto( udp_slots[ _sock ].pcb, p, &a, _tx_port );
	pbuf_free( p );
	_tx	= NULL;
	return err == ERR_OK;
}

int EthernetUDP::parsePacket()
{
	if ( _sock < 0 )
		return 0;

	Lock	lock;
	UdpSlot	*s	= &udp_slots[ _sock ];

	if ( s->cur )
	{
		pbuf_free( s->cur );
		s->cur	= NULL;
	}
	if ( !s->count )
		return 0;

	s->cur		= s->queue[ s->head ];
	s->cur_ip	= s->queue_ip[ s->head ];
	s->cur_port	= s->queue_port[ s->head ];
	s->head		= ( s->head + 1 ) % RX_QUEUE;
	s->count	= s->count - 1;
	return s->cur->tot_len;
}

int EthernetUDP::available()
{
	if ( _sock < 0 )
		return 0;

	Lock	lock;
	struct pbuf	*p	= udp_slots[ _sock ].cur;
	return p ? p->tot_len : 0;
}

int EthernetUDP::read()
{
	unsigned char	b;
	return read( &b, 1 ) == 1 ? b : -1;
}

int EthernetUDP::read( unsigned char *buffer, size_t len )
{
	if ( _sock < 0 )
		return -1;

	Lock	lock;
	UdpSlot	*s	= &udp_slots[ _sock ];
	if ( !s->cur || !s->cur->tot_len )
		return -1;

	uint16_t	n	= pbuf_copy_partial( s->cur, buffer, (uint16_t)min( len, (size_t)s->cur->tot_len ), 0 );
	s->cur	= pbuf_free_header( s->cur, n );
	return n;
}

int EthernetUDP::peek()
{
	if ( _sock < 0 )
		return -1;

	Lock	lock;
	struct pbuf	*p	= udp_slots[ _sock ].cur;
	return ( p && p->tot_len ) ? pbuf_get_at( p, 0 ) : -1;
}

void EthernetUDP::flush()
{
	if ( _sock < 0 )
		return;

	Lock	lock;
	UdpSlot	*s	= &udp_slots[ _sock ];
	if ( s->cur )
	{
		pbuf_free( s->cur );
		s->cur	= NULL;
	}
}

IPAddress EthernetUDP::remoteIP()
{
	return _sock < 0 ? IPAddress() : from_lwip( &udp_slots[ _sock ].cur_ip );
}

uint16_t EthernetUDP::remotePort()
{
	return _sock < 0 ? 0 : udp_slots[ _sock ].cur_port;
}

uint16_t EthernetUDP::localPort()
{
	if ( _sock < 0 )
		return 0;

	Lock	lock;
	return udp_slots[ _sock ].pcb->local_port;
}
