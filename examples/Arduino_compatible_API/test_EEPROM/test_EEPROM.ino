/** EEPROM library test: the API, and that what was written is still there
 *  after a reset and after the next upload.
 *
 *  Wiring: none.
 *
 *  Automatic, in two parts, with one reset the sketch does itself:
 *    1. the API; then 1000 bytes of a pattern unique to this run, and
 *       2000 writes that fill the storage many times over, so it has to
 *       start afresh (erase and copy) several times; then a reset
 *    2. after the reset, everything written in part 1 is read back
 *  Reads "ALL OK" or "N FAILED" at the end of part 2.
 *
 *  Upload it again afterwards: the new run first checks that the previous
 *  run's data came through the upload, and says so. The very first run
 *  has nothing to check there.
 *
 *  Each run starts the storage afresh some twenty times, a tiny part of
 *  what the flash is rated for.
 *
 *  Mirrored as examples/release_check/06_eeprom -- copy any change there
 *  too.
 */

#include <Arduino.h>
#include <EEPROM.h>

const int PATTERN_LEN = 1000;
const int COUNTER_AT = 1000;  // uint32_t
const int RUN_ID_AT = 1010;   // uint16_t
const int STATE_AT = 1012;    // uint32_t
const int FLOAT_AT = 1016;    // float, outside the pattern
const uint32_t STATE_RESET = 0x54455352;  // "RSET": part 1 done, reset under way
const uint32_t STATE_DONE = 0x454E4F44;   // "DONE": part 2 passed
const uint32_t CHURN = 2000;

struct Big {
  char name[20];
  float values[4];
  uint32_t check;
};

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

uint8_t pattern(int i, uint16_t runId) {
  return (uint8_t)(i * 31 + runId * 7 + (i >> 5));
}

bool patternIntact(uint16_t runId) {
  for (int i = 0; i < PATTERN_LEN; i++)
    if (EEPROM.read(i) != pattern(i, runId))
      return false;
  return true;
}

void fillBig(Big &b, uint16_t runId) {
  memset(&b, 0, sizeof(b));
  snprintf(b.name, sizeof(b.name), "run %u", runId);
  for (int i = 0; i < 4; i++)
    b.values[i] = runId * 0.25f + i;
  b.check = runId * 2654435761u;
}

void partOne() {
  Serial.println("--- part 1: API ---");

  check("length() is 1024, E2END 1023", EEPROM.length() == 1024 && E2END == 1023);

  EEPROM.write(10, 0x5A);
  check("write() then read()", EEPROM.read(10) == 0x5A);
  EEPROM.update(10, 0xA5);
  check("update() changes it", EEPROM.read(10) == 0xA5);
  EEPROM[11] = 7;
  EEPROM[11] += 3;
  EEPROM[11]++;
  uint8_t v = EEPROM[11];
  check("EEPROM[i] =, +=, ++", v == 11);
  EEPROM[12] = EEPROM[11];
  check("EEPROM[i] = EEPROM[j]", EEPROM[12] == 11);

  float f = 3.25f, g = 0;
  EEPROM.put(FLOAT_AT, f);
  check("put()/get() float", EEPROM.get(FLOAT_AT, g) == 3.25f);

  Big b, c;
  fillBig(b, 1234);
  EEPROM.put(100, b);  // longer than one record on either board
  EEPROM.get(100, c);
  check("put()/get() 40-byte struct", memcmp(&b, &c, sizeof(b)) == 0);

  int n = 0;
  for (EERef r : EEPROM) {
    (void)r;
    n++;
  }
  check("range-for visits every byte", n == 1024);

  check("out-of-range read gives 0xFF", EEPROM.read(1024) == 0xFF && EEPROM.read(-1) == 0xFF);
  uint8_t first = EEPROM.read(0), last = EEPROM.read(1023);
  EEPROM.write(1024, first ^ 0xFF);
  EEPROM.write(-1, last ^ 0xFF);
  check("out-of-range write is ignored", EEPROM.read(0) == first && EEPROM.read(1023) == last);

  // A run id different from the previous run's, so its pattern can't pass
  // for this one's
  uint16_t runId = 0;
  EEPROM.get(RUN_ID_AT, runId);
  runId = (uint16_t)(runId + 1 + (micros() & 0x3F));
  Serial.print("--- part 1: pattern and churn, run id ");
  Serial.print(runId);
  Serial.println(" ---");

  for (int i = 0; i < PATTERN_LEN; i++)
    EEPROM.write(i, pattern(i, runId));
  check("1000-byte pattern reads back", patternIntact(runId));

  uint32_t maxUs = 0, slow = 0;
  for (uint32_t k = 1; k <= CHURN; k++) {
    uint32_t t = micros();
    EEPROM.put(COUNTER_AT, k);
    t = micros() - t;
    if (t > maxUs)
      maxUs = t;
    if (t > 2000)
      slow++;
  }
  Serial.print("  ");
  Serial.print(CHURN);
  Serial.print(" writes: slowest ");
  Serial.print(maxUs);
  Serial.print("us, ");
  Serial.print(slow);
  Serial.println(" took over 2ms (the storage starting afresh)");
  check("the storage started afresh several times", slow >= 3);
  check("slowest write under 20ms", maxUs < 20000);
  uint32_t k = 0;
  check("counter reads back", EEPROM.get(COUNTER_AT, k) == CHURN);
  check("pattern untouched by the churn", patternIntact(runId));

  EEPROM.put(RUN_ID_AT, runId);
  EEPROM.put(STATE_AT, STATE_RESET);

  Serial.println();
  if (failCount) {
    Serial.print(failCount);
    Serial.println(" FAILED (part 1; not resetting)");
    return;
  }
  Serial.println("resetting...");
  Serial.flush();
  delay(10);
  NVIC_SystemReset();
}

void partTwo() {
  Serial.println("--- part 2: after the reset ---");
  uint16_t runId = 0;
  EEPROM.get(RUN_ID_AT, runId);
  Serial.print("  run id ");
  Serial.println(runId);

  check("1000-byte pattern kept", patternIntact(runId));
  uint32_t k = 0;
  check("counter kept", EEPROM.get(COUNTER_AT, k) == CHURN);
  float g = 0;
  check("float kept", EEPROM.get(FLOAT_AT, g) == 3.25f);

  EEPROM.put(STATE_AT, failCount ? 0u : STATE_DONE);

  Serial.println();
  if (failCount == 0)
    Serial.println("ALL OK");
  else {
    Serial.print(failCount);
    Serial.println(" FAILED");
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  delay(200);
  Serial.println("=== EEPROM test ===");

  uint32_t state = 0;
  EEPROM.get(STATE_AT, state);

  if (state == STATE_RESET) {
    partTwo();
    return;
  }

  if (state == STATE_DONE) {
    uint16_t runId = 0;
    EEPROM.get(RUN_ID_AT, runId);
    Serial.print("the previous run (id ");
    Serial.print(runId);
    Serial.print(") passed; its data after the upload since: ");
    bool kept = patternIntact(runId);
    Serial.println(kept ? "kept (OK)" : "LOST (FAIL)");
    if (!kept)
      failCount++;
  } else
    Serial.println("no finished previous run to check");

  partOne();
}

void loop() {
}
