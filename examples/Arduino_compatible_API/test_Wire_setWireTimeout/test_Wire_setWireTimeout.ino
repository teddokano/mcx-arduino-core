/** Wire.setWireTimeout() / getWireTimeoutFlag() / clearWireTimeoutFlag() test
 *
 *  Wiring, both boards:
 *    D19 (Wire SCL) -- D8
 *    D18 (Wire SDA) -- D7
 *  and on FRDM-MCXN947 also, for Wire2:
 *    MB_SCL -- MB_PWM
 *    MB_SDA -- MB_INT
 *  Nothing else on those buses.
 *
 *  D8/D7 (MB_PWM/MB_INT) hold either bus line low on cue (open-drain
 *  output) -- standing in for a target that stretches the clock or holds
 *  SDA forever -- and let go of it again (input with pull-up). Every
 *  transfer is addressed to 0x50, where nothing answers, so a healthy bus
 *  comes back with a NAK in well under a millisecond.
 *
 *  If the sketch stops printing after a "running:" line, that transfer hung
 *  -- the very thing a timeout is supposed to prevent.
 *
 *  Mirrored as examples/release_check/14_wire_timeout -- copy any change
 *  there too.
 */

#include <Wire.h>

struct Result {
  uint8_t code;
  uint32_t us;
};

const uint8_t TARGET = 0x50;
const uint8_t SELF = 0x42;  // Wire's own target address, for the target-mode checks

// endTransmission() returns this core's status code truncated to uint8_t
const uint8_t CODE_NAK = 134;      // kStatus_LPI2C_Nak (902)
const uint8_t CODE_TIMEOUT = 138;  // kStatus_LPI2C_PinLowTimeout (906)

// The bus under test and its two hold pins
TwoWire *bus;
int sclHold;
int sdaHold;

int fails = 0;

void check(const char *label, bool ok) {
  Serial.print("  ");
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    fails++;
}

uint8_t send(uint8_t addr) {
  bus->beginTransmission(addr);
  bus->write(0);
  return bus->endTransmission();
}

Result transfer(const char *label) {
  Serial.print("running: ");
  Serial.println(label);
  Serial.flush();

  uint32_t t0 = micros();
  uint8_t code = send(TARGET);
  Result r = { code, micros() - t0 };
  Serial.print("  code=");
  Serial.print(r.code);
  Serial.print(" time=");
  Serial.print(r.us);
  Serial.print("us flag=");
  Serial.println(bus->getWireTimeoutFlag() ? "set" : "clear");
  return r;
}

bool healthy(Result r) {
  return r.code == CODE_NAK && r.us < 2000;
}

bool timed_out_near(Result r, uint32_t timeout_us) {
  return r.code == CODE_TIMEOUT && r.us >= timeout_us * 9 / 10 && r.us <= timeout_us * 12 / 10 + 1000;
}

// Open-drain, so it never drives the bus high against the LPI2C.
void hold(int pin) {
  pinMode(pin, OUTPUT_OPENDRAIN);
  digitalWrite(pin, LOW);
}

void release(int pin) {
  pinMode(pin, INPUT_PULLUP);
  delay(1);
}

// A target that clock-stretches forever: grab SCL on its first falling edge
// once the transfer is under way, so the LPI2C is mid-byte when it stalls.
// (Holding it before the transfer starts is a different case -- the LPI2C
// then sees a busy bus and refuses to start at all.)
volatile bool armed = false;

void grab_scl() {
  if (armed) {
    armed = false;
    hold(sclHold);
  }
}

Result transfer_with_stuck_scl(const char *label) {
  attachInterrupt(sclHold, grab_scl, FALLING);
  pinMode(sclHold, INPUT_PULLUP);  // attachInterrupt() re-inits the pin; keep it pulling the bus up
  armed = true;
  Result r = transfer(label);
  armed = false;
  detachInterrupt(sclHold);
  release(sclHold);
  return r;
}

// With reset, the case AVR itself promises recovery for; the no-reset path
// is checked separately near the end.
void scl_timeout_case(const char *label, uint32_t timeout_us) {
  bus->setWireTimeout(timeout_us, true);
  check("setWireTimeout() clears the flag", !bus->getWireTimeoutFlag());

  Result r = transfer_with_stuck_scl(label);

  check("timed out close to the limit", timed_out_near(r, timeout_us));
  check("flag set", bus->getWireTimeoutFlag());

  bus->clearWireTimeoutFlag();
  check("clearWireTimeoutFlag() clears it", !bus->getWireTimeoutFlag());
  r = transfer("  then, SCL released");
  check("bus usable again", healthy(r));
}

// ---- Wire as a target (Wire only: Wire2 can't be one) ----
volatile int rxCalls = 0;
volatile int rxFirst = -1;

void onRx(int n) {
  rxCalls = rxCalls + 1;
  rxFirst = n > 0 ? bus->read() : -1;
  while (bus->available())
    bus->read();
}

void onReq() {
  bus->write(0x5A);
  bus->write(0xA5);
}

bool target_answers(uint8_t v) {
  rxCalls = 0;
  bus->beginTransmission(SELF);
  bus->write(v);
  bool wrote = bus->endTransmission() == 0;
  delay(1);  // the target sees the STOP a moment after endTransmission() returns
  bool got = rxCalls == 1 && rxFirst == v;
  bool read = bus->requestFrom(SELF, (size_t)2) == 2 && bus->read() == 0x5A && bus->read() == 0xA5;
  return wrote && got && read;
}

void target_cases() {
  Serial.println("--- Wire as its own target at 0x42, through timeouts ---");
  bus->onReceive(onRx);
  bus->onRequest(onReq);
  bus->setWireTimeout(25000, true);
  bus->begin(SELF);
  check("target answers", target_answers(0x11));

  Result r = transfer_with_stuck_scl("SCL stuck mid-transfer, 25ms, with reset");
  check("timed out", timed_out_near(r, 25000));
  check("still a target after the reset", target_answers(0x22));

  bus->setWireTimeout(5000, false);
  r = transfer_with_stuck_scl("SCL stuck mid-transfer, 5ms, no reset");
  check("timed out", timed_out_near(r, 5000));
  check("still a target", target_answers(0x33));

  bus->setWireTimeout(0);
  bus->begin();
}

void suite(TwoWire &w, const char *name, int scl, int sda, uint32_t cap400k_us) {
  Serial.print("=== ");
  Serial.print(name);
  Serial.println(" ===");

  bus = &w;
  sclHold = scl;
  sdaHold = sda;
  release(sclHold);
  release(sdaHold);

  bus->begin();

  Result r = transfer("bus free, timeout off");
  check("NAK, fast", healthy(r));

  bus->setWireTimeout();
  r = transfer("bus free, timeout 25ms");
  check("NAK, fast (no false trip on an idle bus)", healthy(r));
  check("flag clear", !bus->getWireTimeoutFlag());

  // Straight after begin() or setWireTimeout(), with nothing in between: the
  // timeout also turns on the LPI2C's bus-idle detection, and a freshly
  // enabled controller counts the bus as busy until that has seen it idle.
  // The transfers above had a Serial.flush() in between, long enough to
  // hide that.
  bus->setWireTimeout(25000, true);
  uint8_t c1 = send(TARGET);
  bus->begin();
  uint8_t c2 = send(TARGET);
  Serial.print("running: at once after setWireTimeout() / begin(): codes ");
  Serial.print(c1);
  Serial.print(" / ");
  Serial.println(c2);
  check("not refused as busy", c1 == CODE_NAK && c2 == CODE_NAK);

  scl_timeout_case("SCL stuck mid-transfer, timeout 25ms", 25000);
  scl_timeout_case("SCL stuck mid-transfer, timeout 5ms", 5000);
  scl_timeout_case("SCL stuck mid-transfer, timeout 60ms", 60000);

  bus->setWireTimeout(1000000, true);
  r = transfer_with_stuck_scl("SCL stuck mid-transfer, timeout 1s (beyond the hardware counter)");
  check("timed out, clamped well short of 1s", r.code == CODE_TIMEOUT && r.us < 500000);
  r = transfer("  then, SCL released");
  check("bus usable again", healthy(r));

  // The counter's unit depends on the prescaler the baud rate picked. 10ms,
  // because the counter's cap shrinks with speed: on FRDM-MCXA153 it is
  // ~21.8ms at 400kHz, below even the 25ms default.
  bus->setClock(400000);
  scl_timeout_case("SCL stuck mid-transfer at 400kHz, timeout 10ms", 10000);
  bus->setWireTimeout(1000000, true);
  r = transfer_with_stuck_scl("SCL stuck mid-transfer at 400kHz, timeout 1s");
  check("clamped to the hardware cap at this speed", r.code == CODE_TIMEOUT && r.us >= cap400k_us * 9 / 10 && r.us <= cap400k_us * 11 / 10 + 1000);
  bus->setClock(100000);

  // A line already low when the transfer starts: the LPI2C sees a busy bus
  // and refuses to start (kStatus_LPI2C_Busy) rather than timing out. Once
  // released, BUSIDLE lets it see the bus idle again without needing a STOP.
  bus->setWireTimeout(25000, true);
  hold(sclHold);
  r = transfer("SCL held before the transfer starts");
  release(sclHold);
  check("refused at once instead of hanging", r.code != 0 && r.us < 1000);
  r = transfer("  then, SCL released");
  check("bus usable again", healthy(r));

  hold(sdaHold);
  r = transfer("SDA held before the transfer starts");
  release(sdaHold);
  check("refused at once instead of hanging", r.code != 0 && r.us < 1000);
  r = transfer("  then, SDA released");
  check("bus usable again", healthy(r));

  // No reset. AVR promises nothing here, but this core queues a STOP on a
  // timeout, so the transfer ends by itself once the line is let go --
  // without it, the LPI2C went on holding the bus itself.
  bus->setWireTimeout(5000, false);
  r = transfer_with_stuck_scl("SCL stuck mid-transfer, timeout 5ms, no reset");
  check("timed out close to the limit", timed_out_near(r, 5000));
  r = transfer("  then, SCL released");
  check("bus usable again", healthy(r));

  bus->setWireTimeout(0);
  r = transfer("timeout off again");
  check("NAK, fast", healthy(r));
  check("setWireTimeout(0) clears the flag", !bus->getWireTimeoutFlag());
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Wire.setWireTimeout() test");

  // The counter's cap at 400kHz: 4095 x 256 x prescaler / functional clock,
  // with the prescaler the baud rate picked -- 2 on FRDM-MCXA153 (96MHz),
  // 1 on FRDM-MCXN947 (12MHz)
#if defined(FRDM_MCXN947)
  const uint32_t cap400k = 87360;
#else
  const uint32_t cap400k = 21840;
#endif

  suite(Wire, "Wire (D19-D8, D18-D7)", D8, D7, cap400k);
  target_cases();

#if defined(FRDM_MCXN947)
  suite(Wire2, "Wire2 (MB_SCL-MB_PWM, MB_SDA-MB_INT)", MB_PWM, MB_INT, cap400k);
#endif

  // Wire1 runs on I3C, not LPI2C: setWireTimeout() must leave it working.
  Serial.println("=== Wire1 ===");
  Wire1.begin();
  Wire1.setWireTimeout();
  Wire1.beginTransmission(0x48);  // on-board P3T1755, temperature register
  Wire1.write(0);
  bool w1 = Wire1.endTransmission(false) == 0 && Wire1.requestFrom(0x48, 2) == 2;
  check("Wire1 still reads the on-board sensor", w1);
  check("Wire1 flag clear", !Wire1.getWireTimeoutFlag());

  Serial.println();
  if (fails) {
    Serial.print(fails);
    Serial.println(" FAIL");
  } else {
    Serial.println("ALL PASS");
  }
}

void loop() {
}
