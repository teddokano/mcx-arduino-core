#include <Arduino.h>

byte b = 200;
word w = 5000;
boolean flag = true;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.print("min(3,7)="); Serial.println(min(3, 7));
  Serial.print("max(3,7)="); Serial.println(max(3, 7));
  Serial.print("abs(-5)="); Serial.println(abs(-5));
  Serial.print("constrain(15,0,10)="); Serial.println(constrain(15, 0, 10));
  Serial.print("sq(4)="); Serial.println(sq(4));
  Serial.print("map(512,0,1023,0,255)="); Serial.println(map(512, 0, 1023, 0, 255));

  uint8_t v = 0;
  bitSet(v, 3);
  Serial.print("bitSet/bitRead="); Serial.println(bitRead(v, 3));
  bitClear(v, 3);
  Serial.print("after bitClear="); Serial.println(bitRead(v, 3));

  Serial.print("lowByte(0x1234)="); Serial.println(lowByte(0x1234), HEX);
  Serial.print("highByte(0x1234)="); Serial.println(highByte(0x1234), HEX);
  Serial.print("bit(3)="); Serial.println(bit(3));

  noInterrupts();
  interrupts();
  Serial.println("interrupts()/noInterrupts() OK");

  Serial.print("byte/word/boolean="); Serial.print(b); Serial.print(" "); Serial.print(w); Serial.print(" "); Serial.println(flag);
}

void loop() {
}
