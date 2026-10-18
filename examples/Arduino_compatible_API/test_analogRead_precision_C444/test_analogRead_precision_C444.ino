/** analogRead() precision test for FRDM-MCXC444, using known voltages
 *
 *  Feed a known voltage (0 - 3.3V) into any of A0-A5, and compare the
 *  computed voltage below against it: a regulated source, or the board's
 *  own 3.3V and GND. Measure the 3.3V pin with a meter and put the reading
 *  in VDDA_V: the ADC's reference is VDDA (VALTH), the board's 3.3V, since
 *  the board leaves the VREFH pin open (JP6).
 *
 *  Each line averages SAMPLES readings at 16 bits and gives their min and
 *  max, so a driven channel can be told from a floating one by its noise.
 *
 *  Three of the ADC's internal inputs are read too, needing no wiring:
 *    VREFL (channel 30): 0
 *    VALTH (channel 29): full scale, 65535
 *    bandgap (channel 27): VBG, 1.00V (0.97 - 1.03V in the data sheet).
 *      VDDA = VBG * 65535 / reading, so this gives VDDA to about 3%
 */

#include <Arduino.h>

#if !defined(FRDM_MCXC444)
#error "This sketch is for FRDM-MCXC444"
#endif

const float VDDA_V  = 3.3f;     // the board's 3.3V, as measured
const float VBG     = 1.00f;    // the data sheet's typical bandgap voltage
const int   SAMPLES = 64;

struct stats {
  float mean;
  long  min;
  long  max;
};

//  The ADC is set up by the first analogRead(); this starts a conversion
//  on an internal channel the same way the core does, keeping its
//  hardware averaging
long read_internal(uint8_t channel) {
  ADC0->SC1[0] = ADC_SC1_ADCH(channel);
  while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK))
    ;
  return ADC0->R[0];
}

stats sample(int pin, int channel) {
  stats s = { 0.0f, 65535, 0 };
  long  sum = 0;

  for (int i = 0; i < SAMPLES; i++) {
    long v = (pin >= 0) ? analogRead(pin) : read_internal(channel);
    sum += v;
    if (v < s.min) s.min = v;
    if (v > s.max) s.max = v;
  }
  s.mean = (float)sum / SAMPLES;
  return s;
}

void print_stats(const char *name, stats s, float full_scale_volts) {
  Serial.print(name);
  Serial.print(" mean=");
  Serial.print(s.mean, 1);
  Serial.print(" min=");
  Serial.print(s.min);
  Serial.print(" max=");
  Serial.print(s.max);
  Serial.print(" volts=");
  Serial.println(s.mean * full_scale_volts / 65535.0f, 4);
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  analogReadResolution(16);
  (void)analogRead(A0);         // sets up and calibrates the ADC

  PMC->REGSC |= PMC_REGSC_BGBE_MASK;    // the bandgap buffer, for channel 27
  delay(10);

  Serial.println("analogRead precision test -- feed known voltages into A0-A5");
}

void loop() {
  static const struct {
    const char *name;
    int         pin;
  } pins[] = {
    { "A0", A0 }, { "A1", A1 }, { "A2", A2 },
    { "A3", A3 }, { "A4", A4 }, { "A5", A5 },
  };

  for (auto &p : pins)
    print_stats(p.name, sample(p.pin, 0), VDDA_V);

  stats bg = sample(-1, 27);
  print_stats("VREFL  ", sample(-1, 30), VDDA_V);
  print_stats("VALTH  ", sample(-1, 29), VDDA_V);
  print_stats("bandgap", bg, VDDA_V);
  Serial.print("VDDA from the bandgap = ");
  Serial.print(VBG * 65535.0f / bg.mean, 3);
  Serial.println("V");
  Serial.println("---");

  delay(1000);
}
