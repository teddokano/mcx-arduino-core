# variants/frdm_mcxn236/

0.9.0で追加したFRDM-MCXN236（Rev C）のボード定義。

```
variants/frdm_mcxn236/
├── include/   ← デバイスヘッダ、CMSISヘッダ、ボードテンプレートのヘッダ、
│                 チップごとに内容が異なるSDKドライバのヘッダ
├── linker/
│   └── MCXN236.ld    ← リンカスクリプト（メモリマップ定義）
├── src/       ← pin_mux, clock_config, board, peripherals,
│                 デバイススタートアップ, チップごとに内容が異なるSDKドライバ
└── svd/
    └── MCXN236.svd   ← IDEのCORTEX PERIPHERALS表示用
```

## 前提にしているボードの改造

ピン配置の詳細は[`PIN_MAPPING_N236.md`](../../../../../PIN_MAPPING_N236.md)。

- **R25・R67を外す**。工場出荷時はA1（`P4_15`）・A2（`P4_16`）がCANトランシーバ（TJA1057）の`RXD`・`TXD`にもつながっていて、
  トランシーバは常に電源が入ったノーマルモードなので、A1はその出力に駆動される。外すとFlexCANは使えなくなる
- A3（`P4_17`）は青色LEDと共用で切り離せないので、コアはデジタルピンとして扱い、`analogRead(A3)`は`-1`を返す
- SJ1・SJ2は工場出荷時の1-2のまま（D10・D11が`SPI`）

## ファイルの出どころ

`include/`・`src/`・`svd/`のファイルはすべて`SDK_2_16_000_FRDM-MCXN236.zip`（SDK Builderから取得）から**無改変で**コピーしたもの。
コアが取り込んでいる共有ドライバ（`cores/arduino/sdk/`）と同じ2.16.000で、そちらはこのzipのものとバイト単位で一致する。

| ファイル | zip内の場所 |
|---|---|
| `MCXN236.h`・`MCXN236_features.h`・`fsl_device_registers.h`・`system_MCXN236.{h,c}` | `devices/MCXN236/` |
| `fsl_*.{h,c}`（下の3つを除く） | `devices/MCXN236/drivers/` |
| `fsl_flash.{h,c}`・`fsl_flash_ffr.h`・`fsl_flexspi_nor_flash.h` | `devices/MCXN236/drivers/romapi/flash/`（`.c`は`src/`の下） |
| `board`・`clock_config`・`peripherals`・`pin_mux`の`.{h,c}` | `boards/frdmmcxn236/project_template/` |
| `startup_mcxn236.cpp` | `devices/MCXN236/mcuxpresso/` |
| `cmsis_*.h`・`core_cm33.h`・`mpu_armv8.h`・`tz_context.h` | `CMSIS/Core/Include/` |
| `svd/MCXN236.svd` | `devices/MCXN236/MCXN236.xml`（拡張子だけ変更。`SW-Content-Register.txt`の`SDK_Device`でBSD-3-Clause） |

ボード用のテンプレート（`boards/frdmmcxn236/`）を使った。デバイス用（`devices/MCXN236/project_template/`）とは`board.h`・`pin_mux.c`などが違い、
ボード用はデバッグUART（`P1_8`/`P1_9`）のピン設定とLEDの定義を持つ。

FRDM-MCXN947のvariantとの違い:
- **`clock_config.c`・`pin_mux.c`がSDKのテンプレートそのもの**。`BOARD_BootClockPLL150M()`はN947と同じく周辺のクロックをつながない。
  `BOARD_InitBootPins()`はPORT1のクロックとデバッグUARTの2本だけを設定する（N947のConfig Tools版はPORT0〜4のクロックと多くのピンの入力バッファを入れる）。
  残りは`cores/arduino/r01lib/mcu.cpp`の`init_mcu()`のN236分岐で行う
- `fsl_*`のドライバは、`fsl_clock.{h,c}`・`fsl_reset.h`・`fsl_inputmux_connections.h`以外はN947のものとバイト単位で同じ
- シングルコアなので`boot_multicore_slave.c`は無い
- FPUあり（`-mfpu=fpv5-sp-d16 -mfloat-abi=hard`）、コアクロック150MHz（PLL0）はN947と同じ

## リンカスクリプト

メモリマップはSDKのマニフェスト（`FRDM-MCXN236_manifest_v3_14.xml`）の値:
`PROGRAM_FLASH0`・`PROGRAM_FLASH1`が512KBずつ（連続して1MB）、`SRAM` 224KB（`0x20000000`）、`SRAMX` 96KB（`0x04000000`）。
フラッシュの末尾64KB（第2バンク、32KBずつ2つ）を`EEPROM`ライブラリ用に`PROGRAM_FLASH`から外している（N947と同じ作り）。
セクションの並びはA156のスクリプトと同じ。ヒープ16KB・スタック4KBは他のボードと同じ。

## FlexComm2の共有

`Serial1`（D0/D1、`LPUART2`）と`Wire1`（MikroBusのI2C、`LPI2C2`）は同じLP_FLEXCOMM2にある。D0/D1は`FC2_P3`/`FC2_P2`で、
LPUARTがこの2本に出るのはFlexCommを「LPI2CとLPUARTの両方」のモード（`LP_FLEXCOMM_PERIPH_LPI2CAndLPUART`）にしたときだけ。
SDKの`LPUART_Init()`・`LPI2C_MasterInit()`・`LPI2C_SlaveInit()`はモードを自分の分だけに書き換え、`LPUART_Deinit()`・`LPI2C_MasterDeinit()`は
FlexComm全体をリセットする。そこでコアは:
- 3つの初期化のあとで`flexcomm_keep_shared()`（`mcu.h`）を呼び、両方のモードに戻す
- FC2では、`Serial`のコンストラクタでFlexCommをリセットせず、`Serial::reinit()`で`LPUART_Deinit()`を、`I2C`のデストラクタで`LPI2C_MasterDeinit()`を呼ばない
  （`LPUART_Init()`はLPUARTだけをソフトウェアリセットし、`I2C`のデストラクタはLPI2Cのマスタだけをリセットする）
- 割り込みは`Serial.cpp`の`LP_FLEXCOMM2_IRQHandler()`が受け、LPUARTの分を`Serial1`に渡したあと、SDKのハンドラ（`Wire1`のターゲットモード）を呼ぶ

## 実機確認

開発用の基板（Rev C、R25・R67を外したもの）で確認した。

| 項目 | 結果 |
|---|---|
| 起動、`Serial`（USB）、RGB LED | `hello_world`で確認 |
| `release_check/01` | ALL OK（クロックの値、`Wire1`の加速度センサーFXLS8974CFのWHO_AM_I（`0x86`）と静止時の約1g、`Wire`のターゲットモード） |
| `Wire`・`Wire1`のターゲットモード | `test_Wire_target_self`が両方ともALL OK |
| `release_check/09` | ALL OK |
| `release_check/04` | CONFLICT・MISMATCHなし |
| `release_check/06` | 2回ともALL OK（自分でかけるリセットをまたいだ保持、前回のデータが書き込みのあとも残る） |
| `release_check/07` | ALL OK（書き込み中のリセット1000回） |
| `release_check/08` | ALL OK（アナログピンは`A0`・`A4`） |
| `release_check/03`（SW2＝`P0_20`） | 立ち下がりエッジで3回とも数えLEDが切り替わる、`detachInterrupt()`後は反応しない、LOWレベル割り込み |
| `analogRead` | `A0`・`A1`・`A2`・`A4`・`A5`が12ビットでGNDのとき0〜1、3V3のとき4083〜4095。1本ずつ3V3にしても、ほかのピンは追従しない。`analogRead(A3)`は`-1` |
| `analogWrite`・`tone` | `PWM0`〜`PWM5`の1kHzとデューティ比、`analogWriteFrequency()`、`D2`の`tone()`をPDIRで確認。`test_analogWrite_pairs_and_pinMode`（組の周期、`pinMode()`のあとの`analogWrite()`、D-ピンの別名）が23項目ALL OK |
| `release_check/11`（D0-D1、D2-D3） | ALL OK。`Serial1`と`Wire1`の交互・同時の使用、初期化と`end()`の順番の入れ替えも通った |
| `release_check/12`（D11-D12） | ALL OK |
| `release_check/13`（D0-D1、D2-D3、D4-D5、D6-D7） | ALL OK（`mcxRCServo`のパルスは`PWM0`＝D6から出し、D7で499/1449/2399us） |
| `release_check/14`（D19-D8、D18-D7、`MB_SCL`-`MB_PWM`、`MB_SDA`-`MB_INT`） | ALL PASS（`Wire`・`Wire1`とも。400kHzでの上限の切り詰めも） |
| `release_check/21` | N947のオンボードのP3T1755を、MikroBus（`MB_SDA`-N947の`MB_RX`、`MB_SCL`-`MB_TX`、GND-GND）経由でセンサーに使って約285周、WARNINGなし |
| `release_check/24`（N947とD18-D18、D19-D19、GND-GND） | 両ボードともALL OK（N236はA153と同じ側） |
| LinkServer 26.9.130 | フラッシュを`MCXNxxx (1024KB)`と正しく判定し、106KBの`release_check/01`を書き込めた |
| `release_check/22`（外付けのLM75系モジュールをD18/D19に） | `stop=false`・`stop=true`とも毎回読めて、温度は20℃台 |
| `release_check/23`（Waveshare 2.8インチTFTタッチシールド） | ライブラリ1.3.1の`SDBitmapViewer`で、SDカードのBMPが正しく、問題ない速さで描かれた（LCDとSDカードが`SPI`を分け合い、CSは`D10`・`D5`） |
| `release_check/02`（目で見る確認） | 問題なし。`D2`のトグルは`digitalWrite()`で2.046MHz、SDKのAPIで68.166MHz |
| Arduino IDE（macOS） | Debugボタンでブレークポイント、ステップ実行、変数、SVDの表示 |
