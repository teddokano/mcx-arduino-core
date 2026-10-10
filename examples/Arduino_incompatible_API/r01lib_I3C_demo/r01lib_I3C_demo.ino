/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * P3T1755 (temperature sensor) I3C demo: DAA, CCC and IBI
 *
 * NXP's r01lib demo "P3T1755_FRDM_MCX_demo_DAA", converted for this core.
 * Like r01lib_I3C, it uses r01lib's I3C class directly, and the P3T1755
 * and LM75B classes of r01lib's device drivers (TempSensor.h/.cpp and
 * I2C_device.h/.cpp in this sketch's folder, from r01lib's r01device), which
 * take an r01lib I2C or I3C object.
 *
 *  - DAA (Dynamic Address Assignment) gives each I3C target on the bus a
 *    dynamic address from dynamic_address_list[], and prints what it found
 *  - Each target gets T_LOW/T_HIGH 1 and 2 degrees above its first reading,
 *    its ALERT in interrupt mode, and IBI (In-Band Interrupt) enabled with
 *    the ENEC CCC. Warm the sensor with a finger and it sends an IBI
 *  - About once a second it prints the temperatures, and the address of
 *    the target that sent an IBI, if one came
 *  - An LM75B-compatible I2C sensor at 0x4F on the same bus, if there is
 *    one, is read too, with the bus switched to I2C mode for it
 *  - The RGB LED blinks blue, green when the first target is more than
 *    1 degree over its first reading, red when more than 2 degrees over
 *  - D2 goes LOW when an IBI comes in and back HIGH at the next reading,
 *    as a trigger for an oscilloscope or a logic analyzer
 *
 * The bus is I3C_SDA/I3C_SCL. The on-board P3T1755 (static address 0x48)
 * is on it on FRDM-MCXA153, FRDM-MCXN947 and FRDM-MCXA156 (D18/D19 there).
 * FRDM-MCXN236 has no I3C sensor on board: connect one to D18/D19.
 *
 * The sensor keeps the dynamic address DAA gave it for as long as it has
 * power: a reset or an upload doesn't clear it. A sketch that reads it at
 * its static address 0x48 afterwards gets no answer until the board is
 * unplugged and plugged back in. On FRDM-MCXA156, release_check/01 failed
 * its sensor checks this way (its Wire reads the sensor at 0x48). This
 * demo itself starts by resetting the dynamic addresses, so running it
 * again is fine.
 *
 * Changes from the original:
 *  - setup()/loop(), and Serial in place of PRINTF
 *  - The LEDs and D2 go through pinMode()/digitalWrite(): with <Arduino.h>
 *    included, RED/GREEN/BLUE/D2 are Arduino's pin numbers, which r01lib's
 *    DigitalOut does not take
 *  - Each target is addressed by the dynamic address DAA reports for it
 *  - The LED follows the first target's temperature. The original passed
 *    the LED a temperature that was never updated, so it stayed blue
 */

#include <Arduino.h>
#include "P3T1755.h"

I3C i3c(I3C_SDA, I3C_SCL);  //	SDA, SCL

const uint8_t static_address_list[] = { 0x48, 0x4A, 0x4C };
const uint8_t dynamic_address_list[] = { 0x1A, 0x2B, 0x3C };
const int MAX_TARGETS = sizeof(dynamic_address_list);

P3T1755 sensor[] = {
  P3T1755(i3c, static_address_list[0]),
  P3T1755(i3c, static_address_list[1]),
  P3T1755(i3c, static_address_list[2])
};

LM75B lm75b(i3c, 0x4F);  //	I2C target

const uint8_t DCR_TEMPERATURE_SENSOR = 0x63;

const unsigned long WAIT_MS = 960;

const int TRIGGER = D2;  //	IBI detection trigger output for oscilloscope
const int TRIGGER_POLARITY = LOW;

Ticker ticker;  //	blinks the LED, every 10ms

int ndev = 0;
float ref_temp[MAX_TARGETS];
bool i2c_device = false;

volatile int led_target = BLUE;

int DAA(const uint8_t *address_list, uint8_t list_length);
bool check_i2c_device(LM75B &ts);
void info(LM75B &ts);
void led_control_callback(void);
void led_set_color(float temp, float ref);
void ibi_trigger_output(void);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(TRIGGER, OUTPUT);
  digitalWrite(TRIGGER, !TRIGGER_POLARITY);

  ticker.attach(led_control_callback, 0.01);
  i3c.set_IBI_callback(ibi_trigger_output);

  Serial.printf("\r\nP3T1755 (Temperature sensor) I3C operation sample: getting temperature data and IBI\r\n");

  ndev = DAA(dynamic_address_list, sizeof(dynamic_address_list));

  for (int i = 0; i < ndev; i++) {
    ref_temp[i] = sensor[i];
    float low = ref_temp[i] + 1.0;
    float high = ref_temp[i] + 2.0;

    sensor[i].thresholds(low, high);

    sensor[i].bit_op8(P3T1755::Conf, ~0x02, 0x02);  //	ALERT pin configured to INT mode
    sensor[i].ccc_set(CCC::DIRECT_ENEC, 0x01);       //	Enable IBI

    info(sensor[i]);
  }

  i2c_device = check_i2c_device(lm75b);
}

void loop() {
  float temp = 0;
  uint8_t ibi_addr;

  if ((ibi_addr = i3c.check_IBI()))
    Serial.printf("\r\n*** IBI : Got IBI from target_address: 0x%02X", ibi_addr);

  Serial.printf("\r\n");

  for (int i = 0; i < ndev; i++) {
    float t = sensor[i];

    if (i == 0)
      temp = t;

    Serial.printf("  %8.4f°C @%02X", t, sensor[i].address());
  }

  if (i2c_device) {
    i3c.mode(I3C::I2C_MODE);
    Serial.printf("  %7.3f°C @%02X(I2C)", (float)lm75b, lm75b.address());
    i3c.mode(I3C::I3C_MODE);
  }

  led_set_color(temp, ref_temp[0]);
  delay(WAIT_MS);
}

//	Runs DAA, prints what each target reports, and points sensor[] at the
//	dynamic addresses the targets got
int DAA(const uint8_t *address_list, uint8_t list_length) {
  i3c_device_info_t *list_p;
  int n_devices;

  i3c.ccc_broadcast(CCC::BROADCAST_RSTDAA, NULL, 0);  //	Reset DAA
  n_devices = i3c.DAA(address_list, list_length, &list_p);

  Serial.printf("\r\n=== DAA result: total %d target(s) found\r\n", n_devices);
  for (int i = 0; i < n_devices; i++) {
    Serial.printf("Target %d\r\n", i);
    Serial.printf("  dynamicAddr    = 0x%02X\r\n", list_p[i].dynamicAddr);
    Serial.printf("  staticAddr     = 0x%02X\r\n", list_p[i].staticAddr);
    Serial.printf("  dcr            = 0x%02X\r\n", list_p[i].dcr);
    Serial.printf("  bcr            = 0x%02X\r\n", list_p[i].bcr);
    Serial.printf("  vendorID       = 0x%04X\r\n", list_p[i].vendorID);
    Serial.printf("  partNumber     = 0x%08lX\r\n", (unsigned long)list_p[i].partNumber);
    Serial.printf("  maxReadLength  = 0x%04X\r\n", list_p[i].maxReadLength);
    Serial.printf("  maxWriteLength = 0x%04X\r\n", list_p[i].maxWriteLength);
    Serial.printf("  hdrMode        = 0x%02X\r\n", list_p[i].hdrMode);
    Serial.printf("\r\n");
  }

  if (n_devices > MAX_TARGETS)
    n_devices = MAX_TARGETS;

  for (int i = 0; i < n_devices; i++)
    sensor[i].address_overwrite(list_p[i].dynamicAddr);

  return n_devices;
}

bool check_i2c_device(LM75B &ts) {
  uint8_t dummy = 0;

  i3c.mode(I3C::I2C_MODE);
  int r = i3c.write(ts.address(), &dummy, 1);
  i3c.mode(I3C::I3C_MODE);

  return !r;
}

void info(LM75B &ts) {
  uint8_t pid[I3C::PID_LENGTH];
  uint8_t bcr, dcr;

  uint8_t a = ts.address();
  uint16_t t = ts.read_r16(P3T1755::Temp);
  uint8_t c = ts.read_r8(P3T1755::Conf);
  uint16_t l = ts.read_r16(P3T1755::T_LOW);
  uint16_t h = ts.read_r16(P3T1755::T_HIGH);

  ts.ccc_get(CCC::DIRECT_GETPID, pid, sizeof(pid));
  ts.ccc_get(CCC::DIRECT_GETBCR, &bcr, 1);
  ts.ccc_get(CCC::DIRECT_GETDCR, &dcr, 1);

  Serial.printf("\r\nRegister dump - I3C target address:7'h%02X (0x%02X)\r\n", a, a << 1);
  Serial.printf("  - Temp   (0x0): 0x%04X (%8.4f°C)\r\n", t, (int16_t)t / 256.0);
  Serial.printf("  - Conf   (0x1): 0x  %02X\r\n", c);
  Serial.printf("  - T_LOW  (0x2): 0x%04X (%8.4f°C)\r\n", l, (int16_t)l / 256.0);
  Serial.printf("  - T_HIGH (0x3): 0x%04X (%8.4f°C)\r\n", h, (int16_t)h / 256.0);

  Serial.printf("  * PID    (CCC:Provisioned ID)                 : 0x");
  for (int i = 0; i < I3C::PID_LENGTH; i++)
    Serial.printf(" %02X", pid[i]);
  Serial.printf("\r\n");
  Serial.printf("  * BCR    (CCC:Bus Characteristics Register)   : 0x%02X\r\n", bcr);
  Serial.printf("  * DCR    (CCC:Device Characteristics Register): 0x%02X (= %s)\r\n", dcr,
                (DCR_TEMPERATURE_SENSOR == dcr) ? "Temperature sensor" : "Unknown");

  Serial.printf("\r\n");
}

//	Called by the Ticker every 10ms: the LED is off for 270ms, then on for
//	50ms in the color led_set_color() chose
void led_control_callback(void) {
  static int count = 0;
  const int k = 32;

  if ((count % k) < (k - 5)) {
    digitalWrite(RED, PIN_LED_OFF);
    digitalWrite(GREEN, PIN_LED_OFF);
    digitalWrite(BLUE, PIN_LED_OFF);
  } else {
    digitalWrite(led_target, PIN_LED_ON);
  }

  count++;
}

void led_set_color(float temp, float ref) {
  if ((ref + 2) < temp)
    led_target = RED;
  else if ((ref + 1) < temp)
    led_target = GREEN;
  else
    led_target = BLUE;

  digitalWrite(TRIGGER, !TRIGGER_POLARITY);
}

//	Called from the I3C interrupt when an IBI comes in
void ibi_trigger_output(void) {
  digitalWrite(TRIGGER, TRIGGER_POLARITY);
}
