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

まだ行っていない。
