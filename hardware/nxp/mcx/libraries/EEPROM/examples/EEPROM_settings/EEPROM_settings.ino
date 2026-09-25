/** Keep settings, and a count of how many times the board has started,
 *  across resets, power cycles and uploads.
 *
 *  Open the Serial Monitor (115200) and type a number and Enter: it
 *  becomes the new LED blink interval in milliseconds, and is still there
 *  after the next reset. The start count goes up by one on each reset.
 *
 *  Works on FRDM-MCXA153 and FRDM-MCXN947. Wiring: none.
 */

#include <Arduino.h>
#include <EEPROM.h>

// Everything kept, in one struct, at address 0. The magic number tells a
// struct this sketch wrote from whatever was there before -- 0xFF bytes on
// a board that has never stored anything, or another sketch's data.
struct Settings {
  uint32_t magic;
  uint32_t starts;
  uint16_t blinkMs;
};

const uint32_t MAGIC = 0x53455431;  // "1TES", any value this sketch alone uses

Settings settings;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  EEPROM.get(0, settings);
  if (settings.magic != MAGIC) {
    settings.magic = MAGIC;
    settings.starts = 0;
    settings.blinkMs = 500;
  }
  settings.starts++;
  EEPROM.put(0, settings);  // writes only the bytes that changed

  delay(1000);  // time to open the Serial Monitor
  Serial.print("started ");
  Serial.print(settings.starts);
  Serial.print(" times; blinking every ");
  Serial.print(settings.blinkMs);
  Serial.println("ms. Type a new interval and Enter to change it.");
}

void loop() {
  if (Serial.available()) {
    long ms = Serial.parseInt();
    if (ms >= 50 && ms <= 5000) {
      settings.blinkMs = (uint16_t)ms;
      EEPROM.put(0, settings);
      Serial.print("blinking every ");
      Serial.print(ms);
      Serial.println("ms from now on, after a reset too");
    }
  }

  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  delay(settings.blinkMs);
}
