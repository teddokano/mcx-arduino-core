/** WebServerLed: a web page that shows the board's state and turns the green LED on and off.
 *
 *  Connect the board's Ethernet jack to a router, or straight to a PC that
 *  hands out addresses (on a Mac, "Internet Sharing" to the Ethernet port).
 *  Open the Serial Monitor (115200) to see the address, then open
 *  http://<that address>/ in a browser on the same network.
 *
 *  The 1 ms interrupt that runs the network keeps connections going while
 *  loop() is in delay(), as a W5x00 shield does.
 *
 *  FRDM-MCXN947 only (the other boards have no Ethernet). Wiring: none.
 */

#include <MCXEthernet.h>

EthernetServer server(80);

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // the board's LEDs light at LOW
  delay(1000);  // time to open the Serial Monitor

  if (Ethernet.begin() == 0) {
    Serial.println("no address from DHCP: is the cable in?");
    while (true) delay(1);
  }
  server.begin();
  Serial.print("open http://");
  Serial.print(Ethernet.localIP());
  Serial.println("/");
}

void loop() {
  EthernetClient client = server.available();
  if (!client) return;

  // the request line, as "GET /on HTTP/1.1"; the headers after it are skipped
  String request = client.readStringUntil('\n');
  while (client.connected()) {
    String header = client.readStringUntil('\n');
    if (header.length() <= 1) break;  // the empty line ("\r") ends the headers
  }

  if (request.startsWith("GET /on ")) digitalWrite(LED_BUILTIN, LOW);
  if (request.startsWith("GET /off ")) digitalWrite(LED_BUILTIN, HIGH);
  Serial.print(client.remoteIP());
  Serial.print(": ");
  Serial.println(request);

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html><body>");
  client.print("<p>up for ");
  client.print(millis() / 1000);
  client.println(" s</p>");
  client.print("<p>green LED: ");
  client.print(digitalRead(LED_BUILTIN) == LOW ? "on" : "off");
  client.println("</p>");
  client.println("<p><a href=\"/on\">on</a> <a href=\"/off\">off</a></p>");
  client.println("</body></html>");
  client.stop();
}
