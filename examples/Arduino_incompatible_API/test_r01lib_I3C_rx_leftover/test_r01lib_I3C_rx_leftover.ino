/*
 * test_r01lib_I3C_rx_leftover: r01lib's I3C must not hang on a byte left in
 * the controller's receive FIFO
 *
 * r01lib_I3C_demo used to stop after a few minutes on an FRDM-MCXN947 with
 * three P3T1755s sending IBIs: a read came back right but left one more byte
 * in the receive FIFO. The SDK's I3C_MasterTransferBlocking() turns the
 * controller's interrupts back on at its end, RXPEND among them, and the
 * SDK's interrupt handler reads nothing while no transfer of its own is
 * going on, so the interrupt came straight back without end and loop()
 * never ran again. r01lib's I3C (i3c.cpp, master_irq_handler()) now throws
 * such a byte away from the interrupt.
 *
 * That leftover byte came only now and then. This sketch leaves one there on
 * purpose, 20 times: it starts a 3-byte read of the sensor's temperature,
 * takes 2 bytes, stops, and turns on the interrupts as
 * I3C_MasterTransferBlocking() does. Each round passes if the FIFO is empty
 * 5 ms later and an ordinary read of the sensor then works. It ends with
 * "ALL OK" or "FAIL".
 *
 * If the fix is gone, the sketch hangs right after the header line, with
 * no round printed: that is a failure too.
 *
 * It uses the SDK's I3C functions and registers directly, so it belongs
 * here and not in Arduino_compatible_API.
 *
 * Wiring: none on FRDM-MCXA153, FRDM-MCXN947 and FRDM-MCXA156 (the on-board
 * P3T1755). FRDM-MCXN236 has no I3C sensor on board: connect one to D18/D19.
 * An FRDM-MCXN947's on-board one does: D18 to its MB_RX, D19 to MB_TX, GND
 * to GND, with the FRDM-MCXN947 running a sketch that leaves those pins
 * alone (hello_world).
 * FRDM-MCXC444 has no I3C: there the sketch only says so.
 *
 * The sensor keeps the dynamic address DAA gives it until it loses power, so
 * a sketch that reads it at its static address 0x48 afterwards needs the
 * board unplugged and plugged back in (see r01lib_I3C_demo).
 */

#include <Arduino.h>

#if defined(FRDM_MCXC444)
void setup() {
  Serial.begin(115200);
  Serial.println("FRDM-MCXC444 has no I3C");
}

void loop() {
}
#else

extern "C" {
#include "fsl_i3c.h"
}

#if defined(FRDM_MCXN947) || defined(FRDM_MCXN236)
I3C_Type *const BUS = I3C1;
#elif defined(FRDM_MCXA153) || defined(FRDM_MCXA156)
I3C_Type *const BUS = I3C0;
#else
#error "This sketch has no settings for this board yet"
#endif

I3C i3c(I3C_SDA, I3C_SCL);  //	installs r01lib's interrupt handler

const uint8_t dynamic_address_list[] = { 0x1A, 0x2B, 0x3C };
const int ROUNDS = 20;

uint8_t target = 0;
int round_no = 0;
int fails = 0;

static uint32_t rx_count(void) {
  return (BUS->MDATACTRL & I3C_MDATACTRL_RXCOUNT_MASK) >> I3C_MDATACTRL_RXCOUNT_SHIFT;
}

static void wait_until(bool (*done)(void), uint32_t us) {
  uint32_t t0 = micros();
  while (!done() && (micros() - t0) < us)
    ;
}

static bool three_bytes_in(void) {
  return rx_count() >= 3;
}

static bool bus_idle(void) {
  return I3C_MasterGetState(BUS) == kI3C_MasterStateIdle;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.printf("\r\ntest_r01lib_I3C_rx_leftover\r\n");

  i3c_device_info_t *list;
  i3c.ccc_broadcast(CCC::BROADCAST_RSTDAA, NULL, 0);
  int n = i3c.DAA(dynamic_address_list, sizeof(dynamic_address_list), &list);

  if (n < 1) {
    Serial.printf("FAIL: DAA found no I3C target (unplug and plug the board back in, if a sketch read the sensor at 0x48 before)\r\n");
    return;
  }

  target = list[0].dynamicAddr;
  Serial.printf("target at 0x%02X\r\n", target);
}

void loop() {
  if (!target || round_no > ROUNDS)
    return;

  if (round_no == ROUNDS) {
    Serial.printf("%s: %d rounds, %d failed\r\n", fails ? "FAIL" : "ALL OK", ROUNDS, fails);
    round_no++;
    return;
  }

  round_no++;

  uint8_t reg = 0;  //	the temperature register
  i3c.write(target, &reg, 1);

  //	A 3-byte read, of which only 2 bytes are taken
  BUS->MINTCLR = 0xFFFFFFFF;
  I3C_MasterClearStatusFlags(BUS, 0xFFFFFFFF);
  I3C_MasterStartWithRxSize(BUS, kI3C_TypeI3CSdr, target, kI3C_Read, 3);
  wait_until(three_bytes_in, 10000);
  uint8_t b0 = BUS->MRDATAB;
  uint8_t b1 = BUS->MRDATAB;
  I3C_MasterStop(BUS);
  wait_until(bus_idle, 10000);
  uint32_t left = rx_count();

  //	What I3C_MasterTransferBlocking() turns on at its end, RXPEND among them
  BUS->MINTSET = I3C_MINTSET_RXPEND_MASK | I3C_MINTSET_MCTRLDONE_MASK | I3C_MINTSET_COMPLETE_MASK
                 | I3C_MINTSET_SLVSTART_MASK | I3C_MINTSET_IBIWON_MASK | I3C_MINTSET_ERRWARN_MASK;
  delay(5);
  uint32_t after = rx_count();

  uint8_t t[2] = { 0, 0 };
  i3c.write(target, &reg, 1, false);
  status_t r = i3c.read(target, t, 2);

  bool ok = (left == 1) && (after == 0) && (r == kStatus_Success);
  if (!ok)
    fails++;

  Serial.printf("%2d %s: took %02X %02X, left %lu byte(s), %lu after 5 ms, then read status %ld: %.4f C\r\n",
                round_no, ok ? "OK  " : "FAIL", b0, b1, (unsigned long)left, (unsigned long)after, (long)r,
                (int16_t)(t[0] << 8 | t[1]) / 256.0);
  delay(200);
}
#endif
