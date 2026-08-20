/** SPI.transfer(buf, count) bulk-transfer chunking test
 *
 *  Needs a MOSI-MISO loopback wire (same setup as
 *  test_SPI_loopback_with_a_wire / test_SPI_bitorder_end_transfer16).
 *
 *  SPIClass::txrx() copies through a fixed 128-byte stack buffer, chunking
 *  any larger transfer into multiple back-to-back r01lib write() calls
 *  (fixed after a stack-overflow bug where a >128-byte transfer copied
 *  straight past the end of that buffer). This sketch exercises sizes
 *  chosen to land exactly on and around the 128-byte chunk boundary --
 *  and further boundaries at 256/384 bytes for a >2-chunk transfer -- to
 *  catch any off-by-one or chunk-stitching error in that loop.
 *
 *  CS is held low by the sketch (digitalWrite(SS, ...)) across each whole
 *  transfer, so the underlying multi-chunk implementation should be
 *  invisible on the wire -- a logic analyzer capture should show one
 *  continuous burst of clock/data per transfer, not a series of separate
 *  CS pulses.
 *
 *  Each buffer is filled with an incrementing byte pattern ((uint8_t)i)
 *  so any corruption, truncation, or chunk-boundary misalignment shows up
 *  as a clear mismatch at a specific index. All transfers run back-to-back
 *  before any Serial output (for a clean logic analyzer capture); results
 *  print together at the end.
 */

#include <Arduino.h>

// Global, not on the stack -- this is the buffer under test, sized to the
// largest case below (384 bytes, spanning 3 full 128-byte chunks).
uint8_t buf[384];

const size_t sizes[] = { 1, 127, 128, 129, 255, 256, 257, 383, 384 };
const int numSizes = sizeof(sizes) / sizeof(sizes[0]);
bool ok[numSizes];
int firstMismatch[numSizes];

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("SPI.transfer(buf, count) chunking test");

  pinMode(SS, OUTPUT);
  digitalWrite(SS, HIGH);
  SPI.begin();
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

  // Run every transfer back-to-back first, no Serial calls in between, so
  // a logic analyzer capture shows a clean, uninterrupted burst per size.
  for (int s = 0; s < numSizes; s++) {
    size_t n = sizes[s];

    for (size_t i = 0; i < n; i++)
      buf[i] = (uint8_t)i;

    digitalWrite(SS, LOW);
    SPI.transfer(buf, n);
    digitalWrite(SS, HIGH);

    firstMismatch[s] = -1;
    for (size_t i = 0; i < n; i++) {
      if (buf[i] != (uint8_t)i) {
        firstMismatch[s] = (int)i;
        break;
      }
    }
    ok[s] = (firstMismatch[s] == -1);
  }

  SPI.endTransaction();

  // All transfers done -- now report results.
  for (int s = 0; s < numSizes; s++) {
    Serial.print("size=");
    Serial.print((unsigned)sizes[s]);
    Serial.print(": ");
    if (ok[s]) {
      Serial.println("OK");
    } else {
      Serial.print("FAIL (first mismatch at index ");
      Serial.print(firstMismatch[s]);
      Serial.println(")");
    }
  }
}

void loop() {
}
