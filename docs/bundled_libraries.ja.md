# 同梱ライブラリ

[English](bundled_libraries.md) | 日本語

mcx-arduino-coreには8つのライブラリが付いています。ボードのパッケージと一緒に入るので、
ライブラリマネージャーで足すものはありません。ヘッダーを`#include`すれば使えます。この
案内は[チュートリアル](../TUTORIAL.ja.md)の続きです。それぞれのライブラリが何のためのもの
かと、短いスケッチ、このボードで知っておくことを書きます。APIの全体は
[API_COMPATIBILITY.md](../API_COMPATIBILITY.md)と各ライブラリのREADMEにあり、作例はどれも
IDEの**ファイル → スケッチ例**の、ライブラリの名前の下にあります。

`Servo`・`MsTimer2`・`TimerOne`は、UNO向けのスケッチが使っているライブラリと同じ名前・
同じインターフェースなので、そのスケッチはそのままビルドできます。同じ名前のライブラリを
ライブラリマネージャーで入れていても、ビルドはこのボード向けに書いた同梱のほうを選びます。

## どれを使うか

| したいこと | ライブラリ | ヘッダー |
|---|---|---|
| 一定時間ごとに関数を呼ぶ（いくつでも）、または一定時間のあとに1回呼ぶ | `mcxTicker` | `<mcxTicker.h>` |
| AVRのMsTimer2のスケッチのように、1つの関数を数ミリ秒ごとに呼ぶ | `MsTimer2` | `<MsTimer2.h>` |
| 1つの関数を1ミリ秒より細かい周期で呼ぶ | `TimerOne` | `<TimerOne.h>` |
| ホビー用のRCサーボを、どのデジタルピンからでも動かす | `Servo` | `<Servo.h>` |
| SG90や連続回転のFS90Rを、角度や速さで動かす | `mcxRCServo` | `<SG90.h>`、`<FS90R.h>` |
| 設定や回数を、リセットや書き込みをまたいで残す | `EEPROM` | `<EEPROM.h>` |
| FRDM-MCXN947のオンボードのEthernetを使う | `MCXEthernet` | `<MCXEthernet.h>` |
| 2つの機能が1本のピンを取り合うとき、何がピンを持っているかを調べる | `mcxPinState` | `<PinState.h>` |

## タイマーで関数を呼ぶ

3つのタイマーのライブラリは、スケッチの関数を割り込みから呼びます。その関数は`loop()`の
どこにでも割り込むので、短くしておきます。フラグを立てる、数える、ピンを切り替える、
くらいにして、時間のかかること（たくさんの表示、`delay()`、センサーとのやりとり）は
`loop()`に任せます。両方から使う変数は`volatile`にします。関数はいちばん低い割り込みの
優先度で動くので、その間も`Serial`やほかの割り込みは動きます。

### `mcxTicker`: いくつでも

`mcxTicker`のオブジェクトは、それぞれ自分の関数を、一定時間ごと（`attach()`は秒、
`attach_ms()`はミリ秒）か、一定時間のあとに1回（`once()`、`once_ms()`）呼びます。
オブジェクトは必要なだけ作れます:

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

コアの1msの刻み（`millis()`が数えるもの）で動くので、タイマーもピンも使わず、時間は
ミリ秒単位です。`detach()`で止め、`active()`でまだ呼ぶかが分かります。どのメソッドにも、
関数に引数を1つ渡す形があります（`blinker.attach_ms(250, toggle, RED)`は`toggle(RED)`を
呼ぶ）。ESP32・ESP8266のコアの`Ticker`と同じインターフェースで、クラスの名前が
`mcxTicker`です。それらのスケッチは、`#include`とクラスの名前を替えれば動きます。
作例: `Blink`、`ArgumentAndOnce`。

### `MsTimer2`: 1つの関数を、決まったミリ秒ごとに

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

AVRのMsTimer2ライブラリと同じインターフェースで、`mcxTicker`と同じ1msの刻みで動くので、
これもタイマーもピンも使いません。UNO向けのスケッチが使っているときに使います。新しく
書くスケッチなら、`mcxTicker`が同じことをもっと自由にできます。作例: `BlinkEvery500ms`と、
1つのタイマーでいくつもの周期を作る`MultipleTickers`。

### `TimerOne`: 1つの関数を、決まったマイクロ秒ごとに

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

AVRのTimerOneライブラリと同じインターフェースで、1ミリ秒より細かい周期や、ミリ秒の
整数でない周期に使います。MCXのボードではCTIMER1、FRDM-MCXC444ではPITのチャネル1を
使います。PWMの関数（`pwm()`、`setPwmDuty()`、`disablePwm()`）は無く、使うとビルドが
止まります。代わりに`analogWrite()`と`analogWriteFrequency()`を使います。作例:
`BlinkEvery150ms`。

## サーボ

### `Servo`: 公式のライブラリと同じインターフェース

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

どのデジタルピンでも使え、サーボは12個まで。PWMのピンではPWMのハードウェアがパルスを
作るので、クロックどおりにぶれません。ほかのピンでは、公式のライブラリがUNOでするように
タイマーの割り込みがピンを上げ下げするので、ほかの割り込みでパルスの端が数マイクロ秒
ずれることがあります。どのピンにPWMがあるかは、ボードごとのピン配置
（`PIN_MAPPING_*.md`）にあります。PWMのピンの、同じFlexPWMのサブモジュールの相方のピンは、
`analogWrite()`するとサーボと同じ50Hzになります。ほかのピンに使うタイマーはCTIMER2
（FRDM-MCXN947では`MCXEthernet`がCTIMER2を使うのでCTIMER3）、FRDM-MCXC444ではTPM2で、
そういうサーボが`attach()`されている間だけ動きます。作例: `Sweep`と、`A2`の
ポテンショメータに合わせて動く`Knob`。

SG90より大きいサーボや、いくつも同時に動かすときは、サーボを別の5Vの電源から動かし、
GNDをボードとつなぎます。動き出すサーボは、ボードのUSBが出せるより多くの電流を引くことが
あります。

### `mcxRCServo`: SG90とFS90R

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

2種類のサーボのクラスで、それぞれデータシートのパルスの幅を持っています。`SG90`
（角度）と`FS90R`（連続回転。`speed(-1.0 ... 1.0)`と`stop()`）。PWMのピンが要り、そのピンの
FlexPWMのサブモジュール（FRDM-MCXC444ではTPM）を50Hzにします。ボードごとのピンは
[README](../hardware/nxp/mcx/libraries/mcxRCServo/README.ja.md)にあります。作例:
`SG90_basic`、`SG90_moves`、`FS90R_rotate`。

## `EEPROM`: リセットしても消えないデータ

AVRのEEPROMライブラリと同じインターフェース（`read()`、`write()`、`update()`、`get()`、
`put()`、`EEPROM[i]`）の1024バイトで、オンチップのフラッシュの最後に置かれ、リセット・
電源の入れ直し・書き込みをまたいで残ります。チュートリアルの
[2.14](../TUTORIAL.ja.md#214-リセットしても消えないデータ-eeprom)に、ボードが起動した回数を
数えるスケッチと、書き込みにかかる時間があります。書くのは何かが変わったときにして、
`loop()`のたびには書きません。作例: `EEPROM_settings`。

## `MCXEthernet`: FRDM-MCXN947のオンボードのEthernet

公式のEthernetライブラリのクラス（`Ethernet`、`EthernetClient`、`EthernetServer`、
`EthernetUDP`）を、ボードのRJ45のコネクタでlwIPを通して使います。`<Ethernet.h>`の代わりに
`#include <MCXEthernet.h>`と書けば、公式のライブラリ向けのスケッチの残りはそのままです。
FRDM-MCXN947だけです。5つのどのボードでも、公式のEthernetライブラリとWIZnetのW5500の
モジュールでもEthernetを使えます。チュートリアルの
[2.16](../TUTORIAL.ja.md#216-ethernet)から始め、対応していること・いないこと（TLS、IPv6、
Wi-Fiは無い）はライブラリの[README](../hardware/nxp/mcx/libraries/MCXEthernet/README.md)
（英語）にあります。作例: `DhcpAddress_N947`、`WebServerLed_N947`、`UdpEcho_N947`、
`HttpGet_N947`、`WebTemperature_N947`、`WebButtonsRgb_N947`。

## `mcxPinState`: このピンを持っているのは何か

2つの機能が同じピンを使おうとしたときや、設定したとおりにピンが動かなくなったときに、
ピンごとの設定とピンを持っているオブジェクトの種類、それに`Serial`・`Wire`・`SPI`の
それぞれがどのピンにいるかを表示します:

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

デバッグのための道具で、仕上げたスケッチに残すものではありません。表と取り合いの
読み方は[mcxPinStateのガイド](mcxpinstate_guide.md)（英語）にあります。作例:
`BasicPinDump`、`ConflictDemo`、`PullModeCheck`、`MultiPeripheralDump`、
`CombinedPeripheralsAudit`。

## どの機能がどのタイマーを使うか

同じタイマーを使う2つの機能は、同時には動きません。同梱のライブラリとコアは、どれも
タイマーがぶつからないようにしてあります:

| 機能 | MCXのボード | FRDM-MCXC444 |
|---|---|---|
| `millis()`、`delay()`、`MsTimer2`、`mcxTicker` | SysTick | SysTick |
| `tone()` | CTIMER0 | PITのチャネル0 |
| `TimerOne` | CTIMER1 | PITのチャネル1 |
| PWMの無いピンの`Servo` | CTIMER2（FRDM-MCXN947はCTIMER3） | TPM2 |
| `MCXEthernet`（FRDM-MCXN947） | CTIMER2 | — |
| `analogWrite()`、PWMのピンの`Servo`・`mcxRCServo` | FlexPWM | TPM0、TPM1 |

## 関連する文書

- [API_COMPATIBILITY.md](../API_COMPATIBILITY.md): これらのライブラリのすべての関数と対応状況
- [TUTORIAL.ja.md](../TUTORIAL.ja.md): このボードでのArduino APIを順を追って
- リポジトリの直下の`PIN_MAPPING_*.md`: ボードごとのピンと、PWMのあるピン
