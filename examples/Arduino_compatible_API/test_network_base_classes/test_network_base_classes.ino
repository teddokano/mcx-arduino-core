/** The network base classes: IPAddress, Client, Server and UDP.
 *
 *  IPAddress is checked directly: the ways to make one, text in and out
 *  (IPv4 and IPv6), comparison and the 32-bit form. Client, Server and UDP
 *  are abstract, so the sketch derives small in-memory versions of them,
 *  the way EthernetClient and the like do, and then uses them only through
 *  Client&, Server& and UDP&, as libraries such as HTTP or MQTT clients do.
 *
 *  No wiring. Read the final "ALL PASS"/"N FAIL" line.
 */

#include <Arduino.h>
#include <Client.h>
#include <Server.h>
#include <Udp.h>
#include <IPAddress.h>
#include <cstring>

int fails = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    fails++;
}

// Collects whatever is printed to it, to compare with the expected text.
class TextSink : public Print {
public:
  String text;
  size_t write(uint8_t c) override {
    text += (char)c;
    return 1;
  }
};

void check_text(const char *label, const Printable &p, const char *expect) {
  TextSink sink;
  size_t n = sink.print(p);
  Serial.print(label);
  Serial.print(" -> \"");
  Serial.print(sink.text);
  Serial.print("\": ");
  bool ok = sink.text == expect && n == strlen(expect);
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    fails++;
}

// Parse text that is an address, then check how it prints and that
// toString() gives the same.
void check_parse(const char *text, IPType type, const char *expect) {
  IPAddress ip;
  bool ok = ip.fromString(text) && ip.type() == type && ip.toString() == expect;
  Serial.print("fromString(\"");
  Serial.print(text);
  Serial.print("\") -> ");
  Serial.print(ip);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    fails++;
}

// Text that is not an address: rejected, and the address is left as it was.
void check_reject(const char *text) {
  IPAddress ip(10, 0, 0, 1);
  bool ok = !ip.fromString(text) && ip == IPAddress(10, 0, 0, 1);
  Serial.print("fromString(\"");
  Serial.print(text);
  Serial.print("\") rejected: ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    fails++;
}

// A connection whose "peer" just sends back what it is sent.
class LoopbackClient : public Client {
public:
  uint8_t peer[4] = {};
  uint16_t peer_port = 0;
  bool open = false;

  int connect(IPAddress ip, uint16_t port) override {
    memcpy(peer, rawIPAddress(ip), 4);
    peer_port = port;
    open = true;
    return 1;
  }
  int connect(const char *host, uint16_t port) override {
    IPAddress ip;
    if (!ip.fromString(host))
      return 0;
    return connect(ip, port);
  }
  size_t write(uint8_t c) override { return write(&c, 1); }
  size_t write(const uint8_t *buf, size_t size) override {
    if (!open)
      return 0;
    size_t n = min(size, sizeof(buffer) - length);
    memcpy(buffer + length, buf, n);
    length += n;
    return n;
  }
  int available() override { return length - position; }
  int read() override { return position < length ? buffer[position++] : -1; }
  int read(uint8_t *buf, size_t size) override {
    if (available() == 0)
      return -1;
    size_t n = min(size, (size_t)available());
    memcpy(buf, buffer + position, n);
    position += n;
    return n;
  }
  int peek() override { return position < length ? buffer[position] : -1; }
  void flush() override {}
  void stop() override { open = false; }
  uint8_t connected() override { return open || available(); }
  operator bool() override { return open; }

private:
  uint8_t buffer[128];
  size_t length = 0;
  size_t position = 0;
};

// A server that counts what it would send to its clients.
class CountingServer : public Server {
public:
  bool listening = false;
  size_t sent = 0;
  void begin() override { listening = true; }
  size_t write(uint8_t) override { return listening ? (sent++, 1) : 0; }
};

// A socket whose sent packet comes back as the next received one, from the
// address it was sent to.
class LoopbackUDP : public UDP {
public:
  uint16_t local_port = 0;

  uint8_t begin(uint16_t port) override {
    local_port = port;
    return 1;
  }
  void stop() override { local_port = 0; }
  int beginPacket(IPAddress ip, uint16_t port) override {
    to = ip;
    to_port = port;
    out_length = 0;
    return 1;
  }
  int beginPacket(const char *host, uint16_t port) override {
    IPAddress ip;
    return ip.fromString(host) ? beginPacket(ip, port) : 0;
  }
  int endPacket() override {
    memcpy(packet, out, out_length);
    packet_length = out_length;
    pending = true;
    from = to;
    from_port = to_port;
    return 1;
  }
  size_t write(uint8_t c) override { return write(&c, 1); }
  size_t write(const uint8_t *buffer, size_t size) override {
    size_t n = min(size, sizeof(out) - out_length);
    memcpy(out + out_length, buffer, n);
    out_length += n;
    return n;
  }
  int parsePacket() override {
    if (!pending)
      return 0;
    pending = false;
    in_length = packet_length;
    position = 0;
    return in_length;
  }
  int available() override { return in_length - position; }
  int read() override { return position < in_length ? packet[position++] : -1; }
  int read(unsigned char *buffer, size_t len) override {
    if (available() == 0)
      return -1;
    size_t n = min(len, (size_t)available());
    memcpy(buffer, packet + position, n);
    position += n;
    return n;
  }
  int read(char *buffer, size_t len) override { return read((unsigned char *)buffer, len); }
  int peek() override { return position < in_length ? packet[position] : -1; }
  void flush() override { position = in_length; }
  IPAddress remoteIP() override { return from; }
  uint16_t remotePort() override { return from_port; }

private:
  IPAddress to, from;
  uint16_t to_port = 0, from_port = 0;
  uint8_t out[64], packet[64];
  size_t out_length = 0, packet_length = 0, in_length = 0, position = 0;
  bool pending = false;
};

// What a library sees: only the base classes.
bool talk(Client &c) {
  if (!c.connect("192.168.10.20", 80))
    return false;
  c.print("GET / HTTP/1.1\r\n");
  c.println(42);
  String line = c.readStringUntil('\n');
  long n = c.parseInt();
  return line == "GET / HTTP/1.1\r" && n == 42 && c.connected();
}

void test_ipaddress() {
  Serial.println("--- IPAddress: making one ---");
  IPAddress none;
  check_text("IPAddress()", none, "0.0.0.0");
  check("IPAddress() is IPv4", none.type() == IPv4);
  check_text("IPAddress(IPv6)", IPAddress(IPv6), "::");

  IPAddress ip(192, 168, 0, 1);
  check_text("IPAddress(192, 168, 0, 1)", ip, "192.168.0.1");
  check("[0] and [3]", ip[0] == 192 && ip[3] == 1);
  ip[3] = 99;
  check_text("[3] = 99", ip, "192.168.0.99");

  check("uint32_t form is network byte order in memory", (uint32_t)IPAddress(192, 168, 0, 1) == 0x0100A8C0);
  check_text("IPAddress(0x0100A8C0)", IPAddress((uint32_t)0x0100A8C0), "192.168.0.1");
  const uint8_t octets[4] = { 10, 1, 2, 3 };
  check_text("IPAddress(const uint8_t*)", IPAddress(octets), "10.1.2.3");
  const uint8_t v6[16] = { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
  check_text("IPAddress(IPv6, bytes)", IPAddress(IPv6, v6), "2001:db8::1");
  check_text("IPAddress(16 octets)", IPAddress(0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1), "fe80::1");
  check_text("IPAddress(\"172.16.0.5\")", IPAddress("172.16.0.5"), "172.16.0.5");
  check_text("IPAddress(\"not an address\")", IPAddress("not an address"), "0.0.0.0");
  check("IPv6 [15]", IPAddress(IPv6, v6)[15] == 1);
  check("IPv6 has no uint32_t form", (uint32_t)IPAddress(IPv6, v6) == 0);

  Serial.println("--- IPAddress: text in and out ---");
  check_parse("0.0.0.0", IPv4, "0.0.0.0");
  check_parse("255.255.255.255", IPv4, "255.255.255.255");
  check_parse("8.8.4.4", IPv4, "8.8.4.4");
  check_parse("::", IPv6, "::");
  check_parse("::1", IPv6, "::1");
  check_parse("1::", IPv6, "1::");
  check_parse("fe80::1", IPv6, "fe80::1");
  check_parse("FE80:0000:0000:0000:0000:0000:0000:0001", IPv6, "fe80::1");
  check_parse("2001:db8:0:0:1:0:0:1", IPv6, "2001:db8::1:0:0:1");
  check_parse("2001:db8:0:1:1:1:1:1", IPv6, "2001:db8:0:1:1:1:1:1");
  check_parse("2001:0:0:1:0:0:0:1", IPv6, "2001:0:0:1::1");
  check_parse("1:2:3:4:5:6:7::", IPv6, "1:2:3:4:5:6:7:0");
  check_reject("");
  check_reject("256.1.1.1");
  check_reject("1.2.3");
  check_reject("1.2.3.4.5");
  check_reject("1..2.3");
  check_reject("1.2.3.4 ");
  check_reject("1234.1.1.1");
  check_reject(":");
  check_reject(":::");
  check_reject("1:2:3:4:5:6:7:8:9");
  check_reject("1:2:3:4:5:6:7");
  check_reject("1::2::3");
  check_reject("12345::");
  check_reject("1:");
  check_reject(":1::");
  check_reject("1:2:3:4:5:6:7:8::");
  check_reject("g::1");
  check_reject("::ffff:1.2.3.4");
  check("fromString(String)", ip.fromString(String("1.2.3.4")) && ip == IPAddress(1, 2, 3, 4));

  Serial.println("--- IPAddress: compare and assign ---");
  check("INADDR_NONE is 0.0.0.0", INADDR_NONE == IPAddress(0, 0, 0, 0));
  check("IN6ADDR_ANY is ::", IN6ADDR_ANY == IPAddress(IPv6));
  check("0.0.0.0 != ::", INADDR_NONE != IN6ADDR_ANY);
  check("== bytes", IPAddress(10, 1, 2, 3) == octets);
  check("IPv6 never == bytes", !(IPAddress(IPv6) == (const uint8_t *)v6));
  IPAddress a;
  a = "192.0.2.7";
  check("= \"192.0.2.7\"", a == IPAddress(192, 0, 2, 7));
  a = "junk";
  check("= \"junk\" leaves it", a == IPAddress(192, 0, 2, 7));
  a = (uint32_t)0x0100A8C0;
  check("= 0x0100A8C0", a == IPAddress(192, 168, 0, 1));
  a = octets;
  check("= bytes", a == IPAddress(10, 1, 2, 3));
  a = IPAddress(IPv6, v6);
  a = octets;
  check("IPv6 = bytes becomes IPv4", a.type() == IPv4 && a == IPAddress(10, 1, 2, 3));
  arduino::IPAddress qualified(arduino::IPv4);
  check("arduino::IPAddress", qualified == INADDR_NONE);
  check("toString()", IPAddress(127, 0, 0, 1).toString() == "127.0.0.1");
}

void test_client_server_udp() {
  Serial.println("--- Client ---");
  LoopbackClient lc;
  Client &c = lc;
  check("not connected yet", !c && !c.connected());
  check("connect, print, read back through Client&", talk(c));
  check("rawIPAddress() gave the bytes", lc.peer[0] == 192 && lc.peer[3] == 20 && lc.peer_port == 80);
  c.stop();
  check("stop() leaves the unread \"\\r\\n\" readable", !c && c.connected() && c.available() == 2);
  while (c.read() >= 0)
    ;
  check("connected() is 0 once it is read", !c.connected());
  check("connect(\"bad host\") fails", c.connect("bad host", 80) == 0);

  Serial.println("--- Server ---");
  CountingServer cs;
  Server &s = cs;
  check("write before begin()", s.print("x") == 0);
  s.begin();
  check("print through Server&", s.println("hello") == 7 && cs.sent == 7);

  Serial.println("--- UDP ---");
  LoopbackUDP lu;
  UDP &u = lu;
  u.setTimeout(10);  // parseInt() below waits this long after the last digit
  check("begin(8888)", u.begin(8888) == 1 && lu.local_port == 8888);
  check("beginMulticast() fails by default", u.beginMulticast(IPAddress(239, 0, 0, 1), 5000) == 0);
  check("no packet yet", u.parsePacket() == 0);
  u.beginPacket(IPAddress(192, 168, 1, 50), 123);
  u.print("time?");
  u.write((uint8_t)'!');
  check("endPacket()", u.endPacket() == 1);
  check("parsePacket() size", u.parsePacket() == 6);
  check("remoteIP()", u.remoteIP() == IPAddress(192, 168, 1, 50) && u.remotePort() == 123);
  char text[8] = {};
  check("read(char*)", u.read(text, 5) == 5 && strcmp(text, "time?") == 0);
  check("peek(), read()", u.peek() == '!' && u.read() == '!' && u.read() == -1);
  u.beginPacket("10.0.0.2", 7);
  u.print(1234);
  u.endPacket();
  u.parsePacket();
  check("beginPacket(host) and parseInt()", u.parseInt() == 1234 && u.remoteIP() == IPAddress(10, 0, 0, 2));
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Network base classes test");
  test_ipaddress();
  test_client_server_udp();

  Serial.println();
  if (fails == 0)
    Serial.println("ALL PASS");
  else {
    Serial.print(fails);
    Serial.println(" FAIL");
  }
}

void loop() {
}
