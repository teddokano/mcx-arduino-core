/** analogReference / analogRead(Write)Resolution / yield / ctype.h wrappers
 *
 *  - analogReference(): no-op on this board (fixed hardware reference),
 *    just checked here for "compiles and doesn't crash"
 *  - analogReadResolution(): switches analogRead(A2) from the default
 *    10bit (0-1023) range to 12bit (0-4095) -- verified against A2's
 *    actual reading
 *  - analogWriteResolution(): exercised (0-1023 write at 10bit) but not
 *    independently verified here -- no scope/LED brightness readback
 *    available in an automated test
 *  - yield(): no-op, just checked for "compiles and doesn't hang"
 *  - ctype.h wrappers: pure logic, checked against known characters
 */

#include <Arduino.h>

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("analogReference / resolution / yield / ctype.h test");

  // ---- analogReference (no-op, just check it compiles/doesn't crash) ----
  analogReference(0);
  Serial.println("analogReference(0) called, no crash");

  // ---- analogReadResolution ----
  analogReadResolution(10);
  int v10 = analogRead(A2);
  Serial.print("A2 @ 10bit: ");
  Serial.println(v10);
  check("10bit range", v10 >= 0 && v10 <= 1023);

  analogReadResolution(12);
  int v12 = analogRead(A2);
  Serial.print("A2 @ 12bit: ");
  Serial.println(v12);
  check("12bit range", v12 >= 0 && v12 <= 4095);

  // same physical signal, so the 12bit reading should be roughly 4x the
  // 10bit one (within ADC noise -- generous tolerance)
  check("12bit ~= 10bit*4", abs(v12 - v10 * 4) < 200);

  analogReadResolution(10);  // restore default

  // ---- analogWriteResolution (exercised, not independently verified) ----
  analogWriteResolution(10);
  analogWrite(PWM0, 512);  // ~50% duty at 10bit
  analogWriteResolution(8);
  analogWrite(PWM0, 128);  // ~50% duty at 8bit (back to default)
  Serial.println("analogWriteResolution + analogWrite exercised, no crash");

  // ---- yield ----
  yield();
  Serial.println("yield() called, no crash");

  // ---- ctype.h wrappers ----
  check("isAlpha('a')", isAlpha('a') && !isAlpha('5'));
  check("isDigit('5')", isDigit('5') && !isDigit('a'));
  check("isAlphaNumeric('a')", isAlphaNumeric('a') && isAlphaNumeric('5') && !isAlphaNumeric(' '));
  check("isSpace(' ')", isSpace(' ') && !isSpace('a'));
  check("isWhitespace('\\t')", isWhitespace('\t') && !isWhitespace('a'));
  check("isUpperCase('A')", isUpperCase('A') && !isUpperCase('a'));
  check("isLowerCase('a')", isLowerCase('a') && !isLowerCase('A'));
  check("isPunct(',')", isPunct(',') && !isPunct('a'));
  check("isControl('\\n')", isControl('\n') && !isControl('a'));
  check("isPrintable('a')", isPrintable('a') && !isPrintable('\n'));
  check("isGraph('a')", isGraph('a') && !isGraph(' '));
  check("isAscii('a')", isAscii('a') && isAscii(127) && !isAscii(200));
  check("isHexadecimalDigit('F')", isHexadecimalDigit('F') && isHexadecimalDigit('9') && !isHexadecimalDigit('G'));
}

void loop() {
}
