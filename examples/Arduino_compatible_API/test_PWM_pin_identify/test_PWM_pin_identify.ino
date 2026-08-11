#include <Arduino.h>

// PWM0-PWM5 -> physical pad / package pin number (from board/pin_mux.c comments,
// pin_num blank where not listed there -- cross-reference the FRDM-MCXA153
// schematic to find which header/connector each pad is routed to)
struct PwmPinInfo {
  int         pin;
  const char *name;
  const char *pad;
  const char *pkgPin;
};

PwmPinInfo pwmPins[] = {
  { PWM0, "PWM0", "P3_11", "?"  },
  { PWM1, "PWM1", "P3_10", "40" },
  { PWM2, "PWM2", "P3_9",  "?"  },
  { PWM3, "PWM3", "P3_8",  "42" },
  { PWM4, "PWM4", "P3_7",  "43" },
  { PWM5, "PWM5", "P3_6",  "44" },
};

const int numPwmPins = sizeof(pwmPins) / sizeof(pwmPins[0]);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("PWM0-PWM5 pin identification test");
  Serial.println("Probe the board (LED / multimeter / scope) while watching which pin is announced below.");
}

void loop() {
  for (int i = 0; i < numPwmPins; i++) {
    Serial.print("Driving ");
    Serial.print(pwmPins[i].name);
    Serial.print(" (pad ");
    Serial.print(pwmPins[i].pad);
    Serial.print(", pkg pin ");
    Serial.print(pwmPins[i].pkgPin);
    Serial.println(") HIGH for 3s ...");

    analogWrite(pwmPins[i].pin, 255);
    delay(3000);
    analogWrite(pwmPins[i].pin, 0);
    delay(300);
  }

  Serial.println("--- cycle complete, repeating ---");
  delay(1000);
}
