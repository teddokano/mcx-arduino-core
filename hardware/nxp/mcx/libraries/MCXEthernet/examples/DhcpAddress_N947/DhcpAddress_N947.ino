/** DhcpAddress: gets an address from a DHCP server and shows it.
 *
 *  Connect the board's Ethernet jack to a router, or straight to a PC that
 *  hands out addresses (on a Mac, "Internet Sharing" to the Ethernet port),
 *  and open the Serial Monitor (115200).
 *
 *  begin() with no MAC address makes one from the chip's unique ID, so each
 *  board keeps the same address from start to start. The sketch then shows
 *  the link going down and up as the cable is pulled and put back.
 *
 *  FRDM-MCXN947 only (the other boards have no Ethernet). Wiring: none.
 */

#include <MCXEthernet.h>

EthernetLinkStatus lastLink = Unknown;

void printMAC() {
  uint8_t mac[6];
  Ethernet.MACAddress(mac);
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 0x10) Serial.print('0');
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(':');
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // time to open the Serial Monitor

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("no Ethernet PHY found");
    while (true) delay(1);
  }

  Serial.println("asking for an address...");
  if (Ethernet.begin() == 0) {  // waits up to 60 s
    Serial.print("no address: ");
    Serial.println(Ethernet.linkStatus() == LinkOFF ? "the cable isn't connected" : "no answer from a DHCP server");
    while (true) delay(1);
  }

  Serial.print("MAC address: ");
  printMAC();
  Serial.print("IP address:  ");
  Serial.println(Ethernet.localIP());
  Serial.print("subnet mask: ");
  Serial.println(Ethernet.subnetMask());
  Serial.print("gateway:     ");
  Serial.println(Ethernet.gatewayIP());
  Serial.print("DNS server:  ");
  Serial.println(Ethernet.dnsServerIP());
  lastLink = Ethernet.linkStatus();
}

void loop() {
  EthernetLinkStatus link = Ethernet.linkStatus();
  if (link != lastLink) {
    Serial.println(link == LinkON ? "link up" : "link down");
    lastLink = link;
  }
  Ethernet.maintain();  // lwIP renews the lease by itself; kept for sketches written for the official library
  delay(100);
}
