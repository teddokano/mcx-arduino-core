/** EEPROM: a reset in the middle of a write.
 *
 *  The EEPROM library is meant to leave every completed write intact
 *  whenever a reset cuts the next one short, and that write either done or
 *  not done, nothing in between. This sketch resets the board with the
 *  watchdog at a random moment inside a write, over and over, and checks
 *  that after every reset.
 *
 *  No wiring. It runs by itself, resetting the board about twice a second,
 *  and stops after TRIALS resets with "ALL OK" or "N FAILED". A FAIL line
 *  says what was wrong. Pressing the reset button, or uploading, starts
 *  a new run. It overwrites whatever the EEPROM held.
 *
 *  What it writes: eight 4-byte counters ("slots"), written in turn with
 *  1, 2, 3, ..., and a fixed pattern in every other byte. After any reset
 *  the slots must hold eight consecutive values, each in its own slot, and
 *  the pattern must be untouched. The value being written when the reset
 *  came must be either there or not, and every one before it there. That
 *  value is remembered across the reset in RAM the startup code doesn't
 *  clear (.noinit).
 *
 *  Half the trials aim at an ordinary write, which appends one record. The
 *  other half first fill the log, so that the write under test is the one
 *  that erases the other half and copies everything there, and aim at that.
 *  To know how full the log is, this sketch reads the library's area in
 *  flash itself, following EEPROM.cpp's layout: if that layout changes,
 *  this has to change with it.
 *
 *  On FRDM-MCXN947 a reset in the middle of an erase or a program can leave
 *  flash whose ECC doesn't match, and reading it is a bus fault. The
 *  library used to hang in its first access after such a reset, on every
 *  boot from then on, and this sketch is what found it. Each boot counts
 *  the unreadable 16-byte phrases in the area, and the summary says how
 *  many boots found some, to show that case was met and got through. On
 *  FRDM-MCXA153 none has been seen.
 *
 *  A reset is not a power loss. A power loss can leave flash cells half
 *  programmed or half erased, which a reset does not, so this doesn't
 *  show what a power cut in the middle of a write does.
 *
 *  The watchdog (WWDT0) is programmed through its registers, since the core
 *  carries no SDK driver for it: same peripheral on both boards.
 */

#include <Arduino.h>
#include <EEPROM.h>
#include <stddef.h>
#include "fsl_clock.h"

const uint32_t TRIALS = 1000;

// ---- EEPROM.cpp's layout in flash (white-box, for the log fill level) ----
#if defined(FRDM_MCXA153)
const uint32_t UNIT = 16;           // one record
const uint32_t COMPACT_US = 7000;   // a compacting write takes up to ~6ms
#else
const uint32_t UNIT = 128;
const uint32_t COMPACT_US = 12000;  // up to ~11ms
#endif
const uint32_t APPEND_US = 700;     // an ordinary write takes ~0.1-0.5ms
const uint32_t LOG_OFF = 128 + 1024;
const uint32_t AREA_MAGIC = 0x4545584D;
extern "C" const uint8_t __base_EEPROM_FLASH[];
extern "C" const uint8_t __top_EEPROM_FLASH[];

uint32_t half_size() {
  return (uint32_t)(__top_EEPROM_FLASH - __base_EEPROM_FLASH) / 2;
}

// Read flash that a reset may have left unreadable (an erase or program cut
// short breaks its ECC), without taking the bus fault: as EEPROM.cpp does.
// Returns false if any word couldn't be read
bool read_flash(uint32_t *dst, const uint8_t *src, size_t n) {
  const volatile uint32_t *s = (const volatile uint32_t *)src;
  SCB->CFSR = SCB_CFSR_BUSFAULTSR_Msk;
  __set_FAULTMASK(1);
  SCB->CCR |= SCB_CCR_BFHFNMIGN_Msk;
  __DSB();
  __ISB();
  for (size_t k = 0; k < n / 4; k++)
    dst[k] = s[k];
  __DSB();
  SCB->CCR &= ~SCB_CCR_BFHFNMIGN_Msk;
  __set_FAULTMASK(0);
  __ISB();
  uint32_t bfsr = SCB->CFSR & SCB_CFSR_BUSFAULTSR_Msk;
  SCB->CFSR = bfsr;
  return bfsr == 0;
}

bool header_valid(int h, uint32_t *seq) {
  uint32_t w[4];
  if (!read_flash(w, __base_EEPROM_FLASH + h * half_size(), sizeof w))
    return false;
  if (w[0] != AREA_MAGIC || w[1] != ~w[2] || w[3] != 1024)
    return false;
  *seq = w[1];
  return true;
}

// 16-byte phrases anywhere in the area that can't be read
int unreadable_phrases() {
  int bad = 0;
  for (const uint8_t *p = __base_EEPROM_FLASH; p < __top_EEPROM_FLASH; p += 16) {
    uint32_t w[4];
    if (!read_flash(w, p, sizeof w))
      bad++;
  }
  return bad;
}

// Records the current half's log still has room for, or -1 if no half is valid
int log_free() {
  uint32_t s0, s1;
  bool v0 = header_valid(0, &s0);
  bool v1 = header_valid(1, &s1);
  if (!v0 && !v1)
    return -1;
  int cur = (v0 && (!v1 || (int32_t)(s0 - s1) > 0)) ? 0 : 1;
  const uint8_t *b = __base_EEPROM_FLASH + cur * half_size();
  uint32_t off = LOG_OFF;
  for (; off + UNIT <= half_size(); off += UNIT) {
    uint32_t u[UNIT / 4];
    if (!read_flash(u, b + off, UNIT))
      continue;  // a record cut short: used, and skipped
    bool erased = true;
    for (uint32_t k = 0; k < UNIT / 4; k++)
      if (u[k] != 0xFFFFFFFF)
        erased = false;
    if (erased)
      break;
  }
  return (half_size() - off) / UNIT;
}

// ---- the data under test ----
const int N = 8;
const uint32_t INIT_MARK = 0x31545352;  // "RST1"
int slot_addr(int j) { return 16 + j * 120; }
bool is_slot_byte(int a) {
  return a >= 16 && (a - 16) / 120 < N && (a - 16) % 120 < 4;
}
uint8_t pattern(int a) { return (uint8_t)(a * 37 + 11); }

// ---- kept across the watchdog reset ----
const uint32_t NONE = 0xFFFFFFFF;
struct State {
  uint32_t magic;
  uint32_t armed;       // the value being written when the reset was set, or NONE
  uint32_t compacting;  // that write was the one that compacts
  uint32_t x_us;        // how far into the write the reset was aimed
  uint32_t trials, cut, cut_compacting, landed, rolled_back, fails;
  uint32_t unreadable_boots;  // boots that found flash in the area unreadable
  uint32_t sum;
  // Outside the checksum: set with one store right after the write returns,
  // where the reset may land, and redoing the checksum there could be cut
  // short too
  uint32_t done;        // that write returned before the reset
};
__attribute__((section(".noinit"))) State st;
const uint32_t STATE_MAGIC = 0x57445253;

uint32_t state_sum() {
  const uint32_t *w = (const uint32_t *)&st;
  uint32_t s = 0x12345678;
  for (size_t k = 0; k < offsetof(State, sum) / 4; k++)
    s = (s ^ w[k]) * 16777619;
  return s;
}
void state_save() { st.sum = state_sum(); }
bool state_valid() { return st.magic == STATE_MAGIC && st.sum == state_sum(); }

// ---- the watchdog ----
void wdt_clock_on() {
#if defined(FRDM_MCXA153)
  CLOCK_SetClockDiv(kCLOCK_DivWWDT0, 1U);
  CLOCK_EnableClock(kCLOCK_GateWWDT0);
#else
  SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_FRO1MHZ_ENA_MASK | SYSCON_CLOCK_CTRL_FRO1MHZ_CLK_ENA_MASK;
  CLOCK_SetClkDiv(kCLOCK_DivWdt0Clk, 1U);
  CLOCK_EnableClock(kCLOCK_Wwdt0);
#endif
}

// Whether the last reset came from the watchdog. The WWDT's own WDTOF flag
// doesn't tell: the reset it causes clears it. The CMC's reset status does
bool reset_by_watchdog() {
#if defined(FRDM_MCXA153)
  return CMC->SRS & CMC_SRS_WWDT0_MASK;
#else
  return CMC0->SRS & CMC_SRS_WWDT0_MASK;
#endif
}

// Start it; it cannot be stopped again short of a reset. TV reads the old
// value for a moment after the feed, so give it time before reading it
void wdt_start(uint32_t ticks) {
  WWDT0->TC = ticks;
  WWDT0->MOD = WWDT_MOD_WDEN_MASK | WWDT_MOD_WDRESET_MASK;
  __disable_irq();
  WWDT0->FEED = 0xAA;
  WWDT0->FEED = 0x55;
  __enable_irq();
  delay(1);
}

// ---- checking what survived ----
int failures = 0;
void fail(const char *what, long a = -1, long b = -1) {
  Serial.print("FAIL: ");
  Serial.print(what);
  if (a >= 0) {
    Serial.print(" ");
    Serial.print(a);
  }
  if (b >= 0) {
    Serial.print(" ");
    Serial.print(b);
  }
  Serial.println();
  failures++;
}

// The highest slot value if the contents are consistent, or NONE
uint32_t check_contents() {
  uint32_t mark;
  EEPROM.get(0, mark);
  if (mark != INIT_MARK)
    return NONE;

  for (int a = 4; a < 1024; a++)
    if (!is_slot_byte(a) && EEPROM.read(a) != pattern(a)) {
      fail("pattern byte changed at address/value", a, EEPROM.read(a));
      return NONE;
    }

  uint32_t v[N], m = 0;
  for (int j = 0; j < N; j++) {
    EEPROM.get(slot_addr(j), v[j]);
    if (v[j] > m)
      m = v[j];
  }
  for (int j = 0; j < N; j++)
    if (v[j] % N != (uint32_t)j || v[j] + N <= m) {
      fail("slots not consecutive: slot/value", j, v[j]);
      return NONE;
    }
  return m;
}

void initialize() {
  Serial.println("writing the pattern and slots");
  for (int a = 4; a < 1024; a++)
    if (!is_slot_byte(a))
      EEPROM.update(a, pattern(a));
  for (int j = 0; j < N; j++)
    EEPROM.put(slot_addr(j), (uint32_t)j);
  EEPROM.put(0, INIT_MARK);
}

void write_value(uint32_t i) {
  EEPROM.put(slot_addr(i % N), i);
}

void summary() {
  Serial.print("trials ");
  Serial.print(st.trials);
  Serial.print(", cut mid-write ");
  Serial.print(st.cut);
  Serial.print(" (compacting ");
  Serial.print(st.cut_compacting);
  Serial.print("): landed ");
  Serial.print(st.landed);
  Serial.print(", rolled back ");
  Serial.print(st.rolled_back);
  Serial.print("; boots finding unreadable flash ");
  Serial.print(st.unreadable_boots);
  Serial.print("; failures ");
  Serial.println(st.fails);
}

void setup() {
  Serial.begin(115200);
  wdt_clock_on();
  bool by_watchdog = reset_by_watchdog();

  if (!by_watchdog || !state_valid()) {
    delay(500);  // time to open the Serial Monitor
    Serial.println();
    Serial.print("EEPROM reset-during-write test (a new run; SRS=0x");
#if defined(FRDM_MCXA153)
    Serial.print(CMC->SRS, HEX);
#else
    Serial.print(CMC0->SRS, HEX);
#endif
    Serial.print(by_watchdog ? ", by watchdog" : "");
    Serial.println(state_valid() ? ")" : ", state lost)");
    memset(&st, 0, sizeof st);
    st.magic = STATE_MAGIC;
    st.armed = NONE;
    state_save();
  }

  int bad = unreadable_phrases();
  if (bad) {
    st.unreadable_boots++;
    Serial.print("unreadable 16-byte phrases in the area: ");
    Serial.println(bad);
  }

  uint32_t m = check_contents();
  if (m == NONE && failures == 0) {
    initialize();
    m = check_contents();
    if (m == NONE)
      fail("contents wrong right after writing them");
    st.armed = NONE;
  }

  // What happened to the write the reset was aimed at
  if (st.armed != NONE && m != NONE) {
    uint32_t i = st.armed;
    st.trials++;
    Serial.print("#");
    Serial.print(st.trials);
    Serial.print(st.compacting ? " compacting" : " appending");
    Serial.print(" x=");
    Serial.print(st.x_us);
    Serial.print("us: ");
    if (st.done) {
      Serial.print("write finished first, ");
      if (m != i)
        fail("finished write missing: expected/got", i, m);
      else
        Serial.println("there");
    } else {
      st.cut++;
      if (st.compacting)
        st.cut_compacting++;
      Serial.print("cut, ");
      if (m == i) {
        st.landed++;
        Serial.println("landed");
      } else if (m + 1 == i) {
        st.rolled_back++;
        Serial.println("rolled back");
      } else {
        fail("cut write: expected value/got", i, m);
      }
    }
  }
  if (m == NONE && st.armed != NONE)
    st.trials++;
  st.fails += failures;
  st.armed = NONE;
  state_save();

  if (failures || st.trials >= TRIALS) {
    summary();
    Serial.println(st.fails ? "FAILED" : "ALL OK");
    return;  // no watchdog: the board stays put
  }

  // The next trial
  uint32_t i = m + 1;
  // Seeded by the trial count too: a rolled-back write leaves i as it was
  uint32_t r = (i + st.trials * 40503u) * 2654435761u;
  r ^= r >> 15;
  r *= 2246822519u;
  r ^= r >> 13;
  bool compacting = (r >> 7) & 1;
  if (compacting) {
    int f = log_free();
    for (; f > 0; f--)
      write_value(i++);
    if (log_free() != 0)
      fail("log not full before the compacting write");
  }
  uint32_t x = r % (compacting ? COMPACT_US : APPEND_US);
  if (st.trials % 50 == 0)
    summary();
  Serial.flush();

  // Measure the watchdog's tick, then write when x us are left
  wdt_start(20000);
  uint32_t t0 = micros(), tv0 = WWDT0->TV;
  while (micros() - t0 < 2000)
    ;
  uint32_t tv1 = WWDT0->TV;
  uint32_t t1 = micros();
  if (tv0 <= tv1) {
    Serial.println("FAIL: the watchdog is not counting");
    return;
  }
  float us_per_tick = (float)(t1 - t0) / (tv0 - tv1);
  uint32_t at = (uint32_t)(x / us_per_tick);

  st.armed = i;
  st.done = 0;
  st.compacting = compacting;
  st.x_us = x;
  state_save();
  while (WWDT0->TV > at)
    ;
  write_value(i);
  st.done = 1;
  while (true)
    ;  // until the watchdog resets
}

void loop() {
}
