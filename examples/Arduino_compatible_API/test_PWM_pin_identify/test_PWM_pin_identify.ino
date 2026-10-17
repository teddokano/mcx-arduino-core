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
#if defined(FRDM_MCXC444)
  // No PWM0-PWM5 pins: the TPM pins instead (64 LQFP pin numbers from the
  // data sheet). D5, D6 and D9 share TPM0_CH2, so only one of them drives
  // it at a time; this sketch drives them one by one anyway
  { D3,  "D3",  "PTA12", "28" },
  { D5,  "D5",  "PTA5",  "27" },
  { D6,  "D6",  "PTE29", "17" },
  { D9,  "D9",  "PTD2",  "59" },
  { D10, "D10", "PTD4",  "61" },
#else
  { PWM0, "PWM0", "P3_11", "?"  },
  { PWM1, "PWM1", "P3_10", "40" },
  { PWM2, "PWM2", "P3_9",  "?"  },
  { PWM3, "PWM3", "P3_8",  "42" },
  { PWM4, "PWM4", "P3_7",  "43" },
  { PWM5, "PWM5", "P3_6",  "44" },
#endif
};

const int numPwmPins = sizeof(pwmPins) / sizeof(pwmPins[0]);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("PWM pin identification test");
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
