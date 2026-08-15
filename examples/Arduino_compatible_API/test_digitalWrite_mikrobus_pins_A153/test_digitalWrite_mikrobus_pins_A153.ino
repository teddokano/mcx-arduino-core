/** digitalWrite() output test across the MikroBus header pins, FRDM-MCXA153
 *
 *  Walks a single HIGH pulse through the MikroBus pin macros (defined in
 *  io.h/arduino_io.h): MB_AN, MB_RST, MB_CS, MB_SCK, MB_MISO, MB_MOSI,
 *  MB_PWM, MB_INT, MB_RX, MB_TX, MB_SCL, MB_SDA (12 pins -- unlike N947,
 *  MB_AN is a real, wired pin on this board, not DISABLED_PIN).
 */

#include <Arduino.h>

struct PinInfo {
  int pin;
  const char *name;
};

PinInfo pins[] = {
  { MB_AN, "MB_AN" }, { MB_RST, "MB_RST" }, { MB_CS, "MB_CS" },
  { MB_SCK, "MB_SCK" }, { MB_MISO, "MB_MISO" }, { MB_MOSI, "MB_MOSI" },
  { MB_PWM, "MB_PWM" }, { MB_INT, "MB_INT" }, { MB_RX, "MB_RX" },
  { MB_TX, "MB_TX" }, { MB_SCL, "MB_SCL" }, { MB_SDA, "MB_SDA" },
};

const int numPins = sizeof(pins) / sizeof(pins[0]);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("digitalWrite MikroBus-pin walking-bit test (A153)");

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
