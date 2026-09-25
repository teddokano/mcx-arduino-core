/** Release check 01: automatic OK/FAIL checks, no physical wiring needed.
 *
 *  Consolidates (from examples/Arduino_compatible_API/): test_math_constants,
 *  test_arduino_compat_macros, test_avr_compat_helpers,
 *  test_MOSI_MISO_SCK_macros,
 *  test_Print_writeError, test_String, test_String_64bit,
 *  test_Serial_print_time_t, test_millis_micros, test_delayMicroseconds,
 *  test_analog_resolution_and_misc, test_Analog_read_write,
 *  test_Wire1_onboard_sensor_raw, test_Wire_Stream_requestFrom5, the core
 *  of test_Wire_target_self, and (FRDM-MCXN947 only)
 *  test_Wire2_MikroBus_N947.
 *
 *  On FRDM-MCXN947, take release_check/11's Serial1 loopback jumper
 *  (MB_TX-MB_RX) off before running this: those are Wire1's SDA/SCL pins
 *  there, and the jumper shorts them together.
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

// A minimal Print-derived class that can simulate a failing write(), used
// by the Print::*WriteError() section below. File-scope, not declared
// inside setup() -- a local class with a virtual override here tripped a
// placement-new/char_traits<char32_t> compile error in this toolchain
// that the identical class at file scope doesn't.
class FlakyPrint : public Print {
public:
  size_t write(uint8_t c) override {
    if (fail) {
      setWriteError();
      return 0;
    }
    return 1;
  }
  bool fail = false;
};

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
  Wire1.beginTransmission(0x48);
  Wire1.write(reg);
  if (Wire1.endTransmission(false) != 0 || Wire1.requestFrom((uint8_t)0x48, (size_t)2) != 2)
    return 0xFFFFFFFF;
  uint16_t v = Wire1.read() << 8;
  return v | Wire1.read();
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

// Internal pull-ups on Wire's pins; nothing else pulls the bus up here
void wirePullUps() {
#if defined(FRDM_MCXN947)
  PORT4->PCR[0] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
  PORT4->PCR[1] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
#else
  PORT1->PCR[8] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
  PORT1->PCR[9] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
#endif
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
#endif
  }

  // ---- math constants / trig (was test_math_constants) ----
  Serial.println("--- math constants ---");
  check("PI", fabs(PI - 3.14159265) < 0.00001);
  check("HALF_PI", fabs(HALF_PI - 1.57079633) < 0.00001);
  check("TWO_PI", fabs(TWO_PI - 6.28318531) < 0.00001);
  check("radians(180) == PI", fabs(radians(180.0) - PI) < 0.00001);
  check("degrees(PI) == 180", fabs(degrees(PI) - 180.0) < 0.00001);
  check("sin(HALF_PI) == 1", fabs(sin(HALF_PI) - 1.0) < 0.00001);
  check("sqrt(2.0)", fabs(sqrt(2.0) - 1.41421356) < 0.00001);

  // ---- UNO R3/R4 compat macros (was test_arduino_compat_macros) ----
  Serial.println("--- compat macros ---");
  check("min(3,7)", min(3, 7) == 3);
  check("max(3,7)", max(3, 7) == 7);
  check("abs(-5)", abs(-5) == 5);
  check("constrain(15,0,10)", constrain(15, 0, 10) == 10);
  check("sq(4)", sq(4) == 16);
  check("map(512,0,1023,0,255)", map(512, 0, 1023, 0, 255) == 127);

  uint8_t v = 0;
  bitSet(v, 3);
  check("bitSet/bitRead", bitRead(v, 3) == 1);
  bitClear(v, 3);
  check("bitClear", bitRead(v, 3) == 0);

  check("lowByte(0x1234)", lowByte(0x1234) == 0x34);
  check("highByte(0x1234)", highByte(0x1234) == 0x12);
  check("bit(3)", bit(3) == 8);

  noInterrupts();
  interrupts();
  check("interrupts()/noInterrupts() (reached here without hanging)", true);

  byte b = 200;
  word w = 5000;
  boolean flag = true;
  check("byte/word/boolean types", b == 200 && w == 5000 && flag == true);

  // ---- AVR-era helpers (was test_avr_compat_helpers) ----
  Serial.println("--- AVR-era helpers ---");
  {
    char s[40];
    check("itoa(-123, 10)", strcmp(itoa(-123, s, 10), "-123") == 0);
    check("itoa(-1, 16) (32-bit int)", strcmp(itoa(-1, s, 16), "ffffffff") == 0);
    check("utoa(255, 2)", strcmp(utoa(255, s, 2), "11111111") == 0);
    check("ltoa(-2147483648, 10)", strcmp(ltoa(-2147483647L - 1, s, 10), "-2147483648") == 0);
    check("ultoa(4294967295, 16)", strcmp(ultoa(4294967295UL, s, 16), "ffffffff") == 0);
    check("dtostrf(3.14159, 7, 2)", strcmp(dtostrf(3.14159, 7, 2, s), "   3.14") == 0);
    check("dtostrf(1.999, 1, 2) rounds", strcmp(dtostrf(1.999, 1, 2, s), "2.00") == 0);
    check("dtostrf(-0.26, -7, 1) left-aligned", strcmp(dtostrf(-0.26, -7, 1, s), "-0.3   ") == 0);
    check("word(0x12, 0x34)", word(0x12, 0x34) == 0x1234);
    check("_BV(5)", _BV(5) == 32);
    analogReference(DEFAULT);
    analogReference(AR_DEFAULT);
    check("analogReference(DEFAULT/AR_DEFAULT) accepted", true);
    HardwareSerial &port = Serial;
    check("HardwareSerial& refers to Serial", &port == &Serial);
  }

  // ---- MOSI/MISO/SCK bare macros (was test_MOSI_MISO_SCK_macros) ----
  Serial.println("--- MOSI/MISO/SCK macros ---");
  check("MOSI == ARD_MOSI", MOSI == ARD_MOSI);
  check("MISO == ARD_MISO", MISO == ARD_MISO);
  check("SCK == ARD_SCK", SCK == ARD_SCK);

  // ---- Print::set/get/clearWriteError() (was test_Print_writeError) ----
  Serial.println("--- Print::*WriteError() ---");
  {
    FlakyPrint flaky;
    check("initial getWriteError() == 0", flaky.getWriteError() == 0);
    flaky.fail = true;
    flaky.print("x");
    check("getWriteError() != 0 after failed write", flaky.getWriteError() != 0);
    flaky.clearWriteError();
    check("getWriteError() == 0 after clearWriteError()", flaky.getWriteError() == 0);
    flaky.fail = false;
    flaky.print("x");
    check("getWriteError() == 0 after successful write", flaky.getWriteError() == 0);
    // print(double) rounds (was test_print_float_rounding): "10.00" is 5
    // bytes where truncating gave "9.99", and "2" is 1 where it gave "1."
    check("print(9.9999) rounds up to \"10.00\"", flaky.print(9.9999) == 5);
    check("print(1.999, 0) is \"2\", no decimal point", flaky.print(1.999, 0) == 1);
  }

  // ---- String (was test_String) ----
  Serial.println("--- String ---");
  {
    String a = "Hello";
    String b = String(", ") + "world" + String('!');
    String c = a + b;
    check("concat", c == "Hello, world!");

    String num = String(42) + " / " + String(3.14, 2);
    check("numeric concat", num == "42 / 3.14");
    check("String(1.999) rounds to \"2.00\" (was test_print_float_rounding)", String(1.999) == "2.00");

    check("length", c.length() == 13);
    check("charAt", c.charAt(0) == 'H');
    check("indexOf", c.indexOf("world") == 7);
    check("substring", c.substring(7, 12) == "world");
    check("startsWith", c.startsWith("Hello"));
    check("endsWith", c.endsWith("!"));

    String upper = c;
    upper.toUpperCase();
    check("toUpperCase", upper == "HELLO, WORLD!");

    String spaced = "   trim me   ";
    spaced.trim();
    check("trim", spaced == "trim me");

    String replaced = c;
    replaced.replace("world", "there");
    check("replace", replaced == "Hello, there!");

    check("toInt", String("12345").toInt() == 12345);

    float ft = String("3.5").toFloat();
    check("toFloat", ft > 3.49f && ft < 3.51f);
  }

  // ---- String 64bit (was test_String_64bit) ----
  Serial.println("--- String 64bit ---");
  {
    long long bigNeg = -5000000000LL;
    unsigned long long bigPos = 10000000000ULL;

    check("String(long long)", String(bigNeg) == "-5000000000");
    check("String(unsigned long long)", String(bigPos) == "10000000000");

    String s3 = "value=";
    s3 += bigPos;
    check("operator+=(unsigned long long)", s3 == "value=10000000000");

    String s4;
    s4.concat(bigNeg);
    check("concat(long long)", s4 == "-5000000000");

    check("String(long long, HEX)", String(255LL, HEX) == "ff");
  }

  // ---- Serial.print(time_t)/(long long)/(unsigned long long) overload
  //      resolution (was test_Serial_print_time_t) -- the original point
  //      of this test is that these lines *compile* at all (time_t is
  //      ambiguous between long/long long on this newlib), so there's no
  //      further runtime check beyond that; printed for a human to glance
  //      at if something looks wrong.
  Serial.println("--- Serial.print(time_t / long long / unsigned long long) ---");
  time_t current_time = 1734567890;
  Serial.print("time_t: ");
  Serial.println(current_time);
  Serial.print("long long: ");
  Serial.println(123456789012345LL);
  Serial.print("unsigned long long: ");
  Serial.println(18446744073709551615ULL);

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

  // ---- Wire1 on-board I3C-in-I2C-mode sensor, raw registers
  //      (was test_Wire1_onboard_sensor_raw) ----
  Serial.println("--- Wire1 on-board temperature sensor (raw registers) ---");
  {
    const uint8_t SENSOR_ADDR = 0x48;
    const uint8_t TEMP_REG = 0x00;

    Wire1.begin();
    Wire1.beginTransmission(SENSOR_ADDR);
    Wire1.write(TEMP_REG);
    uint8_t err = Wire1.endTransmission(false);
    check("Wire1 endTransmission(false)", err == 0);

    uint8_t n = Wire1.requestFrom(SENSOR_ADDR, (size_t)2);
    check("Wire1 requestFrom() got 2 bytes", n == 2);

    if (n == 2) {
      uint8_t msb = Wire1.read();
      uint8_t lsb = Wire1.read();
      int16_t raw = (int16_t)((msb << 8) | lsb);
      raw >>= 5;
      float celsius = raw * 0.125f;
      Serial.print("temp = "); Serial.print(celsius, 3); Serial.println(" degC");
      check("on-board sensor reads a sane temperature", celsius > -20.0f && celsius < 60.0f);
    }
  }

  // ---- Wire as a Stream, five-argument requestFrom(), buffer limits
  //      (was test_Wire_Stream_requestFrom5). Writes the sensor's T_LOW
  //      register, which nothing else depends on, and puts it back ----
  Serial.println("--- Wire as a Stream / five-argument requestFrom() ---");
  {
    const uint8_t SENSOR = 0x48;
    const uint8_t T_LOW = 0x02;

    uint32_t saved = readSensorReg16(T_LOW);
    check("T_LOW readable", saved <= 0xFFFF);

    Wire1.beginTransmission(SENSOR);
    Wire1.write(T_LOW);
    size_t n = printThroughPrint(Wire1, "P");  // 0x50
    Wire1.write(0);                            // used to be ambiguous once Wire is a Print
    uint8_t err = Wire1.endTransmission();
    check("Wire print() through a Print& queues 1 byte", n == 1);
    check("T_LOW written with print() + write(0)", err == 0 && readSensorReg16(T_LOW) == 0x5000);

    uint8_t got = Wire1.requestFrom(SENSOR, (uint8_t)2, (uint32_t)T_LOW, (uint8_t)1, (uint8_t)true);
    int msb = Wire1.read();
    int lsb = Wire1.read();
    check("requestFrom(addr, 2, T_LOW, 1, true) reads the register", got == 2 && msb == 0x50 && lsb == 0x00);

    got = Wire1.requestFrom(0x48, 2, 0x02, 1, 1);
    check("the same with int arguments", got == 2 && Wire1.read() == 0x50);

    got = Wire1.requestFrom(SENSOR, (uint8_t)2, (uint32_t)0, (uint8_t)0, (uint8_t)true);
    check("isize 0 just reads (pointer still at T_LOW)", got == 2 && Wire1.read() == 0x50);

    got = Wire1.requestFrom((uint8_t)0x2A, (uint8_t)2, (uint32_t)T_LOW, (uint8_t)1, (uint8_t)true);
    check("absent target: returns 0, available() 0", got == 0 && Wire1.available() == 0);

    Wire1.requestFrom(SENSOR, (uint8_t)2, (uint32_t)T_LOW, (uint8_t)1, (uint8_t)true);
    int a = Wire1.available();
    int p1 = Wire1.peek();
    int p2 = Wire1.peek();
    check("Wire peek() doesn't consume", a == 2 && p1 == 0x50 && p2 == 0x50 && Wire1.available() == 2);
    check("Wire read() through a Stream&", readThroughStream(Wire1) == 0x50);

    Wire1.beginTransmission(SENSOR);
    Wire1.write(T_LOW);
    check("beginTransmission() keeps unread bytes", Wire1.available() == 1 && Wire1.read() == 0x00);
    Wire1.endTransmission();
    check("Wire peek()/read() -1 once empty", Wire1.peek() == -1 && Wire1.read() == -1);

    uint8_t buf[2] = { 0 };
    Wire1.requestFrom(SENSOR, (uint8_t)2, (uint32_t)T_LOW, (uint8_t)1, (uint8_t)true);
    size_t rb = Wire1.readBytes(buf, 2);
    check("Wire readBytes()", rb == 2 && buf[0] == 0x50 && buf[1] == 0x00);

    Wire1.clearWriteError();
    Wire1.beginTransmission(SENSOR);
    size_t total = 0;
    for (int i = 0; i < WIRE_BUFFER_SIZE; i++)
      total += Wire1.write((uint8_t)i);
    size_t over = Wire1.write((uint8_t)0);
    uint8_t more[4] = { 1, 2, 3, 4 };
    size_t overBulk = Wire1.write(more, 4);
    check("Wire write() returns 1 per byte up to WIRE_BUFFER_SIZE", total == WIRE_BUFFER_SIZE);
    check("Wire write() past the end returns 0 and sets the write error",
          over == 0 && overBulk == 0 && Wire1.getWriteError() != 0);
    Wire1.clearWriteError();
    Wire1.beginTransmission(SENSOR);  // never sent: throw the 128 bytes away

    got = Wire1.requestFrom(SENSOR, (size_t)200);
    check("requestFrom(200) is cut down to WIRE_BUFFER_SIZE",
          got == WIRE_BUFFER_SIZE && Wire1.available() == WIRE_BUFFER_SIZE);
    while (Wire1.available())
      Wire1.read();

    Wire1.beginTransmission(SENSOR);
    Wire1.write(T_LOW);
    Wire1.write((uint8_t)(saved >> 8));
    Wire1.write((uint8_t)saved);
    Wire1.endTransmission();
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

    Wire.setWireTimeout(25000);  // also clears a bus-busy latched while the pins floated
    Wire.onReceive(tgtOnReceive);
    Wire.onRequest(tgtOnRequest);
    Wire.begin(ADDR);
    wirePullUps();
    delay(2);
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
