/** Serial.peek() test
 *
 *  Uses Serial1 (D1->D0 jumper) as a loopback source. peek() should return
 *  the next byte without consuming it -- calling it repeatedly must keep
 *  returning the same byte, and available() must stay unchanged, until
 *  read() actually consumes it.
 */

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial1.begin(9600);

  Serial.println("Serial.peek() test (Serial1 loopback, jumper D1->D0)");

  Serial1.print("AB");
  delay(50);  // let the loopback bytes arrive

  Serial.print("available before peek: ");
  Serial.println(Serial1.available());

  int p1 = Serial1.peek();
  int p2 = Serial1.peek();
  Serial.print("peek() x2: ");
  Serial.print((char)p1);
  Serial.print(' ');
  Serial.println((char)p2);

  Serial.print("available after peek (should be unchanged): ");
  Serial.println(Serial1.available());

  int r = Serial1.read();
  Serial.print("read(): ");
  Serial.println((char)r);

  int p3 = Serial1.peek();
  Serial.print("peek() after read (should now be 'B'): ");
  Serial.println((char)p3);

  bool ok = (p1 == 'A') && (p2 == 'A') && (r == 'A') && (p3 == 'B');
  Serial.println(ok ? "peek: OK" : "peek: FAIL");
}

void loop() {
}
