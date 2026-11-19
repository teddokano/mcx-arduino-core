/*
 *  WatchdogRefresh: keep the watchdog fed, then stop feeding it.
 *
 *  The watchdog resets the board unless WDT.refresh() is called within its
 *  time (here 2 seconds). This sketch feeds it for 10 seconds, printing how
 *  much time was left each time, and then stops: the board resets about 2
 *  seconds later and the sketch starts again, saying so.
 *
 *  Open the Serial Monitor at 115200 baud.
 */

#include <WDT.h>
#include <avr/wdt.h>  // only for MCUSR, to tell a watchdog reset

const uint32_t WDT_MS = 2000;

unsigned long started;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  if (MCUSR & _BV(WDRF))
    Serial.println("Started again by the watchdog.");
  else
    Serial.println("Started.");

  if (!WDT.begin(WDT_MS)) {
    Serial.println("The watchdog couldn't take that time.");
    while (true)
      ;
  }

  Serial.print("Watchdog on, ");
  Serial.print(WDT.getTimeout());
  Serial.println(" ms.");
  started = millis();
}

void loop() {
  if (millis() - started < 10000) {
    Serial.print("Feeding it, ");
    Serial.print(WDT.getCounter());
    Serial.println(" ms were left.");
    WDT.refresh();
    delay(500);
  } else {
    Serial.println("Not feeding it any more: it resets the board in about 2 seconds.");
    while (true)
      ;
  }
}
