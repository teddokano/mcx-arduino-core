/** Wire target (slave) mode -- begin(address), onReceive(), onRequest() --
 *  checked with the board talking to its own target: an LPI2C's
 *  controller and target halves run independently on the same pins, so
 *  Wire can address itself.
 *
 *  Wiring: none. Nothing may be connected to D18/D19: the bus runs on the
 *  pins' internal pull-ups, which a real I2C bus would have as resistors.
 *
 *  Automatic: reads "ALL OK" or "N FAILED" at the end.
 */

#include <Arduino.h>
#include <Wire.h>
#include "fsl_lpi2c.h"

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

// The bus under test, for the handlers, which take no bus argument
TwoWire *bus;

// A small register-file device, as target sketches usually are: a write's
// first byte sets the register pointer, the rest are stored from there;
// a read returns 4 registers from the pointer
uint8_t regs[16];
uint8_t regPtr = 0;
bool shortReply = false;  // reply with 2 bytes only, to see the 0xFF fill

int rxCalls = 0;
int rxCount = -1;
uint8_t got[140];
char order[8];
int nOrder = 0;

void onRx(int n) {
  rxCalls++;
  rxCount = n;
  if (nOrder < 7)
    order[nOrder++] = 'R';
  for (int i = 0; i < n && bus->available(); i++)
    got[i] = bus->read();
  if (n >= 1) {
    regPtr = got[0] & 15;
    for (int i = 1; i < n; i++)
      regs[(regPtr + i - 1) & 15] = got[i];
  }
}

void onReq() {
  if (nOrder < 7)
    order[nOrder++] = 'Q';
  if (shortReply) {
    bus->write(0x11);
    bus->write(0x22);
  } else {
    for (int i = 0; i < 4; i++)
      bus->write(regs[(regPtr + i) & 15]);
  }
}

// The target sees the STOP, and runs onReceive(), a moment after the
// controller's endTransmission() has returned -- as it would on a second
// board. Give it that moment before looking at what it got.
void settle() {
  delay(1);
}

void resetLog() {
  rxCalls = 0;
  rxCount = -1;
  nOrder = 0;
  memset(order, 0, sizeof(order));
  memset(got, 0, sizeof(got));
}

// Internal pull-ups on the bus pins. pinMode() can't do this: it would take
// the pins back from the I2C peripheral.
void pullUps(PORT_Type *port, int sda, int scl) {
  port->PCR[sda] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
  port->PCR[scl] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
}

void suite(TwoWire &w, const char *name, uint8_t addr, LPI2C_Type *base, PORT_Type *port, int sda, int scl) {
  Serial.print("--- ");
  Serial.print(name);
  Serial.print(" as its own target at 0x");
  Serial.print(addr, HEX);
  Serial.println(" ---");

  bus = &w;
  w.setWireTimeout(25000);  // also clears a bus-busy latched while the pins floated
  w.onReceive(onRx);
  w.onRequest(onReq);
  w.begin(addr);
  pullUps(port, sda, scl);
  delay(2);
  w.begin(addr);
  for (int i = 0; i < 16; i++)
    regs[i] = 0xA0 + i;

  resetLog();
  w.beginTransmission(addr);
  w.write(1);
  w.write(2);
  w.write(3);
  uint8_t r = w.endTransmission();
  settle();
  check("write 3 bytes: ACKed, onReceive(3) gets 1,2,3",
        r == 0 && rxCalls == 1 && rxCount == 3 && got[0] == 1 && got[1] == 2 && got[2] == 3);

  for (int i = 0; i < 16; i++)
    regs[i] = 0xA0 + i;
  resetLog();
  uint8_t n = w.requestFrom(addr, (uint8_t)4, (uint32_t)5, (uint8_t)1, (uint8_t)true);
  uint8_t b[4] = { 0 };
  for (int i = 0; i < 4 && w.available(); i++)
    b[i] = w.read();
  check("register read (write 5, repeated start, read 4) gets regs 5-8",
        n == 4 && b[0] == 0xA5 && b[1] == 0xA6 && b[2] == 0xA7 && b[3] == 0xA8);
  check("... with onReceive(1) before onRequest", strcmp(order, "RQ") == 0 && rxCount == 1);

  shortReply = true;
  n = w.requestFrom(addr, (size_t)5);
  uint8_t c[5] = { 0 };
  for (int i = 0; i < 5 && w.available(); i++)
    c[i] = w.read();
  shortReply = false;
  check("reading past a 2-byte reply gets 0xFF for the rest",
        n == 5 && c[0] == 0x11 && c[1] == 0x22 && c[2] == 0xFF && c[3] == 0xFF && c[4] == 0xFF);

  w.onRequest(nullptr);
  n = w.requestFrom(addr, (size_t)3);
  bool allFF = n == 3;
  while (w.available())
    allFF = allFF && w.read() == 0xFF;
  w.onRequest(onReq);
  check("no onRequest handler: all 0xFF", allFF);

  resetLog();
  w.beginTransmission(addr);
  for (int i = 0; i < WIRE_BUFFER_SIZE; i++)
    w.write((uint8_t)i);
  r = w.endTransmission();
  settle();
  check("write of WIRE_BUFFER_SIZE bytes arrives whole",
        r == 0 && rxCount == WIRE_BUFFER_SIZE && got[0] == 0 && got[WIRE_BUFFER_SIZE - 1] == WIRE_BUFFER_SIZE - 1);

  // More than the target's buffer holds. Wire itself can't send that many,
  // so this goes through the SDK's master calls directly.
  resetLog();
  uint8_t big[WIRE_BUFFER_SIZE + 2];
  for (unsigned i = 0; i < sizeof(big); i++)
    big[i] = (uint8_t)i;
  status_t st = LPI2C_MasterStart(base, addr, kLPI2C_Write);
  if (st == kStatus_Success)
    st = LPI2C_MasterSend(base, big, sizeof(big));
  // MasterSend() returns once the last bytes are in the FIFO, so a NAK
  // near the end of the data is reported by MasterStop() instead
  status_t stopSt = LPI2C_MasterStop(base);
  if (stopSt != kStatus_Success)
    LPI2C_MasterStop(base);
  if (st == kStatus_Success)
    st = stopSt;
  settle();
  check("writing past the target's buffer is NAKed; onReceive gets the first WIRE_BUFFER_SIZE",
        st == kStatus_LPI2C_Nak && rxCount == WIRE_BUFFER_SIZE && got[WIRE_BUFFER_SIZE - 1] == WIRE_BUFFER_SIZE - 1);

  resetLog();
  w.beginTransmission(addr);
  w.write(7);
  r = w.endTransmission();
  settle();
  check("the bus still works after that", r == 0 && rxCount == 1 && got[0] == 7);

  resetLog();
  w.beginTransmission(addr + 1);
  w.write(9);
  r = w.endTransmission();
  settle();
  check("another address is NAKed, onReceive not called", r == 134 && rxCalls == 0);

  resetLog();
  w.beginTransmission(addr);
  r = w.endTransmission();
  settle();
  check("address-only write (as a bus scan does): onReceive(0), as on AVR", r == 0 && rxCalls == 1 && rxCount == 0);

  w.begin();
  pullUps(port, sda, scl);
  w.beginTransmission(addr);
  w.write(1);
  r = w.endTransmission();
  settle();
  if (r != 134) {
    Serial.print("  (endTransmission returned ");
    Serial.print(r);
    Serial.println(")");
  }
  check("begin() with no address: no longer a target", r == 134);

  w.end();
  w.begin(addr);
  pullUps(port, sda, scl);
  delay(2);
  w.begin(addr);
  resetLog();
  w.beginTransmission(addr);
  w.write(5);
  r = w.endTransmission();
  settle();
  check("end() then begin(address): a target again", r == 0 && rxCount == 1 && got[0] == 5);

  w.end();
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("=== Wire target mode, against itself (no wiring) ===");

#if defined(FRDM_MCXN947)
  suite(Wire, "Wire", 0x42, LPI2C2, PORT4, 0, 1);
#else
  suite(Wire, "Wire", 0x42, LPI2C0, PORT1, 8, 9);
#endif

  Serial.println();
  if (failCount == 0)
    Serial.println("ALL OK");
  else {
    Serial.print(failCount);
    Serial.println(" FAILED");
  }
}

void loop() {
}
