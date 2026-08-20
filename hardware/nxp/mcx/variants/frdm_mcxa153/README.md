# variants/frdm_mcxa153/

`v0.4.0`より、このボードはプリビルド`.a`ライブラリ方式を廃止し、フルソース
配布に移行済み（詳細はCHANGELOG.mdの`[0.4.0]`エントリ、CLAUDE.mdの
「`cores/arduino/`一本化・`platform.txt`書き換え完了」セクション参照）。

```
variants/frdm_mcxa153/
├── include/   ← ボード固有ヘッダ群
├── linker/
│   └── MCXA153.ld    ← リンカスクリプト（メモリマップ定義）
└── src/       ← ボード固有ソース（pin_mux, clock_config, board, デバイス
                  スタートアップ, チップごとに内容が異なるSDKドライバ）
```

このファイル以降は、このボード固有の実機検証・実機バグ修正の記録。

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
