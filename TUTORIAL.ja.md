# mcx-arduino-core 入門チュートリアル（v0.2.0）

NXP FRDM-MCXA153ボードでArduino APIを使うためのハンズオンガイドです。インストールから、v0.2.0で対応している各種ペリフェラルまで一通り扱います。各セクションはそのまま書き込んで動く完結したスケッチです。API対応表・ピン配置の詳細は[README.md](README.md)、バージョン間の変更点は[CHANGELOG.md](CHANGELOG.md)を参照してください。

English version → [TUTORIAL.md](TUTORIAL.md)

## 目次

- [1. インストール](#1-インストール)
  - [1.1. 用意するもの](#11-用意するもの)
  - [1.2. Arduino IDEの入手](#12-arduino-ideの入手)
  - [1.3. NXP LinkServerのインストール](#13-nxp-linkserverのインストール)
  - [1.4. どちらのUSBコネクタを使うか](#14-どちらのusbコネクタを使うか)
  - [1.5. ボードパッケージのインストール](#15-ボードパッケージのインストール)
  - [1.6. Arduino IDEツールバーの基本](#16-arduino-ideツールバーの基本)
- [2. 動作確認](#2-動作確認)
  - [2.1. 最初のスケッチ: オンボードLEDを点滅させる](#21-最初のスケッチ-オンボードledを点滅させる)
  - [2.2. シリアル出力](#22-シリアル出力)
  - [2.3. デジタル入力と割り込み](#23-デジタル入力と割り込み)
  - [2.4. アナログ入力: `analogRead`](#24-アナログ入力-analogread)
  - [2.5. PWM出力: `analogWrite`](#25-pwm出力-analogwrite)
  - [2.6. 時間計測: `millis` / `micros`](#26-時間計測-millis--micros)
  - [2.7. 音: `tone` / `noTone`](#27-音-tone--notone)
  - [2.8. I2C: `Wire`とオンボードセンサー（`Wire1`）](#28-i2c-wireとオンボードセンサーwire1)
  - [2.9. SPI](#29-spi)
  - [2.10. 2つ目のシリアルポート: `Serial1`](#210-2つ目のシリアルポート-serial1)
  - [2.11. ソフトウェア実装のヘルパー: `shiftOut` / `shiftIn` / `pulseIn`](#211-ソフトウェア実装のヘルパー-shiftout--shiftin--pulsein)
  - [2.12. UNO R3/R4互換性](#212-uno-r3r4互換性)
- [次に見るべきもの](#次に見るべきもの)

## 1. インストール

### 1.1. 用意するもの

- [FRDM-MCXA153](https://www.nxp.com/design/design-center/development-boards-and-designs/FRDM-MCXA153)ボード
- Arduino IDE 2.x
- NXP LinkServer
- USB-Cケーブル

このチュートリアルのほとんどの内容は外部部品なしで試せます。ボード上にLED・ボタン・温度センサーが搭載されているためです。

### 1.2. Arduino IDEの入手

**[arduino.cc/en/software](https://www.arduino.cc/en/software)**からお使いのOS用インストーラをダウンロードしてインストールしてください。**2.x系**の最新版であれば問題ありません（本ガイドは旧版の1.8.x系IDEには対応していません）。

### 1.3. NXP LinkServerのインストール

このボードへの書き込みにはArduino標準の書き込みツールではなく**NXP LinkServer**を使用するため、書き込みを試す*前に*別途インストールしておく必要があります。

👉 ダウンロード: [nxp.com/linkserver](https://www.nxp.com/linkserver)

| OS | インストーラ |
|----|-----------|
| macOS | `.pkg`ファイル、ダブルクリックでインストール |
| Windows | `.exe`インストーラ |
| Linux | `.deb.bin`ファイル |

インストール後は、ボードパッケージの書き込みスクリプトが自動的にLinkServerを検出します — パス設定は不要です。

### 1.4. どちらのUSBコネクタを使うか

このボードには**2つ**のUSB-Cコネクタがあり、間違った方に挿すとIDEに何も表示されません。**"MCU-Link USB"**とシルク印刷されたコネクタ（**J15**）を使ってください。このポートがオンボードのデバッグプローブで、LinkServerでの書き込み・ボードへの給電・`Serial`（USB経由のシリアルポート）の出力先すべてを兼ねています。

もう一方の**"MCU USB"**（**J8**）というコネクタは使わないでください。こちらはMCXA153自体のUSBペリフェラルに直結されており、このチュートリアルではこのボードパッケージ側で使用しません。

### 1.5. ボードパッケージのインストール

1. Arduino IDEを開き、**File → Preferences**
2. **Additional boards manager URLs**に以下を追加:
   ```
   https://raw.githubusercontent.com/teddokano/mcx-arduino-core/main/package_nxp_mcx_index.json
   ```
3. **Tools → Board → Boards Manager**で`NXP MCX`を検索し**Install**
4. **Tools → Board**で**FRDM-MCXA153 (NXP Cortex-M33)**を選択
5. ボードを**MCU-Link USB（J15）**コネクタに接続し、**Tools → Port**でポートを選択

詳細は[README.md](README.md#installation)を参照してください。

### 1.6. Arduino IDEツールバーの基本

スケッチウィンドウ上部のツールバーには、頻繁に使うアイコンがいくつかあります:

| アイコン | 機能 |
|---|---|
| ✔（チェックマーク） | **Verify（検証）** — 書き込みせずにコンパイルのみ行う。エラーを素早く確認するのに便利 |
| →（右向き矢印） | **Upload（アップロード）** — コンパイル**して**ボードへ書き込む（LinkServer経由、MCU-Link USBコネクタ越し） |
| 🔌/モニターアイコン（右上） | **Serial Monitor（シリアルモニタ）** — スケッチが`Serial.print`/`println`で送った内容を表示するパネルを開く |

コンパイルエラーやアップロードのログは、ウィンドウ下部の黒い出力ペインに表示されます — アップロードに失敗したら、まずここを確認してください。

このチュートリアルの全スケッチは`Serial.begin(...)`を呼んでいるので、**Upload**をクリックした後にシリアルモニタのアイコンを開き、そのボーレート（シリアルモニタパネル右下）がスケッチの`Serial.begin()`の値（本チュートリアルの大半は115200）と一致しているか確認してください — 一致していないと文字化けするか、何も表示されません。

## 2. 動作確認

### 2.1. 最初のスケッチ: オンボードLEDを点滅させる

ボードには3色のオンボードLED（`RED`, `GREEN`, `BLUE`）があり、**アクティブLow**で配線されています —— `LOW`で点灯、`HIGH`で消灯します。`LED_BUILTIN`は`GREEN`のエイリアスです。

```cpp
#include <Arduino.h>

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, LOW);   // 点灯
  delay(500);
  digitalWrite(LED_BUILTIN, HIGH);  // 消灯
  delay(500);
}
```

**Upload**をクリックしてください。緑色LEDが1秒間隔で点滅します。

### 2.2. シリアル出力

`Serial`はUSB経由のシリアルポートです。`setup()`内の`while (!Serial);`は、シリアルモニタが接続されるまで待つことで、最初の数行が表示されずに失われるのを防ぎます。

```cpp
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Hello, world!");
}

void loop() {
}
```

書き込み後、**Tools → Serial Monitor**（115200bps）を開いてください。

### 2.3. デジタル入力と割り込み

ボードには`SW2`・`SW3`という2つのオンボードボタンがあり、アクティブLowでプルアップが必要（`INPUT_PULLUP`）です。以下の例は、ポーリングではなく`attachInterrupt`を使ってボタン押下で青LEDをトグルします:

```cpp
#include <Arduino.h>

volatile bool sw_pressed = false;
bool led_state = true;

void callback() {
  sw_pressed = true;
}

void setup() {
  Serial.begin(115200);

  pinMode(BLUE, OUTPUT);
  pinMode(SW2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SW2), callback, FALLING);

  digitalWrite(BLUE, led_state);
}

void loop() {
  if (sw_pressed) {
    sw_pressed = false;
    led_state = !led_state;
    digitalWrite(BLUE, led_state);
    Serial.println("SW2 pressed");
    delay(100);  // チャタリング防止
  }
}
```

### 2.4. アナログ入力: `analogRead`

`analogRead`はLPADC経由で`A0`-`A3`ピンを読み取り、従来のArduinoボードと同じ10bit値（0-1023）を返します。（`A4`/`A5`はピン名としては存在しますが、このボードではADCに配線されていません — 詳細は[ピン配置表](README.md#pin-mapping-frdm-mcxa153)を参照）

`A0`に可変抵抗（または0-3.3Vの任意のアナログ信号）を接続するか、未接続のままフローティングノイズを見てみてください:

```cpp
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
}

void loop() {
  int value = analogRead(A0);
  Serial.print("A0 = ");
  Serial.println(value);
  delay(200);
}
```

### 2.5. PWM出力: `analogWrite`

PWMは専用ピン`PWM0`-`PWM5`（FlexPWM0）でのみ使用可能で、任意のデジタルピンでは使えません。周期は1kHz固定で、`analogWrite`が制御できるのはduty比（0-255）のみです（従来のArduinoと同じ）。以下の例は2.4節のADC読み取り値をそのままPWM出力に反映します — 抵抗付きLEDやオシロスコープを`PWM0`に接続して変化を確認してください:

```cpp
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
}

void loop() {
  int value = analogRead(A0);
  analogWrite(PWM0, value >> 2);  // 10bit -> 8bit
  delay(200);
}
```

### 2.6. 時間計測: `millis` / `micros`

標準的なArduinoのタイミング関数で、SysTick（1msティック）+ DWTサイクルカウンタで実装されています。`millis()`は従来のArduinoと同様、約49日でオーバーフローします。

```cpp
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
}

void loop() {
  Serial.print("millis = ");
  Serial.print(millis());
  Serial.print("  micros = ");
  Serial.println(micros());
  delay(500);
}
```

### 2.7. 音: `tone` / `noTone`

`tone()`は`PWM0`-`PWM5`に限定される`analogWrite`と異なり、**任意の**デジタルピンで使用できます（CTIMER0によるソフトウェアトグル方式）。同時に鳴らせる音は1つだけです。圧電ブザーを`D13`とGND間に接続してください:

```cpp
#include <Arduino.h>

#define BUZZER_PIN D13

void setup() {
}

void loop() {
  tone(BUZZER_PIN, 440, 200);  // A4, 200ms
  delay(300);
  noTone(BUZZER_PIN);
  delay(700);
}
```

メロディーを鳴らす完全な例は
[`examples/Arduino_compatible_API/test_tone`](examples/Arduino_compatible_API/test_tone)を参照してください。

### 2.8. I2C: `Wire`とオンボードセンサー（`Wire1`）

ボードにはオンボードのP3T1755温度センサーがMCUのI3Cペリフェラルに接続されていますが、`Wire1`はこれを**I2C互換モード**で駆動するため、通常の`TwoWire`オブジェクトとして普通のI2C通信ができます（動的アドレッシングやIBIなどI3C固有の機能は一切使用しません）。この例には配線不要です:

```cpp
#include <Wire.h>
#include <P3T1755.h>   // Library Managerからインストール、または下記の例を参照

P3T1755 sensor(Wire1, 0x48);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Wire1.begin();
}

void loop() {
  Serial.println(sensor.temp(), 4);
  delay(1000);
}
```

完全なスケッチ:
[`examples/Arduino_compatible_API/onboard_temperature_sensor`](examples/Arduino_compatible_API/onboard_temperature_sensor)

**外部**のI2Cデバイスを使う場合は、通常の`Wire`オブジェクト（`SDA`/`SCL`は`D18`/`D19`）を使い、従来のArduinoと全く同じ`Wire.begin()` / `beginTransmission()` / `write()` / `endTransmission()` / `requestFrom()` / `read()`を呼び出してください。

### 2.9. SPI

`D10`(CS)/`D11`(MOSI)/`D12`(MISO)/`D13`(SCLK)で、標準的な`SPISettings`ベースのAPIが使えます:

```cpp
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  SPI.begin();
  pinMode(SS, OUTPUT);
}

void loop() {
  uint8_t data[] = { 0x00, 0x01, 0x02, 0x03 };

  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(SS, LOW);
  SPI.transfer(data, sizeof(data));
  digitalWrite(SS, HIGH);
  SPI.endTransaction();

  delay(1000);
}
```

応答を確認するには実際のSPIペリフェラル（またはMOSI-MISO間のループバック線）が必要です — 
[`examples/Arduino_compatible_API/test_SPI_loopback_with_a_wire`](examples/Arduino_compatible_API/test_SPI_loopback_with_a_wire)を参照してください。

### 2.10. 2つ目のシリアルポート: `Serial1`

`Serial`はUSB経由ですが、`Serial1`は`D0`(RX)/`D1`(TX)ピン上の独立したハードウェアUARTで、USB接続を占有せずに外部シリアルデバイスと通信できます:

```cpp
#include <Arduino.h>

void setup() {
  Serial1.begin(9600);
}

void loop() {
  Serial1.println("hello from Serial1");
  delay(1000);
}
```

他の外部ハードウェアなしで単体テストするには、`D1`と`D0`をジャンパー線でつなぎ、送信した内容を読み返してみてください —
[`examples/Arduino_compatible_API/test_Serial1`](examples/Arduino_compatible_API/test_Serial1)を参照。

### 2.11. ソフトウェア実装のヘルパー: `shiftOut` / `shiftIn` / `pulseIn`

従来のArduinoと同じシグネチャで、`digitalWrite`/`digitalRead`/`micros()`をベースにソフトウェアで実装されています:

```cpp
shiftOut(dataPin, clockPin, MSBFIRST, myByte);
uint8_t b = shiftIn(dataPin, clockPin, MSBFIRST);
unsigned long width = pulseIn(pin, HIGH);
```

`random()` / `randomSeed()`も、従来のArduinoと同じシグネチャで使用できます。
[`examples/Arduino_compatible_API/test_shiftOut_pulseIn_random`](examples/Arduino_compatible_API/test_shiftOut_pulseIn_random)を参照してください。

### 2.12. UNO R3/R4互換性

Arduino UNO向けに書かれたスケッチが、追加の`#include`なしでそのままコンパイルできます: `PI`, `HALF_PI`, `TWO_PI`, `DEG_TO_RAD`, `RAD_TO_DEG`, `radians()`, `degrees()`, `min()`/`max()`, `abs()`, `constrain()`, `sq()`, `map()`, `lowByte()`/`highByte()`, `bitRead()` / `bitSet()` / `bitClear()` / `bitToggle()` / `bitWrite()` / `bit()`, `interrupts()` / `noInterrupts()`, `boolean`/`byte`/`word`, `LSBFIRST`/`MSBFIRST`。

## 次に見るべきもの

- [README.md](README.md) — API対応表・ピン配置の全体像
- [`examples/Arduino_compatible_API/`](examples/Arduino_compatible_API) — 機能ごとの単体サンプル
- [`examples/Arduino_compatible_API/test_combined_peripherals`](examples/Arduino_compatible_API/test_combined_peripherals) — 全機能同時動作の例
- [CHANGELOG.md](CHANGELOG.md) — バージョン間の変更点
