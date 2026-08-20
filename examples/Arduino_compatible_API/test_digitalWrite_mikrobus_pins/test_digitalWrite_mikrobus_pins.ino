/** digitalWrite() output test across the MikroBus header pins
 *  -- FRDM-MCXA153 / FRDM-MCXN947
 *
 *  Walks a single HIGH pulse through the MikroBus pin macros (defined in
 *  io.h/arduino_io.h): MB_AN, MB_RST, MB_CS, MB_SCK, MB_MISO, MB_MOSI,
 *  MB_PWM, MB_INT, MB_RX, MB_TX, MB_SCL, MB_SDA.
 *
 *  MB_AN is only included on FRDM-MCXA153 (12 pins total) -- on
 *  FRDM-MCXN947 it's DISABLED_PIN (not a real, wired pin), so it's left
 *  out there (11 pins).
 *
 *  Note (N947 only): MB_RX/MB_TX are the same physical pins as
 *  I3C_SDA/I3C_SCL, already exercised via Wire1 -- included here anyway
 *  for a direct digitalWrite() check independent of the I3C peripheral.
 */

#include <Arduino.h>

struct PinInfo {
  int pin;
  const char *name;
};

PinInfo pins[] = {
#if defined(FRDM_MCXA153)
  { MB_AN, "MB_AN" },
#endif
  { MB_RST, "MB_RST" }, { MB_CS, "MB_CS" },
  { MB_SCK, "MB_SCK" }, { MB_MISO, "MB_MISO" }, { MB_MOSI, "MB_MOSI" },
  { MB_PWM, "MB_PWM" }, { MB_INT, "MB_INT" }, { MB_RX, "MB_RX" },
  { MB_TX, "MB_TX" }, { MB_SCL, "MB_SCL" }, { MB_SDA, "MB_SDA" },
};

const int numPins = sizeof(pins) / sizeof(pins[0]);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("digitalWrite MikroBus-pin walking-bit test");

  for (int i = 0; i < numPins; i++) {
    pinMode(pins[i].pin, OUTPUT);
    digitalWrite(pins[i].pin, LOW);
  }
}

void loop() {
  for (int i = 0; i < numPins; i++) {
    Serial.print("HIGH: ");
    Serial.println(pins[i].name);

    digitalWrite(pins[i].pin, HIGH);
    delay(200);
    digitalWrite(pins[i].pin, LOW);
    delay(50);
  }

  Serial.println("--- cycle complete, repeating ---");
}
