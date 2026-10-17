/** FRDM-MCXC444 bring-up: the first sketch to run on a new board.
 *
 *  Goes through docs/porting_a_new_board.md's bring-up order in one sketch,
 *  with nothing wired to the board: the red LED, Serial, clocks, millis()/
 *  micros(), every header pin's pull-up and pull-down, pin interrupts,
 *  analogRead() (against the ADC's own internal references), analogWrite()
 *  and analogWriteFrequency(), tone(), Wire1 (the on-board accelerometer),
 *  SPI, EEPROM, Servo, and last a Wire (D18/D19) bus scan.
 *
 *  Most checks read the hardware back: the TPM's registers for the PWM
 *  period and duty, and the pin itself through its GPIO input register
 *  (PDIR reads any pin set to a digital function, the TPM's included,
 *  per the reference manual's GPIO chapter) for the waveform. Timing checks
 *  compare two rates, so the driver's own overhead cancels out.
 *
 *  - Before Serial starts, the red LED blinks 3 times. If it does not, the
 *    problem is before any output: startup, clocks, the linker script or
 *    the upload.
 *  - Each section prints its name before it runs, so if the board stops,
 *    the last line says where.
 *  - Keep the board still: the accelerometer check expects about 1g.
 *  - A jumper D11-D12 adds the SPI loopback; without it that check SKIPs.
 *  - The board counts its starts in EEPROM: press RESET and the count goes
 *    up by one.
 *  - D4 is the reset-time NMI pin (SW3). Holding SW3 down while pressing
 *    RESET must still start the sketch (the flash option byte blocks the
 *    NMI).
 *
 *  At the end it prints ALL OK or the number of FAILs, then fades the blue
 *  LED (D6, PWM).
 */

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Servo.h>
#include "fsl_clock.h"
#include "fsl_adc16.h"

#if !defined(FRDM_MCXC444)
#error "This sketch is for FRDM-MCXC444 only"
#endif

// The types come before any function: the IDE puts its function prototypes
// above the first function

// A pin, read and pulled at the register level
struct RawPin {
  PORT_Type *port;
  GPIO_Type *gpio;
  uint8_t bit;
};

// What sample() saw on a pin: the share of time it was high, and its
// rising edges
struct Waveform {
  uint32_t samples;
  uint32_t highs;
  uint32_t rises;
  float duty() const { return samples ? (float)highs / (float)samples : 0.0f; }
};

// The EEPROM start counter
struct BootRecord {
  uint32_t magic;
  uint32_t count;
};

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

void section(const char *name) {
  Serial.print("--- ");
  Serial.print(name);
  Serial.println(" ---");
  Serial.flush();  // so it is out before anything below can stop the board
}

bool within(float v, float expect, float tolerance) {
  return v >= expect * (1.0f - tolerance) && v <= expect * (1.0f + tolerance);
}

// ---- pins, read and pulled at the register level ----

// Schematic SCH-93440 Rev A and PIN_MAPPING_C444.md
const RawPin PIN_D2 = { PORTD, GPIOD, 3 };    // PTD3
const RawPin PIN_D3 = { PORTA, GPIOA, 12 };   // PTA12, TPM1_CH0
const RawPin PIN_D5 = { PORTA, GPIOA, 5 };    // PTA5,  TPM0_CH2
const RawPin PIN_D6 = { PORTE, GPIOE, 29 };   // PTE29, TPM0_CH2, blue LED
const RawPin PIN_D8 = { PORTA, GPIOA, 13 };   // PTA13
const RawPin PIN_D9 = { PORTD, GPIOD, 2 };    // PTD2,  TPM0_CH2
const RawPin PIN_D10 = { PORTD, GPIOD, 4 };   // PTD4,  TPM0_CH4

int level(const RawPin &p) {
  return (p.gpio->PDIR >> p.bit) & 1;
}

uint32_t mux(const RawPin &p) {
  return (p.port->PCR[p.bit] & PORT_PCR_MUX_MASK) >> PORT_PCR_MUX_SHIFT;
}

bool drivenLow(const RawPin &p) {
  return mux(p) == 1 && ((p.gpio->PDDR >> p.bit) & 1) && !((p.gpio->PDOR >> p.bit) & 1);
}

// Turns the pin's pull on, up or down. ISF is write-1-to-clear, so it is
// masked out of the value written back.
void setPull(const RawPin &p, bool up) {
  uint32_t v = p.port->PCR[p.bit] & ~(PORT_PCR_ISF_MASK | PORT_PCR_PS_MASK);
  p.port->PCR[p.bit] = v | PORT_PCR_PE_MASK | (up ? PORT_PCR_PS_MASK : 0);
}

// Samples a pin for a while
Waveform sample(const RawPin &p, uint32_t us) {
  Waveform w = {};
  int prev = level(p);
  uint32_t t0 = micros();
  while (micros() - t0 < us) {
    int v = level(p);
    w.samples++;
    w.highs += v;
    if (v && !prev)
      w.rises++;
    prev = v;
  }
  return w;
}

// ---- TPM read-back ----

float tpmFrequency(TPM_Type *tpm) {
  uint32_t ps = (tpm->SC & TPM_SC_PS_MASK) >> TPM_SC_PS_SHIFT;
  return (float)CLOCK_GetFreq(kCLOCK_McgIrc48MClk) / (float)(1u << ps) / (float)(tpm->MOD + 1u);
}

float tpmDuty(TPM_Type *tpm, int ch) {
  return (float)tpm->CONTROLS[ch].CnV / (float)(tpm->MOD + 1u);
}

float tpmPulseUs(TPM_Type *tpm, int ch) {
  uint32_t ps = (tpm->SC & TPM_SC_PS_MASK) >> TPM_SC_PS_SHIFT;
  return (float)tpm->CONTROLS[ch].CnV * (float)(1u << ps) * 1.0e6f / (float)CLOCK_GetFreq(kCLOCK_McgIrc48MClk);
}

bool tpmRunning(TPM_Type *tpm) {
  return (tpm->SC & TPM_SC_CMOD_MASK) != 0;
}

void printPwm(const char *name, TPM_Type *tpm, int ch, const Waveform &w) {
  Serial.print(name);
  Serial.print(": registers ");
  Serial.print(tpmFrequency(tpm), 2);
  Serial.print(" Hz, duty ");
  Serial.print(tpmDuty(tpm, ch), 4);
  Serial.print("; pin duty ");
  Serial.print(w.duty(), 4);
  Serial.print(", ");
  Serial.print(w.rises);
  Serial.println(" rising edges");
}

// ---- ADC16 internal channels (reference manual, ADCx_SC1n[ADCH]) ----

uint16_t adcChannel(uint32_t channel) {
  adc16_channel_config_t ch = {};
  ch.channelNumber = channel;
  ADC16_SetChannelConfig(ADC0, 0u, &ch);
  while (!(ADC16_GetChannelStatusFlags(ADC0, 0u) & kADC16_ChannelConversionDoneFlag))
    ;
  return (uint16_t)ADC16_GetChannelConversionValue(ADC0, 0u);
}

// ---- interrupts ----

volatile int isrCount = 0;
void countIsr() {
  isrCount = isrCount + 1;
}

// Makes edges on a pin with nothing wired to it by switching its pull
int edgesFromPulls(const RawPin &p, int cycles) {
  setPull(p, false);
  delayMicroseconds(200);
  isrCount = 0;
  for (int i = 0; i < cycles; i++) {
    setPull(p, true);
    delayMicroseconds(200);
    setPull(p, false);
    delayMicroseconds(200);
  }
  return isrCount;
}

void checkInterrupts(const char *name, int pin, const RawPin &p) {
  const int CYCLES = 10;
  struct {
    int mode;
    const char *label;
    int expect;
  } modes[] = {
    { CHANGE, "CHANGE", 2 * CYCLES },
    { RISING, "RISING", CYCLES },
    { FALLING, "FALLING", CYCLES },
  };

  for (auto &m : modes) {
    pinMode(pin, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(pin), countIsr, m.mode);
    int n = edgesFromPulls(p, CYCLES);
    detachInterrupt(digitalPinToInterrupt(pin));

    char label[64];
    snprintf(label, sizeof(label), "%s %s: %d interrupts (expect %d)", name, m.label, n, m.expect);
    check(label, n == m.expect);
  }
}

// Marks a BootRecord this sketch wrote
const uint32_t BOOT_MAGIC = 0xC4440001u;

Servo servo;

void setup() {
  // ---- red LED, before anything else ----
  pinMode(RED, OUTPUT);
  for (int i = 0; i < 3; i++) {
    digitalWrite(RED, LOW);  // the LEDs are on when LOW
    delay(150);
    digitalWrite(RED, HIGH);
    delay(150);
  }

  Serial.begin(115200);
  while (!Serial)
    ;
  delay(100);

  Serial.println();
  Serial.println("=== FRDM-MCXC444 bring-up ===");

  // ---- clocks ----
  section("clocks");
  {
    // The SDK's BOARD_BootClockRUN: the 48MHz IRC48M (HIRC) as the core
    // clock, the bus clock at half that. The PIT (tone) counts the bus
    // clock, both I2Cs the system clock, the TPMs IRC48M.
    struct {
      const char *name;
      uint32_t hz;
      uint32_t expect;
    } clocks[] = {
      { "SystemCoreClock", SystemCoreClock, 48000000u },
      { "core/system    ", CLOCK_GetFreq(kCLOCK_CoreSysClk), 48000000u },
      { "bus            ", CLOCK_GetFreq(kCLOCK_BusClk), 24000000u },
      { "IRC48M (TPM)   ", CLOCK_GetFreq(kCLOCK_McgIrc48MClk), 48000000u },
    };
    for (auto &c : clocks) {
      char label[64];
      snprintf(label, sizeof(label), "%s %lu Hz (expect %lu)", c.name, (unsigned long)c.hz, (unsigned long)c.expect);
      check(label, c.hz == c.expect);
    }
  }

  // ---- millis() and micros(), which reads SysTick on Cortex-M0+ ----
  section("millis / micros / delayMicroseconds");
  {
    unsigned long m0 = millis();
    unsigned long u0 = micros();
    delay(200);
    unsigned long dm = millis() - m0;
    unsigned long du = micros() - u0;
    Serial.print("delay(200): millis ");
    Serial.print(dm);
    Serial.print(", micros ");
    Serial.println(du);
    check("millis() advances ~200ms", dm >= 199 && dm <= 202);
    check("micros() advances ~200000us", du >= 199000 && du <= 202000);

    // micros() combines the millisecond count with SysTick's down-counter.
    // Read it back to back across many SysTick reloads: it must never go
    // backwards
    unsigned long prev = micros();
    unsigned long backwards = 0;
    unsigned long start = prev;
    while (micros() - start < 50000) {
      unsigned long now = micros();
      if ((long)(now - prev) < 0)
        backwards++;
      prev = now;
    }
    Serial.print("micros() went backwards ");
    Serial.print(backwards);
    Serial.println(" times in 50ms");
    check("micros() never goes backwards", backwards == 0);

    for (unsigned long target = 10; target <= 10000; target *= 10) {
      unsigned long t0 = micros();
      delayMicroseconds(target);
      unsigned long measured = micros() - t0;
      char label[64];
      snprintf(label, sizeof(label), "delayMicroseconds(%lu): %lu us", target, measured);
      check(label, measured >= target && measured <= target + target / 5 + 30);
    }
  }

  // ---- every header pin's pull-up and pull-down ----
  // Driven to the opposite level first, then released, so a floating pin
  // that keeps its last level fails rather than passing by chance. Pins
  // with on-board parts are printed but not judged: the part may hold the
  // level (D4: SW3, D6/D13: LEDs, D11: the accelerometer's CS, D18/D19 and
  // A4/A5: I2C buses). D0/D1 are Serial.
  section("pull-up / pull-down on every header pin");
  {
    struct {
      const char *name;
      int pin;
      bool judged;
    } pins[] = {
      { "D2", D2, true },
      { "D3", D3, true },
      { "D4", D4, false },
      { "D5", D5, true },
      { "D6", D6, false },
      { "D7", D7, true },
      { "D8", D8, true },
      { "D9", D9, true },
      { "D10", D10, true },
      { "D11", D11, false },
      { "D12", D12, true },
      { "D13", D13, false },
      { "D18", D18, false },
      { "D19", D19, false },
      { "A0", A0, true },
      { "A1", A1, true },
      { "A2", A2, true },
      { "A3", A3, true },
      { "A4", A4, false },
      { "A5", A5, false },
    };
    for (auto &p : pins) {
      pinMode(p.pin, OUTPUT);
      digitalWrite(p.pin, LOW);
      pinMode(p.pin, INPUT_PULLUP);
      delayMicroseconds(100);
      int up = digitalRead(p.pin);

      pinMode(p.pin, OUTPUT);
      digitalWrite(p.pin, HIGH);
      pinMode(p.pin, INPUT_PULLDOWN);
      delayMicroseconds(100);
      int down = digitalRead(p.pin);

      char label[64];
      snprintf(label, sizeof(label), "%-3s pull-up reads %d, pull-down reads %d", p.name, up, down);
      if (p.judged)
        check(label, up == HIGH && down == LOW);
      else {
        Serial.print(label);
        Serial.println(" (not judged: on-board parts)");
      }
      pinMode(p.pin, INPUT);
    }
  }

  // ---- pin interrupts: PORTA has its own vector, PORTC and PORTD share
  //      one. Edges come from switching the pin's own pull ----
  section("attachInterrupt");
  {
    checkInterrupts("D8 (PTA13, PORTA)", D8, PIN_D8);
    checkInterrupts("D2 (PTD3, PORTC/D)", D2, PIN_D2);
  }

  // ---- analogRead(), and the ADC against its own references ----
  section("analogRead");
  {
    const int pins[] = { A0, A1, A2, A3, A4, A5 };
    bool inRange = true;
    for (int i = 0; i < 6; i++) {
      int v = analogRead(pins[i]);  // floating: any value
      Serial.print("A");
      Serial.print(i);
      Serial.print(" = ");
      Serial.println(v);
      inRange = inRange && v >= 0 && v <= 1023;
    }
    check("analogRead(A0)-(A5) in 0-1023", inRange);

    // analogRead() has calibrated ADC0 and set it to 16 bits. VREFSH and
    // VREFSL are the ends of the scale; the bandgap is 0.97-1.03V (data
    // sheet), which gives VREFH (VDDA on this board)
    uint16_t high = adcChannel(29);  // VREFSH
    uint16_t low = adcChannel(30);   // VREFSL
    PMC->REGSC |= PMC_REGSC_BGBE_MASK;
    delay(1);
    uint16_t bandgap = adcChannel(27);
    uint16_t temp = adcChannel(26);
    float vrefh = bandgap ? 1.0f * 65535.0f / (float)bandgap : 0.0f;
    Serial.print("VREFSH ");
    Serial.print(high);
    Serial.print(", VREFSL ");
    Serial.print(low);
    Serial.print(", bandgap ");
    Serial.print(bandgap);
    Serial.print(" (VREFH ~ ");
    Serial.print(vrefh, 3);
    Serial.print(" V), temperature sensor ");
    Serial.println(temp);
    check("VREFSH reads full scale (>= 65000)", high >= 65000);
    check("VREFSL reads zero (<= 300)", low <= 300);
    check("the bandgap puts VREFH at 3.0-3.6V", vrefh >= 3.0f && vrefh <= 3.6f);
  }

  // ---- analogWrite() on the TPMs ----
  section("analogWrite / analogWriteFrequency");
  {
    analogWrite(D3, 128);
    Waveform w = sample(PIN_D3, 20000);
    printPwm("D3 128", TPM1, 0, w);
    check("D3 is muxed to TPM1_CH0 (ALT3), TPM1 running", mux(PIN_D3) == 3 && tpmRunning(TPM1));
    check("D3 registers: 1kHz, duty 128/255", within(tpmFrequency(TPM1), 1000.0f, 0.001f) && within(tpmDuty(TPM1, 0), 128.0f / 255.0f, 0.01f));
    check("D3 pin: duty ~0.50, ~20 periods in 20ms", within(w.duty(), 128.0f / 255.0f, 0.05f) && w.rises >= 19 && w.rises <= 21);

    // The ends: 0% is CnV = 0, 100% is CnV > MOD (reference manual, EPWM)
    analogWrite(D3, 0);
    delay(2);
    w = sample(PIN_D3, 5000);
    check("D3 0: always LOW", w.highs == 0);
    analogWrite(D3, 255);
    delay(2);
    w = sample(PIN_D3, 5000);
    check("D3 255: always HIGH", w.highs == w.samples);

    analogWriteFrequency(D3, 50);
    analogWrite(D3, 64);
    delay(25);
    w = sample(PIN_D3, 100000);
    printPwm("D3 50Hz 64", TPM1, 0, w);
    check("D3 registers: 50Hz, duty 64/255", within(tpmFrequency(TPM1), 50.0f, 0.001f) && within(tpmDuty(TPM1, 0), 64.0f / 255.0f, 0.01f));
    check("D3 pin: duty ~0.25, 5 periods in 100ms", within(w.duty(), 64.0f / 255.0f, 0.05f) && w.rises >= 4 && w.rises <= 6);
    analogWrite(D3, 255);  // 100% at the longest periods too
    delay(25);
    w = sample(PIN_D3, 25000);
    check("D3 255 at 50Hz: always HIGH", w.highs == w.samples);
    analogWriteFrequency(D3, 1000);

    // D10 is TPM0_CH4. D5, D6 and D9 all reach TPM0_CH2: the one given
    // analogWrite() last has it, and the one before goes to GPIO, LOW
    analogWrite(D10, 192);
    w = sample(PIN_D10, 20000);
    printPwm("D10 192", TPM0, 4, w);
    check("D10 is muxed to TPM0_CH4 (ALT4)", mux(PIN_D10) == 4 && tpmRunning(TPM0));
    check("D10 pin: duty ~0.75", within(w.duty(), 192.0f / 255.0f, 0.05f));

    analogWrite(D5, 64);
    check("D5 takes TPM0_CH2 (ALT3)", mux(PIN_D5) == 3);
    analogWrite(D6, 64);  // the blue LED
    check("D6 takes TPM0_CH2 (ALT3), D5 goes to GPIO LOW", mux(PIN_D6) == 3 && drivenLow(PIN_D5));
    analogWrite(D9, 64);
    check("D9 takes TPM0_CH2 (ALT4), D6 goes to GPIO LOW", mux(PIN_D9) == 4 && drivenLow(PIN_D6));
    w = sample(PIN_D9, 20000);
    printPwm("D9 64", TPM0, 2, w);
    check("D9 pin: duty ~0.25", within(w.duty(), 64.0f / 255.0f, 0.05f));

    // One period for the whole TPM
    analogWriteFrequency(D9, 2000);
    check("analogWriteFrequency(D9, 2000) moves D10 too (TPM0)", within(tpmFrequency(TPM0), 2000.0f, 0.001f));
    check("D3 (TPM1) stays at 1kHz", within(tpmFrequency(TPM1), 1000.0f, 0.001f));
    analogWriteFrequency(D9, 1000);

    analogWrite(D3, 0);
    analogWrite(D9, 0);
    analogWrite(D10, 0);
    digitalWrite(D6, HIGH);  // blue LED off
  }

  // ---- tone() on the PIT ----
  section("tone");
  {
    tone(D8, 1000);
    Waveform w = sample(PIN_D8, 100000);
    Serial.print("tone(D8, 1000): ");
    Serial.print(w.rises);
    Serial.print(" rising edges in 100ms, duty ");
    Serial.println(w.duty(), 3);
    check("tone(D8, 1000): 100 periods in 100ms, duty ~0.5", w.rises >= 99 && w.rises <= 101 && within(w.duty(), 0.5f, 0.05f));
    noTone(D8);

    tone(D8, 2000, 50);
    delay(10);
    w = sample(PIN_D8, 20000);
    check("tone(D8, 2000, 50) runs", w.rises >= 39 && w.rises <= 41);
    delay(40);
    w = sample(PIN_D8, 20000);
    check("... and stops after 50ms", w.rises == 0);
  }

  // ---- Wire1: the on-board accelerometer (FXLS8974CF, I2C0) ----
  // Before SPI: D11 (SPI MOSI) is also the accelerometer's CS/WAKE
  section("Wire1 (on-board accelerometer)");
  {
    const uint8_t ADDR = 0x18;
    const uint8_t WHO_AM_I = 0x13;
    const uint8_t SENS_CONFIG1 = 0x15;  // bit 0 ACTIVE; +/-2g
    const uint8_t OUT_X_LSB = 0x04;     // X, Y, Z, each LSB then MSB, 12-bit

    Wire1.begin();
    Wire1.beginTransmission(ADDR);
    Wire1.write(WHO_AM_I);
    uint8_t err = Wire1.endTransmission(false);
    uint8_t n = Wire1.requestFrom(ADDR, (uint8_t)1);
    int id = Wire1.read();
    Serial.print("WHO_AM_I = 0x");
    Serial.println(id, HEX);
    check("WHO_AM_I is FXLS8974CF's (0x86)", err == 0 && n == 1 && id == 0x86);

    Wire1.beginTransmission(ADDR);
    Wire1.write(SENS_CONFIG1);
    Wire1.write(0x01);
    err = Wire1.endTransmission();
    delay(50);

    Wire1.beginTransmission(ADDR);
    Wire1.write(OUT_X_LSB);
    Wire1.endTransmission(false);
    n = Wire1.requestFrom(ADDR, (uint8_t)6);
    float mg[3] = { 0.0f, 0.0f, 0.0f };
    for (int i = 0; i < 3 && n == 6; i++) {
      uint8_t lsb = Wire1.read();
      uint8_t msb = Wire1.read();
      int16_t raw = (int16_t)((msb << 12) | (lsb << 4)) >> 4;
      mg[i] = raw * 0.98f;
    }
    float g = sqrtf(mg[0] * mg[0] + mg[1] * mg[1] + mg[2] * mg[2]) / 1000.0f;
    Serial.print("x, y, z = ");
    Serial.print(mg[0], 0);
    Serial.print(", ");
    Serial.print(mg[1], 0);
    Serial.print(", ");
    Serial.print(mg[2], 0);
    Serial.print(" mg, |a| = ");
    Serial.print(g, 3);
    Serial.println(" g");
    check("activated, and the 6 output bytes read", err == 0 && n == 6);
    check("lying still, it measures about 1g", g > 0.8f && g < 1.2f);

    // SCL rate: the same 6-byte read at 100kHz and 400kHz. The difference
    // is 7 bytes of 9 bits (address + 6 data) times 7.5us, about 470us,
    // whatever the driver's own overhead
    uint32_t t[2];
    const uint32_t rates[2] = { 100000u, 400000u };
    for (int i = 0; i < 2; i++) {
      Wire1.setClock(rates[i]);
      uint32_t best = 0xFFFFFFFFu;
      for (int k = 0; k < 5; k++) {
        uint32_t t0 = micros();
        Wire1.requestFrom(ADDR, (uint8_t)6);
        uint32_t d = micros() - t0;
        while (Wire1.available())
          Wire1.read();
        if (d < best)
          best = d;
      }
      t[i] = best;
    }
    Wire1.setClock(100000u);
    long diff = (long)t[0] - (long)t[1];
    Serial.print("6-byte read: ");
    Serial.print(t[0]);
    Serial.print(" us at 100kHz, ");
    Serial.print(t[1]);
    Serial.print(" us at 400kHz, difference ");
    Serial.print(diff);
    Serial.println(" us (expect ~470)");
    check("Wire1 SCL rates (difference 400-560us)", diff >= 400 && diff <= 560);

    Wire1.beginTransmission(ADDR);
    Wire1.write(SENS_CONFIG1);
    Wire1.write(0x00);  // back to standby
    Wire1.endTransmission();
  }

  // ---- SPI (SPI1, D10-D13) ----
  section("SPI");
  {
    SPI.begin();
    uint8_t buf[64];

    // SCK rate: 64 bytes at 1MHz and at 250kHz differ by 512 bits x 3us
    uint32_t t[2];
    const uint32_t rates[2] = { 1000000u, 250000u };
    for (int i = 0; i < 2; i++) {
      SPI.beginTransaction(SPISettings(rates[i], MSBFIRST, SPI_MODE0));
      for (int k = 0; k < 64; k++)
        buf[k] = (uint8_t)k;
      uint32_t t0 = micros();
      SPI.transfer(buf, sizeof(buf));
      t[i] = micros() - t0;
      SPI.endTransaction();
    }
    long diff = (long)t[1] - (long)t[0];
    Serial.print("64 bytes: ");
    Serial.print(t[0]);
    Serial.print(" us at 1MHz, ");
    Serial.print(t[1]);
    Serial.print(" us at 250kHz, difference ");
    Serial.print(diff);
    Serial.println(" us (expect 1536)");
    check("SPI SCK rates (difference 1536us +/-10%)", within((float)diff, 1536.0f, 0.10f));

    // Loopback, if D11-D12 is jumpered
    SPI.beginTransaction(SPISettings(1000000u, MSBFIRST, SPI_MODE0));
    const uint8_t patterns[] = { 0x55, 0xAA, 0x00, 0xFF, 0x3C, 0x81 };
    int match = 0;
    for (uint8_t p : patterns)
      if (SPI.transfer(p) == p)
        match++;
    uint16_t w16 = SPI.transfer16(0x1234);
    SPI.endTransaction();
    SPI.end();
    if (match == 0)
      Serial.println("SPI loopback: SKIP (no D11-D12 jumper)");
    else
      check("SPI loopback D11-D12: 6 bytes and transfer16()", match == 6 && w16 == 0x1234);
  }

  // ---- EEPROM: the last 8KB of flash (FTFA) ----
  section("EEPROM");
  {
    const int at = EEPROM.length() - (int)sizeof(BootRecord);
    BootRecord r;
    EEPROM.get(at, r);
    if (r.magic != BOOT_MAGIC)
      r = { BOOT_MAGIC, 0 };
    r.count++;
    EEPROM.put(at, r);
    BootRecord back;
    EEPROM.get(at, back);
    Serial.print("EEPROM.length() = ");
    Serial.print(EEPROM.length());
    Serial.print(", start count = ");
    Serial.print(back.count);
    Serial.println(" (press RESET: it goes up by one)");
    check("EEPROM.length() is 1024", EEPROM.length() == 1024);
    check("EEPROM put()/get() read back", back.magic == BOOT_MAGIC && back.count == r.count);
  }

  // ---- Servo on D3 (TPM1): 50Hz, pulse in microseconds ----
  section("Servo");
  {
    servo.attach(D3);
    servo.writeMicroseconds(1500);
    delay(25);
    Waveform w = sample(PIN_D3, 100000);
    Serial.print("Servo D3 1500us: registers ");
    Serial.print(tpmFrequency(TPM1), 2);
    Serial.print(" Hz, ");
    Serial.print(tpmPulseUs(TPM1, 0), 1);
    Serial.print(" us; pin ");
    Serial.print(w.rises);
    Serial.println(" rising edges in 100ms");
    check("Servo: 50Hz, 1500us", within(tpmFrequency(TPM1), 50.0f, 0.001f) && within(tpmPulseUs(TPM1, 0), 1500.0f, 0.005f));
    check("Servo pin: 5 pulses in 100ms, duty ~0.075", w.rises >= 4 && w.rises <= 6 && within(w.duty(), 0.075f, 0.1f));
    servo.detach();
  }

  // ---- Wire (D18/D19, I2C1), last: with nothing on the bus, a missing
  //      pull-up could stop it here ----
  section("Wire (D18/D19) bus scan -- if the board stops here, check the bus pull-ups");
  {
    Wire.begin();
    int found = 0;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
      Wire.beginTransmission(addr);
      if (Wire.endTransmission() == 0) {
        Serial.print("found a device at 0x");
        Serial.println(addr, HEX);
        found++;
      }
    }
    Serial.print(found);
    Serial.println(" device(s) found (0 is fine: nothing needs to be plugged in)");
    check("Wire bus scan completed", true);
  }

  Serial.println();
  if (failCount == 0)
    Serial.println("ALL OK");
  else {
    Serial.print(failCount);
    Serial.println(" FAILED");
  }
  Serial.println("Fading the blue LED (D6, PWM).");
}

void loop() {
  // The LED is on when LOW: 255 is off
  for (int v = 255; v >= 0; v -= 5) {
    analogWrite(D6, v);
    delay(20);
  }
  for (int v = 0; v <= 255; v += 5) {
    analogWrite(D6, v);
    delay(20);
  }
}
