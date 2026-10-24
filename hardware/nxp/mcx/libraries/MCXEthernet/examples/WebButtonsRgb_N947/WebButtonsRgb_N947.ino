/** WebButtonsRgb: the board's buttons and RGB LED on a web page that updates itself.
 *
 *  Connect the board's Ethernet jack to a router, or straight to a PC that
 *  hands out addresses (on a Mac, "Internet Sharing" to the Ethernet port).
 *  Open the Serial Monitor (115200) to see the address, then open
 *  http://<that address>/ in a browser on the same network.
 *
 *  The page shows whether SW2 and SW3 are pressed, and has a button for each
 *  color of the RGB LED. A script in the page asks the board for
 *  /state about 4 times a second, so pressing a button shows on the page at
 *  once; /led?c=<color> sets the LED. The board serves each request on a
 *  new connection, as a W5x00 shield would.
 *
 *    /                 the page
 *    /state            {"sw2":false,"sw3":true,"led":"red"}
 *    /led?c=red        and green, blue, yellow, cyan, magenta, white, off
 *
 *  FRDM-MCXN947 only (the other boards have no Ethernet). Wiring: none.
 *  SW2 is the same pin as A5.
 */

#include <MCXEthernet.h>

EthernetServer server(80);

// the colors, as which of red, green and blue are lit
struct Color {
  const char *name;
  bool r, g, b;
};
const Color COLORS[] = {
  { "off", false, false, false },
  { "red", true, false, false },
  { "green", false, true, false },
  { "blue", false, false, true },
  { "yellow", true, true, false },
  { "cyan", false, true, true },
  { "magenta", true, false, true },
  { "white", true, true, true },
};
const int NUM_COLORS = sizeof(COLORS) / sizeof(COLORS[0]);
int color = 0;

const char PAGE[] =
  "<!DOCTYPE html><html><head><title>FRDM-MCXN947</title>"
  "<style>body{font-family:sans-serif}b{display:inline-block;width:5em}"
  "button{margin:2px;padding:6px 10px}</style></head><body>"
  "<p><b>SW2</b><span id=sw2>?</span></p>"
  "<p><b>SW3</b><span id=sw3>?</span></p>"
  "<p><b>LED</b><span id=led>?</span></p><p>"
  "<button onclick=\"set('red')\">red</button>"
  "<button onclick=\"set('green')\">green</button>"
  "<button onclick=\"set('blue')\">blue</button>"
  "<button onclick=\"set('yellow')\">yellow</button>"
  "<button onclick=\"set('cyan')\">cyan</button>"
  "<button onclick=\"set('magenta')\">magenta</button>"
  "<button onclick=\"set('white')\">white</button>"
  "<button onclick=\"set('off')\">off</button></p>"
  "<script>"
  "function show(s){"
  "sw2.textContent=s.sw2?'pressed':'-';"
  "sw3.textContent=s.sw3?'pressed':'-';"
  "led.textContent=s.led}"
  "function set(c){fetch('/led?c='+c).then(r=>r.json()).then(show)}"
  "function poll(){fetch('/state').then(r=>r.json()).then(show)"
  ".finally(()=>setTimeout(poll,250))}"
  "poll()"
  "</script></body></html>";

void showColor() {
  // the board's LEDs light at LOW
  digitalWrite(RED, COLORS[color].r ? LOW : HIGH);
  digitalWrite(GREEN, COLORS[color].g ? LOW : HIGH);
  digitalWrite(BLUE, COLORS[color].b ? LOW : HIGH);
}

void sendState(EthernetClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Cache-Control: no-store");
  client.println("Connection: close");
  client.println();
  client.print("{\"sw2\":");
  client.print(digitalRead(SW2) == LOW ? "true" : "false");  // pressed is LOW
  client.print(",\"sw3\":");
  client.print(digitalRead(SW3) == LOW ? "true" : "false");
  client.print(",\"led\":\"");
  client.print(COLORS[color].name);
  client.println("\"}");
}

void setup() {
  Serial.begin(115200);
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  showColor();
  pinMode(SW2, INPUT_PULLUP);
  pinMode(SW3, INPUT_PULLUP);
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

  // the request line, as "GET /led?c=red HTTP/1.1"; the headers after it are skipped
  String request = client.readStringUntil('\n');
  while (client.connected()) {
    String header = client.readStringUntil('\n');
    if (header.length() <= 1) break;  // the empty line ("\r") ends the headers
  }

  if (request.startsWith("GET /led?c=")) {
    String name = request.substring(11, request.indexOf(' ', 11));
    for (int i = 0; i < NUM_COLORS; i++)
      if (name == COLORS[i].name) color = i;
    showColor();
    Serial.print("LED: ");
    Serial.println(COLORS[color].name);
    sendState(client);
  } else if (request.startsWith("GET /state ")) {
    sendState(client);
  } else if (request.startsWith("GET / ")) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.print(PAGE);
  } else {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Connection: close");
    client.println();
  }
  client.stop();
}
