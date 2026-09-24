/** Wire as a Stream, and the five-argument requestFrom(), checked
 *  against the on-board temperature sensor on Wire1 (P3T1755 at 0x48).
 *
 *  Wiring: none -- and on FRDM-MCXN947, take the Serial1 loopback jumper
 *  (MB_TX-MB_RX) off first: those are Wire1's SDA/SCL pins there, and the
 *  jumper shorts them together.
 *
 *  The sensor's T_LOW register (0x02) is the one written to: it holds
 *  whatever is put in it and reads it back, and nothing else on the board
 *  depends on it. Its old value is put back at the end.
 *
 *  Automatic: reads "ALL OK" or "N FAILED" at the end.
 */

#include <Arduino.h>
#include <Wire.h>

const uint8_t SENSOR = 0x48;
const uint8_t T_LOW = 0x02;

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

// The classic three-step register read, to compare the new form against.
// 0xFFFFFFFF if the read failed, which no 16-bit register value can be.
uint32_t readReg16(uint8_t reg) {
  Wire1.beginTransmission(SENSOR);
  Wire1.write(reg);
  if (Wire1.endTransmission(false) != 0 || Wire1.requestFrom(SENSOR, (size_t)2) != 2)
    return 0xFFFFFFFF;
  uint16_t v = Wire1.read() << 8;
  return v | Wire1.read();
}

size_t sendThroughPrint(Print &p, const char *s) {
  return p.print(s);
}

int readThroughStream(Stream &s) {
  s.flush();
  return s.read();
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("=== Wire as a Stream / five-argument requestFrom() (no wiring) ===");

  Wire1.begin();
  uint32_t saved = readReg16(T_LOW);
  Serial.print("T_LOW was 0x");
  Serial.println(saved, HEX);
  check("the sensor answers at all (nothing else below means much if not)", saved <= 0xFFFF);

  Serial.println("--- print() into a transaction, through a Print& ---");
  {
    Wire1.beginTransmission(SENSOR);
    Wire1.write(T_LOW);
    size_t n = sendThroughPrint(Wire1, "P");  // 0x50
    Wire1.write(0);                           // used to be ambiguous once Wire is a Print
    uint8_t err = Wire1.endTransmission();
    check("print() through a Print& queues 1 byte", n == 1);
    check("T_LOW written with print() + write(0)", err == 0 && readReg16(T_LOW) == 0x5000);
  }

  Serial.println("--- five-argument requestFrom() ---");
  {
    uint8_t n = Wire1.requestFrom(SENSOR, (uint8_t)2, (uint32_t)T_LOW, (uint8_t)1, (uint8_t)true);
    int msb = Wire1.read();
    int lsb = Wire1.read();
    check("requestFrom(addr, 2, T_LOW, 1, true) reads the register", n == 2 && msb == 0x50 && lsb == 0x00);

    // The same with plain int arguments, as sketches write it
    n = Wire1.requestFrom(0x48, 2, 0x02, 1, 1);
    check("the same with int arguments", n == 2 && Wire1.read() == 0x50);

    // isize == 0: nothing is written first, so this reads on from wherever
    // the register pointer was left -- still T_LOW
    n = Wire1.requestFrom(SENSOR, (uint8_t)2, (uint32_t)0, (uint8_t)0, (uint8_t)true);
    check("isize 0 just reads", n == 2 && Wire1.read() == 0x50);

    // Nothing answers at 0x2A: no bytes, and nothing left to read
    n = Wire1.requestFrom((uint8_t)0x2A, (uint8_t)2, (uint32_t)T_LOW, (uint8_t)1, (uint8_t)true);
    check("absent target: returns 0, available() 0", n == 0 && Wire1.available() == 0);
  }

  Serial.println("--- reading as a Stream ---");
  {
    Wire1.requestFrom(SENSOR, (uint8_t)2, (uint32_t)T_LOW, (uint8_t)1, (uint8_t)true);
    int a = Wire1.available();
    int p1 = Wire1.peek();
    int p2 = Wire1.peek();
    check("peek() doesn't consume", a == 2 && p1 == 0x50 && p2 == 0x50 && Wire1.available() == 2);

    int r = readThroughStream(Wire1);
    check("read() through a Stream&", r == 0x50);

    // A transaction started now must not throw away the byte still unread
    Wire1.beginTransmission(SENSOR);
    Wire1.write(T_LOW);
    check("beginTransmission() keeps unread bytes", Wire1.available() == 1 && Wire1.read() == 0x00);
    Wire1.endTransmission();

    check("peek()/read() -1 once empty", Wire1.peek() == -1 && Wire1.read() == -1);

    uint8_t buf[2] = { 0 };
    Wire1.requestFrom(SENSOR, (uint8_t)2, (uint32_t)T_LOW, (uint8_t)1, (uint8_t)true);
    size_t got = Wire1.readBytes(buf, 2);
    check("readBytes()", got == 2 && buf[0] == 0x50 && buf[1] == 0x00);
  }

  Serial.println("--- buffer limits ---");
  {
    Wire1.clearWriteError();
    Wire1.beginTransmission(SENSOR);
    size_t total = 0;
    for (int i = 0; i < WIRE_BUFFER_SIZE; i++)
      total += Wire1.write((uint8_t)i);
    size_t over = Wire1.write((uint8_t)0);
    uint8_t more[4] = { 1, 2, 3, 4 };
    size_t overBulk = Wire1.write(more, 4);
    check("write() returns 1 per byte up to WIRE_BUFFER_SIZE", total == WIRE_BUFFER_SIZE);
    check("write() past the end returns 0 and sets the write error",
          over == 0 && overBulk == 0 && Wire1.getWriteError() != 0);
    Wire1.clearWriteError();
    Wire1.beginTransmission(SENSOR);  // never sent: throw the 128 bytes away

    // Asking for more than the buffer holds is cut down, not overrun
    uint8_t n = Wire1.requestFrom(SENSOR, (size_t)200);
    check("requestFrom(200) is cut down to WIRE_BUFFER_SIZE", n == WIRE_BUFFER_SIZE && Wire1.available() == WIRE_BUFFER_SIZE);
    while (Wire1.available())
      Wire1.read();
  }

  // Put T_LOW back
  Wire1.beginTransmission(SENSOR);
  Wire1.write(T_LOW);
  Wire1.write((uint8_t)(saved >> 8));
  Wire1.write((uint8_t)saved);
  Wire1.endTransmission();
  check("T_LOW restored", readReg16(T_LOW) == saved);

  Serial.println();
  if (failCount == 0)
    Serial.println("ALL OK");
  else {
    Serial.print(failCount);
    Serial.println(" FAILED");
  }
}

void loop() {
}
