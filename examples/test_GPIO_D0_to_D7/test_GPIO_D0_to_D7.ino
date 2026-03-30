/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include "arduino.h"

void setup(void) {
	Serial.begin(115200);
	Serial.println("Hello, world!");

	for (int i = 0; i < 8; i++)
		pinMode(i, OUTPUT);
}

void loop(void) {
	static int count = 0;

	for (int i = 0; i < 8; i++)
		digitalWrite(i, count & (0x1 << i));
	
	count++;
}
