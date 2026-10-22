# MCXEthernet

The on-board Ethernet of **FRDM-MCXN947**, with the official
[Ethernet](https://github.com/arduino-libraries/Ethernet) library's interface.
The other boards have no Ethernet; on them the library stops the build with an
error.

> **Status**: builds, and the official library's 13 examples build with it
> (below). It has not been run on the board yet.

## Using it

A sketch written for the official library moves over by changing one line:

```cpp
#include <MCXEthernet.h>   // in place of #include <Ethernet.h>
```

The classes and the global object keep the official names: `Ethernet`,
`EthernetClient`, `EthernetServer` and `EthernetUDP`. `MCXEthernet.h` also
declares `EthernetUDP`, so drop `#include <EthernetUdp.h>`; `#include <SPI.h>`
can stay. The header has its own name so that the official library, for W5x00
shields, stays usable on every board, but the two can't be included in one
sketch.

```cpp
#include <MCXEthernet.h>

EthernetServer	server( 80 );

void setup()
{
	Serial.begin( 115200 );
	if ( !Ethernet.begin() )	// DHCP, with a MAC address made from the chip's ID
		Serial.println( "no DHCP lease" );
	Serial.println( Ethernet.localIP() );
	server.begin();
}

void loop()
{
	EthernetClient	client	= server.available();
	if ( client )
		while ( client.available() )
			client.write( client.read() );
}
```

The official library's 13 examples (`WebServer`, `WebClient`, `UdpNtpClient`,
`ChatServer`, ...) build with only that `#include` changed.

## How it differs from the official library

| | |
|---|---|
| `Ethernet.begin()` with no MAC address | Not in the official library. Makes a locally administered address (first byte `0x02`) from the chip's 16-byte unique ID, the same on every start. `begin(mac)` and the fixed-address forms take the address as the official library does |
| `Ethernet.hardwareStatus()` | `EthernetMCXENET` once `begin()` has found the PHY, `EthernetNoHardware` before or if it didn't. The W5x00 values are there, so sketches that test for them build |
| `Ethernet.maintain()` | lwIP renews and rebinds the DHCP lease by itself, so it always returns `0` (nothing to do) |
| `Ethernet.hostByName(name, ip)` | Not in the official library: looks a name up with the DNS server, as `EthernetClient::connect(host, port)` does |
| `Ethernet.init()`, `setRetransmissionTimeout()`, `setRetransmissionCount()`, `begin()`'s `responseTimeout` | Accepted and ignored: there is no chip-select pin, and lwIP times its own retries |
| `EthernetClient::status()` | The W5x00 socket status codes the official library returns: `0x17` (established), `0x1C` (closed by the peer), `0x15` (connecting) or `0x00` |
| Sockets | Up to `MAX_SOCK_NUM` (8) TCP connections and, separately, 8 UDP sockets. A UDP socket keeps up to 4 received packets and drops more until `parsePacket()` takes one; a packet to send is up to 1472 bytes |
| `mcx_ethernet_tick_max_us()` | Not in the official library: the longest time the 1 ms interrupt below has taken |

## How it works

TCP/IP is [lwIP](https://savannah.nongnu.org/projects/lwip/) 2.2.1 on the
chip's ENET and the board's LAN8741A PHY (RMII; the pins are in
[PIN_MAPPING_N947.md](../../../../../PIN_MAPPING_N947.md)).

- lwIP runs with no RTOS, in a 1 ms CTIMER2 interrupt: it takes received
  frames, frees sent ones and runs lwIP's timers (DHCP, ARP, TCP retransmission,
  link polling). Connections are served while the sketch is busy or in
  `delay()`, as a W5x00 serves them on its own, so a sketch written for one
  works unchanged.
- The ENET's own interrupt stays off; the 1 ms interrupt polls the driver.
- Every call from the sketch into lwIP masks that one interrupt only (not
  SysTick or the serial ports), and lwIP stops the sketch with `panic()` if a
  call reaches it any other way.
- Received data is copied out of the driver's receive buffers as it arrives,
  so data a sketch hasn't read yet doesn't stall reception.
- The interrupt has the lowest priority, as SysTick (`millis()`) does.

The library uses **CTIMER2**; a sketch using MCXEthernet can't use it too.

A sketch with DHCP, a TCP server, a TCP client and UDP takes about 115KB of
flash and 74KB of RAM (the board has 2MB and 384KB).

## Files

| | |
|---|---|
| `src/MCXEthernet.h`, `src/MCXEthernet.cpp`, `src/Ethernet*.cpp`, `src/mcx_eth_internal.h`, `src/lwipopts.h` | This library's own code, written from scratch to the official library's interface (MIT, as the rest of this package) |
| `src/lwip/`, `src/netif/`, `src/compat/` (headers), `src/lwip_src/` (sources) | lwIP 2.2.1, unmodified, as in the MCUXpresso SDK. Its license is in `LICENSE.lwIP` (BSD-3-Clause) |
| `src/arch/`, `src/mcx_port/` | From the network package of the NXP MCUXpresso SDK 2.16.000 for FRDM-MCXN947, unmodified: the SDK's lwIP port (`arch/`, `ethernetif*`, `enet_ethernetif*.c`, `enet_configchecks.h`) and its ENET, LAN8741 PHY and GPIO adapter drivers (BSD-3-Clause, each with its own copyright header) |

See the package's [LICENSE](../../../../../LICENSE).
