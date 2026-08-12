/** Serial.flush() test
 *
 *  Uses Serial1 (a real hardware UART, not the USB-CDC bridge) at a slow
 *  baud rate so the physical transmission time is large enough to measure.
 *  If flush() only waited for the software TX ring buffer to drain (and
 *  not for the hardware shift register to actually finish), the measured
 *  time would be much shorter than the expected transmission time.
 */

#include <Arduino.h>

#define BAUD 9600
const char *msg = "Hello, Serial1 world!\r\n";  // 24 bytes

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial1.begin(BAUD);

  Serial.println("Serial.flush() test (via Serial1 @ 9600 baud)");
}

void loop() {
  Serial1.print(msg);

  unsigned long t0 = micros();
  Serial1.flush();
  unsigned long elapsed = micros() - t0;

  // 10 bits/byte (start + 8 data + stop) at BAUD bps
  unsigned long expected_us = (unsigned long)strlen(msg) * 10UL * 1000000UL / BAUD;

  Serial.print("flush() blocked for ");
  Serial.print(elapsed);
  Serial.print(" us (expected roughly ");
  Serial.print(expected_us);
  Serial.println(" us)");

  if (elapsed < expected_us / 2)
    Serial.println("  <-- WARNING: flush() returned much sooner than expected!");

  delay(2000);
}
