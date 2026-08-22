/** Release check 1/N: automatic OK/FAIL checks, no physical wiring needed.
 *
 *  Consolidates (from examples/Arduino_compatible_API/): test_math_constants,
 *  test_arduino_compat_macros, test_MOSI_MISO_SCK_macros,
 *  test_Print_writeError, test_String, test_String_64bit,
 *  test_Serial_print_time_t, test_millis_micros, test_delayMicroseconds,
 *  test_analog_resolution_and_misc, test_Analog_read_write,
 *  test_Wire1_onboard_sensor_raw, and (FRDM-MCXN947 only)
 *  test_Wire2_MikroBus_N947.
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

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("=== Release check 1/N: no-wiring automatic checks ===");

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
