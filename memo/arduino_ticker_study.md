# Arduino で mbed の Ticker 相当を実現する検討

## 1. MsTimer2 で複数インスタンスは作れるか

**作れない。**

- MsTimer2 はクラスではなく `namespace MsTimer2` として実装されている。
- 中身は `set()` / `start()` / `stop()` の関数と、周期・コールバック用のグローバル変数1組のみ。
- `MsTimer2 t1, t2;` のようなインスタンス化は文法上書けない。
- AVR の Timer2 を1つ占有する作りなので、コールバックは常に1つ・周期も1つ。
- AVR(と一部 Teensy)向け実装のため、他アーキテクチャのコアではそのまま使えない。

### 複数周期が欲しい場合の代替

1. **1つのコールバック内で分周する**(最も手軽)

    ```cpp
    #include <MsTimer2.h>

    volatile uint16_t cntA = 0, cntB = 0;

    void tick() {            // 1ms周期
      if (++cntA >= 10)  { cntA = 0; /* 10ms周期の処理 */ }
      if (++cntB >= 500) { cntB = 0; /* 500ms周期の処理 */ }
    }

    void setup() {
      MsTimer2::set(1, tick);
      MsTimer2::start();
    }

    void loop() {}
    ```

2. **別のハードウェアタイマを併用する**(TimerOne など)
   - Timer1 は `Servo`、Timer2 は `tone()` や一部 PWM ピンと競合する点に注意。

3. **ソフトウェアタイマライブラリを使う**(後述)

## 2. mbed の Timer と Ticker の違い

| mbed クラス | 役割 | Arduino での実現 |
|---|---|---|
| `Timer` | 経過時間の計測(ストップウォッチ) | `millis()` / `micros()` のラッパで十分。ライブラリ不要 |
| `Ticker` | 周期的なコールバック呼び出し | ライブラリ利用、または自作 |

### 参考: Timer 相当(ライブラリ不要)

```cpp
class Timer {
  uint32_t t0 = 0, acc = 0;
  bool running = false;
public:
  void start() { if (!running) { t0 = micros(); running = true; } }
  void stop()  { if (running)  { acc += micros() - t0; running = false; } }
  void reset() { acc = 0; t0 = micros(); }
  uint32_t read_us() const { return acc + (running ? micros() - t0 : 0); }
  uint32_t read_ms() const { return read_us() / 1000; }
};
```

符号なし減算のため `micros()` のオーバーフロー(約70分)をまたいでも、1回の計測が約70分未満なら正しく差分が取れる。

## 3. Ticker 相当: ライブラリを使う場合

| ライブラリ | 方式 | 特徴 |
|---|---|---|
| **Ticker**(sstaub)/ **TickTwo** | ポーリング | 1オブジェクト=1周期処理。書き味が mbed の Ticker に最も近い |
| **arduino-timer**(contrem) | ポーリング | `every()` / `in()` / `at()` でタスク登録。軽量・アーキ非依存 |
| **TaskScheduler** | ポーリング(協調型) | タスク数が多い・状態遷移が複雑なアプリ向け |
| **TimerInterrupt / ISR_Timer 系**(khoih-prog) | 割り込み | 1つの HW タイマ上に十数個の ISR タイマ。ボードごとに別ライブラリ |

- ESP32 / ESP8266 はコアに `Ticker` が標準搭載されている。
- ポーリング型は `loop()` 内のブロッキング処理(`delay()` 等)で呼び出しが遅れる。
- 割り込み型はコールバックが ISR 内で動くため重い処理は書けない。

### TickTwo の例

```cpp
#include <TickTwo.h>

void blink();
void readSensor();

TickTwo led(blink, 500);        // 500ms周期
TickTwo sensor(readSensor, 100);

void setup() { led.start(); sensor.start(); }
void loop()  { led.update(); sensor.update(); }   // ポーリングが必要
```

## 4. Ticker 相当: 割り込み駆動で自作する場合

mbed と同様に「割り込みで自動的に呼ばれる」挙動が欲しい場合は自作が素直。
ハードウェアタイマを1つだけ使い、そこから全インスタンスへ配送する構造にする。

### 実装

```cpp
class Ticker {
public:
  using Callback = void (*)();
  ~Ticker() { detach(); }

  void attach(Callback f, uint32_t period_ms) {
    if (!f || period_ms == 0) return;
    noInterrupts();
    func = f; period = period_ms; remain = period_ms;
    if (!linked) { next = head; head = this; linked = true; }
    interrupts();
  }

  void detach() {
    noInterrupts();
    for (Ticker** p = &head; *p; p = &(*p)->next) {
      if (*p == this) { *p = next; break; }
    }
    linked = false;
    interrupts();
  }

  static void tick() {   // ハードウェアタイマから1msごとに呼ぶ
    for (Ticker* t = head; t; t = t->next) {
      if (--t->remain == 0) { t->remain = t->period; t->func(); }
    }
  }

private:
  static Ticker* head;
  Ticker* next = nullptr;
  Callback func = nullptr;
  uint32_t period = 0;
  volatile uint32_t remain = 0;
  bool linked = false;
};
Ticker* Ticker::head = nullptr;
```

### 使用例(AVR: tick 源に MsTimer2 を流用)

```cpp
#include <MsTimer2.h>

Ticker led, sensor;
volatile bool sensorReady = false;

void blink()      { digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); }
void readSensor() { sensorReady = true; }   // 重い処理は loop 側へ

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  led.attach(blink, 500);
  sensor.attach(readSensor, 100);
  MsTimer2::set(1, Ticker::tick);
  MsTimer2::start();
}

void loop() {
  if (sensorReady) { sensorReady = false; /* センサ読み出し */ }
}
```

### 注意点

- コールバックは ISR 内で実行される(mbed と同じ)。短く保ち、重い処理はフラグ経由で `loop()` へ。
- コールバック内から `attach()` / `detach()` を呼ぶと、`interrupts()` により ISR 中に割り込みが再許可されてしまう。対応する場合は以下に置き換える。
  - AVR: `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)`
  - ARM: PRIMASK の保存・復元
- 関数ポインタのためメンバ関数は直接渡せない。mbed の `callback(this, &Foo::bar)` 相当が必要なら、
  - `void*` コンテキスト引数を追加する
  - `std::function` を使う(AVR では不可)

### 改善の方向性

上記は 1ms ごとに全インスタンスを走査する単純な方式。mbed 本体は、

- 1本のハードウェアタイマのコンペアマッチに「次に発火する時刻」だけを設定し、
- イベントを時刻順のキューで管理する

方式を取っている。分解能を上げたい場合や BSP 側に組み込む場合は、この構造にすると割り込み回数を減らせる。
