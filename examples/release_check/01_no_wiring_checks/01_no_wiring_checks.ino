/** Release check 01: automatic OK/FAIL checks, no physical wiring needed.
 *
 *  Consolidates (from examples/Arduino_compatible_API/):
 *  test_millis_micros, test_delayMicroseconds,
 *  test_analog_resolution_and_misc, test_Analog_read_write,
 *  test_Wire1_onboard_sensor_raw, test_Wire_Stream_requestFrom5, the core
 *  of test_Wire_target_self, and (FRDM-MCXN947 only)
 *  test_Wire2_MikroBus_N947.
 *
 *  The checks that need no peripheral (math constants, compat macros, the
 *  AVR-era helpers, Print, String) moved to release_check/09 in 0.9.0,
 *  when this sketch had grown to fill FRDM-MCXA153's flash.
 *
 *  On FRDM-MCXN947, take release_check/11's Serial1 loopback jumper
 *  (MB_TX-MB_RX) off before running this: those are Wire1's SDA/SCL pins
 *  there, and the jumper shorts them together.
 *
 *  On FRDM-MCXA156 the on-board sensor is on Wire (D18/D19), and the
 *  sensor checks use that; their messages still say Wire1. Its Wire1, the
 *  MikroBus I2C, gets the bus scan FRDM-MCXN947's Wire2 does.
 *
 *  FRDM-MCXN236 has no temperature sensor: its on-board sensor is an
 *  FXLS8974CF accelerometer, on Wire1 (the MikroBus I2C). The sensor
 *  section reads its WHO_AM_I and checks that the board, lying still,
 *  measures about 1g. The Wire-as-a-Stream checks write its OFF_X/OFF_Y
 *  registers where the other boards use the temperature sensor's T_LOW,
 *  and their messages still say T_LOW. Keep the board still while it runs.
 *
 *  Every check here is fully automatic -- read the final "ALL OK"/
 *  "N FAILED" line, no jumpers, no scope, no button presses. Sketches
 *  that need a human to watch a scope/LED/piezo, press a button, or
 *  install a jumper live in the other release_check/ sketches instead. Sketches needing an external library (P3T1755.h) or
 *  external hardware (a real LM75-family sensor, an external voltage
 *  source) aren't part of this consolidated set at all -- they stay as
 *  individual examples under Arduino_compatible_API/, run only when
 *  that hardware/library happens to be available.
 */

#include <Arduino.h>
#include <cstring>
#include "fsl_clock.h"

// The on-board P3T1755's bus: Wire1 on FRDM-MCXA153 and FRDM-MCXN947,
// Wire (D18/D19) on FRDM-MCXA156, whose sensor is on the Arduino I2C pins.
// On FRDM-MCXN236 the on-board sensor is the accelerometer, on Wire1
#if defined(FRDM_MCXA153) || defined(FRDM_MCXN947) || defined(FRDM_MCXN236)
#define SENSOR_WIRE Wire1
#elif defined(FRDM_MCXA156)
#define SENSOR_WIRE Wire
#else
#error "This sketch has no settings for this board yet"
#endif

// The on-board sensor's address, and a register pair the Wire-as-a-Stream
// checks may write and then put back: the P3T1755's T_LOW, or on
// FRDM-MCXN236 the accelerometer's OFF_X/OFF_Y (output offsets, written
// while it is in standby)
#if defined(FRDM_MCXN236)
#define SENSOR_I2C_ADDR 0x18
#define SCRATCH_REG 0x22
#elif defined(FRDM_MCXA153) || defined(FRDM_MCXA156) || defined(FRDM_MCXN947)
#define SENSOR_I2C_ADDR 0x48
#define SCRATCH_REG 0x02
#else
#error "This sketch has no settings for this board yet"
#endif

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

// Source-clock check. Compared exactly rather than with a tolerance: these
// are integer dividers off a fixed oscillator or a PLL, so any difference is
// a real configuration difference and never measurement noise.
void checkClock(const char *label, uint32_t actual, uint32_t expect) {
  Serial.print(label);
  Serial.print(": ");
  Serial.print(actual);
  Serial.print(" Hz (expect ");
  Serial.print(expect);
  Serial.print(") -> ");
  Serial.println(actual == expect ? "OK" : "FAIL");
  if (actual != expect)
    failCount++;
}

// For the Wire-as-a-Stream section: the classic three-step register read
// on Wire1's on-board sensor, to compare the new forms against.
// 0xFFFFFFFF if the read failed, which no 16-bit register value can be.
uint32_t readSensorReg16(uint8_t reg) {
  SENSOR_WIRE.beginTransmission(SENSOR_I2C_ADDR);
  SENSOR_WIRE.write(reg);
  if (SENSOR_WIRE.endTransmission(false) != 0 || SENSOR_WIRE.requestFrom((uint8_t)SENSOR_I2C_ADDR, (size_t)2) != 2)
    return 0xFFFFFFFF;
  uint16_t v = SENSOR_WIRE.read() << 8;
  return v | SENSOR_WIRE.read();
}

size_t printThroughPrint(Print &p, const char *s) {
  return p.print(s);
}

int readThroughStream(Stream &s) {
  s.flush();
  return s.read();
}

// For the Wire-target section: Wire as a target of its own controller.
// A register file, as target sketches usually are: a write's first byte
// sets the pointer, a read returns 4 registers from it.
uint8_t tgtRegs[16];
uint8_t tgtPtr = 0;
int tgtRxCount = -1;
char tgtOrder[4];
int tgtNOrder = 0;

void tgtOnReceive(int n) {
  tgtRxCount = n;
  if (tgtNOrder < 3)
    tgtOrder[tgtNOrder++] = 'R';
  for (int i = 0; i < n && Wire.available(); i++) {
    uint8_t v = Wire.read();
    if (i == 0)
      tgtPtr = v & 15;
    else
      tgtRegs[(tgtPtr + i - 1) & 15] = v;
  }
}

void tgtOnRequest() {
  if (tgtNOrder < 3)
    tgtOrder[tgtNOrder++] = 'Q';
  for (int i = 0; i < 4; i++)
    Wire.write(tgtRegs[(tgtPtr + i) & 15]);
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("=== Release check 01: no-wiring automatic checks ===");

  // ---- peripheral source clocks ----
  // Placed first because everything after it sits downstream: the
  // delayMicroseconds() accuracy check here, and on real hardware every bit
  // rate the other release_check sketches produce. A wrong source clock is
  // how N947's default SPI came to ask for 24MHz and put out 31kHz, and how
  // SPI1 ran at half rate -- both were peripherals that mcu.cpp attached and
  // clock_config.c never re-attached, and both were found only once someone
  // put a logic analyser on the pin. Reading the numbers back catches that
  // class of fault without any instrument.
  //
  // The expected values are the ones the 0.6 clock audit derived by reading
  // mcu.cpp and clock_config.c; this is the first time they are checked
  // against a running board. So a FAIL here may well mean the audit was
  // wrong rather than the hardware -- read the printed value before
  // calling it a regression.
  Serial.println("--- peripheral source clocks ---");
  {
#if defined(FRDM_MCXA153)
    // clock_config.c re-attaches every peripheral here, so mcu.cpp's own
    // attach calls are overwritten and all of these land on FRO_HF_DIV --
    // except Serial1 (LPUART2), whose clock is attached lazily by
    // Serial::_setup_clock() (Serial.cpp's s_pinMap[], kFRO12M_to_LPUART2)
    // at the constructor's static-init time, not by mcu.cpp or
    // clock_config.c. clock_config.c never touches LPUART2, so that 12MHz
    // attach is the one that survives.
    checkClock("core", CLOCK_GetCoreSysClkFreq(), 96000000u);
    checkClock("Wire    (LPI2C0) ", CLOCK_GetLpi2cClkFreq(), 96000000u);
    checkClock("Wire1   (I3C0)   ", CLOCK_GetI3CFClkFreq(), 48000000u);
    checkClock("SPI     (LPSPI1) ", CLOCK_GetLpspiClkFreq(1u), 96000000u);
    checkClock("SPI1    (LPSPI0) ", CLOCK_GetLpspiClkFreq(0u), 96000000u);
    checkClock("Serial1 (LPUART2)", CLOCK_GetLpuartClkFreq(2u), 12000000u);
#elif defined(FRDM_MCXN947)
    // The mirror image: clock_config.c touches no peripheral clock at all,
    // so mcu.cpp's attach calls are the only thing that decides these.
    checkClock("core", CLOCK_GetCoreSysClkFreq(), 150000000u);
    checkClock("Wire  (FlexComm2)", CLOCK_GetLPFlexCommClkFreq(2u), 12000000u);
    checkClock("Wire1 (I3C1)     ", CLOCK_GetI3cClkFreq(1u), 25000000u);
    checkClock("Wire2 (FlexComm3)", CLOCK_GetLPFlexCommClkFreq(3u), 12000000u);
    checkClock("SPI   (FlexComm1)", CLOCK_GetLPFlexCommClkFreq(1u), 48000000u);
    checkClock("SPI1  (FlexComm6)", CLOCK_GetLPFlexCommClkFreq(6u), 48000000u);
    // FlexComm5 is attached, just not from mcu.cpp. Serial.cpp's N947 pin map
    // carries kFRO12M_to_FLEXCOMM5 for the MB_TX/MB_RX entry, and the Serial
    // constructor applies it through _setup_clock() during static init -- so
    // it is already in effect by the time this line runs. mcu.cpp sets only
    // the divider, and says so in a comment right there. The 0.6 audit read
    // mcu.cpp alone, concluded the attach was missing, and filed it as a bug
    // to fix after measuring; the measurement is what showed there was
    // nothing to fix. Asserted like the rest now that the value is known and
    // traceable to a line of code rather than to a reset default.
    checkClock("Serial1 (FlexComm5)", CLOCK_GetLPFlexCommClkFreq(5u), 12000000u);
#elif defined(FRDM_MCXA156)
    // clock_config.c is the SDK's template and attaches nothing; mcu.cpp
    // puts the LPI2Cs and LPSPIs on FRO_HF_DIV, to match FRDM-MCXA153. The
    // two LPUARTs attach FRO12M themselves, as Serial1 does on FRDM-MCXA153.
    checkClock("core", CLOCK_GetCoreSysClkFreq(), 96000000u);
    checkClock("Wire    (LPI2C0) ", CLOCK_GetLpi2cClkFreq(0u), 96000000u);
    checkClock("Wire1   (LPI2C3) ", CLOCK_GetLpi2cClkFreq(3u), 96000000u);
    checkClock("SPI     (LPSPI1) ", CLOCK_GetLpspiClkFreq(1u), 96000000u);
    checkClock("SPI1    (LPSPI0) ", CLOCK_GetLpspiClkFreq(0u), 96000000u);
    checkClock("Serial1 (LPUART2)", CLOCK_GetLpuartClkFreq(2u), 12000000u);
    checkClock("Serial2 (LPUART1)", CLOCK_GetLpuartClkFreq(1u), 12000000u);
#elif defined(FRDM_MCXN236)
    // As on FRDM-MCXN947, clock_config.c attaches nothing and mcu.cpp
    // decides these. Wire1 and Serial1 share FlexComm2.
    checkClock("core", CLOCK_GetCoreSysClkFreq(), 150000000u);
    checkClock("Wire          (FlexComm5)", CLOCK_GetLPFlexCommClkFreq(5u), 12000000u);
    checkClock("Wire1/Serial1 (FlexComm2)", CLOCK_GetLPFlexCommClkFreq(2u), 12000000u);
    checkClock("SPI           (FlexComm3)", CLOCK_GetLPFlexCommClkFreq(3u), 48000000u);
#else
#error "This sketch has no settings for this board yet"
#endif
  }

  // ---- millis()/micros() actually advance (was test_millis_micros) ----
  Serial.println("--- millis/micros ---");
  {
    unsigned long m0 = millis();
    unsigned long u0 = micros();
    delay(200);
    unsigned long m1 = millis();
    unsigned long u1 = micros();
    unsigned long dm = m1 - m0;
    unsigned long du = u1 - u0;
    Serial.print("millis delta = "); Serial.print(dm);
    Serial.print("  micros delta = "); Serial.println(du);
    check("millis() advances ~200ms", dm >= 190 && dm <= 260);
    check("micros() advances ~200000us", du >= 190000 && du <= 260000);
  }

  // ---- delayMicroseconds() accuracy (was test_delayMicroseconds) ----
  Serial.println("--- delayMicroseconds ---");
  for (unsigned long target = 10; target <= 10000; target *= 10) {
    unsigned long t0 = micros();
    delayMicroseconds(target);
    unsigned long measured = micros() - t0;

    Serial.print("requested="); Serial.print(target);
    Serial.print(" measured="); Serial.print(measured);
    Serial.println(" us");

    char label[48];
    snprintf(label, sizeof(label), "delayMicroseconds(%lu) accurate", target);
    // Generous but bounded tolerance (20% + 30us slack): loose enough to
    // absorb call overhead at small targets, tight enough at the large
    // end to catch a real regression (the pre-DWT-fix overshoot was
    // ~26-28%, well outside this).
    check(label, measured >= target && measured <= target + target / 5 + 30);
  }

  // ---- analogRead/analogWrite basic sanity (was test_Analog_read_write) ----
  Serial.println("--- analogRead / analogWrite ---");
  {
    int value = analogRead(A2);
    Serial.print("A2 = "); Serial.println(value);
    check("analogRead(A2) in range", value >= 0 && value <= 1023);
    analogWrite(PWM0, value >> 2);
    check("analogWrite(PWM0, ...) (reached here without crashing)", true);
  }

  // ---- analogWrite on a pin FlexPWM cannot reach ----
  // Neither board routes FlexPWM to any of D0-D13 (N947: none at all;
  // A153: only D3/D7, on channels PWM5/PWM4 already own), so a sketch
  // written for a classic Arduino -- analogWrite(9, 128) -- can never get
  // real PWM here. It used to reach PwmOut's constructor and panic(),
  // killing the sketch with an SOS blink. It now falls back to
  // digitalWrite the way AVR's core does for a pin with no timer.
  // Reaching the checks below at all is itself the test that it no longer
  // panics; the pin is read back to confirm it really was driven.
  Serial.println("--- analogWrite fallback on a non-PWM pin ---");
  {
    const int PIN = D2;   // plain GPIO on both boards, nothing wired to it

    analogWrite(PIN, 0);
    check("analogWrite(D2, 0) drives LOW", digitalRead(PIN) == LOW);

    analogWrite(PIN, 255);
    check("analogWrite(D2, 255) drives HIGH", digitalRead(PIN) == HIGH);

    analogWrite(PIN, 200);            // above the 8-bit midpoint
    check("analogWrite(D2, 200) drives HIGH", digitalRead(PIN) == HIGH);

    analogWrite(PIN, 50);             // below it
    check("analogWrite(D2, 50) drives LOW", digitalRead(PIN) == LOW);

    // The midpoint has to follow analogWriteResolution(), not a hardcoded
    // 128: at 12-bit, 2000 is below half of 4095 and must read LOW.
    analogWriteResolution(12);
    analogWrite(PIN, 2000);
    check("12-bit: analogWrite(D2, 2000) drives LOW", digitalRead(PIN) == LOW);
    analogWrite(PIN, 3000);
    check("12-bit: analogWrite(D2, 3000) drives HIGH", digitalRead(PIN) == HIGH);
    analogWriteResolution(8);         // restore the default for later checks

    // analogWriteFrequency() deliberately still panics on such a pin --
    // it has no fallback (a GPIO has no period) and no ported sketch calls
    // it by accident -- so there is nothing to check for here.
  }

#if defined(FRDM_MCXN236)
  // ---- Wire1 on-board accelerometer (FXLS8974CF), raw registers ----
  Serial.println("--- Wire1 on-board accelerometer (raw registers) ---");
  {
    const uint8_t WHO_AM_I = 0x13;
    const uint8_t SENS_CONFIG1 = 0x15;  // bit 0 ACTIVE; FSR (bits 2:1) 0 is +/-2g
    const uint8_t OUT_X_LSB = 0x04;     // X, Y, Z, each LSB then MSB, 12-bit two's complement

    SENSOR_WIRE.begin();
    SENSOR_WIRE.beginTransmission(SENSOR_I2C_ADDR);
    SENSOR_WIRE.write(WHO_AM_I);
    uint8_t err = SENSOR_WIRE.endTransmission(false);
    check("Wire1 endTransmission(false)", err == 0);

    uint8_t n = SENSOR_WIRE.requestFrom((uint8_t)SENSOR_I2C_ADDR, (size_t)1);
    check("Wire1 requestFrom() got 1 byte", n == 1);
    int id = SENSOR_WIRE.read();
    Serial.print("WHO_AM_I = 0x");
    Serial.println(id, HEX);
    check("WHO_AM_I is FXLS8974CF's (0x86)", id == 0x86);

    // Measure at +/-2g, 0.98mg per count, then back to standby
    SENSOR_WIRE.beginTransmission(SENSOR_I2C_ADDR);
    SENSOR_WIRE.write(SENS_CONFIG1);
    SENSOR_WIRE.write(0x01);
    err = SENSOR_WIRE.endTransmission();
    delay(50);  // several output periods at the rate it resets to

    SENSOR_WIRE.beginTransmission(SENSOR_I2C_ADDR);
    SENSOR_WIRE.write(OUT_X_LSB);
    SENSOR_WIRE.endTransmission(false);
    n = SENSOR_WIRE.requestFrom((uint8_t)SENSOR_I2C_ADDR, (size_t)6);
    float mg[3] = { 0.0f, 0.0f, 0.0f };
    for (int i = 0; i < 3 && n == 6; i++) {
      uint8_t lsb = SENSOR_WIRE.read();
      uint8_t msb = SENSOR_WIRE.read();
      int16_t raw = (int16_t)((msb << 12) | (lsb << 4)) >> 4;
      mg[i] = raw * 0.98f;
    }

    SENSOR_WIRE.beginTransmission(SENSOR_I2C_ADDR);
    SENSOR_WIRE.write(SENS_CONFIG1);
    SENSOR_WIRE.write(0x00);
    SENSOR_WIRE.endTransmission();

    float g = sqrtf(mg[0] * mg[0] + mg[1] * mg[1] + mg[2] * mg[2]) / 1000.0f;
    Serial.print("x, y, z = ");
    Serial.print(mg[0], 0);
    Serial.print(", ");
    Serial.print(mg[1], 0);
    Serial.print(", ");
    Serial.print(mg[2], 0);
    Serial.print(" mg, |a| = ");
    Serial.print(g, 3);
    Serial.println(" g");
    check("activated, and the 6 output bytes read", err == 0 && n == 6);
    check("lying still, it measures about 1g", g > 0.8f && g < 1.2f);
  }
#elif defined(FRDM_MCXA153) || defined(FRDM_MCXA156) || defined(FRDM_MCXN947)
  // ---- Wire1 on-board I3C-in-I2C-mode sensor, raw registers
  //      (was test_Wire1_onboard_sensor_raw) ----
  Serial.println("--- Wire1 on-board temperature sensor (raw registers) ---");
  {
    const uint8_t SENSOR_ADDR = 0x48;
    const uint8_t TEMP_REG = 0x00;

    SENSOR_WIRE.begin();
    SENSOR_WIRE.beginTransmission(SENSOR_ADDR);
    SENSOR_WIRE.write(TEMP_REG);
    uint8_t err = SENSOR_WIRE.endTransmission(false);
    check("Wire1 endTransmission(false)", err == 0);

    uint8_t n = SENSOR_WIRE.requestFrom(SENSOR_ADDR, (size_t)2);
    check("Wire1 requestFrom() got 2 bytes", n == 2);

    if (n == 2) {
      uint8_t msb = SENSOR_WIRE.read();
      uint8_t lsb = SENSOR_WIRE.read();
      int16_t raw = (int16_t)((msb << 8) | lsb);
      raw >>= 5;
      float celsius = raw * 0.125f;
      Serial.print("temp = "); Serial.print(celsius, 3); Serial.println(" degC");
      check("on-board sensor reads a sane temperature", celsius > -20.0f && celsius < 60.0f);
    }
  }
#else
#error "This sketch has no settings for this board yet"
#endif

  // ---- Wire as a Stream, five-argument requestFrom(), buffer limits
  //      (was test_Wire_Stream_requestFrom5). Writes the sensor's T_LOW
  //      register, which nothing else depends on, and puts it back ----
  Serial.println("--- Wire as a Stream / five-argument requestFrom() ---");
  {
    const uint8_t SENSOR = SENSOR_I2C_ADDR;
    const uint8_t T_LOW = SCRATCH_REG;

    uint32_t saved = readSensorReg16(T_LOW);
    check("T_LOW readable", saved <= 0xFFFF);

    SENSOR_WIRE.beginTransmission(SENSOR);
    SENSOR_WIRE.write(T_LOW);
    size_t n = printThroughPrint(SENSOR_WIRE, "P");  // 0x50
    SENSOR_WIRE.write(0);                            // used to be ambiguous once Wire is a Print
    uint8_t err = SENSOR_WIRE.endTransmission();
    check("Wire print() through a Print& queues 1 byte", n == 1);
    check("T_LOW written with print() + write(0)", err == 0 && readSensorReg16(T_LOW) == 0x5000);

    uint8_t got = SENSOR_WIRE.requestFrom(SENSOR, (uint8_t)2, (uint32_t)T_LOW, (uint8_t)1, (uint8_t)true);
    int msb = SENSOR_WIRE.read();
    int lsb = SENSOR_WIRE.read();
    check("requestFrom(addr, 2, T_LOW, 1, true) reads the register", got == 2 && msb == 0x50 && lsb == 0x00);

    got = SENSOR_WIRE.requestFrom(SENSOR_I2C_ADDR, 2, SCRATCH_REG, 1, 1);
    check("the same with int arguments", got == 2 && SENSOR_WIRE.read() == 0x50);

#if defined(FRDM_MCXN236)
    // FXLS8974CF advances its register pointer past the bytes it has sent
    // (to 0x24 here), where P3T1755 keeps it; point it back at OFF_X first
    SENSOR_WIRE.beginTransmission(SENSOR);
    SENSOR_WIRE.write(T_LOW);
    SENSOR_WIRE.endTransmission();
#endif
    got = SENSOR_WIRE.requestFrom(SENSOR, (uint8_t)2, (uint32_t)0, (uint8_t)0, (uint8_t)true);
    check("isize 0 just reads (pointer still at T_LOW)", got == 2 && SENSOR_WIRE.read() == 0x50);

    got = SENSOR_WIRE.requestFrom((uint8_t)0x2A, (uint8_t)2, (uint32_t)T_LOW, (uint8_t)1, (uint8_t)true);
    check("absent target: returns 0, available() 0", got == 0 && SENSOR_WIRE.available() == 0);

    SENSOR_WIRE.requestFrom(SENSOR, (uint8_t)2, (uint32_t)T_LOW, (uint8_t)1, (uint8_t)true);
    int a = SENSOR_WIRE.available();
    int p1 = SENSOR_WIRE.peek();
    int p2 = SENSOR_WIRE.peek();
    check("Wire peek() doesn't consume", a == 2 && p1 == 0x50 && p2 == 0x50 && SENSOR_WIRE.available() == 2);
    check("Wire read() through a Stream&", readThroughStream(SENSOR_WIRE) == 0x50);

    SENSOR_WIRE.beginTransmission(SENSOR);
    SENSOR_WIRE.write(T_LOW);
    check("beginTransmission() keeps unread bytes", SENSOR_WIRE.available() == 1 && SENSOR_WIRE.read() == 0x00);
    SENSOR_WIRE.endTransmission();
    check("Wire peek()/read() -1 once empty", SENSOR_WIRE.peek() == -1 && SENSOR_WIRE.read() == -1);

    uint8_t buf[2] = { 0 };
    SENSOR_WIRE.requestFrom(SENSOR, (uint8_t)2, (uint32_t)T_LOW, (uint8_t)1, (uint8_t)true);
    size_t rb = SENSOR_WIRE.readBytes(buf, 2);
    check("Wire readBytes()", rb == 2 && buf[0] == 0x50 && buf[1] == 0x00);

    SENSOR_WIRE.clearWriteError();
    SENSOR_WIRE.beginTransmission(SENSOR);
    size_t total = 0;
    for (int i = 0; i < WIRE_BUFFER_SIZE; i++)
      total += SENSOR_WIRE.write((uint8_t)i);
    size_t over = SENSOR_WIRE.write((uint8_t)0);
    uint8_t more[4] = { 1, 2, 3, 4 };
    size_t overBulk = SENSOR_WIRE.write(more, 4);
    check("Wire write() returns 1 per byte up to WIRE_BUFFER_SIZE", total == WIRE_BUFFER_SIZE);
    check("Wire write() past the end returns 0 and sets the write error",
          over == 0 && overBulk == 0 && SENSOR_WIRE.getWriteError() != 0);
    SENSOR_WIRE.clearWriteError();
    SENSOR_WIRE.beginTransmission(SENSOR);  // never sent: throw the 128 bytes away

    got = SENSOR_WIRE.requestFrom(SENSOR, (size_t)200);
    check("requestFrom(200) is cut down to WIRE_BUFFER_SIZE",
          got == WIRE_BUFFER_SIZE && SENSOR_WIRE.available() == WIRE_BUFFER_SIZE);
    while (SENSOR_WIRE.available())
      SENSOR_WIRE.read();

    SENSOR_WIRE.beginTransmission(SENSOR);
    SENSOR_WIRE.write(T_LOW);
    SENSOR_WIRE.write((uint8_t)(saved >> 8));
    SENSOR_WIRE.write((uint8_t)saved);
    SENSOR_WIRE.endTransmission();
    check("T_LOW restored", readSensorReg16(T_LOW) == saved);
  }

  // ---- Wire target (slave) mode, Wire talking to its own target
  //      (the core of test_Wire_target_self; see that sketch and
  //      release_check/24 for more). Needs nothing on D18/D19 ----
  Serial.println("--- Wire as its own target (begin(address)/onReceive/onRequest) ---");
  {
    const uint8_t ADDR = 0x42;
    for (int i = 0; i < 16; i++)
      tgtRegs[i] = 0xA0 + i;

    // Nothing but the internal pull-ups Wire.begin() turns on holds the bus up
    Wire.setWireTimeout(25000);  // so a failure shows as FAIL rather than a hang
    Wire.onReceive(tgtOnReceive);
    Wire.onRequest(tgtOnRequest);
    Wire.begin(ADDR);

    Wire.beginTransmission(ADDR);
    Wire.write(2);
    Wire.write(0x11);
    Wire.write(0x22);
    uint8_t r = Wire.endTransmission();
    delay(1);  // the target runs onReceive() a moment after the STOP
    check("write to own target: onReceive(3), registers set",
          r == 0 && tgtRxCount == 3 && tgtRegs[2] == 0x11 && tgtRegs[3] == 0x22);

    tgtNOrder = 0;
    memset(tgtOrder, 0, sizeof(tgtOrder));
    uint8_t n = Wire.requestFrom(ADDR, (uint8_t)6, (uint32_t)2, (uint8_t)1, (uint8_t)true);
    uint8_t b[6] = { 0 };
    for (int i = 0; i < 6 && Wire.available(); i++)
      b[i] = Wire.read();
    check("register read: onReceive(1) then onRequest, 4 registers then 0xFF",
          n == 6 && strcmp(tgtOrder, "RQ") == 0 && b[0] == 0x11 && b[1] == 0x22 && b[2] == 0xA4 && b[3] == 0xA5 && b[4] == 0xFF && b[5] == 0xFF);

    Wire.beginTransmission(ADDR + 1);
    check("another address is NAKed", Wire.endTransmission() == 134);

    Wire.end();
  }

#if defined(FRDM_MCXN947)
  // ---- Wire2 (MikroBus I2C) bus scan (was test_Wire2_MikroBus_N947) ----
  Serial.println("--- Wire2 (MikroBus I2C) bus scan (N947 only) ---");
  {
    Wire2.begin();
    int found = 0;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
      Wire2.beginTransmission(addr);
      if (Wire2.endTransmission() == 0) {
        Serial.print("found device at 0x");
        Serial.println(addr, HEX);
        found++;
      }
    }
    Serial.print(found);
    Serial.println(" device(s) found (0 is fine -- nothing needs to be plugged in)");
    check("Wire2 bus scan completed without hanging", true);
  }
#elif defined(FRDM_MCXA156)
  // ---- Wire1 (MikroBus I2C, LPI2C3 on this board) bus scan ----
  Serial.println("--- Wire1 (MikroBus I2C) bus scan (A156 only) ---");
  {
    Wire1.begin();
    int found = 0;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
      Wire1.beginTransmission(addr);
      if (Wire1.endTransmission() == 0) {
        Serial.print("found device at 0x");
        Serial.println(addr, HEX);
        found++;
      }
    }
    Serial.print(found);
    Serial.println(" device(s) found (0 is fine -- nothing needs to be plugged in)");
    check("Wire1 bus scan completed without hanging", true);
  }
#elif defined(FRDM_MCXN236)
  // ---- Wire1 (MikroBus I2C, LPI2C2) bus scan: the on-board accelerometer
  //      and the other on-board parts on this bus answer ----
  Serial.println("--- Wire1 (MikroBus I2C) bus scan (N236 only) ---");
  {
    int found = 0;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
      Wire1.beginTransmission(addr);
      if (Wire1.endTransmission() == 0) {
        Serial.print("found device at 0x");
        Serial.println(addr, HEX);
        found++;
      }
    }
    Serial.print(found);
    Serial.println(" device(s) found");
    check("Wire1 bus scan completed without hanging", true);
  }
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
