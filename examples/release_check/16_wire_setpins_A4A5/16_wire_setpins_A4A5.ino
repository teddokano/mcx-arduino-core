/** Release check 16: Wire.setPins() moves a bus to A4/A5 and back --
 *  jumper wires only, so it sits in the 1n group (see
 *  examples/release_check/README.md for the numbering).
 *
 *  A4/A5 have an I2C on FRDM-MCXA156 (LPI2C1), FRDM-MCXN236 (LPI2C2,
 *  Wire1's own) and FRDM-MCXC444 (I2C1, Wire's own). On those it checks
 *  that a bus moved there really comes out of A4/A5 (a GPIO jumpered to
 *  A5 sees its SCL edges, and holding it low trips setWireTimeout()), that
 *  the pins it left go back to GPIO, that it works there as a controller
 *  and, where one is on the bus, reads the board's own sensor, and that
 *  setPins() says false where it should. FRDM-MCXA153 and FRDM-MCXN947
 *  have no I2C on A4/A5: only the false cases run there.
 *
 *  Wiring, one jumper wire per pin:
 *    FRDM-MCXA156, FRDM-MCXN236: A4-D18, A5-D19, nothing else on them.
 *      One bus out of A4/A5 and Wire's D18/D19 (with FRDM-MCXA156's
 *      P3T1755 on it), so that Wire1 moved to A4/A5 and Wire on D18/D19
 *      can be each other's target. D19 watches and holds SCL while Wire
 *      is off it. FRDM-MCXN236's accelerometer is on the MikroBus I2C,
 *      which A4/A5 don't reach here: it is read once Wire1 is back there
 *    FRDM-MCXC444: A5-D8, A4-D7, nothing else on A4/A5. D8 watches and
 *      holds SCL (D18/D19 are on port E, which has no pin interrupts)
 *    FRDM-MCXA153, FRDM-MCXN947: none
 *  A4/A5 have no pull-up resistors on FRDM-MCXA156 and FRDM-MCXN236; the
 *  bus is pulled up by the ones on D18/D19 there. FRDM-MCXC444 has 4.7k
 *  on A4/A5.
 *
 *  Whether a pin is a GPIO is read from its PCR with two of the core's
 *  internal functions (pin_registry_read_pcr(), arduino_pin_raw()), which
 *  a sketch otherwise has no use for.
 *
 *  The panic() that begin() stops in when another TwoWire runs the same
 *  peripheral isn't checked here: it ends the sketch.
 *
 *  Automatic: reads "ALL OK" or "N FAILED" at the end.
 */

#include <Arduino.h>
#include <Wire.h>
#include "pin_registry.h"

int arduino_pin_raw(int pin_num);  // the core's; see above

#if defined(FRDM_MCXA156) || defined(FRDM_MCXN236)
#define HAS_A4A5 1
const int WATCH_SCL = D19;
#elif defined(FRDM_MCXC444)
#define HAS_A4A5 1
const int WATCH_SCL = D8;
#elif defined(FRDM_MCXA153) || defined(FRDM_MCXN947)
#define HAS_A4A5 0
#else
#error "This sketch has no settings for this board yet"
#endif

const uint8_t NOBODY = 0x50;  // nothing answers here

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

bool isGpio(int pin) {
  return pin_registry_read_pcr((uint8_t)arduino_pin_raw(pin)).mux == kPORT_MuxAsGpio;
}

#if HAS_A4A5
volatile int edges;

void onEdge() {
  edges = edges + 1;
}

// SCL's falling edges, seen on WATCH_SCL, during one address-only transfer
// at 10kHz, so that every edge's interrupt has time to run
int sclEdges(TwoWire &w) {
  w.setClock(10000);
  edges = 0;
  attachInterrupt(digitalPinToInterrupt(WATCH_SCL), onEdge, FALLING);
  w.beginTransmission(NOBODY);
  w.endTransmission();
  detachInterrupt(digitalPinToInterrupt(WATCH_SCL));
  w.setClock(100000);
  return edges;
}

// Hold SCL low through WATCH_SCL before a transfer, as 14 does: the bus on
// A4/A5 must see it and not hang -- an LPI2C takes the bus for busy and
// refuses at once, FRDM-MCXC444's I2C starts and times out (AVR's 5) --
// and work again once SCL is let go
void heldLow(TwoWire &w) {
  w.setWireTimeout(25000, true);
  pinMode(WATCH_SCL, OUTPUT);
  digitalWrite(WATCH_SCL, LOW);
  unsigned long t0 = micros();
  w.beginTransmission(NOBODY);
  uint8_t r = w.endTransmission();
  unsigned long us = micros() - t0;
  Serial.print("  returned ");
  Serial.print(r);
  Serial.print(" in ");
  Serial.print(us);
  Serial.println("us");
#if defined(FRDM_MCXC444)
  check("SCL held low: times out (5) instead of hanging", r == 5 && us <= 25000 * 12 / 10 + 1000);
#else
  check("SCL held low: refused at once (4) instead of hanging", r == 4 && us < 1000);
#endif

  pinMode(WATCH_SCL, INPUT);
  delay(1);
  w.clearWireTimeoutFlag();
  w.beginTransmission(NOBODY);
  r = w.endTransmission();
  check("let go: an empty address is NAKed (2) again", r == 2);
  w.setWireTimeout(0);
}
#endif

#if defined(FRDM_MCXA156)
// The on-board P3T1755: 12 bits, left aligned, 1/16 degree
bool sensorOk(TwoWire &w) {
  if (w.requestFrom((uint8_t)0x48, (uint8_t)2, (uint32_t)0, (uint8_t)1, (uint8_t)true) != 2)
    return false;
  uint8_t hi = w.read();
  uint8_t lo = w.read();
  float t = (int16_t)((hi << 8) | lo) / 16 * 0.0625f;
  Serial.print("  ");
  Serial.print(t);
  Serial.println(" C");
  return t > 0 && t < 50;
}
#elif defined(FRDM_MCXN236) || defined(FRDM_MCXC444)
// The on-board FXLS8974CF's WHO_AM_I
bool sensorOk(TwoWire &w) {
  if (w.requestFrom((uint8_t)0x18, (uint8_t)1, (uint32_t)0x13, (uint8_t)1, (uint8_t)true) != 1)
    return false;
  return w.read() == 0x86;
}
#endif

#if defined(FRDM_MCXA156) || defined(FRDM_MCXN236)
// The target side, as in 15: a write's first byte sets the register
// pointer, the rest are stored from there; a read returns 4 registers
TwoWire *target;
uint8_t regs[16];
uint8_t regPtr = 0;
int rxCalls = 0;
int rxCount = -1;
uint8_t got[8];

void onRx(int n) {
  rxCalls++;
  rxCount = n;
  for (int i = 0; i < n && target->available(); i++) {
    uint8_t b = target->read();
    if (i < (int)sizeof(got))
      got[i] = b;
  }
  if (n >= 1) {
    regPtr = got[0] & 15;
    for (int i = 1; i < n && i < (int)sizeof(got); i++)
      regs[(regPtr + i - 1) & 15] = got[i];
  }
}

void onReq() {
  for (int i = 0; i < 4; i++)
    target->write(regs[(regPtr + i) & 15]);
}

// ctrl and targ are begun already
void crossSuite(TwoWire &ctrl, const char *ctrlName, TwoWire &targ, const char *targName, uint8_t addr) {
  Serial.print("--- ");
  Serial.print(ctrlName);
  Serial.print(" -> ");
  Serial.print(targName);
  Serial.println(" as a target ---");

  target = &targ;
  targ.onReceive(onRx);
  targ.onRequest(onReq);
  targ.begin(addr);
  for (int i = 0; i < 16; i++)
    regs[i] = 0xA0 + i;

  rxCalls = 0;
  rxCount = -1;
  memset(got, 0, sizeof(got));
  ctrl.beginTransmission(addr);
  ctrl.write(1);
  ctrl.write(0x5A);
  ctrl.write(0xC3);
  uint8_t r = ctrl.endTransmission();
  delay(1);
  check("write 3 bytes: ACKed, onReceive(3) gets them",
        r == 0 && rxCalls == 1 && rxCount == 3 && got[0] == 1 && got[1] == 0x5A && got[2] == 0xC3);

  uint8_t n = ctrl.requestFrom(addr, (uint8_t)4, (uint32_t)1, (uint8_t)1, (uint8_t)true);
  uint8_t b[4] = { 0 };
  for (int i = 0; i < 4 && ctrl.available(); i++)
    b[i] = ctrl.read();
  check("register read from 1 gets what was written, then regs 3-4",
        n == 4 && b[0] == 0x5A && b[1] == 0xC3 && b[2] == 0xA3 && b[3] == 0xA4);

  targ.begin();  // plain controller again
}
#endif

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("=== Wire.setPins() and A4/A5 ===");

  Serial.println("--- on every board ---");
#if defined(WIRE_HAS_END)
  check("WIRE_HAS_END is 1, as on AVR", WIRE_HAS_END == 1);
#else
  check("WIRE_HAS_END is defined", false);
#endif
  check("Wire.setPins(D2, D3): false, no I2C there", !Wire.setPins(D2, D3));
  check("Wire.setPins(D18, D19), its own: true", Wire.setPins(D18, D19));
  Wire.begin();
  check("after begin(): false", !Wire.setPins(D18, D19));
  Wire.end();
  check("after end(): true again", Wire.setPins(D18, D19));

#if !HAS_A4A5
  check("Wire.setPins(A4, A5): false, no I2C on A4/A5 here", !Wire.setPins(A4, A5));
  check("Wire1.setPins(A4, A5): false", !Wire1.setPins(A4, A5));
#if defined(FRDM_MCXN947)
  check("Wire2.setPins(A4, A5): false", !Wire2.setPins(A4, A5));
#endif
#else
  Serial.println("--- Wire on A4/A5 ---");
  Wire.begin();  // on D18/D19 first, so that they have something to give back
  check("D18/D19 are the I2C's while Wire runs there", !isGpio(D18) && !isGpio(D19));
  Wire.end();
  check("Wire.setPins(A4, A5): true", Wire.setPins(A4, A5));
  check("D18/D19 are GPIOs again", isGpio(D18) && isGpio(D19));
  Wire.begin();
  check("A4/A5 are the I2C's now", !isGpio(A4) && !isGpio(A5));
  int e = sclEdges(Wire);
  Serial.print("  ");
  Serial.print(e);
  Serial.println(" SCL edges");
  check("SCL comes out of A5 (9 or more falling edges)", e >= 9);
  Wire.beginTransmission(NOBODY);
  check("an empty address is NAKed (2)", Wire.endTransmission() == 2);
#if defined(FRDM_MCXA156)
  check("the P3T1755 reads from A4/A5", sensorOk(Wire));
#endif
#if defined(FRDM_MCXC444)
  check("Wire1.setPins(A4, A5): false (Wire only, on this board)", !Wire1.setPins(A4, A5));
  Wire1.begin();
  check("Wire1 (I2C0) reads its accelerometer meanwhile", sensorOk(Wire1));
  Wire1.end();
#endif
  heldLow(Wire);

#if defined(FRDM_MCXA156)
  check("Wire1.setPins(A4, A5) while Wire runs there: false", !Wire1.setPins(A4, A5));
#endif

  Serial.println("--- Wire back on D18/D19 ---");
  Wire.end();
  check("Wire.setPins(D18, D19): true", Wire.setPins(D18, D19));
  check("A4/A5 are GPIOs again", isGpio(A4) && isGpio(A5));
  Wire.begin();
  check("D18/D19 are the I2C's again", !isGpio(D18) && !isGpio(D19));
  Wire.beginTransmission(NOBODY);
  check("an empty address is NAKed (2)", Wire.endTransmission() == 2);
#if defined(FRDM_MCXA156)
  check("the P3T1755 reads from D18/D19", sensorOk(Wire));
#endif
  Wire.end();

#if defined(FRDM_MCXA156) || defined(FRDM_MCXN236)
  Serial.println("--- Wire1 on A4/A5 ---");
  Wire1.begin();  // on the MikroBus I2C first
  check("MB_SDA/MB_SCL are the I2C's while Wire1 runs there", !isGpio(MB_SDA) && !isGpio(MB_SCL));
#if defined(FRDM_MCXN236)
  check("Wire.setPins(A4, A5) while Wire1 runs LPI2C2: false", !Wire.setPins(A4, A5));
#endif
  Wire1.end();
  check("Wire1.setPins(A4, A5): true", Wire1.setPins(A4, A5));
  check("MB_SDA/MB_SCL are GPIOs again", isGpio(MB_SDA) && isGpio(MB_SCL));
  Wire1.begin();
  e = sclEdges(Wire1);
  Serial.print("  ");
  Serial.print(e);
  Serial.println(" SCL edges");
  check("SCL comes out of A5 (9 or more falling edges)", e >= 9);
#if defined(FRDM_MCXA156)
  check("the P3T1755 reads from Wire1 on A4/A5", sensorOk(Wire1));
#endif
  heldLow(Wire1);
  Wire.begin();  // D18/D19, joined to A4/A5 by the jumpers
  crossSuite(Wire1, "Wire1 on A4/A5", Wire, "Wire", 0x42);
  crossSuite(Wire, "Wire", Wire1, "Wire1 on A4/A5", 0x43);
  Wire.end();
  Wire1.end();
  check("Wire1.setPins(MB_SDA, MB_SCL), its own: true", Wire1.setPins(MB_SDA, MB_SCL));
  check("A4/A5 are GPIOs again", isGpio(A4) && isGpio(A5));
  Wire1.begin();
  Wire1.beginTransmission(NOBODY);
  check("Wire1 on MB_SDA/MB_SCL: an empty address is NAKed (2)", Wire1.endTransmission() == 2);
#if defined(FRDM_MCXN236)
  check("the accelerometer reads from MB_SDA/MB_SCL", sensorOk(Wire1));
#endif
  Wire1.end();
#endif

#if defined(FRDM_MCXN236)
  Serial.println("--- Wire on A4/A5, Wire1's LPI2C2 ---");
  check("Wire.setPins(A4, A5): true", Wire.setPins(A4, A5));
  Wire.begin();
  // Wire1's end() left MB_SDA/MB_SCL on LPI2C2: Wire's begin() on the same
  // LPI2C2 gives them back, or it would drive them too, and reach the
  // accelerometer, which these jumpers don't
  check("Wire1's MB_SDA/MB_SCL went back to GPIO", isGpio(MB_SDA) && isGpio(MB_SCL));
  check("so the accelerometer is out of Wire's reach", !sensorOk(Wire));
  check("Wire1.setPins(MB_SDA, MB_SCL) meanwhile: false", !Wire1.setPins(MB_SDA, MB_SCL));
  Wire.end();
  check("Wire.setPins(D18, D19): true", Wire.setPins(D18, D19));
#endif
#endif

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
