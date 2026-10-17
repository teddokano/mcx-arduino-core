# variants/frdm_mcxc444/

0.10.0で追加したFRDM-MCXC444のボード定義。Kinetis系のCortex-M0+（48MHz、FPU無し）で、ほかの4ボード（MCX A/N）とは周辺回路が違う。

```
variants/frdm_mcxc444/
├── include/   ← デバイスヘッダ、CMSISヘッダ、ボードテンプレートのヘッダ、
│                 このチップのSDKドライバのヘッダ
├── linker/
│   └── MCXC444.ld    ← リンカスクリプト（メモリマップ定義）
├── src/       ← pin_mux, clock_config, board, peripherals,
│                 デバイススタートアップ, このチップのSDKドライバ
└── svd/
    └── MCXC444.svd   ← IDEのCORTEX PERIPHERALS表示用
```

ピン配置の詳細は[`PIN_MAPPING_C444.md`](../../../../../PIN_MAPPING_C444.md)。

## ファイルの出どころ

`include/`・`src/`・`svd/`のファイルは`SDK_2_16_000_FRDM-MCXC444.zip`からコピーしたもの。変えたのは`src/startup_mcxc444.cpp`のフラッシュ設定（FOPT）だけ（下記）。

| ファイル | zipの中の場所 |
|---|---|
| `MCXC444.h`・`MCXC444_features.h`・`fsl_device_registers.h`・`system_MCXC444.{h,c}` | `devices/MCXC444/` |
| `startup_mcxc444.cpp` | `devices/MCXC444/mcuxpresso/` |
| `board.{h,c}`・`clock_config.{h,c}`・`pin_mux.{h,c}`・`peripherals.{h,c}` | `boards/frdmmcxc444/project_template/` |
| `core_cm0plus.h`・`cmsis_*.h`・`mpu_armv7.h` | `CMSIS/Core/Include/` |
| `fsl_clock`・`fsl_port`・`fsl_i2c`・`fsl_spi`・`fsl_lpuart`・`fsl_uart`・`fsl_adc16`・`fsl_tpm`・`fsl_smc`・`fsl_pit`・`fsl_flash`・`fsl_ftfx_*` | `devices/MCXC444/drivers/` |
| `MCXC444.svd` | `devices/MCXC444/MCXC444.xml`の名前を変えたもの |

`fsl_common`・`fsl_gpio`はコアの`cores/arduino/sdk/`のものと同じで、そちらを使う。

### フラッシュ設定（FOPT）

`startup_mcxc444.cpp`の`Flash_Config`の4語目を`0xFFFF3FFE`から`0xFFFF3BFE`に変えた（FOPTを`0x3F`から`0x3B`）。
`NMI_DIS`を0にして、NMIを常に止める。D4（`PTA4`、`SW3`）はリセット直後NMIの端子で、LOWのまま起動するとNMIで止まっていた。
r01libのFRDM-MCXC444のテンプレートのプロジェクトと同じ値。`FSEC`は`0xFE`（保護なし）のまま。

## リンカスクリプト

A153のものを元に、SRAMを1つ（`0x1FFFE000`から32KB）にし、Kinetisのフラッシュ設定（`.FlashConfig`）を`0x400`に置いた。
フラッシュ256KBのうち最後の8KB（`0x3E000`〜）が`EEPROM`ライブラリの領域（1KBのセクタ4つずつの2面）で、スケッチは248KB。

## 実機確認

2026-10-18に開発用のボードで確かめた（0.10.0の作業中。書き込みはLinkServer 26.6.137）。

- `test_bringup_C444`（下記）がALL OK
- `release_check`の`01`・`06`（2回。書き込みをまたいだ保持も）・`09`・`11`（D2-D3のジャンパだけ。`Serial1`の節はSKIP）・`12`（D11-D12のジャンパ）がALL OK。
  `02`（人が見る）は最後まで流れ、`03`（SW2を押す）は立ち下がり・`detachInterrupt()`・LOWの割り込みとも期待どおり
- `test_avr_pgmspace`（アドレスのそろっていない`pgm_read_*`も）と`test_network_base_classes`がALL PASS
- `test_fault_report`: 8種類のうち7種類で`error: HardFault at PC ...`がクラッシュした行を指す。
  Cortex-M0+には故障の種類を示すレジスタが無いので、種類は出ない。スタックあふれ（4）は報告されず、ボードがリセットを繰り返す（M0+にはスタックの上限のレジスタが無い）

まだのもの: ジャンパの要る`13`・`14`、外部の部品が要る`2n`、`Wire`のターゲットモードと`setWireTimeout()`（未実装）。

ADCの基準電圧はVALTH（VDDA）にしている。このボードではVREFHの端子がJP6で切り離されていて、VREFHを基準にすると全部の入力が最大値になる（ボードの`board.h`の`BOARD_ADC_USE_ALT_VREF`、SDKのADC16の作例と同じ）。

最初に流すのは[`examples/Arduino_compatible_API/test_bringup_C444`](../../../../../examples/Arduino_compatible_API/test_bringup_C444)。
配線なしで、`docs/porting_a_new_board.md`の立ち上げの順（赤のLED、`Serial`、クロック、`millis()`/`micros()`、全ピンのプルアップ・プルダウン、割り込み、`analogRead()`、`analogWrite()`、`tone()`、`Wire1`の加速度センサー、`SPI`、`EEPROM`、`Servo`、`Wire`のスキャン）を1本で確かめる。
TPMのレジスタと、ピンを`GPIOx_PDIR`で読んだ波形の両方を見る。D11-D12のジャンパがあれば`SPI`のループバックも見る。
