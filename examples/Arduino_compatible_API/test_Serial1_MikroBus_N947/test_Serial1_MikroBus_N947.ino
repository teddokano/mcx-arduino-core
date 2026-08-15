/** Serial1 (MikroBus hardware UART, MB_TX/MB_RX) test for FRDM-MCXN947
 *
 *  Serial1 here is on the MikroBus header (MB_TX=P1_17, MB_RX=P1_16), backed
 *  by its own peripheral (LPUART5/FlexComm5) -- not D0/D1, which this board
 *  can't support as Serial1 (FlexComm2 conflict with Wire). These are the
 *  same physical pins as I3C_SDA/I3C_SCL (Wire1) and plain GPIO; only one
 *  function is active at a time, switched by whichever begin()/pinMode()
 *  was called most recently.
 *
 *  Wiring: jumper MB_TX <-> MB_RX to loop Serial1 back to itself. Serial
 *  (USB) is used to report what was received.
 */

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial1.begin(9600);

  Serial.println("Serial1 loopback test (MB_TX->MB_RX jumper)");

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
