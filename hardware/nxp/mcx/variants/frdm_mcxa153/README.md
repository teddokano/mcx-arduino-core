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
