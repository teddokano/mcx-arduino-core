/** I2C target (slave) demo -- pair it with Wire_controller_demo on
 *  another board.
 *
 *  This board answers at address 0x08 and does nothing on the bus by
 *  itself:
 *    - when the controller writes, it prints the text it got and toggles
 *      the green LED
 *    - when the controller reads, it replies with how many times it has
 *      been asked, as 7 characters ("ok    3")
 *
 *  Wiring, between the two boards:
 *    D18 (SDA) -- D18 (SDA)
 *    D19 (SCL) -- D19 (SCL)
 *    GND       -- GND
 *
 *  Works on FRDM-MCXA153 and FRDM-MCXN947, on Wire only (not Wire1 or
 *  Wire2).
 */

#include <Arduino.h>
#include <Wire.h>

const uint8_t MY_ADDRESS = 0x08;

// Filled in by receiveEvent(), printed by loop()
char message[33];
volatile bool gotMessage = false;
volatile uint32_t requests = 0;

// Runs from the I2C interrupt when the controller has written to us.
// Keep it short: store what came in, and print it from loop().
void receiveEvent(int howMany) {
  int i = 0;
  while (Wire.available() && i < (int)sizeof(message) - 1)
    message[i++] = (char)Wire.read();
  message[i] = '\0';
  while (Wire.available())  // anything longer than message[] is dropped
    Wire.read();
  (void)howMany;
  gotMessage = true;
}

// Runs from the I2C interrupt when the controller reads from us.
// Whatever write() queues here is the reply.
void requestEvent() {
  requests = requests + 1;
  char reply[8];
  snprintf(reply, sizeof(reply), "ok%5lu", (unsigned long)requests);
  Wire.write((const uint8_t *)reply, 7);
}

// This demo's bus has no pull-up resistors, so it uses the pins' internal
// ones. A real I2C bus should have external pull-ups (e.g. 4.7k to 3.3V)
// on SDA and SCL instead; with those fitted, leave this out.
void enableInternalPullUps() {
#if defined(FRDM_MCXN947)
  PORT4->PCR[0] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;  // D18 = P4_0
  PORT4->PCR[1] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;  // D19 = P4_1
#else
  PORT1->PCR[8] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;  // D18 = P1_8
  PORT1->PCR[9] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;  // D19 = P1_9
#endif
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  Wire.begin(MY_ADDRESS);
  enableInternalPullUps();

  Serial.print("I2C target ready at address 0x");
  Serial.println(MY_ADDRESS, HEX);
}

void loop() {
  if (gotMessage) {
    gotMessage = false;
    Serial.print("received: \"");
    Serial.print(message);
    Serial.print("\"   (replies sent so far: ");
    Serial.print(requests);
    Serial.println(")");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}
