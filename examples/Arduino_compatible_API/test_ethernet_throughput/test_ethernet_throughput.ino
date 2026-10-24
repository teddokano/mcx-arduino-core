/** Ethernet throughput, measured with a PC: FRDM-MCXN947's on-board
 *  Ethernet (MCXEthernet), or the official Ethernet library with a W5500
 *  module on any of the five boards.
 *
 *  On FRDM-MCXN947 it uses MCXEthernet and the RJ45 (J16); to measure a
 *  W5500 on it instead, set USE_W5500 to 1 below. On the other boards it
 *  uses the official Ethernet library (2.0.2, from the Library Manager) and
 *  a W5500 module, wired as in examples/release_check/README.md, #26:
 *  MOSI D11, MISO D12, SCLK D13, CS D10, RST D8.
 *
 *  The board takes an address by DHCP and prints it; then run, on a PC on
 *  the same network (or sharing its connection to the board):
 *
 *      python3 pc_throughput.py <the address printed>
 *
 *  The script prints the rates; this is a measurement, with no pass or fail.
 *  What the board serves:
 *
 *    TCP 5002  "<n>\n", then n bytes: reads and drops them, then replies
 *              with the microseconds from the first byte to the last
 *    TCP 5003  "<n> <block>\n": sends n bytes, block bytes per write(),
 *              then closes
 *    UDP 5004  counts what arrives
 *    UDP 5001  commands, each answered on this port:
 *      SEND <count> <size> <port>  sends count packets of size bytes to the
 *                                  PC's <port>, then replies with
 *                                  "<sent> <us>"
 *      COUNT                       replies "<packets> <bytes>" received on
 *                                  5004 since the last COUNT, and zeroes them
 *      TICK                        replies with the longest 1 ms lwIP
 *                                  interrupt so far in us (MCXEthernet), or
 *                                  "-" (W5500)
 */

#ifndef USE_W5500
#define USE_W5500 0  // 1: the W5500 module on FRDM-MCXN947 too
#endif

#if defined(FRDM_MCXN947) && !USE_W5500
#define ON_CHIP 1
#include <MCXEthernet.h>
#elif defined(FRDM_MCXN947) || defined(FRDM_MCXA153) || defined(FRDM_MCXA156) || defined(FRDM_MCXN236) \
  || defined(FRDM_MCXC444)
#define ON_CHIP 0
#include <SPI.h>
#include <Ethernet.h>
const int W5500_RST = D8;
const int W5500_CS = D10;
#else
#error "This sketch has no settings for this board yet"
#endif

const uint16_t CMD_PORT = 5001;
const uint16_t SINK_PORT = 5002;
const uint16_t SOURCE_PORT = 5003;
const uint16_t UDP_SINK_PORT = 5004;

uint8_t mac[6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // the official examples' one; MCXEthernet makes its own

EthernetServer sink(SINK_PORT);
EthernetServer source(SOURCE_PORT);
EthernetUDP cmd;
EthernetUDP udpSink;

uint32_t udpPackets = 0;
uint32_t udpBytes = 0;

static uint8_t buf[4096];

void setup() {
  Serial.begin(115200);
  delay(500);
#if ON_CHIP
  Serial.println("\r\n=== Ethernet throughput: MCXEthernet (on-chip ENET) ===");
  int leased = Ethernet.begin();
#else
  Serial.println("\r\n=== Ethernet throughput: official Ethernet, W5500 ===");
  pinMode(W5500_RST, OUTPUT);
  digitalWrite(W5500_RST, LOW);
  delay(1);
  digitalWrite(W5500_RST, HIGH);
  delay(50);
  Ethernet.init(W5500_CS);
  int leased = Ethernet.begin(mac);
#endif
  if (!leased) {
    Serial.println("no DHCP lease: check the cable and the DHCP server");
    while (true)
      delay(1000);
  }

  sink.begin();
  source.begin();
  cmd.begin(CMD_PORT);
  udpSink.begin(UDP_SINK_PORT);

  Serial.print("IP ");
  Serial.println(Ethernet.localIP());
  Serial.print("Now on the PC:  python3 pc_throughput.py ");
  Serial.println(Ethernet.localIP());
}

// count numbers separated by spaces, from p (no sscanf(): it doesn't fit FRDM-MCXA153's flash with the rest)
bool parseNumbers(const char *p, uint32_t *v, int count) {
  for (int i = 0; i < count; i++) {
    char *end;
    v[i] = strtoul(p, &end, 10);
    if (end == p)
      return false;
    p = end;
  }
  return true;
}

// "<numbers separated by spaces>\n", up to 2 s
bool readHeader(EthernetClient &c, uint32_t *v, int count) {
  c.setTimeout(2000);
  String line = c.readStringUntil('\n');
  return parseNumbers(line.c_str(), v, count);
}

void serveSink() {
  EthernetClient c = sink.accept();
  if (!c)
    return;

  uint32_t n;
  if (readHeader(c, &n, 1)) {
    uint32_t got = 0, first = 0, last = 0;
    uint32_t heard = millis();
    while (got < n && millis() - heard < 5000) {
      int k = c.read(buf, min((uint32_t)sizeof(buf), n - got));
      if (k > 0) {
        if (got == 0)
          first = micros();
        got += k;
        last = micros();
        heard = millis();
      } else if (!c.connected()) {
        break;
      }
    }
    c.print(got == n ? last - first : 0);
    c.print("\n");
    c.flush();

    Serial.print("TCP in:  ");
    Serial.print(got);
    Serial.print(" bytes in ");
    Serial.print(last - first);
    Serial.println(" us");
  }
  delay(5);  // a W5500 takes about 200 ms to close right after a write
  c.stop();
}

void serveSource() {
  EthernetClient c = source.accept();
  if (!c)
    return;

  uint32_t v[2];
  if (readHeader(c, v, 2) && v[1] >= 1 && v[1] <= sizeof(buf)) {
    for (unsigned i = 0; i < sizeof(buf); i++)
      buf[i] = (uint8_t)i;

    uint32_t start = micros();
    uint32_t sent = 0;
    while (sent < v[0]) {
      size_t k = c.write(buf, min(v[1], v[0] - sent));
      if (k == 0)
        break;
      sent += k;
    }
    c.flush();
    uint32_t took = micros() - start;

    Serial.print("TCP out: ");
    Serial.print(sent);
    Serial.print(" bytes, ");
    Serial.print(v[1]);
    Serial.print(" per write(), in ");
    Serial.print(took);
    Serial.println(" us");
  }
  delay(5);
  c.stop();
}

void reply(const String &text) {
  cmd.beginPacket(cmd.remoteIP(), cmd.remotePort());
  cmd.write((const uint8_t *)text.c_str(), text.length());
  cmd.endPacket();
}

void serveCommands() {
  int n = cmd.parsePacket();
  if (n <= 0)
    return;

  char line[64];
  n = cmd.read((unsigned char *)line, sizeof(line) - 1);
  if (n < 0)
    return;
  line[n] = '\0';

  uint32_t v[3];
  if (strncmp(line, "SEND ", 5) == 0 && parseNumbers(line + 5, v, 3) && v[1] >= 1 && v[1] <= 1472) {
    uint32_t count = v[0], size = v[1];
    uint16_t port = v[2];
    IPAddress pc = cmd.remoteIP();
    for (unsigned i = 0; i < size; i++)
      buf[i] = (uint8_t)i;

    uint32_t sent = 0;
    uint32_t start = micros();
    for (uint32_t i = 0; i < count; i++) {
      buf[0] = (uint8_t)i;
      buf[1] = (uint8_t)(i >> 8);
      udpSink.beginPacket(pc, port);
      udpSink.write(buf, size);
      if (udpSink.endPacket())
        sent++;
    }
    uint32_t took = micros() - start;

    reply(String(sent) + " " + took);
    Serial.print("UDP out: ");
    Serial.print(sent);
    Serial.print(" of ");
    Serial.print(count);
    Serial.print(" packets in ");
    Serial.print(took);
    Serial.println(" us");
  } else if (strcmp(line, "COUNT") == 0) {
    reply(String(udpPackets) + " " + udpBytes);
    Serial.print("UDP in:  ");
    Serial.print(udpPackets);
    Serial.println(" packets");
    udpPackets = 0;
    udpBytes = 0;
  } else if (strcmp(line, "TICK") == 0) {
#if ON_CHIP
    reply(String(mcx_ethernet_tick_max_us()));
#else
    reply("-");
#endif
  } else {
    reply("?");
  }
}

void serveUdpSink() {
  int n;
  while ((n = udpSink.parsePacket()) > 0) {
    udpPackets++;
    udpBytes += n;  // the next parsePacket() drops it
  }
}

void loop() {
  serveUdpSink();
  serveCommands();
  serveSink();
  serveSource();
}
