# variants/frdm_mcxa153/ — セットアップ手順

## 1. プリビルドライブラリの配置

MCUXpressoで `_r01lib_frdm_mcxa153` プロジェクトをDebugビルドし、
生成された `.a` ファイルをここにコピーしてください。

```
MCUXpresso の出力:
  _r01lib_frdm_mcxa153/Debug/lib_r01lib_frdm_mcxa153.a

コピー先:
  variants/frdm_mcxa153/lib/lib_r01lib_frdm_mcxa153.a
```

## 2. リンカスクリプトの配置

MCUXpressoのDebugフォルダにある `.ld` ファイルのうち、
メモリマップを定義するものをコピーしてください。

```
MCUXpresso の出力 (例):
  _r01lib_frdm_mcxa153/Debug/TEST_xxx_Debug.ld  ← メインLD
  またはプロジェクト内の *.ld

コピー先:
  variants/frdm_mcxa153/linker/MCXA153.ld
```

> **ヒント**: MCUXpressoが生成する `.ld` は複数あります。
> `MEMORY { }` セクションを含むメインのリンカスクリプトを使ってください。

## 3. 確認

```
variants/frdm_mcxa153/
├── include/          ← 自動配置済み（ヘッダ群）
├── lib/
│   └── lib_r01lib_frdm_mcxa153.a   ← ★手動配置
└── linker/
    └── MCXA153.ld                   ← ★手動配置
```

## MikroBusのGPIO/SPI: `SPI1`（実機検証済み）

MikroBusヘッダの全12ピン（`MB_AN`/`MB_RST`/`MB_CS`/`MB_SCK`/`MB_MISO`/
`MB_MOSI`/`MB_PWM`/`MB_INT`/`MB_RX`/`MB_TX`/`MB_SCL`/`MB_SDA`）は
`pinMode`/`digitalWrite`によるプレーンGPIOとして実機確認済み
（`test_digitalWrite_mikrobus_pins_A153`）。

SPIについては、N947と同様に独立した`SPI1`インスタンスを追加した
（`MB_MOSI`/`MB_MISO`/`MB_SCK`/`MB_CS`、`P1_0`/`P1_2`/`P1_1`/`P1_3`）。
このチップは`LPSPI0`/`LPSPI1`の2系統を持ち、既存の`SPI`（Arduinoヘッダ、
D10-D13）は`LPSPI1`のみを使用していたため、未使用の`LPSPI0`を
`SPI1`用に割り当てた——Zephyrの`MCXA153VLH-pinctrl.h`（シリコン正確）で
確認したところ、MikroBusの4ピンは全てAlt2で`LPSPI0`に接続されており、
既存`SPI`のAlt2（`LPSPI1`側）と偶然同じ値だったため、ALT値の分岐は不要
だった。

I2CとUARTについては、このチップの物理制約によりMikroBus版の独立インス
タンスは**実現不可能**と判明したため見送った:
- I2C: このチップには`LPI2C0`が1系統しかなく（デバイスヘッダで確認）、
  MikroBusの`MB_SCL`/`MB_SDA`も同じ`LPI2C0`への別ピンルートに過ぎない
  （`Wire`と排他利用にしかならない）
- UART: 既存の`Serial1`（D0/D1）はSerial.cppの`s_pinMap[]`で既に
  `LPUART2`を使用しており、MikroBusの`MB_TX`/`MB_RX`もpinctrlで確認した
  ところ同じ`LPUART2`にしか接続されていない（`Serial1`と排他利用にしか
  ならない）

- **実機バグ発見・修正: `LPSPI0`にクロックが供給されていなかった**:
  `SPI1`実装直後の実機確認で、CS(`MB_CS`)は正常に切り替わるのに
  `MB_MOSI`/`MB_MISO`/`MB_SCK`には何も出ないという症状が発生。`mcu.cpp`
  を確認したところ、既存の`SPI`用`LPSPI1`にはクロック分周・アタッチの
  設定があったが、新規追加した`LPSPI0`向けの設定が漏れていた（PORT MUX
  は正しくAlt2に切り替わっていたが、ペリフェラル自体にクロックが供給
  されておらず動作しなかった）。`CLOCK_SetClockDiv(kCLOCK_DivLPSPI0,1u)`
  ＋`CLOCK_AttachClk(kFRO12M_to_LPSPI0)`を追加して解消——ロジアナ波形・
  Serial出力（`transfer()`/`transfer16()`のループバック）とも実機確認済み
- `pinMode()`もN947と同様、呼ばれるたびに明示的にALT0(GPIO)へ再設定する
  よう修正済み（`arduino_io.cpp`）

## 含まれるオブジェクト (.a の内容)

```
fsl_assert, fsl_debug_console, fsl_str    ← utilities
startup_mcxa153                            ← スタートアップ
BusInOut, InterruptIn, Serial, Ticker     ← r01lib コア
i2c, i3c, io, irq, mcu, obj, spi         ← r01lib コア
arduino_i2c, arduino_io, arduino_main     ← arduino layer
arduino_serial, arduino_spi               ← arduino layer
fsl_clock, fsl_gpio, fsl_lpi2c, ...      ← NXP SDK drivers
board, clock_config, pin_mux              ← board files
TempSensor, LEDDriver, RTC_NXP, ...      ← r01device
```
