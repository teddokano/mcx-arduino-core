/** Release check 25: FRDM-MCXN947's on-board Ethernet (MCXEthernet) with a
 *  PC -- needs the PC and a LAN cable, so it sits in the 2n group (see
 *  examples/release_check/README.md for the numbering).
 *
 *  Connect the board's RJ45 (J16) to a network with a DHCP server and the
 *  PC on it -- or straight to the PC, with the PC sharing its connection
 *  (macOS: System Settings > General > Sharing > Internet Sharing, to the
 *  wired port), which makes the PC the DHCP server and the gateway.
 *
 *  The board takes an address by DHCP, checks what it can by itself, prints
 *  its address, then serves the PC script in this folder:
 *
 *      python3 pc_check.py <the address printed>
 *
 *  The script prints each check and "ALL OK" or "N FAILED". It drives the
 *  board over UDP port 5001 (an echo, plus the commands below); TCP port
 *  5000 echoes what each connection sends.
 *
 *    CONNECT <port>  connect back to the PC on <port>, send 8192 bytes of a
 *                    pattern, close
 *    DELAY <ms>      reply, then sit in delay(<ms>): lwIP keeps answering
 *    STATS           reply with the longest 1ms lwIP interrupt so far, in us
 */

#include <MCXEthernet.h>

const uint16_t TCP_PORT = 5000;
const uint16_t UDP_PORT = 5001;
const int MAX_CLIENTS = 6;  // MAX_SOCK_NUM is 8; the board's own connect-back takes one

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

void printMac(const uint8_t *mac) {
  for (int i = 0; i < 6; i++) {
    if (i)
      Serial.print(":");
    if (mac[i] < 0x10)
      Serial.print("0");
    Serial.print(mac[i], HEX);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\r\n=== release_check 25: Ethernet with a PC (FRDM-MCXN947) ===");

  uint32_t start = millis();
  int leased = Ethernet.begin(30000);  // MAC address from the chip's ID
  uint32_t took = millis() - start;

  uint8_t mac[6], mac2[6];
  Ethernet.MACAddress(mac);
  Ethernet.MACAddress(mac2);

  check("PHY found (hardwareStatus)", Ethernet.hardwareStatus() == EthernetMCXENET);
  check("link up (linkStatus)", Ethernet.linkStatus() == LinkON);
  check("DHCP lease within 30 s", leased == 1);
  check("MAC address locally administered, unicast", (mac[0] & 0x03) == 0x02);
  check("MAC address not all zero past the first byte",
        (mac[1] | mac[2] | mac[3] | mac[4] | mac[5]) != 0);
  check("MACAddress() gives the same twice", memcmp(mac, mac2, 6) == 0);
  check("localIP() set", (uint32_t)Ethernet.localIP() != 0);
  check("subnetMask() set", (uint32_t)Ethernet.subnetMask() != 0);

  Serial.print("MAC ");
  printMac(mac);
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

  IPAddress ip;
  check("hostByName() of an address in text",
        Ethernet.hostByName("10.1.2.3", ip) == 1 && ip == IPAddress(10, 1, 2, 3));

  // depends on the network reaching the internet, so it is shown, not checked
  start = millis();
  if (Ethernet.hostByName("example.com", ip) == 1) {
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

  Serial.print("\r\nNow on the PC:  python3 pc_check.py ");
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
    char text[32];
    snprintf(text, sizeof(text), "%lu", (unsigned long)mcx_ethernet_tick_max_us());
    reply(text);
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
