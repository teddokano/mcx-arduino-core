/** FRDM-MCXA156 only: Wire (D18/D19, LPI2C0) and Wire1 (MikroBus
 *  MB_SDA/MB_SCL, LPI2C3) joined into one bus with two jumpers, so that
 *
 *    - Wire1 reaches the on-board P3T1755 (0x48), which sits on D18/D19
 *    - each bus can talk to the other as a target, both ways round
 *    - Wire1 gives up instead of hanging when D18/D19 are held low as
 *      GPIOs, and works again once they are let go
 *
 *  Wiring: MB_SDA-D18 and MB_SCL-D19, nothing else on either bus. The
 *  pull-ups are the board's own: 2.2k on the sensor side and 4.7k on the
 *  MikroBus side, in parallel once jumpered.
 *
 *  Automatic: reads "ALL OK" or "N FAILED" at the end.
 */

#include <Arduino.h>
#include <Wire.h>

#if !defined(FRDM_MCXA156)
#error "This sketch is for FRDM-MCXA156 only"
#endif

const uint8_t SENSOR = 0x48;

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

// The P3T1755's temperature register: 12 bits, left aligned, 1/16 degree
bool readTemp(TwoWire &w, float &t, uint8_t &err) {
  err = w.requestFrom(SENSOR, (uint8_t)2, (uint32_t)0, (uint8_t)1, (uint8_t)true) == 2 ? 0 : 1;
  if (err)
    return false;
  uint8_t hi = w.read();
  uint8_t lo = w.read();
  t = (int16_t)((hi << 8) | lo) / 16 * 0.0625f;
  return true;
}

// The target side, as in test_Wire_target_self: a write's first byte sets
// the register pointer, the rest are stored from there; a read returns 4
// registers from the pointer
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

void resetLog() {
  rxCalls = 0;
  rxCount = -1;
  memset(got, 0, sizeof(got));
}

void crossSuite(TwoWire &ctrl, const char *ctrlName, TwoWire &targ, const char *targName, uint8_t addr) {
  Serial.print("--- ");
  Serial.print(ctrlName);
  Serial.print(" -> ");
  Serial.print(targName);
  Serial.print(" as a target at 0x");
  Serial.print(addr, HEX);
  Serial.println(" ---");

  target = &targ;
  targ.onReceive(onRx);
  targ.onRequest(onReq);
  targ.begin(addr);
  ctrl.begin();
  ctrl.setWireTimeout(25000);
  for (int i = 0; i < 16; i++)
    regs[i] = 0xA0 + i;

  resetLog();
  ctrl.beginTransmission(addr);
  ctrl.write(1);
  ctrl.write(0x5A);
  ctrl.write(0xC3);
  uint8_t r = ctrl.endTransmission();
  delay(1);  // the target runs onReceive() just after the STOP
  check("write 3 bytes: ACKed, onReceive(3) gets them",
        r == 0 && rxCalls == 1 && rxCount == 3 && got[0] == 1 && got[1] == 0x5A && got[2] == 0xC3);

  uint8_t n = ctrl.requestFrom(addr, (uint8_t)4, (uint32_t)1, (uint8_t)1, (uint8_t)true);
  uint8_t b[4] = { 0 };
  for (int i = 0; i < 4 && ctrl.available(); i++)
    b[i] = ctrl.read();
  check("register read from 1 gets what was written, then regs 3-4",
        n == 4 && b[0] == 0x5A && b[1] == 0xC3 && b[2] == 0xA3 && b[3] == 0xA4);

  float t;
  uint8_t err;
  check("the sensor still answers the controller while the target is armed",
        readTemp(ctrl, t, err) && t > 0 && t < 50);

  resetLog();
  ctrl.beginTransmission(addr + 1);
  ctrl.write(9);
  r = ctrl.endTransmission();
  delay(1);
  check("another address is NAKed, onReceive not called", r == 134 && rxCalls == 0);

  targ.end();
  targ.begin();  // plain controller again, for the next suite
}

// Hold one of D18/D19 low as a GPIO, with Wire out of the way, and try
// the sensor from Wire1
void heldLow(int pin, const char *name) {
  Serial.print("--- Wire1 with ");
  Serial.print(name);
  Serial.println(" held low ---");

  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  Wire1.setWireTimeout(25000, true);

  float t;
  uint8_t err;
  unsigned long t0 = micros();
  bool ok = readTemp(Wire1, t, err);
  unsigned long us = micros() - t0;
  Serial.print("  took ");
  Serial.print(us);
  Serial.println("us");
  check("fails instead of hanging", !ok && us < 40000);

  pinMode(pin, INPUT);
  delay(1);
  Wire1.clearWireTimeoutFlag();
  ok = readTemp(Wire1, t, err);
  check("reads the sensor again once let go", ok && t > 0 && t < 50);
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("=== Wire and Wire1 jumpered together (MB_SDA-D18, MB_SCL-D19), FRDM-MCXA156 ===");

  Wire.begin();
  Wire1.begin();
  Wire.setWireTimeout(25000);
  Wire1.setWireTimeout(25000);

  Serial.println("--- the on-board sensor from both buses ---");
  Wire1.beginTransmission(SENSOR);
  uint8_t r = Wire1.endTransmission();
  check("Wire1 finds the P3T1755 at 0x48", r == 0);

  float t0 = 0, t1 = 0;
  uint8_t e0, e1;
  bool ok0 = readTemp(Wire, t0, e0);
  bool ok1 = readTemp(Wire1, t1, e1);
  Serial.print("  Wire: ");
  Serial.print(t0);
  Serial.print("  Wire1: ");
  Serial.println(t1);
  check("both read a plausible temperature", ok0 && ok1 && t0 > 0 && t0 < 50 && t1 > 0 && t1 < 50);
  check("and the same one, within 0.5 degrees", fabsf(t0 - t1) <= 0.5f);

  Wire1.setClock(400000);
  ok1 = readTemp(Wire1, t1, e1);
  check("Wire1 reads it at 400kHz too", ok1 && t1 > 0 && t1 < 50);
  Wire1.setClock(100000);

  crossSuite(Wire1, "Wire1", Wire, "Wire", 0x42);
  crossSuite(Wire, "Wire", Wire1, "Wire1", 0x43);

  Wire.end();  // D18/D19 back to GPIO
  heldLow(D19, "SCL (D19)");
  heldLow(D18, "SDA (D18)");

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
