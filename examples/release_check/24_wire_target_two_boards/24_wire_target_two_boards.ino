/** Release check 24: Wire target (slave) mode between two boards -- needs
 *  the other board, so it sits in the 2n group (see
 *  examples/release_check/README.md for the numbering).
 *
 *  Mirrors examples/Arduino_compatible_API/test_Wire_target_two_boards
 *  exactly -- edit that one, then copy it here.
 *
 *  Wiring: FRDM-MCXA153 D18-D18, D19-D19 and GND-GND to an FRDM-MCXN947;
 *  nothing else on D18/D19. Flash this to both; each prints its own
 *  "ALL OK"/"N FAILED". If either was already running it, press RESET on
 *  both, close together, so the two runs meet.
 */

#include <Arduino.h>
#include <Wire.h>

#if defined(FRDM_MCXN947)
const uint8_t SELF = 0x43, PEER = 0x42;
const bool GOES_FIRST = false;
#define PULL_UPS() do { PORT4->PCR[0] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK; PORT4->PCR[1] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK; } while (0)
#else
const uint8_t SELF = 0x42, PEER = 0x43;
const bool GOES_FIRST = true;
#define PULL_UPS() do { PORT1->PCR[8] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK; PORT1->PCR[9] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK; } while (0)
#endif

const uint8_t YOUR_TURN = 0x10;

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

// ---- this board as a target ----
uint8_t regs[16];
uint8_t regPtr = 0;
volatile bool myTurn = false;
volatile int writesSeen = 0;

void onRx(int n) {
  if (n < 1)
    return;
  uint8_t p = Wire.read();
  if (p == YOUR_TURN) {
    myTurn = true;
    return;
  }
  writesSeen = writesSeen + 1;
  regPtr = p & 15;
  for (int i = 1; i < n && Wire.available(); i++)
    regs[(regPtr + i - 1) & 15] = Wire.read();
}

void onReq() {
  for (int i = 0; i < 4; i++)
    Wire.write(regs[(regPtr + i) & 15]);
}

// ---- this board as the controller of the other ----
bool peerAnswers() {
  Wire.beginTransmission(PEER);
  return Wire.endTransmission() == 0;
}

void runChecks() {
  Serial.println("--- as controller of the other board ---");

  Wire.beginTransmission(PEER);
  Wire.write(2);
  Wire.write(0x11);
  Wire.write(0x22);
  Wire.write(0x33);
  check("write 3 registers from 2", Wire.endTransmission() == 0);

  uint8_t n = Wire.requestFrom(PEER, (uint8_t)4, (uint32_t)2, (uint8_t)1, (uint8_t)true);
  uint8_t b[4] = { 0 };
  for (int i = 0; i < 4 && Wire.available(); i++)
    b[i] = Wire.read();
  check("read them back (register 2, repeated start, 4 bytes)",
        n == 4 && b[0] == 0x11 && b[1] == 0x22 && b[2] == 0x33 && b[3] == 0xA5);

  Wire.beginTransmission(PEER);
  Wire.write(14);
  Wire.endTransmission(false);
  n = Wire.requestFrom(PEER, (size_t)6);
  uint8_t c[6] = { 0 };
  for (int i = 0; i < 6 && Wire.available(); i++)
    c[i] = Wire.read();
  check("reading 6 of a 4-byte reply: 4 registers, then 0xFF",
        n == 6 && c[0] == 0xAE && c[1] == 0xAF && c[2] == 0xA0 && c[3] == 0xA1 && c[4] == 0xFF && c[5] == 0xFF);

  Wire.beginTransmission(PEER ^ 0x08);
  Wire.write(0);
  check("an address nobody has is NAKed", Wire.endTransmission() == 134);

  Wire.beginTransmission(PEER);
  Wire.write(YOUR_TURN);
  check("hand over to the other board", Wire.endTransmission() == 0);
}

bool waitFor(bool (*cond)(), unsigned long ms) {
  unsigned long t = millis();
  while (!cond())
    if (millis() - t > ms)
      return false;
  return true;
}

bool isMyTurn() {
  return myTurn;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.print("=== Wire target mode between two boards: this one is 0x");
  Serial.print(SELF, HEX);
  Serial.println(" ===");

  for (int i = 0; i < 16; i++)
    regs[i] = 0xA0 + i;

  Wire.setWireTimeout(25000);  // also clears a bus-busy latched while the pins floated
  Wire.onReceive(onRx);
  Wire.onRequest(onReq);
  Wire.begin(SELF);
  PULL_UPS();
  delay(2);
  Wire.begin(SELF);

  if (GOES_FIRST) {
    check("the other board answers", waitFor(peerAnswers, 20000));
    runChecks();
    check("the other board takes its turn and hands back", waitFor(isMyTurn, 20000));
  } else {
    check("the other board takes its turn and hands over", waitFor(isMyTurn, 30000));
    runChecks();
  }

  Serial.println("--- as target of the other board ---");
  check("the other board's writes arrived", writesSeen >= 1);
  check("... and landed in the registers", regs[2] == 0x11 && regs[3] == 0x22 && regs[4] == 0x33);

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
