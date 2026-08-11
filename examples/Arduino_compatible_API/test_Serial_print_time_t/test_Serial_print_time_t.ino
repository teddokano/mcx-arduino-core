#include <Arduino.h>
#include <time.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  time_t current_time = 1734567890;  // arbitrary fixed value, no RTC needed for this test
  Serial.print(current_time);
  Serial.println();

  long long ll_val = 123456789012345LL;
  Serial.println(ll_val);

  unsigned long long ull_val = 18446744073709551615ULL;  // ULLONG_MAX
  Serial.println(ull_val);
}

void loop() {
}
