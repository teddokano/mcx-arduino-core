/** Release check 2/N: no physical wiring needed, but a human has to
 *  watch/listen (logic analyzer, scope, multimeter, or just eyes/ears) --
 *  no automatic OK/FAIL here.
 *
 *  Consolidates (from examples/Arduino_compatible_API/):
 *  test_digitalWrite_all_pins, test_digitalWrite_analog_pins,
 *  test_digitalWrite_mikrobus_pins, test_analogWrite_all_channels,
 *  test_analogWriteFrequency, test_tone, test_GPIO_toggle_speed_SDK_API.
 *  (test_GPIO_D0_to_D7 and test_analogWrite_duty aren't included --
 *  superseded by the more complete _all_pins/_all_channels versions
 *  above. test_PWM_pin_identify is a "find my pins" reference tool, not
 *  a pass/fail check, and stays as its own example.)
 *
 *  Runs the whole sequence once, then settles into a continuous SDK-API
 *  GPIO square wave on D2 for scope probing (same as
 *  test_GPIO_toggle_speed_SDK_API on its own) -- probe/listen as each
 *  section runs; nothing here waits for you, so have your logic
 *  analyzer/scope/multimeter/ears ready before starting.
 *
 *  For interactive checks that need you to actually press SW2 at a
 *  specific moment, see 03_sw2_interrupts instead.
 */

#include <Arduino.h>
#include "fsl_gpio.h"

static GPIO_Type *toggleSpeedPort;
static uint32_t   toggleSpeedMask;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("=== Release check 2/N: manual observation (no wiring) ===");

  // ---- digitalWrite() pin walk: D-pins + analog pins + MikroBus pins
  //      (was test_digitalWrite_all_pins + _analog_pins + _mikrobus_pins) ----
  Serial.println("--- digitalWrite() pin walk (single HIGH pulse per pin) ---");
  {
    struct PinInfo { int pin; const char *name; };

    PinInfo pins[] = {
      { D0, "D0" }, { D1, "D1" }, { D2, "D2" }, { D3, "D3" },
      { D4, "D4" }, { D5, "D5" }, { D6, "D6" }, { D7, "D7" },
      { D8, "D8" }, { D9, "D9" }, { D10, "D10" }, { D11, "D11" },
      { D12, "D12" }, { D13, "D13" }, { D18, "D18" }, { D19, "D19" },
#if defined(FRDM_MCXA153)
      { A0, "A0" }, { A1, "A1" },
#endif
      { A2, "A2" }, { A3, "A3" }, { A4, "A4" }, { A5, "A5" },
#if defined(FRDM_MCXA153)
      { MB_AN, "MB_AN" },
#endif
      { MB_RST, "MB_RST" }, { MB_CS, "MB_CS" },
      { MB_SCK, "MB_SCK" }, { MB_MISO, "MB_MISO" }, { MB_MOSI, "MB_MOSI" },
      { MB_PWM, "MB_PWM" }, { MB_INT, "MB_INT" }, { MB_RX, "MB_RX" },
      { MB_TX, "MB_TX" }, { MB_SCL, "MB_SCL" }, { MB_SDA, "MB_SDA" },
    };
    const int numPins = sizeof(pins) / sizeof(pins[0]);

    for (int i = 0; i < numPins; i++) {
      pinMode(pins[i].pin, OUTPUT);
      digitalWrite(pins[i].pin, LOW);
    }

    for (int i = 0; i < numPins; i++) {
      Serial.print("HIGH: ");
      Serial.println(pins[i].name);
      digitalWrite(pins[i].pin, HIGH);
      delay(150);
      digitalWrite(pins[i].pin, LOW);
      delay(50);
    }
  }

  // ---- analogWrite(): all 6 PWM channels + one independence pair
  //      (was test_analogWrite_all_channels, trimmed to one pair instead
  //      of all three to keep this section shorter) ----
  Serial.println("--- analogWrite(): all 6 channels, distinct duty each ---");
  {
    struct { int pin; const char *name; uint8_t value; } chans[] = {
      { PWM0, "PWM0", 26 },   // ~10%
      { PWM1, "PWM1", 77 },   // ~30%
      { PWM2, "PWM2", 128 },  // ~50%
      { PWM3, "PWM3", 179 },  // ~70%
      { PWM4, "PWM4", 230 },  // ~90%
      { PWM5, "PWM5", 51 },   // ~20%
    };
    for (auto &c : chans) {
      Serial.print(c.name); Serial.print(" = "); Serial.println(c.value);
      analogWrite(c.pin, c.value);
    }
    Serial.println("holding for 3s ...");
    delay(3000);

    Serial.println("--- PWM1 fixed 50%, sweeping PWM0 (independence check) ---");
    analogWrite(PWM1, 128);
    uint8_t steps[] = { 0, 64, 128, 191, 255 };
    for (uint8_t v : steps) {
      Serial.print("PWM0 = "); Serial.print(v);
      Serial.println(" (PWM1 should stay at 128/50%)");
      analogWrite(PWM0, v);
      delay(1500);
    }
  }

  // ---- analogWriteFrequency() (was test_analogWriteFrequency) ----
  Serial.println("--- analogWriteFrequency(): PWM0 through a few rates, 50% duty ---");
  {
    uint32_t freqs[] = { 1000, 50, 20, 5000 };
    for (uint32_t f : freqs) {
      analogWriteFrequency(PWM0, f);
      analogWrite(PWM0, 128);
      Serial.print("PWM0: "); Serial.print(f); Serial.print(" Hz (period ");
      Serial.print(1000000UL / f); Serial.println(" us), 50% duty");
      delay(2000);
    }
    analogWriteFrequency(PWM0, 1000);  // restore default before moving on
    analogWrite(PWM0, 0);
  }

  // ---- tone()/noTone() melody (was test_tone) ----
  Serial.println("--- tone()/noTone(): \"Twinkle Twinkle\" on D13 ---");
  {
    const int BUZZER_PIN = D13;
    int melody[] = { 262, 262, 392, 392, 440, 440, 392, 349, 349, 330, 330, 294, 294, 262 };
    int noteDurations[] = { 4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 2 };
    const int numNotes = sizeof(melody) / sizeof(melody[0]);

    for (int i = 0; i < numNotes; i++) {
      int noteDuration = 1000 / noteDurations[i];
      tone(BUZZER_PIN, melody[i], noteDuration);
      delay((int)(noteDuration * 1.30));
      noTone(BUZZER_PIN);
    }
  }

  // ---- GPIO toggle speed: digitalWrite() vs raw MCUXpresso SDK
  //      (was test_GPIO_toggle_speed_SDK_API) ----
  Serial.println("--- GPIO toggle speed: digitalWrite() vs raw SDK GPIO_Port*() ---");
  {
#define TEST_PIN     D2
#define UNROLL       10UL
#define TOGGLE_COUNT (100000UL * UNROLL)
#define DW_TOGGLE  digitalWrite(TEST_PIN, HIGH); digitalWrite(TEST_PIN, LOW);
#define SDK_TOGGLE GPIO_PortSet(toggleSpeedPort, toggleSpeedMask); GPIO_PortClear(toggleSpeedPort, toggleSpeedMask);

    pinMode(TEST_PIN, OUTPUT);
    toggleSpeedPort = digitalPinToPort(TEST_PIN);
    toggleSpeedMask = digitalPinToBitMask(TEST_PIN);

    uint32_t t0 = micros();
    for (uint32_t i = 0; i < TOGGLE_COUNT / UNROLL; i++) {
      DW_TOGGLE DW_TOGGLE DW_TOGGLE DW_TOGGLE DW_TOGGLE
      DW_TOGGLE DW_TOGGLE DW_TOGGLE DW_TOGGLE DW_TOGGLE
    }
    uint32_t digitalWrite_us = micros() - t0;

    t0 = micros();
    for (uint32_t i = 0; i < TOGGLE_COUNT / UNROLL; i++) {
      SDK_TOGGLE SDK_TOGGLE SDK_TOGGLE SDK_TOGGLE SDK_TOGGLE
      SDK_TOGGLE SDK_TOGGLE SDK_TOGGLE SDK_TOGGLE SDK_TOGGLE
    }
    uint32_t sdk_us = micros() - t0;

    Serial.print("digitalWrite(): "); Serial.print(digitalWrite_us);
    Serial.print(" us total, "); Serial.print((double)TOGGLE_COUNT / (double)digitalWrite_us, 3);
    Serial.println(" MHz toggle rate");
    Serial.print("SDK GPIO_Port*: "); Serial.print(sdk_us);
    Serial.print(" us total, "); Serial.print((double)TOGGLE_COUNT / (double)sdk_us, 3);
    Serial.println(" MHz toggle rate");
    Serial.print("Speedup: "); Serial.print((double)digitalWrite_us / (double)sdk_us, 2);
    Serial.println("x");
  }

  Serial.println();
  Serial.println("--- manual observation sequence complete ---");
  Serial.println("Now toggling continuously via the SDK API on D2 -- probe to see the square wave.");
}

void loop() {
  GPIO_PortSet(toggleSpeedPort, toggleSpeedMask);
  GPIO_PortClear(toggleSpeedPort, toggleSpeedMask);
}
