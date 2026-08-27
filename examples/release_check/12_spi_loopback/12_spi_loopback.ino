/** Release check 5/N: automatic OK/FAIL checks needing MOSI-MISO loopback
 *  wires on both the default SPI (D11-D12) and SPI1 (MikroBus) -- they're
 *  independent peripherals (see PIN_MAPPING_*.md) and can be used in the
 *  same sketch, so both jumpers can stay installed at once for one flash.
 *
 *  Consolidates (from examples/Arduino_compatible_API/):
 *  test_SPI_bitorder_end_transfer16, test_SPI_large_transfer,
 *  test_SPI_legacy_api, test_SPI1_MikroBus. (test_SPI_loopback_with_a_wire
 *  isn't included -- an older, weaker sanity check with no pass/fail
 *  verdict, superseded by these.)
 *
 *  Wiring needed: jumper D11 (MOSI) <-> D12 (MISO), and jumper the
 *  MikroBus header's MOSI <-> MISO pins.
 *
 *  All SPI transfers in each section run back-to-back before any Serial
 *  output, so a logic analyzer capture shows clean, uninterrupted bursts
 *  per section; results print together afterward.
 */

#include <Arduino.h>

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

// Global, not on the stack -- sized to the largest case in the chunking
// section below (384 bytes, spanning 3 full 128-byte chunks).
uint8_t chunkBuf[384];

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("=== Release check 5/N: SPI loopback checks (D11-D12 jumper) ===");

  pinMode(SS, OUTPUT);
  digitalWrite(SS, HIGH);
  SPI.begin();

  // ---- transfer16() / bitOrder / end() (was test_SPI_bitorder_end_transfer16) ----
  Serial.println("--- transfer16 / bitOrder / end ---");
  {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(SS, LOW);
    uint8_t b = SPI.transfer(0xA5);
    digitalWrite(SS, HIGH);

    digitalWrite(SS, LOW);
    uint16_t w1 = SPI.transfer16(0x1234);
    digitalWrite(SS, HIGH);
    SPI.endTransaction();

    // switch bit order mid-session -- should not break subsequent transfers
    SPI.beginTransaction(SPISettings(1000000, LSBFIRST, SPI_MODE0));
    digitalWrite(SS, LOW);
    uint16_t w2 = SPI.transfer16(0x5678);
    digitalWrite(SS, HIGH);
    SPI.endTransaction();

    // end() then begin() again -- should be able to restart cleanly
    SPI.end();
    SPI.begin();
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(SS, LOW);
    uint8_t b2 = SPI.transfer(0x5A);
    digitalWrite(SS, HIGH);
    SPI.endTransaction();

    check("transfer(uint8_t) loopback", b == 0xA5);
    check("transfer16 (MSBFIRST) loopback", w1 == 0x1234);
    check("transfer16 (LSBFIRST) loopback", w2 == 0x5678);
    check("transfer after end()+begin()", b2 == 0x5A);
  }

  // ---- transfer(buf, count) chunking, around the 128-byte boundary
  //      (was test_SPI_large_transfer) ----
  Serial.println("--- transfer(buf, count) chunking ---");
  {
    const size_t sizes[] = { 1, 127, 128, 129, 255, 256, 257, 383, 384 };
    const int numSizes = sizeof(sizes) / sizeof(sizes[0]);

    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

    for (int s = 0; s < numSizes; s++) {
      size_t n = sizes[s];

      for (size_t i = 0; i < n; i++)
        chunkBuf[i] = (uint8_t)i;

      digitalWrite(SS, LOW);
      SPI.transfer(chunkBuf, n);
      digitalWrite(SS, HIGH);

      bool ok = true;
      for (size_t i = 0; i < n; i++) {
        if (chunkBuf[i] != (uint8_t)i) {
          ok = false;
          break;
        }
      }

      char label[32];
      snprintf(label, sizeof(label), "transfer(buf,%u) round-trips", (unsigned)n);
      check(label, ok);
    }

    SPI.endTransaction();
  }

  // ---- legacy pre-1.6 API: setBitOrder/setDataMode/setClockDivider
  //      (was test_SPI_legacy_api) ----
  Serial.println("--- legacy setBitOrder/setDataMode/setClockDivider ---");
  {
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    SPI.setClockDivider(SPI_CLOCK_DIV8);

    digitalWrite(SS, LOW);
    uint8_t b1 = SPI.transfer(0x3C);
    digitalWrite(SS, HIGH);
    check("transfer after legacy setup (DIV8)", b1 == 0x3C);

    SPI.setBitOrder(LSBFIRST);
    SPI.setClockDivider(SPI_CLOCK_DIV32);

    digitalWrite(SS, LOW);
    uint8_t b2 = SPI.transfer(0xC3);
    digitalWrite(SS, HIGH);
    check("transfer after changing bitOrder/divider (DIV32)", b2 == 0xC3);

    SPI.setDataMode(SPI_MODE2);

    digitalWrite(SS, LOW);
    uint8_t b3 = SPI.transfer(0x5A);
    digitalWrite(SS, HIGH);
    check("transfer after changing dataMode", b3 == 0x5A);
  }

  // ---- SPI1 (MikroBus SPI), independent peripheral from SPI
  //      (was test_SPI1_MikroBus) ----
  Serial.println("--- SPI1 (MikroBus SPI) ---");
  {
    pinMode(MB_CS, OUTPUT);
    digitalWrite(MB_CS, HIGH);
    SPI1.begin();

    SPI1.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

    digitalWrite(MB_CS, LOW);
    uint8_t sb1 = SPI1.transfer(0xA5);
    digitalWrite(MB_CS, HIGH);

    digitalWrite(MB_CS, LOW);
    uint16_t sw1 = SPI1.transfer16(0x1234);
    digitalWrite(MB_CS, HIGH);

    SPI1.endTransaction();

    check("SPI1 transfer(uint8_t) loopback", sb1 == 0xA5);
    check("SPI1 transfer16 loopback", sw1 == 0x1234);
  }

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
