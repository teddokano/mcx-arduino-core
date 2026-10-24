/** Release check 26 (optional): the official Ethernet library with a W5500
 *  module, and a PC -- needs the library, the module and a LAN cable, so it
 *  sits in the 2n group (see examples/release_check/README.md). Any of the
 *  five boards.
 *
 *  Needs the official Ethernet library (2.0.2, from the Library Manager).
 *  The W5500 Lite module is wired the same on every board:
 *
 *      3.3V, GND
 *      MOSI  D11      MISO  D12      SCLK  D13
 *      CS    D10      RST   D8
 *      INT   not connected
 *
 *  D8 is free on all five boards (on FRDM-MCXA153 it is MikroBus MB_TX too,
 *  so leave the MikroBus empty). On FRDM-MCXA156, SPI is on D10-D13 only with R59/R60 moved to 2-3.
 *
 *  Connect the module's RJ45 to a network with a DHCP server and the PC on
 *  it -- or straight to the PC, with the PC sharing its connection (macOS:
 *  Internet Sharing, to the wired port), which makes the PC the DHCP server.
 *
 *  The board takes an address by DHCP, checks what it can by itself, prints
 *  its address, then serves #25's PC script, run from this folder:
 *
 *      python3 ../25_ethernet_pc_N947/pc_check.py <the address printed>
 *
 *  It answers the same as #25 (TCP echo on port 5000, UDP echo and the
 *  commands on port 5001), except STATS, which is about MCXEthernet's lwIP
 *  interrupt: here it replies "-", and the script prints SKIP for it.
 */

#include <SPI.h>
#include <Ethernet.h>
#include <Dns.h>

const int W5500_RST = D8;
const int W5500_CS = D10;

const uint16_t TCP_PORT = 5000;
const uint16_t UDP_PORT = 5001;
// the W5500 has 8 sockets: the server's listening one, the UDP one and the
// board's own connect-back take three
const int MAX_CLIENTS = 5;

uint8_t mac[6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // the official examples' one, locally administered

EthernetServer server(TCP_PORT);
EthernetUDP udp;
EthernetClient clients[MAX_CLIENTS];

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

void printMac(const uint8_t *m) {
  for (int i = 0; i < 6; i++) {
    if (i)
      Serial.print(":");
    if (m[i] < 0x10)
      Serial.print("0");
    Serial.print(m[i], HEX);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\r\n=== release_check 26: official Ethernet with a W5500 and a PC ===");

  // a hardware reset first, so the module doesn't carry sockets over from
  // the sketch that ran before (the W5500 wants RST low for 500 us or more)
  pinMode(W5500_RST, OUTPUT);
  digitalWrite(W5500_RST, LOW);
  delay(1);
  digitalWrite(W5500_RST, HIGH);
  delay(50);

  Ethernet.init(W5500_CS);

  uint32_t start = millis();
  int leased = Ethernet.begin(mac, 30000);
  uint32_t took = millis() - start;

  uint8_t back[6];
  Ethernet.MACAddress(back);

  check("W5500 found (hardwareStatus)", Ethernet.hardwareStatus() == EthernetW5500);
  check("link up (linkStatus)", Ethernet.linkStatus() == LinkON);
  check("DHCP lease within 30 s", leased == 1);
  check("MACAddress() gives the one set", memcmp(mac, back, 6) == 0);
  check("localIP() set", (uint32_t)Ethernet.localIP() != 0);
  check("subnetMask() set", (uint32_t)Ethernet.subnetMask() != 0);

  Serial.print("MAC ");
  printMac(back);
  Serial.print("\r\nDHCP took ");
  Serial.print(took);
  Serial.print(" ms\r\nIP ");
  Serial.println(Ethernet.localIP());
  Serial.print("subnet ");
  Serial.println(Ethernet.subnetMask());
  Serial.print("gateway ");
  Serial.println(Ethernet.gatewayIP());
  Serial.print("DNS ");
  Serial.println(Ethernet.dnsServerIP());

  DNSClient dns;
  IPAddress ip;
  dns.begin(Ethernet.dnsServerIP());
  check("getHostByName() of an address in text",
        dns.getHostByName("10.1.2.3", ip) == 1 && ip == IPAddress(10, 1, 2, 3));

  // depends on the network reaching the internet, so it is shown, not checked
  start = millis();
  if (dns.getHostByName("example.com", ip) == 1) {
    Serial.print("DNS lookup of example.com: ");
    Serial.print(ip);
  } else {
    Serial.print("DNS lookup of example.com: no answer (no internet here?)");
  }
  Serial.print(", ");
  Serial.print(millis() - start);
  Serial.println(" ms");

  server.begin();
  check("server listening", (bool)server);
  check("UDP socket open", udp.begin(UDP_PORT) == 1);

  if (failCount == 0)
    Serial.println("board side: ALL OK");
  else {
    Serial.print("board side: ");
    Serial.print(failCount);
    Serial.println(" FAILED");
  }

  Serial.print("\r\nNow on the PC:  python3 ../25_ethernet_pc_N947/pc_check.py ");
  Serial.println(Ethernet.localIP());
}

// ---- TCP echo, to every connection the server has handed out ----
void serveTcp() {
  EthernetClient c = server.accept();
  if (c) {
    bool placed = false;
    for (int i = 0; i < MAX_CLIENTS && !placed; i++) {
      if (!clients[i]) {
        clients[i] = c;
        placed = true;
      }
    }
    if (!placed)
      c.stop();
  }

  static uint8_t buf[1460];
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (!clients[i])
      continue;
    int n = clients[i].available();
    if (n > 0) {
      n = clients[i].read(buf, min(n, (int)sizeof(buf)));
      if (n > 0)
        clients[i].write(buf, n);
    } else if (!clients[i].connected()) {
      clients[i].stop();
    }
  }
}

void reply(const char *text) {
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.write((const uint8_t *)text, strlen(text));
  udp.endPacket();
}

void connectBack(IPAddress pc, uint16_t port) {
  EthernetClient c;
  c.setConnectionTimeout(3000);
  if (c.connect(pc, port) != 1) {
    Serial.println("CONNECT: could not connect to the PC");
    return;
  }
  static uint8_t block[512];
  for (int sent = 0; sent < 8192; sent += sizeof(block)) {
    for (unsigned i = 0; i < sizeof(block); i++)
      block[i] = (uint8_t)((sent + i) * 7);
    if (c.write(block, sizeof(block)) != sizeof(block)) {
      Serial.println("CONNECT: write() came up short");
      break;
    }
  }
  c.flush();
  c.stop();
}

// ---- UDP: echo, or a command ----
void serveUdp() {
  int n = udp.parsePacket();
  if (n <= 0)
    return;

  static char buf[1500];
  n = udp.read((unsigned char *)buf, sizeof(buf) - 1);
  if (n < 0)
    return;
  buf[n] = '\0';

  if (strncmp(buf, "CONNECT ", 8) == 0) {
    reply("OK");
    connectBack(udp.remoteIP(), (uint16_t)atoi(buf + 8));
  } else if (strncmp(buf, "DELAY ", 6) == 0) {
    reply("OK");
    delay(atoi(buf + 6));
  } else if (strcmp(buf, "STATS") == 0) {
    reply("-");  // no lwIP interrupt here: the W5500 runs TCP/IP itself
  } else {
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write((const uint8_t *)buf, n);
    udp.endPacket();
  }
}

void loop() {
  serveTcp();
  serveUdp();
}
