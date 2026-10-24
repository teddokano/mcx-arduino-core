/** HttpGet: looks up a host name with DNS and fetches a web page from it.
 *
 *  Connect the board's Ethernet jack to a router with an Internet
 *  connection, or straight to a Mac sharing its connection ("Internet
 *  Sharing" to the Ethernet port), and open the Serial Monitor (115200).
 *  The sketch prints the reply's status line and headers, and how many
 *  bytes of page came after them.
 *
 *  FRDM-MCXN947 only (the other boards have no Ethernet). Wiring: none.
 */

#include <MCXEthernet.h>

const char HOST[] = "example.com";

EthernetClient client;

void setup() {
  Serial.begin(115200);
  delay(1000);  // time to open the Serial Monitor

  if (Ethernet.begin() == 0) {
    Serial.println("no address from DHCP: is the cable in?");
    while (true) delay(1);
  }

  IPAddress ip;
  if (Ethernet.hostByName(HOST, ip) == 0) {
    Serial.print("couldn't look up ");
    Serial.println(HOST);
    while (true) delay(1);
  }
  Serial.print(HOST);
  Serial.print(" is ");
  Serial.println(ip);

  if (!client.connect(ip, 80)) {
    Serial.println("couldn't connect");
    while (true) delay(1);
  }
  client.print("GET / HTTP/1.1\r\nHost: ");
  client.print(HOST);
  client.print("\r\nConnection: close\r\n\r\n");

  // the status line and the headers, up to the empty line
  while (client.connected() || client.available()) {
    String line = client.readStringUntil('\n');
    if (line.length() <= 1) break;
    Serial.println(line);
  }

  // the page itself: count it until the server closes the connection
  unsigned long body = 0;
  while (client.connected() || client.available()) {
    if (client.available()) {
      client.read();
      body++;
    }
  }
  client.stop();
  Serial.print(body);
  Serial.println(" bytes of page");
}

void loop() {
}
