/** Wire.setWireTimeout() / getWireTimeoutFlag() / clearWireTimeoutFlag() test
 *
 *  Wiring (same on FRDM-MCXA153 and FRDM-MCXN947):
 *    D19 (Wire SCL) -- D8
 *    D18 (Wire SDA) -- D7
 *  Nothing else on Wire.
 *
 *  D8/D7 hold either bus line low on cue (open-drain output) -- standing in
 *  for a target that stretches the clock or holds SDA forever -- and let go
 *  of it again (input with pull-up). Those internal pull-ups are also the
 *  bus's only pull-ups: FRDM-MCXA153 has none on D18/D19, and a bus left
 *  floating reads low and hangs the very first transfer. Every transfer is addressed to
 *  0x50, where nothing answers, so a healthy bus comes back with a NAK in
 *  well under a millisecond.
 *
 *  If the sketch stops printing after a "running:" line, that transfer hung
 *  -- the very thing a timeout is supposed to prevent.
 */

#include <Wire.h>

const uint8_t TARGET = 0x50;
const int SCL_HOLD = D8;
const int SDA_HOLD = D7;

// endTransmission() returns this core's status code truncated to uint8_t
const uint8_t CODE_NAK = 134;      // kStatus_LPI2C_Nak (902)
const uint8_t CODE_TIMEOUT = 138;  // kStatus_LPI2C_PinLowTimeout (906)

int fails = 0;

struct Result {
  uint8_t code;
  uint32_t us;
};

void check(const char *label, bool ok) {
  Serial.print("  ");
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    fails++;
}

Result transfer(const char *label) {
  Serial.print("running: ");
  Serial.println(label);
  Serial.flush();

  uint32_t t0 = micros();
  Wire.beginTransmission(TARGET);
  Wire.write(0);
  uint8_t code = Wire.endTransmission();
  Result r = { code, micros() - t0 };

  Serial.print("  code=");
  Serial.print(r.code);
  Serial.print(" time=");
  Serial.print(r.us);
  Serial.print("us flag=");
  Serial.println(Wire.getWireTimeoutFlag() ? "set" : "clear");
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
    hold(SCL_HOLD);
  }
}

Result transfer_with_stuck_scl(const char *label) {
  attachInterrupt(SCL_HOLD, grab_scl, FALLING);
  pinMode(SCL_HOLD, INPUT_PULLUP);  // attachInterrupt() re-inits the pin; keep it pulling the bus up
  armed = true;
  Result r = transfer(label);
  armed = false;
  detachInterrupt(SCL_HOLD);
  release(SCL_HOLD);
  return r;
}

// With reset, the case AVR itself promises recovery for; the no-reset path
// is checked separately near the end.
void scl_timeout_case(const char *label, uint32_t timeout_us) {
  Wire.setWireTimeout(timeout_us, true);
  check("setWireTimeout() clears the flag", !Wire.getWireTimeoutFlag());

  Result r = transfer_with_stuck_scl(label);

  check("timed out close to the limit", timed_out_near(r, timeout_us));
  check("flag set", Wire.getWireTimeoutFlag());

  Wire.clearWireTimeoutFlag();
  check("clearWireTimeoutFlag() clears it", !Wire.getWireTimeoutFlag());
  r = transfer("  then, SCL released");
  check("bus usable again", healthy(r));
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Wire.setWireTimeout() test");

  release(SCL_HOLD);
  release(SDA_HOLD);

  Wire.begin();

  Result r = transfer("bus free, timeout off");
  check("NAK, fast", healthy(r));

  Wire.setWireTimeout();
  r = transfer("bus free, timeout 25ms");
  check("NAK, fast (no false trip on an idle bus)", healthy(r));
  check("flag clear", !Wire.getWireTimeoutFlag());

  scl_timeout_case("SCL stuck mid-transfer, timeout 25ms", 25000);
  scl_timeout_case("SCL stuck mid-transfer, timeout 5ms", 5000);
  scl_timeout_case("SCL stuck mid-transfer, timeout 60ms", 60000);

  Wire.setWireTimeout(1000000, true);
  r = transfer_with_stuck_scl("SCL stuck mid-transfer, timeout 1s (beyond the hardware counter)");
  check("timed out, clamped well short of 1s", r.code == CODE_TIMEOUT && r.us < 500000);
  r = transfer("  then, SCL released");
  check("bus usable again", healthy(r));

  // The counter's unit depends on the prescaler the baud rate picked. 10ms,
  // because the counter's cap shrinks with speed: on FRDM-MCXA153 it is
  // ~21.8ms at 400kHz, below even the 25ms default.
  Wire.setClock(400000);
  scl_timeout_case("SCL stuck mid-transfer at 400kHz, timeout 10ms", 10000);
  Wire.setClock(100000);

  // A line already low when the transfer starts: the LPI2C sees a busy bus
  // and refuses to start (kStatus_LPI2C_Busy) rather than timing out. Once
  // released, BUSIDLE lets it see the bus idle again without needing a STOP.
  Wire.setWireTimeout(25000, true);
  hold(SCL_HOLD);
  r = transfer("SCL held before the transfer starts");
  release(SCL_HOLD);
  check("refused at once instead of hanging", r.code != 0 && r.us < 1000);
  r = transfer("  then, SCL released");
  check("bus usable again", healthy(r));

  hold(SDA_HOLD);
  r = transfer("SDA held before the transfer starts");
  release(SDA_HOLD);
  check("refused at once instead of hanging", r.code != 0 && r.us < 1000);
  r = transfer("  then, SDA released");
  check("bus usable again", healthy(r));

  // No reset. AVR promises nothing here, but this core queues a STOP on a
  // timeout, so the transfer ends by itself once the line is let go --
  // without it, the LPI2C went on holding the bus itself.
  Wire.setWireTimeout(5000, false);
  r = transfer_with_stuck_scl("SCL stuck mid-transfer, timeout 5ms, no reset");
  check("timed out close to the limit", timed_out_near(r, 5000));
  r = transfer("  then, SCL released");
  check("bus usable again", healthy(r));

  Wire.setWireTimeout(0);
  r = transfer("timeout off again");
  check("NAK, fast", healthy(r));
  check("setWireTimeout(0) clears the flag", !Wire.getWireTimeoutFlag());

  // Wire1 runs on I3C, not LPI2C: setWireTimeout() must leave it working.
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
