/** Wire.setClock() test
 *
 *  Uses the on-board P3T1755 temperature sensor over Wire1 (I3C in I2C
 *  mode) -- no external wiring needed. Reads the sensor at the default
 *  clock, switches clock speed at runtime with setClock(), and confirms
 *  the sensor still responds with a sane reading afterward.
 */

#include <P3T1755.h>
#include <Wire.h>

P3T1755 sensor(Wire1, 0x48);

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
}

bool sane(float t) {
  return t > -40.0 && t < 125.0;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Wire.setClock() test");

  Wire1.begin();

  float t1 = sensor.temp();
  Serial.print("temp @ default clock: ");
  Serial.println(t1, 2);
  check("default clock read", sane(t1));

  Wire1.setClock(400000);
  float t2 = sensor.temp();
  Serial.print("temp @ 400kHz: ");
  Serial.println(t2, 2);
  check("400kHz read", sane(t2));

  Wire1.setClock(100000);
  float t3 = sensor.temp();
  Serial.print("temp @ 100kHz (back to default): ");
  Serial.println(t3, 2);
  check("100kHz read", sane(t3));
}

void loop() {
}
