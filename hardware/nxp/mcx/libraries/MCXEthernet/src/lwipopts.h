/*
 *  lwIP options for MCXEthernet
 *
 *  lwIP runs with NO_SYS=1 in one context only: the 1 ms CTIMER2 interrupt, or
 *  the sketch with that interrupt masked (MCXEthernet.cpp). The sizes started
 *  from the MCUXpresso SDK's lwip_dhcp_bm example for FRDM-MCXN947; the heap,
 *  the TCP window and send buffer and the ENET rings were then set from what
 *  examples/Arduino_compatible_API/test_ethernet_throughput measured.
 */

#ifndef MCX_ETHERNET_LWIPOPTS_H
#define MCX_ETHERNET_LWIPOPTS_H

#define NO_SYS			1
#define LWIP_NETCONN	0
#define LWIP_SOCKET		0

//  the SDK's port requires it with NO_SYS; here its frees happen in the CTIMER2 interrupt with the rest of lwIP
#define LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT	1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
void		sys_check_core_locking( void );
uint32_t	mcx_eth_rand( void );
#ifdef __cplusplus
}
#endif

//  every lwIP call is checked to come from the CTIMER2 interrupt or with it masked
#define LWIP_ASSERT_CORE_LOCKED()	sys_check_core_locking()

//  memory
#define MEM_ALIGNMENT			4
//  the heap holds received data the sketch hasn't read (up to TCP_WND per
//  connection: 8 connections took 47KB), UDP packets waiting, and data written
//  but not yet acknowledged (up to TCP_SND_BUF)
#define MEM_SIZE				( 64 * 1024 )
#define MEMP_NUM_PBUF			15
#define MEMP_NUM_UDP_PCB		10	//  MAX_SOCK_NUM for EthernetUDP, + DHCP and DNS
#define MEMP_NUM_TCP_PCB		10
#define MEMP_NUM_TCP_PCB_LISTEN	6
#define MEMP_NUM_TCP_SEG		( TCP_SND_QUEUELEN + 4 )
#define MEMP_NUM_REASSDATA		2
#define IP_REASS_MAX_PBUFS		4	//  below the port's spare receive buffers (enet_configchecks.h)
#define MEMP_NUM_SYS_TIMEOUT	( LWIP_NUM_SYS_TIMEOUT_INTERNAL + 2 )	//  + the port's link polling
#define PBUF_POOL_SIZE			5
#define LWIP_DISABLE_PBUF_POOL_SIZE_SANITY_CHECKS	1

//  the ENET rings, for the SDK's port (enet_ethernetif_lpc.c): a frame that
//  finds every transmit descriptor busy waits for one inside the 1 ms
//  interrupt, so there are about as many as a full send queue (TCP_SND_QUEUELEN);
//  8 receive descriptors take what a 4 * TCP_MSS window brings in 1 ms
#define ENET_RXBD_NUM			8
#define ENET_TXBD_NUM			32

//  protocols
#define LWIP_ARP		1
#define LWIP_ICMP		1
#define LWIP_DHCP		1
#define LWIP_DNS		1
#define LWIP_IGMP		1
#define LWIP_UDP		1
#define LWIP_TCP		1
#define LWIP_IPV6		0
#define SO_REUSE		1

#define UDP_TTL				255
#define TCP_TTL				255
#define TCP_QUEUE_OOSEQ		0
#define TCP_MSS				( 1500 - 40 )
//  lwIP opens the window and handles ACKs once a millisecond, so these bound
//  the rate: 35-47 Mbit/s in and about 70 Mbit/s out on a 100 Mbit/s link
#define TCP_SND_BUF			( 12 * TCP_MSS )
#define TCP_SND_QUEUELEN	( ( 3 * TCP_SND_BUF ) / TCP_MSS )
#define TCP_WND				( 4 * TCP_MSS )
#define TCP_LISTEN_BACKLOG	1

//  the ENET inserts and checks IP, UDP, TCP and ICMP checksums
#define CHECKSUM_GEN_IP		0
#define CHECKSUM_GEN_UDP	0
#define CHECKSUM_GEN_TCP	0
#define CHECKSUM_GEN_ICMP	0
#define CHECKSUM_CHECK_IP	0
#define CHECKSUM_CHECK_UDP	0
#define CHECKSUM_CHECK_TCP	0
#define CHECKSUM_CHECK_ICMP	0
#define CHECKSUM_GEN_ICMP6		0
#define CHECKSUM_CHECK_ICMP6	0

//  interface
#define LWIP_SINGLE_NETIF				1
#define LWIP_NETIF_EXT_STATUS_CALLBACK	1
#define LWIP_NETIF_LINK_CALLBACK		0
#define LWIP_NETIF_STATUS_CALLBACK		0

#define LWIP_STATS			0
#define LWIP_PROVIDE_ERRNO	1

//  random numbers for DNS query IDs and ports, IGMP and DHCP, seeded from the chip's ELS (MCXEthernet.cpp)
#define LWIP_RAND()	mcx_eth_rand()

//  a random initial sequence number for each TCP connection; lwIP's own counts
//  from the same value at every boot, so a connection made again from the same
//  port after a reset could look like the old one to the peer
#define LWIP_HOOK_TCP_ISN( local_ip, local_port, remote_ip, remote_port )	mcx_eth_rand()

//  no debug console in the Arduino core; lwIP's assertions stop the sketch with panic()
#define LWIP_PLATFORM_DIAG( x ) \
	do                          \
	{                           \
	} while ( 0 )

#endif
