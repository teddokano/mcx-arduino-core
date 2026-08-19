/*
 * Since this Arduino variant is including "r01lib" layer as its base class,
 * user can use r01lib APIs.
 * One thing that user should care is some definitions like pin name r01lib/soure/io.h is
 * over-ridden by arduino_io.h. The pins should be referenced like P3_13.
 * This is a sample of the using DigitalOut class.
 *
 * I3C_SDA/I3C_SCL are safe to use directly here: unlike D0-D19/A0-A5/MB_*,
 * these names are deliberately excluded from arduino_io.h's pin
 * renumbering, so they always mean the same raw r01lib value whether or
 * not <Arduino.h> has been included -- no need to spell out the physical
 * pin per board.
 */

#include <Arduino.h>

I3C i3c(I3C_SDA, I3C_SCL);  //	SDA, SCL

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
