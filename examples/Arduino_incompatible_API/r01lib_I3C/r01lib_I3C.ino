/*
 * Since this Arduino variant is including "r01lib" layer as its base class,
 * user can use r01lib APIs.
 * One thing that user should care is some definitions like pin name r01lib/soure/io.h is
 * over-ridden by arduino_io.h. The pins should be referenced like P3_13.
 * This is a sample of the using DigitalOut class.
 *
 * I3C_SDA/I3C_SCL themselves are one of the names arduino_io.h renumbers, so
 * under <Arduino.h> they resolve to small Arduino pin numbers, not the raw
 * r01lib pin values I3C's constructor checks against internally -- passing
 * them here would panic() at construction. Use the raw physical pin names
 * instead (I3C_SDA/I3C_SCL's r01lib definition for each board: P0_16/P0_17
 * on FRDM-MCXA153, P1_16/P1_17 on FRDM-MCXN947).
 */

#include <Arduino.h>

#if defined(FRDM_MCXN947)
I3C i3c(P1_16, P1_17);  //	SDA, SCL (== I3C_SDA/I3C_SCL's raw r01lib pins on N947)
#elif defined(FRDM_MCXA153)
I3C i3c(P0_16, P0_17);  //	SDA, SCL (== I3C_SDA/I3C_SCL's raw r01lib pins on A153)
#else
#error This example I3C pins have only been worked out for FRDM-MCXA153 and FRDM-MCXN947 so far
#endif

constexpr uint8_t static_address = 0x48;
constexpr uint8_t dynamic_address = 0x08;
uint8_t w_data[] = { 0 };
uint8_t r_data[2];


int main(void) {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.printf("P3T1755 basic operation sample\r\n");

  i3c.ccc_broadcast(CCC::BROADCAST_RSTDAA, NULL, 0);                       // Reset DAA
  i3c.ccc_set(CCC::DIRECT_SETDASA, static_address, dynamic_address << 1);  // Set Dynamic Address from Static Address

  while (true) {
    i3c.write(dynamic_address, w_data, sizeof(w_data), I3C::NO_STOP);
    i3c.read(dynamic_address, r_data, sizeof(r_data));

    Serial.printf("%f\r\n", (((int)r_data[0]) << 8 | r_data[1]) / 256.0);
    wait(1);
  }
}
