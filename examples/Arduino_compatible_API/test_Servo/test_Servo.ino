/** The bundled Servo library, with the official library's interface.
 *
 *  The board measures its own servo pulses with pulseIn(); no servo motor
 *  is needed. One jumper, the same as release_check/13's servo check:
 *    FRDM-MCXA153, FRDM-MCXA156, FRDM-MCXN947: PWM0 <-> D8
 *    FRDM-MCXN236: D6 <-> D7 (PWM0 is D6 itself there)
 *
 *  Checks the pulse widths for write() in degrees and microseconds, the
 *  clamping, attach() with its own min/max, a value written before
 *  attach(), the 20ms period, detach(), attach() on a pin with no PWM,
 *  more objects than MAX_SERVOS, and that analogWrite() keeps its own
 *  8-bit scaling afterwards.
 *
 *  Read the final "ALL PASS"/"N FAIL" line.
 */

#include <Arduino.h>
#include <Servo.h>

#define SERVO_PIN PWM0
#if defined(FRDM_MCXN236)
#define MONITOR_PIN D7
#elif defined(FRDM_MCXA153) || defined(FRDM_MCXA156) || defined(FRDM_MCXN947)
#define MONITOR_PIN D8
#else
#error "No monitor pin for this board yet"
#endif
#define NO_PWM_PIN D2   // no PWM on any of the boards

Servo servo;
Servo early;

int fails = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    fails++;
}

// Measure the HIGH pulse and compare it with the expected width.
void check_pulse(const char *label, long expect_us) {
  delay(50);  // let the new width reach the pin
  long w = pulseIn(MONITOR_PIN, HIGH, 100000UL);
  Serial.print(label);
  Serial.print(" -> ");
  Serial.print(w);
  Serial.print("us (expect ");
  Serial.print(expect_us);
  Serial.print("): ");
  bool ok = labs(w - expect_us) <= 15;
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    fails++;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  pinMode(MONITOR_PIN, INPUT);

  Serial.println("Servo library test");

  Serial.println("--- attach and the default width ---");
  check("not attached before attach()", !servo.attached());
  check("readMicroseconds() before attach() is 1500", servo.readMicroseconds() == DEFAULT_PULSE_WIDTH);
  uint8_t index = servo.attach(SERVO_PIN);
  check("attach(PWM0) gives index 0", index == 0);
  check("attached()", servo.attached());
  check_pulse("default width", 1500);

  Serial.println("--- write() in degrees ---");
  servo.write(0);
  check_pulse("write(0)", 544);
  servo.write(90);
  check_pulse("write(90)", 1472);
  check("read() after write(90)", servo.read() == 90);
  servo.write(180);
  check_pulse("write(180)", 2400);
  servo.write(45);
  check("read() after write(45)", servo.read() == 45);
  servo.write(-10);
  check("write(-10) is 0 degrees", servo.readMicroseconds() == 544);
  servo.write(300);
  check("write(300) is 180 degrees", servo.readMicroseconds() == 2400);

  Serial.println("--- microseconds ---");
  servo.writeMicroseconds(1000);
  check_pulse("writeMicroseconds(1000)", 1000);
  check("readMicroseconds()", servo.readMicroseconds() == 1000);
  servo.write(1800);
  check_pulse("write(1800) is microseconds", 1800);
  servo.writeMicroseconds(3000);
  check("writeMicroseconds(3000) clamps to 2400", servo.readMicroseconds() == 2400);
  servo.writeMicroseconds(100);
  check("writeMicroseconds(100) clamps to 544", servo.readMicroseconds() == 544);

  Serial.println("--- period ---");
  servo.writeMicroseconds(1500);
  delay(50);
  long low = pulseIn(MONITOR_PIN, LOW, 100000UL);
  Serial.print("LOW time -> ");
  Serial.print(low);
  Serial.println("us");
  check("period 20ms (LOW time ~18500us)", labs(low - 18500) <= 100);

  Serial.println("--- attach(pin, min, max) ---");
  servo.attach(SERVO_PIN, 1000, 2000);
  check("re-attach keeps the width within the new range", servo.readMicroseconds() == 1500);
  servo.write(0);
  check_pulse("write(0) with min 1000", 1000);
  servo.write(180);
  check_pulse("write(180) with max 2000", 2000);
  servo.write(90);
  check("read() with min/max", servo.read() == 90);
  servo.writeMicroseconds(500);
  check("clamps to min 1000", servo.readMicroseconds() == 1000);

  Serial.println("--- detach ---");
  servo.detach();
  check("not attached after detach()", !servo.attached());
  delay(50);
  check("no pulses after detach()", pulseIn(MONITOR_PIN, HIGH, 60000UL) == 0 && digitalRead(MONITOR_PIN) == LOW);

  Serial.println("--- a value written before attach() ---");
  early.write(180);
  check("early.attach(PWM0) gives index 1", early.attach(SERVO_PIN) == 1);
  check_pulse("sends 2400us from the start", 2400);
  early.detach();

  Serial.println("--- pins with no PWM, and too many objects ---");
  Servo on_gpio;
  check("attach(D2) fails", on_gpio.attach(NO_PWM_PIN) == INVALID_SERVO && !on_gpio.attached());
  Servo extra[MAX_SERVOS];
  Servo &last = extra[MAX_SERVOS - 1];
  check("object beyond MAX_SERVOS can't attach", last.attach(SERVO_PIN) == INVALID_SERVO && last.readMicroseconds() == 0);

  Serial.println("--- analogWrite() afterwards ---");
  // Still 8-bit: 128 of 255 at the servo's 50Hz is a ~10ms pulse. Had
  // the library changed the resolution to 16 bits, it would be ~39us.
  analogWrite(SERVO_PIN, 128);
  check_pulse("analogWrite(PWM0, 128) at 50Hz", 10039);
  analogWrite(SERVO_PIN, 0);
  analogWriteFrequency(SERVO_PIN, 1000);

  Serial.println();
  if (fails == 0)
    Serial.println("ALL PASS");
  else {
    Serial.print(fails);
    Serial.println(" FAIL");
  }
}

void loop() {
}
