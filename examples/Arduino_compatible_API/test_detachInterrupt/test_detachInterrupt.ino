/** detachInterrupt() test
 *
 *  Press SW2:
 *   - first 3 presses toggle the blue LED and count up (interrupt active)
 *   - after the 3rd press, detachInterrupt() is called -- further presses
 *     should have NO effect (LED stops toggling, count stops increasing)
 *   - after 3 seconds, attachInterrupt() is called again -- presses should
 *     resume working
 */

#include <Arduino.h>

volatile bool sw_pressed = false;
bool led_state = true;
int count = 0;

void callback(void) {
  sw_pressed = true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("detachInterrupt test");
  Serial.println("Press SW2 up to 3 times");

  pinMode(BLUE, OUTPUT);
  pinMode(SW2, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(SW2), callback, FALLING);
  digitalWrite(BLUE, led_state);
}

void loop() {
  if (sw_pressed) {
    sw_pressed = false;
    led_state = !led_state;
    digitalWrite(BLUE, led_state);
    count++;
    Serial.print("SW2 pressed: ");
    Serial.println(count);
    delay(100);  // debounce

    if (count == 3) {
      Serial.println("detachInterrupt() -- further presses should do nothing");
      detachInterrupt(digitalPinToInterrupt(SW2));

      delay(3000);

      Serial.println("attachInterrupt() again -- presses should resume working");
      attachInterrupt(digitalPinToInterrupt(SW2), callback, FALLING);
    }
  }
}
