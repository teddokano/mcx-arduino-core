/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include "arduino.h"

#define TARGET_ADDRESS 0x4C

void setup(void) {
	Serial.begin(115200);
	Serial.println("Hello, world!");

	Wire.begin();

	pinMode(BLUE, OUTPUT);
}

void loop(void) {
	Wire.beginTransmission( TARGET_ADDRESS );
	Wire.write( 0x00 );
	Wire.endTransmission( false );

	uint8_t buf[2];
	int i = 0;

	Wire.requestFrom(TARGET_ADDRESS, 2);

	while (Wire.available())
		buf[i++] = Wire.read();

	Serial.print("P3T1085 temperature = ");
	Serial.print((double)(((int16_t)buf[0] << 8) | buf[1]) / 256.00);
	Serial.println("℃");

	Serial.println("LED - ON");
	digitalWrite(BLUE, LOW);
	delay(50);

	Serial.println("LED - OFF");
	digitalWrite(BLUE, HIGH);
	delay(950);
}
