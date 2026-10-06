/** Wire as a Stream, and the five-argument requestFrom(), checked
 *  against the on-board temperature sensor on Wire1 (P3T1755 at 0x48).
 *
 *  Wiring: none -- and on FRDM-MCXN947, take the Serial1 loopback jumper
 *  (MB_TX-MB_RX) off first: those are Wire1's SDA/SCL pins there, and the
 *  jumper shorts them together.
 *
 *  On FRDM-MCXA156 the sensor is on Wire (D18/D19), and the sketch uses
 *  that; the messages still say Wire1.
 *
 *  The sensor's T_LOW register (0x02) is the one written to: it holds
 *  whatever is put in it and reads it back, and nothing else on the board
 *  depends on it. Its old value is put back at the end.
 *
 *  Automatic: reads "ALL OK" or "N FAILED" at the end.
 */

#include <Arduino.h>
#include <Wire.h>

// The on-board P3T1755's bus: Wire1 on FRDM-MCXA153 and FRDM-MCXN947,
// Wire (D18/D19) on FRDM-MCXA156, whose sensor is on the Arduino I2C pins
// FRDM-MCXN236 has none on board: connect one to its MikroBus I2C (Wire1)
#if defined(FRDM_MCXA153) || defined(FRDM_MCXN947) || defined(FRDM_MCXN236)
#define SENSOR_WIRE Wire1
#elif defined(FRDM_MCXA156)
#define SENSOR_WIRE Wire
#else
#error "This sketch has no settings for this board yet"
#endif

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
  SENSOR_WIRE.beginTransmission(SENSOR);
  SENSOR_WIRE.write(reg);
  if (SENSOR_WIRE.endTransmission(false) != 0 || SENSOR_WIRE.requestFrom(SENSOR, (size_t)2) != 2)
    return 0xFFFFFFFF;
  uint16_t v = SENSOR_WIRE.read() << 8;
  return v | SENSOR_WIRE.read();
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

  SENSOR_WIRE.begin();
  uint32_t saved = readReg16(T_LOW);
  Serial.print("T_LOW was 0x");
  Serial.println(saved, HEX);
  check("the sensor answers at all (nothing else below means much if not)", saved <= 0xFFFF);

  Serial.println("--- print() into a transaction, through a Print& ---");
  {
    SENSOR_WIRE.beginTransmission(SENSOR);
    SENSOR_WIRE.write(T_LOW);
    size_t n = sendThroughPrint(SENSOR_WIRE, "P");  // 0x50
    SENSOR_WIRE.write(0);                           // used to be ambiguous once Wire is a Print
    uint8_t err = SENSOR_WIRE.endTransmission();
    check("print() through a Print& queues 1 byte", n == 1);
    check("T_LOW written with print() + write(0)", err == 0 && readReg16(T_LOW) == 0x5000);
  }

  Serial.println("--- five-argument requestFrom() ---");
  {
    uint8_t n = SENSOR_WIRE.requestFrom(SENSOR, (uint8_t)2, (uint32_t)T_LOW, (uint8_t)1, (uint8_t)true);
    int msb = SENSOR_WIRE.read();
    int lsb = SENSOR_WIRE.read();
    check("requestFrom(addr, 2, T_LOW, 1, true) reads the register", n == 2 && msb == 0x50 && lsb == 0x00);

    // The same with plain int arguments, as sketches write it
    n = SENSOR_WIRE.requestFrom(0x48, 2, 0x02, 1, 1);
    check("the same with int arguments", n == 2 && SENSOR_WIRE.read() == 0x50);

    // isize == 0: nothing is written first, so this reads on from wherever
    // the register pointer was left -- still T_LOW
    n = SENSOR_WIRE.requestFrom(SENSOR, (uint8_t)2, (uint32_t)0, (uint8_t)0, (uint8_t)true);
    check("isize 0 just reads", n == 2 && SENSOR_WIRE.read() == 0x50);

    // Nothing answers at 0x2A: no bytes, and nothing left to read
    n = SENSOR_WIRE.requestFrom((uint8_t)0x2A, (uint8_t)2, (uint32_t)T_LOW, (uint8_t)1, (uint8_t)true);
    check("absent target: returns 0, available() 0", n == 0 && SENSOR_WIRE.available() == 0);
  }

  Serial.println("--- reading as a Stream ---");
  {
    SENSOR_WIRE.requestFrom(SENSOR, (uint8_t)2, (uint32_t)T_LOW, (uint8_t)1, (uint8_t)true);
    int a = SENSOR_WIRE.available();
    int p1 = SENSOR_WIRE.peek();
    int p2 = SENSOR_WIRE.peek();
    check("peek() doesn't consume", a == 2 && p1 == 0x50 && p2 == 0x50 && SENSOR_WIRE.available() == 2);

    int r = readThroughStream(SENSOR_WIRE);
    check("read() through a Stream&", r == 0x50);

    // A transaction started now must not throw away the byte still unread
    SENSOR_WIRE.beginTransmission(SENSOR);
    SENSOR_WIRE.write(T_LOW);
    check("beginTransmission() keeps unread bytes", SENSOR_WIRE.available() == 1 && SENSOR_WIRE.read() == 0x00);
    SENSOR_WIRE.endTransmission();

    check("peek()/read() -1 once empty", SENSOR_WIRE.peek() == -1 && SENSOR_WIRE.read() == -1);

    uint8_t buf[2] = { 0 };
    SENSOR_WIRE.requestFrom(SENSOR, (uint8_t)2, (uint32_t)T_LOW, (uint8_t)1, (uint8_t)true);
    size_t got = SENSOR_WIRE.readBytes(buf, 2);
    check("readBytes()", got == 2 && buf[0] == 0x50 && buf[1] == 0x00);
  }

  Serial.println("--- buffer limits ---");
  {
    SENSOR_WIRE.clearWriteError();
    SENSOR_WIRE.beginTransmission(SENSOR);
    size_t total = 0;
    for (int i = 0; i < WIRE_BUFFER_SIZE; i++)
      total += SENSOR_WIRE.write((uint8_t)i);
    size_t over = SENSOR_WIRE.write((uint8_t)0);
    uint8_t more[4] = { 1, 2, 3, 4 };
    size_t overBulk = SENSOR_WIRE.write(more, 4);
    check("write() returns 1 per byte up to WIRE_BUFFER_SIZE", total == WIRE_BUFFER_SIZE);
    check("write() past the end returns 0 and sets the write error",
          over == 0 && overBulk == 0 && SENSOR_WIRE.getWriteError() != 0);
    SENSOR_WIRE.clearWriteError();
    SENSOR_WIRE.beginTransmission(SENSOR);  // never sent: throw the 128 bytes away

    // Asking for more than the buffer holds is cut down, not overrun
    uint8_t n = SENSOR_WIRE.requestFrom(SENSOR, (size_t)200);
    check("requestFrom(200) is cut down to WIRE_BUFFER_SIZE", n == WIRE_BUFFER_SIZE && SENSOR_WIRE.available() == WIRE_BUFFER_SIZE);
    while (SENSOR_WIRE.available())
      SENSOR_WIRE.read();
  }

  // Put T_LOW back
  SENSOR_WIRE.beginTransmission(SENSOR);
  SENSOR_WIRE.write(T_LOW);
  SENSOR_WIRE.write((uint8_t)(saved >> 8));
  SENSOR_WIRE.write((uint8_t)saved);
  SENSOR_WIRE.endTransmission();
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
