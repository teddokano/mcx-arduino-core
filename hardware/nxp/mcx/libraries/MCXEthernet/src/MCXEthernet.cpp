/*
 *  MCXEthernet: EthernetClass, the 1 ms CTIMER2 tick that runs lwIP, and lwIP's system interface
 */

#include	"mcx_eth_internal.h"

extern "C" {
#include	"lwip/init.h"
#include	"lwip/netif.h"
#include	"lwip/dhcp.h"
#include	"lwip/dns.h"
#include	"lwip/timeouts.h"
#include	"netif/ethernet.h"
#include	"mcx_port/ethernetif.h"
#include	"mcx_port/ethernetif_priv.h"
#include	"mcx_port/fsl_enet.h"
#include	"mcx_port/fsl_phylan8741.h"
#include	"fsl_ctimer.h"
#include	"fsl_flash.h"
#include	"fsl_flash_ffr.h"
void	ETHERNET_DriverIRQHandler( void );
}

void	panic( const char *s );

EthernetClass	Ethernet;

namespace mcx_eth
{

static struct netif				enet_netif;
static ethernetif_config_t		enet_config;
static phy_lan8741_resource_t	phy_resource;
static phy_handle_t				phy_handle;
static bool						hardware_found	= false;
static volatile bool			running			= false;
static int						lock_depth		= 0;
static volatile uint32_t		tick_max_us		= 0;
static uint8_t					mac_address[ 6 ];
static uint32_t					rand_state		= 0;

Lock::Lock()
{
	if ( lock_depth++ == 0 )
	{
		NVIC_DisableIRQ( CTIMER2_IRQn );
		__DSB();
		__ISB();
	}
}

Lock::~Lock()
{
	if ( --lock_depth == 0 && running )
		NVIC_EnableIRQ( CTIMER2_IRQn );
}

void idle()
{
	delay( 1 );
}

bool started()
{
	return running;
}

//  ---- the tick ----

extern "C" void CTIMER2_IRQHandler( void )
{
	CTIMER_ClearStatusFlags( CTIMER2, CTIMER_GetStatusFlags( CTIMER2 ) );

	uint32_t	start	= micros();

	ETHERNET_DriverIRQHandler();	//  frees sent frames; the ENET interrupt itself stays off
	ethernetif_input( &enet_netif );
	sys_check_timeouts();

	uint32_t	t	= micros() - start;
	if ( t > tick_max_us )
		tick_max_us	= t;

	SDK_ISR_EXIT_BARRIER;
}

static void tick_start()
{
	CLOCK_AttachClk( kFRO12M_to_CTIMER2 );
	CLOCK_SetClkDiv( kCLOCK_DivCtimer2Clk, 1u );

	ctimer_config_t	cfg;
	CTIMER_GetDefaultConfig( &cfg );
	CTIMER_Init( CTIMER2, &cfg );

	ctimer_match_config_t	match	= {};
	match.matchValue			= CLOCK_GetCTimerClkFreq( 2 ) / 1000 - 1;
	match.enableCounterReset	= true;
	match.enableInterrupt		= true;
	match.outControl			= kCTIMER_Output_NoAction;
	CTIMER_SetupMatch( CTIMER2, kCTIMER_Match_0, &match );

	//  the lowest priority, as SysTick's (millis()): the peripherals' interrupts come first
	NVIC_SetPriority( CTIMER2_IRQn, ( 1u << __NVIC_PRIO_BITS ) - 1 );

	running	= true;
	if ( lock_depth == 0 )
		NVIC_EnableIRQ( CTIMER2_IRQn );
	CTIMER_StartTimer( CTIMER2 );
}

//  ---- ENET and PHY ----

static status_t mdio_write( uint8_t phyAddr, uint8_t regAddr, uint16_t data )
{
	return ENET_MDIOWrite( ENET0, phyAddr, regAddr, data );
}

static status_t mdio_read( uint8_t phyAddr, uint8_t regAddr, uint16_t *pData )
{
	return ENET_MDIORead( ENET0, phyAddr, regAddr, pData );
}

//  RMII on PORT1, ALT9: TXCLK (the board's 50MHz reference), TXEN, TXD0-1, RXDV, RXD0-1, MDC, MDIO
static void enet_pins()
{
	static const uint8_t	pins[]	= { 4, 5, 6, 7, 13, 14, 15, 20, 21 };

	CLOCK_EnableClock( kCLOCK_Port1 );
	for ( unsigned i = 0; i < sizeof( pins ); i++ )
		PORT1->PCR[ pins[ i ] ]	= PORT_PCR_MUX( 9 ) | PORT_PCR_IBE( 1 );
}

static constexpr uint8_t	PHY_ADDRESS	= 0;

//  @return true if the PHY answers on MDIO
static bool hardware_init()
{
	static bool	done	= false;

	if ( done )
		return hardware_found;
	done	= true;

	enet_pins();
	CLOCK_AttachClk( kNONE_to_ENETRMII );
	CLOCK_EnableClock( kCLOCK_Enet );
	SYSCON0->PRESETCTRL2	= SYSCON_PRESETCTRL2_ENET_RST_MASK;
	SYSCON0->PRESETCTRL2	&= ~SYSCON_PRESETCTRL2_ENET_RST_MASK;
	ENET_SetSMI( ENET0, CLOCK_GetCoreSysClkFreq() );

	uint16_t	id1	= 0;
	hardware_found	= mdio_read( PHY_ADDRESS, PHY_ID1_REG, &id1 ) == kStatus_Success && id1 == 0x0007;	//  LAN8741A
	return hardware_found;
}

//  @return true once the interface is up
static bool interface_start( const uint8_t *mac )
{
	if ( !hardware_init() )
		return false;

	if ( running )
	{
		Lock	lock;
		if ( memcmp( mac_address, mac, 6 ) != 0 )
		{
			memcpy( mac_address, mac, 6 );
			memcpy( enet_netif.hwaddr, mac, 6 );
			ENET_SetMacAddr( ENET0, mac_address );
		}
		return true;
	}

	memcpy( mac_address, mac, 6 );
	rand_state	^= ( (uint32_t)mac[ 2 ] << 24 | (uint32_t)mac[ 3 ] << 16 | (uint32_t)mac[ 4 ] << 8 | mac[ 5 ] ) ^ micros();
	if ( rand_state == 0 )
		rand_state	= 0x2545F491;

	phy_resource.read	= mdio_read;
	phy_resource.write	= mdio_write;

	enet_config.phyHandle	= &phy_handle;
	enet_config.phyAddr		= PHY_ADDRESS;
	enet_config.phyOps		= &phylan8741_ops;
	enet_config.phyResource	= &phy_resource;
	enet_config.srcClockHz	= 50000000U;
	memcpy( enet_config.macAddress, mac, 6 );

	lwip_init();
	netif_add( &enet_netif, NULL, NULL, NULL, &enet_config, ethernetif0_init, ethernet_input );
	DisableIRQ( ETHERNET_IRQn );	//  ENET_CreateHandler() enables it; the tick polls the driver instead
	netif_set_default( &enet_netif );
	netif_set_up( &enet_netif );

	tick_start();
	return true;
}

//  a locally administered MAC address from the chip's 16-byte UUID
static void mac_from_uuid( uint8_t *mac )
{
	uint8_t			uuid[ FLASH_FFR_UUID_SIZE ]	= {};
	flash_config_t	config						= {};

	if ( FLASH_Init( &config ) == kStatus_Success && FFR_Init( &config ) == kStatus_Success )
		FFR_GetUUID( &config, uuid );

	uint64_t	h	= 0xCBF29CE484222325ull;	//  FNV-1a
	for ( unsigned i = 0; i < sizeof( uuid ); i++ )
	{
		h	^= uuid[ i ];
		h	*= 0x100000001B3ull;
	}

	mac[ 0 ]	= 0x02;	//  locally administered, unicast
	for ( int i = 1; i < 6; i++ )
		mac[ i ]	= (uint8_t)( h >> ( 8 * ( i - 1 ) ) );
}

//  ---- DNS ----

static struct
{
	volatile bool	done;
	volatile bool	found;
	ip_addr_t		addr;
	uint32_t		id;
} lookup;

static void dns_found( const char *name, const ip_addr_t *ipaddr, void *arg )
{
	(void)name;
	if ( (uint32_t)(uintptr_t)arg != lookup.id )
		return;	//  an answer to a lookup that timed out

	if ( ipaddr )
		lookup.addr	= *ipaddr;
	lookup.found	= ipaddr != NULL;
	lookup.done		= true;
}

}	//  namespace mcx_eth

using namespace mcx_eth;

//  ---- lwIP's system interface (NO_SYS=1) ----

extern "C" u32_t sys_now( void )
{
	return millis();
}

extern "C" sys_prot_t sys_arch_protect( void )
{
	return DisableGlobalIRQ();
}

extern "C" void sys_arch_unprotect( sys_prot_t v )
{
	EnableGlobalIRQ( v );
}

extern "C" void sys_check_core_locking( void )
{
	bool	in_tick	= ( __get_IPSR() & 0x1FF ) == (uint32_t)CTIMER2_IRQn + 16;
	bool	masked	= !NVIC_GetEnableIRQ( CTIMER2_IRQn );

	if ( !in_tick && !masked )
		panic( "MCXEthernet: lwIP called without the lock" );
}

extern "C" void sys_assert( const char *msg )
{
	panic( msg );
}

extern "C" uint32_t mcx_eth_rand( void )
{
	rand_state	^= rand_state << 13;	//  xorshift32
	rand_state	^= rand_state >> 17;
	rand_state	^= rand_state << 5;
	return rand_state;
}

uint32_t mcx_ethernet_tick_max_us()
{
	return tick_max_us;
}

//  ---- EthernetClass ----

int EthernetClass::begin( uint8_t *mac, unsigned long timeout, unsigned long responseTimeout )
{
	(void)responseTimeout;

	if ( !interface_start( mac ) )
		return 0;

	{
		Lock	lock;
		netif_set_addr( &enet_netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4 );
		if ( dhcp_start( &enet_netif ) != ERR_OK )
			return 0;
	}

	uint32_t	start	= millis();
	while ( millis() - start < timeout )
	{
		{
			Lock	lock;
			if ( dhcp_supplied_address( &enet_netif ) )
				return 1;
		}
		idle();
	}

	Lock	lock;
	dhcp_stop( &enet_netif );
	return 0;
}

int EthernetClass::begin( unsigned long timeout )
{
	uint8_t	mac[ 6 ];

	mac_from_uuid( mac );
	return begin( mac, timeout );
}

void EthernetClass::begin( uint8_t *mac, IPAddress ip )
{
	IPAddress	dns	= ip;
	dns[ 3 ]	= 1;
	begin( mac, ip, dns );
}

void EthernetClass::begin( uint8_t *mac, IPAddress ip, IPAddress dns )
{
	IPAddress	gateway	= ip;
	gateway[ 3 ]	= 1;
	begin( mac, ip, dns, gateway );
}

void EthernetClass::begin( uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway )
{
	begin( mac, ip, dns, gateway, IPAddress( 255, 255, 255, 0 ) );
}

void EthernetClass::begin( uint8_t *mac, IPAddress ip, IPAddress dns, IPAddress gateway, IPAddress subnet )
{
	if ( !interface_start( mac ) )
		return;

	Lock		lock;
	ip_addr_t	a	= to_lwip( ip );
	ip_addr_t	m	= to_lwip( subnet );
	ip_addr_t	g	= to_lwip( gateway );
	ip_addr_t	d	= to_lwip( dns );

	dhcp_stop( &enet_netif );
	netif_set_addr( &enet_netif, ip_2_ip4( &a ), ip_2_ip4( &m ), ip_2_ip4( &g ) );
	dns_setserver( 0, &d );
}

int EthernetClass::maintain()
{
	return 0;
}

EthernetLinkStatus EthernetClass::linkStatus()
{
	if ( !hardware_found )
		return Unknown;

	bool	link	= false;
	Lock	lock;
	if ( PHY_GetLinkStatus( &phy_handle, &link ) != kStatus_Success )
		return Unknown;
	return link ? LinkON : LinkOFF;
}

EthernetHardwareStatus EthernetClass::hardwareStatus()
{
	return hardware_found ? EthernetMCXENET : EthernetNoHardware;
}

void EthernetClass::MACAddress( uint8_t *mac )
{
	memcpy( mac, mac_address, 6 );
}

IPAddress EthernetClass::localIP()
{
	Lock	lock;
	return IPAddress( (uint32_t)ip4_addr_get_u32( netif_ip4_addr( &enet_netif ) ) );
}

IPAddress EthernetClass::subnetMask()
{
	Lock	lock;
	return IPAddress( (uint32_t)ip4_addr_get_u32( netif_ip4_netmask( &enet_netif ) ) );
}

IPAddress EthernetClass::gatewayIP()
{
	Lock	lock;
	return IPAddress( (uint32_t)ip4_addr_get_u32( netif_ip4_gw( &enet_netif ) ) );
}

IPAddress EthernetClass::dnsServerIP()
{
	if ( !running )
		return IPAddress();

	Lock	lock;
	return from_lwip( dns_getserver( 0 ) );
}

void EthernetClass::setMACAddress( const uint8_t *mac )
{
	if ( running )
		interface_start( mac );
	else
		memcpy( mac_address, mac, 6 );
}

void EthernetClass::setLocalIP( const IPAddress local_ip )
{
	if ( !running )
		return;

	Lock		lock;
	ip_addr_t	a	= to_lwip( local_ip );
	netif_set_ipaddr( &enet_netif, ip_2_ip4( &a ) );
}

void EthernetClass::setSubnetMask( const IPAddress subnet )
{
	if ( !running )
		return;

	Lock		lock;
	ip_addr_t	m	= to_lwip( subnet );
	netif_set_netmask( &enet_netif, ip_2_ip4( &m ) );
}

void EthernetClass::setGatewayIP( const IPAddress gateway )
{
	if ( !running )
		return;

	Lock		lock;
	ip_addr_t	g	= to_lwip( gateway );
	netif_set_gw( &enet_netif, ip_2_ip4( &g ) );
}

void EthernetClass::setDnsServerIP( const IPAddress dns_server )
{
	if ( !running )
		return;

	Lock		lock;
	ip_addr_t	d	= to_lwip( dns_server );
	dns_setserver( 0, &d );
}

int EthernetClass::hostByName( const char *hostname, IPAddress &result )
{
	if ( result.fromString( hostname ) )
		return 1;
	if ( !running )
		return 0;

	ip_addr_t	addr;
	err_t		err;
	{
		Lock	lock;
		lookup.id++;
		lookup.done		= false;
		lookup.found	= false;
		err	= dns_gethostbyname( hostname, &addr, dns_found, (void *)(uintptr_t)lookup.id );
	}

	if ( err == ERR_OK )
	{
		result	= from_lwip( &addr );
		return 1;
	}
	if ( err != ERR_INPROGRESS )
		return 0;

	uint32_t	start	= millis();
	while ( !lookup.done && millis() - start < 10000 )
		idle();

	Lock	lock;
	if ( !lookup.done || !lookup.found )
	{
		lookup.id++;	//  ignore a late answer
		return 0;
	}
	result	= from_lwip( &lookup.addr );
	return 1;
}
