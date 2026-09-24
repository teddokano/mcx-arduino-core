/** Serial.begin(baud, config), Serial.end() and serialEvent1(), checked
 *  on Serial1 with its TX looped back to RX.
 *
 *  Wiring: the same Serial1 loopback jumper as release_check/11 -- D0-D1
 *  on FRDM-MCXA153, MikroBus MB_TX-MB_RX on FRDM-MCXN947.
 *
 *  A loopback alone can't show that a format is right: the receiver is
 *  the same LPUART as the transmitter, so it expects whatever the
 *  transmitter sends. So each frame is also read bit by bit off the RX
 *  pin, through the GPIO input register, which follows the pin while the
 *  UART owns it. At 1200 baud a bit lasts 833us, plenty for micros().
 *  That checks the data bits, the parity bit against even/odd, and the
 *  stop bits: two bytes are sent back to back, and the second one's start
 *  bit has to come exactly where the first one's stop bits end.
 *
 *  Dropping a byte with bad parity is checked by taking the TX pin back
 *  as a GPIO and sending frames by hand, one with the wrong parity bit
 *  and one with the right one. Only the right one should arrive.
 *
 *  Automatic: reads "ALL OK" or "N FAILED" at the end.
 */

#include <Arduino.h>
#include "pin_registry.h"

#if defined(FRDM_MCXN947)
const int TX_PIN = MB_TX;
const int RX_PIN = MB_RX;
#else
const int TX_PIN = D1;
const int RX_PIN = D0;
#endif

const unsigned long BAUD = 1200;
const unsigned long BIT_US = 1000000UL / BAUD;

struct Format {
  const char *name;
  uint16_t config;
  int bits;    // data bits
  int parity;  // 0 none, 1 even, 2 odd
  int stop;
};

const Format formats[] = {
  { "7N1", SERIAL_7N1, 7, 0, 1 }, { "8N1", SERIAL_8N1, 8, 0, 1 },
  { "7N2", SERIAL_7N2, 7, 0, 2 }, { "8N2", SERIAL_8N2, 8, 0, 2 },
  { "7E1", SERIAL_7E1, 7, 1, 1 }, { "8E1", SERIAL_8E1, 8, 1, 1 },
  { "7E2", SERIAL_7E2, 7, 1, 2 }, { "8E2", SERIAL_8E2, 8, 1, 2 },
  { "7O1", SERIAL_7O1, 7, 2, 1 }, { "8O1", SERIAL_8O1, 8, 2, 1 },
  { "7O2", SERIAL_7O2, 7, 2, 2 }, { "8O2", SERIAL_8O2, 8, 2, 2 },
};

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

GPIO_Type *rxPort;
uint32_t rxMask;

bool rxLevel() {
  return (*portInputRegister(rxPort) & rxMask) != 0;
}

uint8_t muxOf(int pin) {
  return pin_registry_read_pcr(arduino_pin_by_number[pin]).mux;
}

void drain() {
  delay(30);
  while (Serial1.available())
    Serial1.read();
}

// Send b and a second byte straight after it, and sample the RX pin in
// the middle of each bit cell of b's frame, plus one more cell: the
// second byte's start bit. Returns false if no start bit showed up.
bool capture(uint8_t b, uint8_t *cells, int n) {
  drain();
  uint8_t pair[2] = { b, 0xFF };
  Serial1.write(pair, 2);

  unsigned long t = micros();
  while (rxLevel())
    if (micros() - t > 50000)
      return false;

  unsigned long t0 = micros();
  for (int i = 0; i < n; i++) {
    unsigned long at = t0 + BIT_US * i + BIT_US / 2;
    while ((long)(micros() - at) < 0)
      ;
    cells[i] = rxLevel();
  }
  return true;
}

// One byte with an even number of 1s and one with an odd number, in the
// bits the format sends, so each parity bit is seen as both 0 and 1.
// The 8-bit ones have bit 7 set, so a lost 8th bit shows.
bool checkFormat(const Format &f, uint8_t b) {
  uint8_t cells[16];
  int len = 1 + f.bits + (f.parity ? 1 : 0) + f.stop;
  if (!capture(b, cells, len + 1))
    return false;

  bool ok = (cells[0] == 0);
  int ones = 0;
  for (int i = 0; i < f.bits; i++) {
    int bit = (b >> i) & 1;
    ones += bit;
    ok = ok && (cells[1 + i] == bit);
  }
  int pos = 1 + f.bits;
  if (f.parity) {
    int want = (f.parity == 1) ? (ones & 1) : !(ones & 1);
    ok = ok && (cells[pos] == want);
    pos++;
  }
  for (int i = 0; i < f.stop; i++)
    ok = ok && (cells[pos + i] == 1);
  ok = ok && (cells[len] == 0);  // next start bit, right after the stop bits

  // And the loopback got it back intact
  delay(len * 2 * BIT_US / 1000 + 10);
  int r = Serial1.read();
  uint8_t mask = (f.bits == 7) ? 0x7F : 0xFF;
  ok = ok && (r == (b & mask));

  if (!ok) {
    Serial.print("  cells:");
    for (int i = 0; i <= len; i++)
      Serial.print(cells[i]);
    Serial.print("  read back: 0x");
    Serial.println(r, HEX);
  }
  return ok;
}

// A hand-made frame on the TX pin, taken back as a GPIO: start bit, 8
// data bits, the given parity bit, one stop bit.
void bitbang8E1(uint8_t b, int parityBit) {
  uint16_t frame = 0;  // bit 0 first on the wire
  frame |= (uint16_t)b << 1;
  frame |= (uint16_t)(parityBit & 1) << 9;
  frame |= 1u << 10;
  unsigned long t0 = micros();
  for (int i = 0; i < 11; i++) {
    digitalWrite(TX_PIN, (frame >> i) & 1);
    while (micros() - t0 < BIT_US * (i + 1))
      ;
  }
}

int eventCalls = 0;  // serialEvent1() runs from main(), not an interrupt
int eventByte = -1;

void serialEvent1() {
  eventCalls++;
  eventByte = Serial1.read();
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("=== Serial1 format / end() / serialEvent1 (Serial1 loopback jumper) ===");

  // The GPIO input register reads the pin while the UART owns it only
  // once pinMode() has set the pin up as a GPIO; Serial1.begin() then
  // takes the pin over without undoing that. Without this it reads 0.
  pinMode(RX_PIN, INPUT);
  rxPort = digitalPinToPort(RX_PIN);
  rxMask = digitalPinToBitMask(RX_PIN);

  Serial.println("--- frame formats, read off the RX pin at 1200 baud ---");
  for (const Format &f : formats) {
    Serial1.begin(BAUD, f.config);
    uint8_t evenOnes = (f.bits == 7) ? 0x53 : 0xD2;  // 4 ones each
    uint8_t oddOnes = (f.bits == 7) ? 0x43 : 0xD3;   // 3 and 5
    String label = String("SERIAL_") + f.name + " frame and loopback";
    check(label.c_str(), checkFormat(f, evenOnes) && checkFormat(f, oddOnes));
  }

  Serial.println("--- 7 data bits ---");
  {
    Serial1.begin(9600, SERIAL_7N1);
    drain();
    Serial1.write((uint8_t)0xD3);
    delay(10);
    check("7N1 reads 0xD3 back as 0x53 (bit 7 is not sent)", Serial1.read() == 0x53);
  }

  Serial.println("--- a byte with bad parity is dropped ---");
  {
    Serial1.begin(BAUD, SERIAL_8E1);
    drain();
    pinMode(TX_PIN, OUTPUT);
    digitalWrite(TX_PIN, HIGH);
    delay(5);
    bitbang8E1(0x53, 1);  // 4 ones: even parity is 0, so 1 is wrong
    bitbang8E1(0x43, 1);  // 3 ones: 1 is right
    delay(10);
    int n = Serial1.available();
    int r = Serial1.read();
    check("hand-made 8E1 frames: bad one dropped, good one kept", n == 1 && r == 0x43);
  }

  Serial.println("--- begin() without a format is 8N1 again ---");
  {
    Serial1.begin(BAUD, SERIAL_7O2);
    Serial1.begin(BAUD);
    check("SERIAL_8N1 after begin(baud)", checkFormat(formats[1], 0xD3));
  }

  Serial.println("--- end() ---");
  {
    Serial1.begin(9600);
    uint8_t alt = muxOf(TX_PIN);
    check("begin() puts TX/RX on the UART", alt != 0 && muxOf(RX_PIN) != 0);

    drain();
    Serial1.print("abc");
    Serial1.flush();
    delay(5);
    check("3 bytes waiting before end()", Serial1.available() == 3);

    Serial1.end();
    check("end() hands TX/RX back as GPIO", muxOf(TX_PIN) == 0 && muxOf(RX_PIN) == 0);
    check("end() drops what wasn't read: available() == 0", Serial1.available() == 0);

    unsigned long t = millis();
    int r = Serial1.read();
    int p = Serial1.peek();
    check("read()/peek() after end() are -1 at once", r == -1 && p == -1 && millis() - t < 10);

    // The transmitter keeps running with no pin, so this takes the ~0.4s
    // 400 bytes take at 9600 baud, but must not stall once the 256-byte
    // TX buffer is full
    t = millis();
    for (int i = 0; i < 400; i++)
      Serial1.write('z');
    Serial1.flush();
    check("writing after end() doesn't hang", millis() - t < 1000);

    pinMode(TX_PIN, OUTPUT);
    pinMode(RX_PIN, INPUT);
    digitalWrite(TX_PIN, LOW);
    delay(1);
    bool low = digitalRead(RX_PIN) == LOW;
    digitalWrite(TX_PIN, HIGH);
    delay(1);
    bool high = digitalRead(RX_PIN) == HIGH;
    check("TX/RX work as GPIO after end()", low && high);

    Serial1.begin(9600);
    delay(5);
    check("nothing written while ended arrives", Serial1.available() == 0);
    Serial1.print("ok");
    delay(10);
    char buf[3] = { 0 };
    buf[0] = (char)Serial1.read();
    buf[1] = (char)Serial1.read();
    check("begin() after end() works again", strcmp(buf, "ok") == 0 && muxOf(TX_PIN) == alt);
  }

  Serial.println("--- write(0) ---");
  {
    drain();
    size_t n = Serial1.write(0);  // used to be ambiguous: uint8_t or const char*?
    delay(10);
    check("Serial1.write(0) sends one 0x00", n == 1 && Serial1.read() == 0x00);
  }

  Serial.println("--- serialEvent1(), called after loop() ---");
  drain();
}

int loops = 0;

void loop() {
  loops++;
  if (loops == 1) {
    check("serialEvent1() not called with nothing to read", eventCalls == 0);
    Serial1.write('E');
    Serial1.flush();
    delay(5);
  } else if (loops == 2) {
    check("serialEvent1() called once the byte is there", eventCalls == 1 && eventByte == 'E');
  } else if (loops == 3) {
    check("serialEvent1() not called again once it's read", eventCalls == 1);

    Serial.println();
    if (failCount == 0)
      Serial.println("ALL OK");
    else {
      Serial.print(failCount);
      Serial.println(" FAILED");
    }
  }
}
