# MCXEthernet

The on-board Ethernet of **FRDM-MCXN947**, with the official
[Ethernet](https://github.com/arduino-libraries/Ethernet) library's interface.
The other boards have no Ethernet; on them the library stops the build with an
error.

> **Status**: checked on the board with the examples below and the
> package's `release_check/25_ethernet_pc_N947` (DHCP, DNS, ping, TCP and UDP
> both ways, served while the sketch is in `delay()`), with a Mac's Internet
> Sharing as the DHCP server. The official library's 13 examples build with it
> (below); they haven't been run.

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
`ChatServer`, ...) build with only that `#include` changed, and 12 of them
have been run on the board (not `BarometricPressureWebServer`, which needs an
SCP1000 sensor). Change the fixed addresses in them to ones on your network,
and open the Serial Monitor at 9600, as they use.

`WebServer` stops with `error: AnalogIn: unsupported analog pin` when a page
is asked for: it reads `analogRead(0)` to `analogRead(5)`, and FRDM-MCXN947's
`A0` and `A1` have no ADC. Start its loop at `2`
(`for (int analogChannel = 2; ...`).

The library's own examples are short and use `begin()` with no MAC address:

| Example | What it does |
|---|---|
| `DhcpAddress_N947` | Gets an address with DHCP and prints the MAC address, the address, the mask, the gateway and the DNS server; then prints the link going down and up |
| `WebServerLed_N947` | A web page that shows how long the board has been up and turns the green LED on and off |
| `UdpEcho_N947` | Sends every UDP packet on port 8888 back to its sender (`nc -u <address> 8888` on a PC) |
| `HttpGet_N947` | Looks up `example.com` with DNS and fetches its page (needs an Internet connection) |
| `WebTemperature_N947` | The on-board temperature sensor (P3T1755 on `Wire1`) as a page that reloads itself, and as JSON at `/json` |
| `WebButtonsRgb_N947` | A page that shows `SW2` and `SW3` as they are pressed and sets the RGB LED's color, with a script in the page asking the board for `/state` 4 times a second |

For a DHCP server without a router, connect the board straight to a Mac and
turn on "Internet Sharing" to its Ethernet port.

## How it differs from the official library

| | |
|---|---|
| `Ethernet.begin()` with no MAC address | Not in the official library. Makes a locally administered address (first byte `0x02`) from the chip's 16-byte unique ID, the same on every start. `begin(mac)` and the fixed-address forms take the address as the official library does |
| `Ethernet.begin(mac, ip, ...)` with a fixed address | Waits up to 3 s for the link (the PHY's auto-negotiation takes about 1.7 s), as a W5x00 has its link up from power-up: the official examples test `linkStatus()` just after `begin()` and print "Ethernet cable is not connected." if it is off. Without a cable it returns after the 3 s |
| `Ethernet.hardwareStatus()` | `EthernetMCXENET` if the PHY answers, `EthernetNoHardware` if not; like `linkStatus()`, it works before `begin()`. The W5x00 values are there, so sketches that test for them build |
| `Ethernet.maintain()` | lwIP renews and rebinds the DHCP lease by itself, so it always returns `0` (nothing to do) |
| `Ethernet.hostByName(name, ip)` | Not in the official library: looks a name up with the DNS server, as `EthernetClient::connect(host, port)` does |
| `Ethernet.init()`, `setRetransmissionTimeout()`, `setRetransmissionCount()`, `begin()`'s `responseTimeout` | Accepted and ignored: there is no chip-select pin, and lwIP times its own retries |
| `EthernetClient::status()` | The W5x00 socket status codes the official library returns: `0x17` (established), `0x1C` (closed by the peer), `0x15` (connecting) or `0x00` |
| Sockets | Up to `MAX_SOCK_NUM` (8) TCP connections and, separately, 8 UDP sockets. A UDP socket keeps up to 16 received packets and drops more until `parsePacket()` takes one; a packet to send is up to 1472 bytes |
| `EthernetClient::stop()` | Closes gracefully (FIN), even with received data left unread, as a W5x00 does, and returns at once; lwIP finishes the close |
| `mcx_ethernet_tick_max_us()` | Not in the official library: the longest time the 1 ms interrupt below has taken |

## Not supported

- **TLS** (HTTPS, MQTT over TLS): lwIP is built without its TLS layer
  (`altcp_tls` and mbedTLS are not included).
- **IPv6**: lwIP is built for IPv4 only. `IPAddress` can hold an IPv6
  address, but `EthernetClient::connect()` and `EthernetUDP::beginPacket()`
  return `0` for one.
- **Sending a ping**: the official library has no call for it either. lwIP
  answers pings.

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
- The interrupt has the lowest priority, as SysTick (`millis()`) does. Its
  longest run in `release_check/25` was 251µs.
- Each `write()` is sent at once (Nagle's algorithm is off), as a W5x00 sends
  it.
- Running lwIP once a millisecond bounds the rate: with its 4-segment window
  and 12-segment send buffer, `examples/Arduino_compatible_API/test_ethernet_throughput`
  measured 35-47 Mbit/s from a PC to the board and about 70 Mbit/s from the
  board (1460- or 512-byte `write()`s; 64-byte ones about 10 Mbit/s) on a
  100 Mbit/s link, and a burst of 1000 full-size UDP packets arrived whole.
  lwIP's heap (64KB) holds a full window for each of the 8 connections.
- lwIP's random numbers (its first local ports, DNS query IDs, DHCP) are
  seeded from the chip's ELS, whose PRNG the ELS seeds from its TRNG, and each
  TCP connection starts at a random sequence number, so the ports differ from
  one boot to the next. The ELS is brought up as the SDK does it: if its PRNG
  isn't ready, one key slot that holds no key is deleted, which seeds it;
  slots holding a key are left alone.

Call the library from `setup()`, `loop()` and what they call, not from an
interrupt handler (an `attachInterrupt()` function, say): it stops the sketch
with `panic()` if called from one, since masking the 1 ms interrupt can't
stop one that the handler has cut into.

The library uses **CTIMER2**; a sketch using MCXEthernet can't use it too.

If it doesn't work, `examples/Arduino_compatible_API/test_enet_bringup_N947`
in this package's repository checks the hardware on its own, with the same
ENET and PHY drivers and no lwIP: the PHY's ID over MDIO, the link, and a
DHCP DISCOVER and its OFFER as raw frames.

A sketch with DHCP, a TCP server, a TCP client and UDP takes about 130KB of
flash and 133KB of RAM (the board has 2MB and 384KB); most of the RAM is
lwIP's heap and the ENET's buffers.

## Files

| | |
|---|---|
| `src/MCXEthernet.h`, `src/MCXEthernet.cpp`, `src/Ethernet*.cpp`, `src/mcx_eth_internal.h`, `src/lwipopts.h` | This library's own code, written from scratch to the official library's interface (MIT, as the rest of this package) |
| `src/lwip/`, `src/netif/`, `src/compat/` (headers), `src/lwip_src/` (sources) | lwIP 2.2.1, unmodified, as in the MCUXpresso SDK. Its license is in `LICENSE.lwIP` (BSD-3-Clause) |
| `src/arch/`, `src/mcx_port/` | From the network package of the NXP MCUXpresso SDK 2.16.000 for FRDM-MCXN947, unmodified except for a cast in four comparisons of signed and unsigned integers (in `enet_ethernetif.c`, `enet_ethernetif_lpc.c` and `ethernetif.c`, so that nothing warns with the compiler warnings set to "All"): the SDK's lwIP port (`arch/`, `ethernetif*`, `enet_ethernetif*.c`, `enet_configchecks.h`) and its ENET, LAN8741 PHY and GPIO adapter drivers (BSD-3-Clause, each with its own copyright header) |

See the package's [LICENSE](../../../../../LICENSE).
