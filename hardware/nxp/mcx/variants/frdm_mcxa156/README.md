# variants/frdm_mcxa156/

0.8.0で追加したFRDM-MCXA156のボード定義。

```
variants/frdm_mcxa156/
├── include/   ← デバイスヘッダ、CMSISヘッダ、ボードテンプレートのヘッダ、
│                 チップごとに内容が異なるSDKドライバのヘッダ
├── linker/
│   └── MCXA156.ld    ← リンカスクリプト（メモリマップ定義）
├── src/       ← pin_mux, clock_config, board, peripherals,
│                 デバイススタートアップ, チップごとに内容が異なるSDKドライバ
└── svd/
    └── MCXA156.svd   ← IDEのCORTEX PERIPHERALS表示用
```

## 前提にしているボードの改造

ピン配置の詳細は[`PIN_MAPPING_A156.md`](../../../../../PIN_MAPPING_A156.md)。

- **R59・R60を2-3側へ付け替える**（工場出荷時は1-2側）。
  付け替えるとArduinoヘッダのD10が`P2_6`（`LPSPI1_PCS1`）、D11が`P2_13`（`LPSPI1_SDO`）になり、
  `SPI`がD10〜D13に出る。付け替えていないボードでは、D10・D11は`P3_13`・`P3_15`（PWM出力、`P3_13`は緑LEDとも共用）につながり、
  `SPI`のCSとMOSIはシールドに届かない。このコアは付け替えたボードを前提にしている
- **R75・R76を外す**。工場出荷時はA4（`P1_12`）・A5（`P1_13`）がCANトランシーバ（TJA1057）にもつながっていて、
  A4はトランシーバに駆動される。A0〜A3は影響を受けない

## ファイルの出どころ

`include/`・`src/`・`svd/`のファイルはすべて`SDK_2_16_000_FRDM-MCXA156.zip`（SDK Builderから取得）から**無改変で**コピーしたもの。
コアが取り込んでいる共有ドライバ（`cores/arduino/sdk/`）と同じ2.16.000で、そちらはこのzipのものとバイト単位で一致する。

| ファイル | zip内の場所 |
|---|---|
| `MCXA156.h`・`MCXA156_features.h`・`fsl_device_registers.h`・`system_MCXA156.{h,c}` | `devices/MCXA156/` |
| `fsl_*.{h,c}` | `devices/MCXA156/drivers/` |
| `board`・`clock_config`・`peripherals`・`pin_mux`の`.{h,c}` | `devices/MCXA156/project_template/` |
| `startup_mcxa156.cpp` | `devices/MCXA156/mcuxpresso/` |
| `cmsis_*.h`・`core_cm33.h`・`mpu_armv8.h`・`tz_context.h` | `CMSIS/Core/Include/` |
| `svd/MCXA156.svd` | `devices/MCXA156/MCXA156.xml`（拡張子だけ変更。`SW-Content-Register.txt`の`SDK_Device`でBSD-3-Clause） |

FRDM-MCXA153のvariantとの違い:
- **`clock_config.c`・`pin_mux.c`がSDKのテンプレートそのもの**。A153はConfig Toolsで作った版で、
  `BOARD_BootClockFRO96M()`がLPI2C・LPSPI・LPUART・I3C0などのクロックを`FRO_HF_DIV`に付け替え、
  `BOARD_InitPins()`が全ポートのクロックとリセットを扱う。A156のテンプレートはどちらもしない
  （`BOARD_InitPins()`はPORT0とデバッグUARTのピンだけ）ので、その分は`cores/arduino/r01lib/mcu.cpp`の
  `init_mcu()`のA156分岐で行う。結果のクロックはA153と同じで、`Wire`・`Wire1`・`SPI`・`SPI1`は`FRO_HF_DIV`（96MHz）
- **`fsl_lpadc`・`fsl_ctimer`が版どおり**。A153のこの2つは新しいSDKから持ってきたもの（`fsl_lpadc` 2.9.5）で、
  A156は2.16.000の版（`fsl_lpadc` 2.8.4。N947と同じ）
- **FPUあり**（`-mfpu=fpv5-sp-d16 -mfloat-abi=hard`、SDKのプロジェクトと同じ）

## リンカスクリプト

メモリマップはSDKのマニフェスト（`FRDM-MCXA156_manifest_v3_14.xml`）の値:
`PROGRAM_FLASH` 1MB、`SRAM` 120KB（`0x20000000`）、`SRAMX` 12KB（`0x04000000`）。
フラッシュの末尾16KB（8KBセクタ2つ）を`EEPROM`ライブラリ用に`PROGRAM_FLASH`から外している。
セクションの並びはA153のスクリプトと同じで、A153の`SRAMX1`に当たる領域が無いのでその分のセクションを除いた。
ヒープ16KB・スタック4KBはN947と同じ。

## 実機で分かったこと

- **リセット後、ほぼ全ピンの入力バッファ（`PCR.IBE`）が切れている**。`P0_6`（SW3）と、`pin_mux.c`がデバッグUART用に設定する`P0_2`以外は0。
  そのままでは`digitalRead()`が常に0になり、`Wire`は最初の転送で止まる（LPI2CがSCLのHighを見られない）。
  `cores/arduino/r01lib/io.cpp`の`DigitalInOut`が、このチップに限ってコンストラクタと`pin_mux()`で入れる
  （`pin_mux()`でも入れるのは、`AnalogIn`がアナログピンで切り、`pinMode()`が`pin_mux()`を呼ぶため）

## 実機確認

| 項目 | 結果 |
|---|---|
| 起動、`Serial`（USB） | `hello_world`で確認 |
| `EEPROM` | `test_EEPROM`がALL OK（リセットをまたいだ保持も）。もう一度書き込んでも前回のデータが残った |
| `Wire`（D18/D19） | スキャンでオンボードのP3T1755（`0x48`）が見つかり、温度が読めた |
| `Wire`・`Wire1`のターゲットモード | `test_Wire_target_self`が両方ともALL OK（`Wire1`の`LPI2C3`も応答する。N947の`LPI2C3`は応答しない） |
| `release_check/01` | ALL OK（クロックの値、`Wire`でのセンサー、`Wire1`のスキャンを含む） |
| `release_check/11`（D0-D1、D2-D3） | ALL OK（`Serial1`の12形式をRXピンからビット単位で、`end()`・`serialEvent1`、高速GPIO） |
| `SPI`（D11-D12） | `test_SPI_bitorder_end_transfer16`が全項目OK |
| `SPI1`（`MB_MOSI`-`MB_MISO`） | `test_SPI1_MikroBus`が全項目OK |
| `release_check/12`（D11-D12、`MB_MOSI`-`MB_MISO`） | ALL OK（`SPI`の分割転送・旧API、`SPI1`を含む） |
| `release_check/13`（D0-D1、D2-D3、D4-D5、D6-D7、`PWM0`（J3の5番）-D8） | ALL OK（`shiftOut`・`shiftIn`、`tone`を`pulseIn`で496us、`mcxRCServo`の16ビット分解能のパルスが499/1448/2399us） |
| `release_check/21`（上の配線＋`MB_TX`-`MB_RX`） | 約57秒・285周でWARNINGなし。`Serial1`・`Serial2`のループバックは欠落なし、`SPI1`の折り返しは毎回一致、`Wire1`（何もつないでいないMikroBus）は毎回NAK（`134`）で止まらない |

| `release_check/14`（D19-D8、D18-D7、`MB_SCL`-`MB_PWM`、`MB_SDA`-`MB_INT`） | ALL PASS（`Wire`・`Wire1`とも。上限は100kHzで約88ms、400kHzで約22ms） |
| `Wire`と`Wire1`をつないだ形（`MB_SDA`-D18、`MB_SCL`-D19） | `test_Wire_Wire1_jumpered_A156`がALL OK（`Wire1`からP3T1755が`Wire`と同じ値で読める、400kHzでも読める、`Wire`↔`Wire1`のターゲットモードが両方向で動く、D18・D19をLOWに押さえると`Wire1`は8usで失敗を返し、離すと戻る） |

| `release_check/04` | CONFLICT・MISMATCHなし（`Wire`・`Wire1`・`Serial1`・`Serial2`・`SPI1`・`PwmOut`・`AnalogIn`を同時に） |
| `release_check/06` | 2回ともALL OK（自分でかけるリセットをまたいだ保持、前回のデータが書き込みのあとも残る） |
| `release_check/07` | ALL OK（1000回のうち書き込み中に切られたのが759回、すべて書く前か書いた後の値に戻り、読めないフラッシュで起動したのは0回） |

まだのもの: `analogRead`・`analogWrite`（`PWM0`以外）の波形、割り込み、`release_check`の残り（`02`・`03`・`22`・`24`）。
