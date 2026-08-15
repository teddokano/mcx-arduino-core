/** Print::setWriteError()/getWriteError()/clearWriteError() test
 *
 *  Standard Arduino Print API for tracking whether a write() call failed --
 *  used internally by libraries that derive from Print (e.g. the official
 *  SD library's SdFile/File classes). No wiring needed; this only checks
 *  the class hierarchy and logic, not real hardware.
 */

#include <Arduino.h>

// A minimal Print-derived class that can simulate a failing write(),
// exactly the pattern SD's SdFile::write() uses internally.
class FlakyPrint : public Print {
public:
  size_t write(uint8_t c) override {
    if (fail) {
      setWriteError();
      return 0;
    }
    Serial.write(c);
    return 1;
  }

  bool fail = false;
};

FlakyPrint flaky;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Print write-error test");

  check("initial getWriteError() == 0", flaky.getWriteError() == 0);

  flaky.fail = true;
  flaky.print("this write should fail");
  Serial.println();

  check("getWriteError() != 0 after failed write", flaky.getWriteError() != 0);

  flaky.clearWriteError();
  check("getWriteError() == 0 after clearWriteError()", flaky.getWriteError() == 0);

  flaky.fail = false;
  flaky.println("this write should succeed and print above");
  check("getWriteError() == 0 after successful write", flaky.getWriteError() == 0);
}

void loop() {
}
