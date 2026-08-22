/** Release check 3/N: SW2-interactive interrupt checks -- no wiring, but
 *  you have to actually press the on-board SW2 button as prompted. No
 *  automatic OK/FAIL; the running counts/prints are for you to judge.
 *
 *  Consolidates (from examples/Arduino_compatible_API/): test_Interrupt_SW2,
 *  test_detachInterrupt, test_Interrupt_LOW.
 *
 *  Each phase has a generous timeout (it moves on rather than hanging
 *  forever if SW2 never gets pressed) -- but this is fundamentally an
 *  interactive sketch, so have your finger on SW2 before starting.
 */

#include <Arduino.h>

volatile bool          sw_pressed = false;
volatile unsigned long levelCount = 0;
bool                    led_state = true;

void onFalling() { sw_pressed = true; }
void onLevel()   { levelCount = levelCount + 1; }

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("=== Release check 3/N: SW2 interactive checks ===");

  pinMode(BLUE, OUTPUT);
  pinMode(SW2, INPUT_PULLUP);
  digitalWrite(BLUE, led_state);

  // ---- Phase 1: FALLING-edge interrupt + detachInterrupt()
  //      (was test_Interrupt_SW2 + test_detachInterrupt) ----
  Serial.println();
  Serial.println("--- Phase 1: FALLING-edge interrupt + detachInterrupt() ---");
  Serial.println("Press SW2 up to 3 times (LED toggles, count prints each press)");
  attachInterrupt(digitalPinToInterrupt(SW2), onFalling, FALLING);

  int presses = 0;
  unsigned long phase1Start = millis();
  while (presses < 3 && millis() - phase1Start < 30000) {
    if (sw_pressed) {
      sw_pressed = false;
      led_state = !led_state;
      digitalWrite(BLUE, led_state);
      presses++;
      Serial.print("SW2 pressed: ");
      Serial.println(presses);
      delay(100);  // debounce
    }
  }
  if (presses < 3)
    Serial.println("(timed out waiting for presses -- moving on)");

  Serial.println("detachInterrupt() -- presses for the next 3s should do nothing");
  detachInterrupt(digitalPinToInterrupt(SW2));
  sw_pressed = false;
  unsigned long silentStart = millis();
  bool sawSpuriousPress = false;
  while (millis() - silentStart < 3000) {
    if (sw_pressed) {
      sawSpuriousPress = true;
      sw_pressed = false;
    }
  }
  Serial.println(sawSpuriousPress
                    ? "  (unexpected: a press registered while detached!)"
                    : "  no presses registered while detached, as expected");

  // ---- Phase 2: LOW-level-triggered interrupt (was test_Interrupt_LOW) ----
  Serial.println();
  Serial.println("--- Phase 2: LOW-level-triggered interrupt ---");
  Serial.println("Press and HOLD SW2 for ~1 second, then release (up to 10s window)");
  levelCount = 0;
  attachInterrupt(digitalPinToInterrupt(SW2), onLevel, LOW);

  unsigned long phase2Start = millis();
  unsigned long lastPrinted = 0;
  while (millis() - phase2Start < 10000) {
    if (levelCount != lastPrinted) {
      lastPrinted = levelCount;
      Serial.print("count = ");
      Serial.println(levelCount);
    }
    delay(100);
  }
  detachInterrupt(digitalPinToInterrupt(SW2));

  Serial.print("final LOW-mode interrupt count = ");
  Serial.println(levelCount);
  Serial.println(levelCount > 20
                    ? "count jumped by a lot while held -- level-triggered behavior confirmed"
                    : "count stayed low -- SW2 may not have been held down, or level-triggering isn't working");

  Serial.println();
  Serial.println("--- SW2 interactive sequence complete ---");
}

void loop() {
}
