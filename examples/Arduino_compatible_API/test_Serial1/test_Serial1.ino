/** Serial1 (D0/D1 hardware UART, separate from the USB-bridged Serial) test
 *
 *  Wiring: jumper D1 (Serial1 TX) <-> D0 (Serial1 RX) to loop Serial1 back
 *  to itself. Serial (USB) is used to report what was received.
 */

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial1.begin(9600);

  Serial.println("Serial1 loopback test (D1->D0 jumper)");

  Serial1.println("hello from Serial1");
  delay(50);  // let the loopback bytes arrive

  Serial.print("Serial1.available() = ");
  Serial.println(Serial1.available());

  Serial.print("Serial1 received: \"");
  while (Serial1.available()) {
    Serial.print((char)Serial1.read());
  }
  Serial.println("\"");
}

void loop() {
}
