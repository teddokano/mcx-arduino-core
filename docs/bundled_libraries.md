# Bundled libraries

English | [日本語](bundled_libraries.ja.md)

mcx-arduino-core comes with eight libraries. They install with the board package, so
there is nothing to add in the Library Manager: `#include` the header and use it. This
guide picks up where the [tutorial](../TUTORIAL.md) ends. It says what each library is
for, shows a short sketch, and lists what to know on these boards. The full API of
each is in [API_COMPATIBILITY.md](../API_COMPATIBILITY.md) and the library's own README,
and every library's examples are under **File → Examples** in the IDE, in the
library's name.

`Servo`, `MsTimer2` and `TimerOne` have the same names and interfaces as the
libraries sketches for UNO already use, so those sketches build unchanged. If one of
those libraries is installed from the Library Manager too, the build picks the
bundled one, which is the one written for these boards.

## Which one to use

| To do this | Use | Header |
|---|---|---|
| Call a function every given time, several of them, or once after a delay | `mcxTicker` | `<mcxTicker.h>` |
| Call one function every few milliseconds, as an AVR sketch with MsTimer2 does | `MsTimer2` | `<MsTimer2.h>` |
| Call one function at a period finer than a millisecond | `TimerOne` | `<TimerOne.h>` |
| Turn a hobby RC servo, from any digital pin | `Servo` | `<Servo.h>` |
| Turn an SG90 or a continuous-rotation FS90R, in degrees or speed | `mcxRCServo` | `<SG90.h>`, `<FS90R.h>` |
| Keep settings or counts across resets and uploads | `EEPROM` | `<EEPROM.h>` |
| Use FRDM-MCXN947's on-board Ethernet | `MCXEthernet` | `<MCXEthernet.h>` |
| Find what holds a pin, when two features fight over one | `mcxPinState` | `<PinState.h>` |

## Calling a function on a timer

The three timer libraries call a function of yours from an interrupt. That function
cuts into `loop()` wherever it happens to be, so keep it short: set a flag, count, or
toggle a pin, and leave the slow work (printing a lot, `delay()`, talking to a sensor)
to `loop()`. A variable that both sides use should be `volatile`. The functions run at
the lowest interrupt priority, so `Serial` and the other interrupts still work while
one runs.

### `mcxTicker`: any number of functions

Each `mcxTicker` object calls its own function, either every given time (`attach()`
in seconds, `attach_ms()` in milliseconds) or once after it (`once()`, `once_ms()`).
Make as many objects as you need:

```cpp
#include <Arduino.h>
#include <mcxTicker.h>

mcxTicker blinker;
mcxTicker reporter;
volatile bool report_due = false;

void blink() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void report() {
  report_due = true;  // printing is left to loop()
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  blinker.attach_ms(250, blink);
  reporter.attach(2.0, report);
}

void loop() {
  if (report_due) {
    report_due = false;
    Serial.println(millis());
  }
}
```

It runs on the core's 1 ms tick, the one `millis()` counts, so it takes no timer and
no pin, and times are whole milliseconds. `detach()` stops an object, and `active()`
tells whether it will still call. Each method also comes in a form that passes one
argument to the function (`blinker.attach_ms(250, toggle, RED)` calls `toggle(RED)`).
This is the interface of the `Ticker` in the ESP32 and ESP8266 cores, under the class
name `mcxTicker`: a sketch written for one of those moves here by changing the
`#include` and the class name. Examples: `Blink`, `ArgumentAndOnce`.

### `MsTimer2`: one function, every given milliseconds

```cpp
#include <Arduino.h>
#include <MsTimer2.h>

void blink() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  MsTimer2::set(500, blink);  // every 500 ms
  MsTimer2::start();
}

void loop() {
}
```

The interface of the MsTimer2 library for AVR, on the same 1 ms tick as `mcxTicker`,
so it too takes no timer and no pin. Use it when a sketch written for UNO uses it;
for new sketches `mcxTicker` does the same and more. Examples: `BlinkEvery500ms`, and
`MultipleTickers`, which builds several periods on its one timer.

### `TimerOne`: one function, every given microseconds

```cpp
#include <Arduino.h>
#include <TimerOne.h>

void blink() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Timer1.initialize(150000);  // every 150000 us
  Timer1.attachInterrupt(blink);
}

void loop() {
}
```

The interface of the TimerOne library for AVR, for periods finer than a millisecond
or not a whole number of them. It takes CTIMER1 on the MCX boards and channel 1 of
the PIT on FRDM-MCXC444. Its PWM functions (`pwm()`, `setPwmDuty()`, `disablePwm()`)
are not there and stop the build: use `analogWrite()` and `analogWriteFrequency()`
instead. Example: `BlinkEvery150ms`.

## Servos

### `Servo`: the official library's interface

```cpp
#include <Arduino.h>
#include <Servo.h>

Servo servo;

void setup() {
  servo.attach(D9);
}

void loop() {
  servo.write(0);    // degrees, 0 to 180
  delay(1000);
  servo.write(180);
  delay(1000);
}
```

Any digital pin works, up to 12 servos. On a PWM pin the PWM hardware makes the
pulses, steady to the clock; on another pin a timer interrupt raises and lowers the
pin, as the official library does on UNO, and other interrupts can move a pulse
edge by a few microseconds. Which pins have PWM is in each board's pin mapping
(`PIN_MAPPING_*.md`). A PWM pin's partner in the same FlexPWM submodule runs at the
servo's 50 Hz too, if you `analogWrite()` it. The timer for the other pins is CTIMER2,
CTIMER3 on FRDM-MCXN947 (where `MCXEthernet` uses CTIMER2), and TPM2 on FRDM-MCXC444;
it runs only while such a servo is attached. Examples: `Sweep`, and `Knob`, which
follows a potentiometer on `A2`.

Power the servo from its own 5 V supply, with its ground tied to the board's, when
it is larger than an SG90 or when several move at once: a servo starting up can pull
more current than the board's USB can give.

### `mcxRCServo`: SG90 and FS90R

```cpp
#include <Arduino.h>
#include <SG90.h>

#if defined(FRDM_MCXC444)
#define SERVO_PIN D3  // FRDM-MCXC444 has no PWM0
#else
#define SERVO_PIN PWM0
#endif

SG90 servo(SERVO_PIN);

void setup() {
  servo.range(-90, 90);  // work in degrees, centered on 0
}

void loop() {
  servo.position(90);
  delay(500);
  servo.position(-90);
  delay(500);
}
```

Classes for two servos, with each one's pulse timing from its datasheet: `SG90`
(positional) and `FS90R` (continuous rotation, `speed(-1.0 ... 1.0)` and `stop()`).
It needs a PWM pin, and sets that pin's FlexPWM submodule (the TPM on FRDM-MCXC444) to
50 Hz. See its [README](../hardware/nxp/mcx/libraries/mcxRCServo/README.md) for the
pins on each board. Examples: `SG90_basic`, `SG90_moves`, `FS90R_rotate`.

## `EEPROM`: data that outlives a reset

1024 bytes with the interface of the AVR EEPROM library (`read()`, `write()`,
`update()`, `get()`, `put()`, `EEPROM[i]`), kept in the top of the on-chip flash
across resets, power cycles and uploads. The tutorial's
[2.14](../TUTORIAL.md#214-keeping-data-across-resets-eeprom) has a sketch that counts
how many times the board has started, and what a write costs. Write when something
changes, not on every pass through `loop()`. Example: `EEPROM_settings`.

## `MCXEthernet`: FRDM-MCXN947's on-board Ethernet

The official Ethernet library's classes (`Ethernet`, `EthernetClient`,
`EthernetServer`, `EthernetUDP`) on the board's own RJ45 port, through lwIP:
`#include <MCXEthernet.h>` in place of `<Ethernet.h>`, and the rest of a sketch for the
official library stays as it is. FRDM-MCXN947 only. On any of the five boards, the
official Ethernet library works with a WIZnet W5500 module instead. The tutorial's
[2.16](../TUTORIAL.md#216-ethernet) starts there, and the library's
[README](../hardware/nxp/mcx/libraries/MCXEthernet/README.md) has what is supported
and what is not (no TLS, IPv6 or Wi-Fi). Examples: `DhcpAddress_N947`,
`WebServerLed_N947`, `UdpEcho_N947`, `HttpGet_N947`, `WebTemperature_N947`,
`WebButtonsRgb_N947`.

## `mcxPinState`: who holds this pin?

When two features want the same pin, or a pin stops doing what you set it to,
this prints each pin's settings and the kind of object that holds it, and the pins
each `Serial`, `Wire` and `SPI` is on:

```cpp
#include <Arduino.h>
#include <PinState.h>

PinState pins;  // tracking starts when this is made

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  pinMode(LED_BUILTIN, OUTPUT);
  Wire.begin();
  pins.print();  // the pins, then the instances
}

void loop() {
}
```

A tool for debugging, not for a finished sketch. The
[mcxPinState guide](mcxpinstate_guide.md) explains how to read the table and a
conflict. Examples: `BasicPinDump`, `ConflictDemo`, `PullModeCheck`,
`MultiPeripheralDump`, `CombinedPeripheralsAudit`.

## Which timer each feature takes

Two features that want the same timer can't both run. The bundled libraries and the
core are set up so that none of them share one:

| Feature | MCX boards | FRDM-MCXC444 |
|---|---|---|
| `millis()`, `delay()`, `MsTimer2`, `mcxTicker` | SysTick | SysTick |
| `tone()` | CTIMER0 | PIT channel 0 |
| `TimerOne` | CTIMER1 | PIT channel 1 |
| `Servo` on a pin without PWM | CTIMER2 (CTIMER3 on FRDM-MCXN947) | TPM2 |
| `MCXEthernet` (FRDM-MCXN947) | CTIMER2 | — |
| `analogWrite()`, `Servo` and `mcxRCServo` on a PWM pin | FlexPWM | TPM0, TPM1 |

## See also

- [API_COMPATIBILITY.md](../API_COMPATIBILITY.md): every function of these libraries and its status
- [TUTORIAL.md](../TUTORIAL.md): the Arduino API on these boards, step by step
- `PIN_MAPPING_*.md` at the top of the repository: the pins of each board, and which have PWM
