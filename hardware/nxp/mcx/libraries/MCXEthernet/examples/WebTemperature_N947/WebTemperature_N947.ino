/** WebTemperature: the on-board temperature sensor, as a web page and as JSON.
 *
 *  Connect the board's Ethernet jack to a router, or straight to a PC that
 *  hands out addresses (on a Mac, "Internet Sharing" to the Ethernet port).
 *  Open the Serial Monitor (115200) to see the address, then open
 *  http://<that address>/ in a browser on the same network: the page
 *  reloads itself every 2 seconds. http://<that address>/json gives the
 *  same reading for a program, as {"celsius":24.56,"uptime_s":42}
 *  (curl http://<that address>/json on a PC).
 *
 *  The sensor is the board's P3T1755, on Wire1 at 0x48, read here by its
 *  registers so that the sketch needs no other library.
 *
 *  FRDM-MCXN947 only (the other boards have no Ethernet). Wiring: none.
 */

#include <MCXEthernet.h>
#include <Wire.h>

const uint8_t SENSOR_ADDR = 0x48;  // P3T1755
const uint8_t TEMP_REG = 0x00;

EthernetServer server(80);

// the temperature in degrees Celsius, NAN if the sensor doesn't answer
float readCelsius() {
  Wire1.beginTransmission(SENSOR_ADDR);
  Wire1.write(TEMP_REG);
  if (Wire1.endTransmission(false) != 0) return NAN;
  if (Wire1.requestFrom(SENSOR_ADDR, (uint8_t)2) != 2) return NAN;
  uint8_t msb = Wire1.read();
  uint8_t lsb = Wire1.read();
  return (int16_t)((msb << 8) | lsb) / 256.0;  // 12 bits, 0.0625 degC a step
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // time to open the Serial Monitor
  Wire1.begin();

  if (Ethernet.begin() == 0) {
    Serial.println("no address from DHCP: is the cable in?");
    while (true) delay(1);
  }
  server.begin();
  Serial.print("open http://");
  Serial.print(Ethernet.localIP());
  Serial.println("/  (or /json)");
}

void loop() {
  EthernetClient client = server.available();
  if (!client) return;

  // the request line, as "GET /json HTTP/1.1"; the headers after it are skipped
  String request = client.readStringUntil('\n');
  while (client.connected()) {
    String header = client.readStringUntil('\n');
    if (header.length() <= 1) break;  // the empty line ("\r") ends the headers
  }

  float celsius = readCelsius();
  Serial.print(client.remoteIP());
  Serial.print(": ");
  Serial.print(request);
  Serial.print(" -> ");
  Serial.println(celsius, 2);

  client.println("HTTP/1.1 200 OK");
  client.println("Connection: close");
  if (request.startsWith("GET /json ")) {
    client.println("Content-Type: application/json");
    client.println();
    client.print("{\"celsius\":");
    if (isnan(celsius))
      client.print("null");  // JSON has no NaN
    else
      client.print(celsius, 2);
    client.print(",\"uptime_s\":");
    client.print(millis() / 1000);
    client.println("}");
  } else {
    client.println("Content-Type: text/html");
    client.println();
    client.println("<!DOCTYPE html><html><head>");
    client.println("<meta http-equiv=\"refresh\" content=\"2\">");
    client.println("<title>FRDM-MCXN947</title></head><body>");
    client.print("<h1>");
    if (isnan(celsius))
      client.print("no answer from the sensor");
    else {
      client.print(celsius, 2);
      client.print(" &deg;C");
    }
    client.println("</h1>");
    client.print("<p>up for ");
    client.print(millis() / 1000);
    client.println(" s</p>");
    client.println("</body></html>");
  }
  client.stop();
}
