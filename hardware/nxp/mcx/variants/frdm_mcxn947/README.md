# variants/frdm_mcxn947/ — セットアップ手順

## 1. プリビルドライブラリの配置

MCUXpressoで `_r01lib_frdm_mcxn947` プロジェクトをDebugビルドし、
生成された `.a` ファイルをここにコピーしてください。

```
MCUXpresso の出力:
  _r01lib_frdm_mcxn947/Debug/lib_r01lib_frdm_mcxn947.a

コピー先:
  variants/frdm_mcxn947/lib/lib_r01lib_frdm_mcxn947.a
```

## 2. リンカスクリプトの配置

`variants/frdm_mcxn947/linker/MCXN947.ld` はA153の `MCXA153.ld` のセクション
配置ロジックをベースに、MCXN947の実メモリマップ（MCUXpresso生成の
`app_template_FRDM_MCXN947` プロジェクトの `.ld` から抽出）に合わせて手書き
したもの。PROGRAM_FLASH0/1（各1M、連続）を単一のPROGRAM_FLASH(2M)に統合、
SRAMX(96K)/SRAMH(32K)をA153のSRAMX0/SRAMX1相当として配置。

## 3. 確認

```
variants/frdm_mcxn947/
├── include/          ← 自動配置済み（ヘッダ群）
├── lib/
│   └── lib_r01lib_frdm_mcxn947.a   ← ★手動配置
└── linker/
    └── MCXN947.ld                   ← 済（手書き）
```

## 含まれるオブジェクト (.a の内容)

```
fsl_assert, fsl_debug_console, fsl_str    ← utilities
startup_mcxn947                            ← スタートアップ
BusInOut, InterruptIn, Serial, Ticker     ← r01lib コア
i2c, i3c, io, irq, mcu, obj, r01lib_spi   ← r01lib コア
arduino_i2c, arduino_io, arduino_main     ← arduino layer
arduino_serial, arduino_spi, arduino_string ← arduino layer
Print, Stream                              ← arduino layer
fsl_clock, fsl_gpio, fsl_lpi2c, ...      ← NXP SDK drivers
board, clock_config, pin_mux              ← board files
```

## Wire / Wire1 の実機検証済み動作（既知の癖あり）

- `Wire`（Arduino connectorのI2Cピン、D18/D19、プレーンI2Cクラス）: 実機で
  バススキャン（ハング・クラッシュなし）に加え、**実デバイスとの実通信も
  確認済み**。P3T1035xUK-ARD（LM75B/P3T1755と同じ温度レジスタフォーマット、
  7-bitアドレス0x72）をD18/D19/3V3/GNDに接続し、`test_Wire_LM75B`
  （レジスタポインタ0x00へのwrite→リピートスタートで2バイトread、
  11-bit値をシフト＆0.125倍でdegC変換）で実機確認。ロジックアナライザで
  `Write[0x72]+ACK`→`0x00+ACK`→`Read[0x72]+ACK`→データ2バイト
  （最終バイトNAK）という正しいI2C読み出しシーケンスを確認し、Serial出力
  の`temp = 27.375 degC`も室温として妥当な値だった。
- `Wire1`（オンボードI3Cセンサーバス、MB_RX/MB_TX＝P1_16/P1_17、I3CをI2C_MODEで
  使用）: オンボードP3T1755からの生レジスタ読み取りで実機動作確認済み。ただし
  実装に2件の実機バグ修正が必要だった:
  1. **I3C1_SDA/I3C1_SCLの入力バッファが有効化されていなかった**: pin_mux.cの
     `BOARD_InitDEBUG_UARTPins()`（名前に反してI3C1のIBE設定も含む）がどこか
     らも呼ばれておらず、`PORT_SetPinMux()`はMUXフィールドしか触らないため、
     IBEがリセット後デフォルトのまま（無効）だった。I3Cペリフェラルはターゲッ
     トへの書き込みはできてもACK/データを読み取れず、writeは成功するが
     readは`kStatus_I3C_Nak`で失敗する、という形で発現した。`I3C::I3C()`
     コンストラクタで`_scl.input_buffer(true)`/`_sda.input_buffer(true)`を
     追加して解消（A153のSerial1 D0/D1バグと同じ問題パターン）。
  2. **I2C_MODEに切り替える前に一度も実I3Cバス動作をしていないと、以降の
     I2Cモード読み取りがアドレスフェーズで確実にNAKする**: `TwoWire::begin()`
     で`new I3C(...)`した直後、`mode(I2C_MODE)`に切り替える前に
     `ccc_broadcast(CCC::BROADCAST_RSTDAA, ...)`（それ自体は
     `kStatus_I3C_WriteAbort`で失敗するのが正常——動的アドレスを持つデバイス
     がまだ存在しないため）を一度実行することで解消。NXP公式デモ
     （`P3T1755_FRDM_MCXN947_demo_DAA`、`ref/dm-i3c-temperature-sensor-main.zip`）
     が常にこの順序（RSTDAA→I2Cモードアクセス）を踏んでいたことから発見。
     根本原因は未特定（I3C仕様上はバスフリー/START/アドレスヘッダ期間の
     プルアップアシストは常時有効なはずだが、`I3C_MasterInit()`だけでは
     プルアップアシストのステートマシンが起動せず、実際のSTART/STOPサイク
     ルを一度経験する必要がある、というSDK/シリコン側の初期化順序の問題で
     はないかと推測）。実機での制御されたA/Bテスト（このコードを入れた版・
     外した版を交互に書き込んで検証）で再現性を確認済み。**A153では確認上
     不要**（A153のI3C_SDA/I3C_SCLは汎用Arduino I2CコネクタとD18/D19を共有
     しており実プルアップ抵抗があるため。N947のオンボードI3Cバスは固定プル
     アップ抵抗R51/R52がschematic上でDNP、I3C1_PUR(P1_11)によるペリフェラル
     駆動プルアップに完全依存する設計）——`#ifdef CPU_MCXN947VDF`でN947限定
     にし、他チップに正体不明の追加I2Cバストランザクションが起きないように
     してある。

## SPI の実機検証済み動作

MOSI(D11)-MISO(D12)ジャンパーによるループバックで`transfer()`/`transfer16()`/
`setBitOrder(LSBFIRST)`/`end()`→`begin()`再初期化まで一通り往復確認OK。

- **テスト設計上の注意（ハマった点）**: 最初LEDでの合否表示に`GREEN`を使った
  ところ「何も点灯していない」ように見えた。原因は`GREEN`がD10で、SPIの
  デフォルトChip Select（`SPI_CS`/`ARD_CS`）と同じ物理ピンだったため、SPI
  トランザクション中のCS制御とLED表示用の`digitalWrite`が同じピンを取り合っ
  ていた。`BLUE`に変更して解消。**N947のLED定義（`RED=D9`, `GREEN=D10`,
  `BLUE=D6`）のうちGREENだけがデフォルトSPI CSと重複するので、SPI関連の
  テストスケッチではLED表示にGREENを使わないこと**（ユーザー提供の
  `ref/r01lib_pin_table.xlsx`にピンの用途一覧あり、以後の確認で参照）。
- **ロジックアナライザによる最終確認済み**: `test_SPI_bitorder_end_transfer16.ino`
  をD10(CS)/D11(MOSI)/D12(MISO)/D13(SCLK)にプローブしてキャプチャ。
  ロジアナのSPIデコーダをMSBファースト（デフォルト）のままにすると、
  `LSBFIRST`設定時の`transfer16(0x5678)`はバイト内ビットが反転した値
  （下位バイト`0x78`→`0x1E`、上位バイト`0x56`→`0x6A`、下位バイトが先）と
  して表示される——デコーダ側を「Bit order: LSB first」に切り替えると
  `0x78`→`0x56`と正しい値・順序で表示されることを確認し、`bitOrder`が
  ハードウェアレベルで正しく反映されていることを実証した。CSが16ビット
  転送の間ずっとLOWを維持していること（1ワード=1トランザクション）も
  波形上で確認済み。Serial出力の`OK`判定と合わせて全項目確認完了。

## analogRead の実機検証済み動作

`analogRead()`（A2-A5、LPADC/ADC0）は実機検証済み。A3をGND/3.3Vにショート
すると読み取り値の変化が実際に確認できた。**A0/A1は非対応**（`io.h`で
`DISABLED_PIN`——このボードでは配線されていない）。

- **訂正**: 当初「N947はLPADCではなくADC0/ADC1(`ADC_Type`)を持つためA153の
  AnalogInクラスはそのまま使えない」と誤って判断していたが、これは誤り。
  NXPのドライバは実際には両チップとも`ADC_Type`という同じ構造体型名を使っ
  ており（A153の`ADC0`も`(ADC_Type*)ADC0_BASE`）、ドライバ自体
  （`fsl_lpadc.c/h`、`LPADC_Init(ADC_Type *base, ...)`等）はA153と共通で
  使える。一度誤って削除した`fsl_lpadc.c/h`を、既存の版元SDK
  （`SDK_2_16_000_FRDM-MCXN947.zip`）から復元して実装した。
- **A153との相違点**（NXP公式のFRDM-MCXN947 LPADC pollingサンプルと突き合わせ
  て確認）:
  - N947はLPADCの前段として`VREF`ブロック（`fsl_vref.c/h`、新規取得）の
    初期化が必要（`SPC_EnableActiveModeAnalogModules(SPC0, kSPC_controlVref)`
    → `VREF_Init(VREF0, ...)`）。A153ではこのステップは不要だった
  - `FSL_FEATURE_LPADC_FIFO_COUNT`がN947では`2`（A153は`1`）のため、
    `LPADC_GetConvResult()`/`LPADC_DoResetFIFO()`はFIFOインデックス引数
    付きの版（`LPADC_DoResetFIFO0()`、`LPADC_GetConvResult(base,&r,0U)`）
    を使う必要がある
  - N947のA2-A5は`ADC0`の同じチャンネル番号をA/B面で共有する構成
    （A2=ch14 Bside, A3=ch14 Aside, A4=ch15 Bside, A5=ch15 Aside）——
    A153は各A-pinが別々のチャンネル番号だったため、この差を意識せずに
    済んでいた。`lpadc_conv_command_config_t.sampleChannelMode`で明示的に
    A/B面を指定する必要がある
  - `port_pin_config_t`はビットフィールド構造体で、実際に存在するフィー
    ルドは`FSL_FEATURE_PORT_HAS_*`マクロの組み合わせでチップごとに変わる
    （N947には`driveStrength1`フィールドが存在しない）。A153のコードは
    位置指定初期化（`{val1, val2, ...}`）を使っていたため、そのままN947
    に持ってくるとコンパイルエラーになった——フィールド名を明示する初期
    化に書き換えて解消
- `analogReadResolution()`は実装済み（デフォルト10bit）。`analogReference()`
  はA153と同様no-op（リファレンス電圧はハードウェア固定）

## analogWrite の実装（実機検証待ち）

`analogWrite()`はPWM0（FlexPWM、A153のPwmOutクラスと同じ設計）で実装済み。
`PWM_0`-`PWM_5`（P3_6..P3_11、A153のPWM0-PWM5と同じ6物理ピン）が対応。
**まだ実機検証していない**（ユーザー方針: 後日確認）。

- **ピン名を`PWM0`ではなく`PWM_0`にした理由（重要）**: このチップのSDK
  は`PWM0`という名前をFlexPWMペリフェラルのインスタンスマクロとして既に
  使っている（`#define PWM0 ((PWM_Type*)PWM0_BASE)`）。論理ピン名を
  `PWM0`にすると、`PwmOut.cpp`内の`PWM_Init(PWM0,...)`等の呼び出しが
  ペリフェラルではなくピン番号に展開されてしまい、コンパイルエラーに
  なった（実際に発生・特定・修正済み）。A153はSDKのインスタンスマクロが
  `FLEXPWM0`だったため、この衝突が起きなかった。他チップへの移植時は
  同様の名前衝突に注意
- **サブモジュール番号がA153と異なる**: 同じ6物理ピン（P3_6..P3_11）だが、
  A153はsm0/1/2、N947はsm1/2/3（pin_mux.cのpin_signal文字列——例えば
  P3_6は`.../CT4_MAT2/PWM0_A1/...`——から、A/Bの後の数字がサブモジュール
  番号であることを確認）
  - **ALT mux値が6ピンで不均一**: A153は6ピン共通の単一ALT値だったが、
    N947は各ピンのalt-function一覧内でPWM0_Ax/Bxが出現する位置が異なる
    （途中に`WUU0_INxx`が挟まるピンとそうでないピンがある）ため、Alt4/
    Alt5が混在する。I3C1_SDAのAlt10確認で使ったのと同じ「pin_signal文字
    列内の位置を数える」方式で導出

## tone / noTone の実装（実機検証待ち）

`tone()`/`noTone()`はCTIMER0ベースで実装済み（A153とほぼ同一のロジック、
任意のデジタルピンでソフトウェアトグル）。**まだ実機検証していない**
（ユーザー方針: 後日確認）。

- このファイル（`arduino_tone.cpp`）はCPU依存の`#ifdef`分岐が一切ない、
  完全にチップ非依存な実装だった。ただしN947のSDKではクロック分周器の
  シンボル名がA153と異なる（A153=`kCLOCK_DivCTIMER0`、N947=
  `kCLOCK_DivCtimer0Clk`——大文字小文字だけでなく"Clk"サフィックスの
  有無も違う）。同様に分周設定関数自体も名前が異なる
  （A153=`CLOCK_SetClockDiv`、N947=`CLOCK_SetClkDiv`）。NXP公式の
  FRDM-MCXN947 CTIMER driver example（`simple_match_interrupt`）で
  該当箇所を確認して修正
- `CTIMER0`インスタンス・`CTIMER0_IRQn`・`kFRO12M_to_CTIMER0`・
  `CLOCK_GetCTimerClkFreq()`はA153と共通の名前でそのまま使える
- `drivers/subdir.mk`に`fsl_ctimer.c`（既にdriversフォルダには存在して
  いたが未ビルドだった）を追加、`arduino_layer/subdir.mk`に
  `arduino_tone.cpp`を追加
- 確認用スケッチ: D3ピンで440Hz→880Hzを繰り返すテスト（ピエゾブザーか
  スコープでの確認を想定）を用意したが、実機確認はWire/SPI/analogWrite
  と同様後日に持ち越し。`test_tone`/`test_shiftOut_pulseIn_random`
  （tone()依存）が以前はN947でリンクエラーになっていたが、今回の実装で
  `arduino-cli compile`成功に転じたことは確認済み
- `Serial1`（D0/D1ハードウェアUART）は**見送り（意図的に未対応）**。
  D0/D1（ARD_D0/ARD_D1、物理ピンP4_3/P4_2）のFlexCommとしての alt機能は
  FC2_P2/FC2_P3のみで、これは`Wire`（I2C_SDA/SCL=D18/D19、`LPI2C2`＝
  FlexComm2）が既に専用で使っているのと**同じFlexComm2インスタンス**。
  LP_FLEXCOMMは1インスタンスにつき同時にひとつのモード（UART/I2C/SPI）
  しか持てないため、D0/D1をSerial1として使うには`Wire`を諦める必要があ
  り、両方を同時に使うスケッチは書けない。この制約をユーザーに提示し、
  見送りの判断となった（A153のようにD0/D1が独立したLPUART専用ピンでは
  ない、N947固有の物理的制約）。
  （経緯として、A153から`arduino_serial.cpp`をそのまま移植した際、D0/D1を
  r01lib `Serial.cpp`の`s_pinMap[]`（USBTX/USBRX→LPUART4の1エントリのみ）に
  解決できず`panic()`が`static`初期化時（`setup()`実行前）に発火し、SOSの
  モールス信号でLEDが点滅し続ける実機バグが最初に見つかった。GDBで`_base`
  が`nullptr`のままなことを確認して特定・修正済み——`Serial1`を参照する
  スケッチは現在コンパイルエラー（`'Serial1' was not declared in this
  scope`）になる。）
