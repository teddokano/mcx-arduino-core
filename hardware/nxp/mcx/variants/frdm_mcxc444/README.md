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

まだ実機では動かしていない（0.10.0の作業中）。
