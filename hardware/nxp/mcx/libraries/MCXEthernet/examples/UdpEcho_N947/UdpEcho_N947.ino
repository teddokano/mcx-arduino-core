/** UdpEcho: sends every UDP packet that arrives on port 8888 back to its sender.
 *
 *  Connect the board's Ethernet jack to a router, or straight to a PC that
 *  hands out addresses (on a Mac, "Internet Sharing" to the Ethernet port).
 *  Open the Serial Monitor (115200) to see the address, then from a PC on
 *  the same network:
 *
 *      nc -u <that address> 8888
 *
 *  and type a line: it comes back.
 *
 *  FRDM-MCXN947 only (the other boards have no Ethernet). Wiring: none.
 */

#include <MCXEthernet.h>

const uint16_t PORT = 8888;

EthernetUDP udp;
uint8_t packet[1472];  // the largest packet the library sends

void setup() {
  Serial.begin(115200);
  delay(1000);  // time to open the Serial Monitor

  if (Ethernet.begin() == 0) {
    Serial.println("no address from DHCP: is the cable in?");
    while (true) delay(1);
  }
  udp.begin(PORT);
  Serial.print("echoing UDP on ");
  Serial.print(Ethernet.localIP());
  Serial.print(" port ");
  Serial.println(PORT);
}

void loop() {
  int size = udp.parsePacket();
  if (size <= 0) return;

  int n = udp.read(packet, sizeof(packet));
  Serial.print(n);
  Serial.print(" bytes from ");
  Serial.print(udp.remoteIP());
  Serial.print(":");
  Serial.println(udp.remotePort());

  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.write(packet, n);
  udp.endPacket();
}
