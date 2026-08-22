# variants/frdm_mcxn947/

`v0.4.0`より、このボードはプリビルド`.a`ライブラリ方式を廃止し、フルソース
配布に移行済み（詳細はCHANGELOG.mdの`[0.4.0]`エントリ、CLAUDE.mdの
「`cores/arduino/`一本化・`platform.txt`書き換え完了」セクション参照）。

```
variants/frdm_mcxn947/
├── include/   ← ボード固有ヘッダ群
├── linker/
│   └── MCXN947.ld    ← リンカスクリプト（メモリマップ定義。A153の
│                        MCXA153.ldのセクション配置ロジックをベースに、
│                        MCXN947の実メモリマップに合わせて手書きしたもの）
└── src/       ← ボード固有ソース（pin_mux, clock_config, board, デバイス
                  スタートアップ, チップごとに内容が異なるSDKドライバ）
```

このファイル以降は、このボード固有の実機検証・実機バグ修正の記録。

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

`analogRead()`（A2-A5、LPADC/ADC0）は実機検証済み。当初A3をGND/3.3Vに
ショートして読み取り値の変化を確認、後日、定電圧源を使って`A2`-`A5`の
4チャンネル全てに既知の電圧（0-3.3V範囲）を与え、`test_analogRead_precision_N947`
で算出した電圧値が実際の設定電圧と正確に一致することを確認済み。
**A0/A1は非対応**（`io.h`で`DISABLED_PIN`——このボードでは配線されて
いない）。

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

## analogWrite の実機検証済み動作

`analogWrite()`はFlexPWM1（`PwmOut`クラス、A153のFlexPWM0版と同じ設計）
で実装済み・実機検証済み。対応ピンは`PWM0`-`PWM5`（`P2_2`-`P2_7`、
"Arduino Shield Compatible Headers"シート上のPWM専用ヘッダ）——A153と
共通の名前で使える。

- **ピン名`PWM0`/`PWM1`の名前衝突と解決（重要）**: このチップのSDKは
  `PWM0`/`PWM1`という名前をFlexPWMペリフェラルのインスタンスマクロとして
  既に使っている（`#define PWM0 ((PWM_Type*)PWM0_BASE)`等）。当初はこの
  衝突を避けるため論理ピン名を`PWM_0`-`PWM_5`（アンダースコア付き）と
  していたが、後日「マクロは共通で`PWM0`が使えるようにしたい」という
  方針でA153と同じ素の`PWM0`-`PWM5`に統一した。`io.h`側でSDKの`PWM0`/
  `PWM1`マクロを`#undef`してからArduinoピン名として再定義（この
  パターン自体は別プロジェクト`LEDDriver_NXP_Arduino`の`LEDDriver.h`
  における同種の衝突回避で既に前例あり）。`PwmOut.cpp`側は、この`#undef`が効く前（`io.h`を
  includeする前）にSDKの`PWM1`（`FlexPWM1`インスタンスへのポインタ）の
  値を`FLEXPWM1`という別名に退避してから使うことで、ドライバ呼び出し
  自体は引き続き正しいペリフェラルを参照するようにした。A153はSDKの
  インスタンスマクロが`FLEXPWM0`だったため、この衝突自体が最初から
  起きていない。他チップへの移植時は同様の名前衝突に注意

- **実機バグ発見・修正1: 物理ピンがA153流用のまま、実際は未配線の
  テストポイントだった**: 初回実装ではA153と同じ物理ピン番号（`P3_6`..
  `P3_11`）をそのまま流用していたが、`pin_mux.c`のラベルを確認すると
  この範囲は`TP8`/`TP12`-`TP18`/`TP31`という**テストポイント**（基板上の
  裸パッド）にしか繋がっておらず、`J3`/`J12`のようなコネクタには一切
  出ていないと判明。実機ロジアナで該当ピンに何も出ていないことから発覚
  し、ユーザー提供の回路図（`ref/FRDM-MCXN947SH.pdf`、"Arduino Shield
  Compatible Headers"シート、12ページ）を確認したところ、実際にヘッダ
  （`J12`、一部`J3`と共有）に出ているPWM対応ピンは`P2_2`-`P2_7`で、
  使用ペリフェラルも`FlexPWM0`ではなく`FlexPWM1`だった。`PWM0`-`PWM5`
  への割り当ては、その回路図のヘッダに直接印字された`PWM0`-`PWM5`の
  シルク印刷ラベルに合わせた（そのため物理ピン順にはなっていない：
  `PWM0`=`P2_3`、`PWM1`=`P2_2`、`PWM2`=`P2_5`、`PWM3`=`P2_4`、
  `PWM4`=`P2_7`、`PWM5`=`P2_6`）

- **実機バグ発見・修正2: ALT値の誤り（`P2_2`/`P2_3`のみ）**: 上記1の修正
  直後、ロジアナを全6ピンに繋いだ状態で`PWM0`（`P2_3`）を単体動作させ
  ても**どのピンにも何も出ない**という報告があり発覚。ALT値は
  I3C1_SDAのAlt10確認で確立した「pin_mux.cのpin_signal文字列内の位置を
  数える」方式で導出していたが（`P2_4`-`P2_7`の4ピンは結果的に正しく
  Alt5だった）、`P2_2`/`P2_3`だけはこの方式が誤った値（Alt6/Alt4）を
  出していた——`P2_2`のalt-function一覧に他のピンには無い`CLKOUT`という
  項目が余分に挟まっており、この項目がマルチプレクサのスロットを消費
  していないため、カウント方式が1つずれていた（`P2_3`も同様の理由で
  ずれ）。ローカルにクローン済みのZephyrプロジェクト
  （`~/dev/ArduinoCore/modules/hal/nxp/dts/nxp/mcx/MCXN947VDF-pinctrl.h`、
  NXP公式データから生成されたシリコン正確なpinctrlヘッダ）の
  `N9X_MUX(port,pin,mux)`マクロで全6ピンを突き合わせたところ、**全ピン
  共通でAlt5**という単純な答えが判明。修正後、ロジアナで`PWM0`単体
  動作を再確認し正常動作を確認
  - **教訓**: pin_mux.cのコメント文字列内での位置カウントは有用な手段
    だが単独では信頼しきれない。今回のように途中に「マルチプレクサの
    スロットを消費しない特殊項目」（`CLOCK_OUT`等）が挟まるケースが
    あるため、可能な場合はZephyrのpinctrlヘッダのような、より権威ある
    ソースと突き合わせ、最終的には必ず実機で確認すること
- **実機確認済み（全6チャンネル・独立性込み）**: `test_analogWrite_all_channels.ino`
  で、(A) 6チャンネル同時に異なる固定duty比（PWM0≈10%〜PWM4≈90%）を
  出力し全チャンネルが個別に正しい値で出ることを確認、(B) `PWM0`/`PWM1`
  （sm2共有）・`PWM2`/`PWM3`（sm1共有）・`PWM4`/`PWM5`（sm0共有）の
  各ペアで片方を50%固定しもう片方を0→100%で掃引しても固定側が影響を
  受けないこと（周期はサブモジュール単位で共有・dutyはチャンネル単位で
  独立という設計どおりの動作）を、ロジアナで全6ピン同時キャプチャして
  確認済み

## tone / noTone の実機検証済み動作

`tone()`/`noTone()`はCTIMER0ベースで実装済み・実機検証済み（A153とほぼ
同一のロジック、任意のデジタルピンでソフトウェアトグル）。

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
- **実機確認済み**: `examples/Arduino_compatible_API/test_tone`
  （D13、"Twinkle Twinkle Little Star"のメロディを262Hz-440Hzの範囲で
  演奏）を実機フラッシュし、圧電サウンダで実際にメロディが鳴ることを
  確認。`tone()`は任意のデジタルピンを使う汎用GPIOトグル実装のため、
  analogWriteのような固定ピンテーブルに起因するバグのリスクはなく、
  一度で正常動作を確認できた。`test_tone`/`test_shiftOut_pulseIn_random`
  （tone()依存）が以前はN947でリンクエラーになっていたが、今回の実装で
  解消したことも確認済み
- `Serial1`（D0/D1ハードウェアUART）は**D0/D1では見送り（意図的に未対応）**。
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
  が`nullptr`のままなことを確認して特定・修正済み——D0/D1で`Serial1`を参照
  するスケッチは現在コンパイルエラー（`'Serial1' was not declared in this
  scope`）になる。）
  **後日、`Serial1`自体はMikroBusヘッダ（`MB_TX`/`MB_RX`）向けに実装・実機
  検証済み——後述の「MikroBusのSPI/I2C/UART」セクション参照。**

## 全GPIOピンの出力確認と`pinMode()`のバグ修正（実機検証済み）

`D0`-`D19`（16本）・`A2`-`A5`（4本）・MikroBusヘッダ（`J5`/`J6`、11本、
`MB_AN`除く）の全`digitalWrite`対応ピンで出力が出ることを、それぞれ
専用のテストスケッチ（1本ずつ200msのHIGHパルスを順番に出す「歩くビット」
パターン）とロジックアナライザで実機確認済み。

- **実機バグ発見・修正: `MB_RX`/`MB_TX`だけ出力が出ない**: MikroBus
  ピンのテスト中、`MB_RX`/`MB_TX`（`P1_16`/`P1_17`）だけ信号が全く出ない
  現象が発覚。原因を`pin_mux.c`で調査したところ、`BOARD_InitPins()`が
  起動時にこの2ピンを**明示的にALT10（I3C1_SDA/I3C1_SCL）へ固定**して
  いると判明——オンボードP3T1755温度センサー用のI3Cバスとして使うための
  設計。一方`DigitalInOut`側の`pinMode()`実装は、ピンが既にGPIO(ALT0)で
  ある前提でGPIOレジスタ（方向・出力値）だけを操作し、PORT MUXフィールド
  には一切触れない作りだった。`I2C`/`I3C`クラスの`begin()`は`pin_mux()`
  を明示的に呼んでALT変更するが、その逆方向（一度I2C/I3Cで使ったピンを
  `pinMode()`でGPIOへ戻す）はコード上どこにも実装されていなかった——
  他の全ピンはブート時デフォルトが偶然ALT0（GPIO）なため今まで問題が
  表面化しなかっただけで、`D18`/`D19`（`Wire`用）も含め本来同じ欠陥を
  抱えていたことが判明。`arduino_layer/arduino_io.cpp`の`pinMode()`を
  修正し、新規ピン生成時・既存ピン再設定時のどちらでも明示的に
  `->pin_mux( 0 )`（ALT0=GPIO）を呼ぶよう変更。これにより`pinMode()`は
  そのピンが直前にどの周辺機能（I2C/I3C/SPI/PWM等）で使われていたかに
  関わらず、確実にGPIOとして再取得する
- **双方向切り替えの実機確認**: `onboard_temperature_sensor.ino`
  （`Wire1`経由でI3Cモードのオンボードセンサーにアクセス）→
  `test_digitalWrite_mikrobus_pins.ino`（同じ`MB_RX`/`MB_TX`ピンを
  プレーンGPIOとして駆動）の順に実機で連続実行し、`MB_RX`/`MB_TX`が
  I3CモードとプレーンGPIOモードの間で正しく切り替わることを確認完了
- 確認用スケッチ: `test_digitalWrite_all_pins`（D0-D13,D18,D19）、
  `test_digitalWrite_mikrobus_pins`（MikroBusヘッダ）、
  `test_digitalWrite_analog_pins`（A2-A5）。いずれも実機ロジアナで
  確認済み

## MikroBusのSPI/I2C/UART: `Wire2`/`SPI1`/`Serial1`（実機検証済み）

MikroBusヘッダの`MB_SDA`/`MB_SCL`（I2C）・`MB_MOSI`/`MB_MISO`/`MB_SCK`/
`MB_CS`（SPI）を、`Wire`/`SPI`とは独立したペリフェラルインスタンスとして
使えるようにした。新規グローバルインスタンス`Wire2`（I2C）・`SPI1`
（SPI）を追加。

- **ペリフェラルインスタンスの特定**: `I2C`/`SPI`クラスのコンストラクタ
  は、渡されたピンに関わらず`unit_base`（実際に叩くLPI2C/LPSPIレジスタ）
  がコンパイル時に単一のマクロへ固定されている作りだった（`Wire`は常に
  `LPI2C2`、`SPI`は常に`LPSPI1`）。そのままMikroBusピンを渡しても、
  MUXだけMikroBus側に切り替わり中身はD18/D19やD10-D13用のペリフェラルの
  ままという不整合になるため、新規ペリフェラル追加が必要だった。
  `pin_mux.c`のalt-function一覧とZephyrの`MCXN947VDF-pinctrl.h`
  （シリコン正確、位置カウント方式の失敗を教訓に最初から採用）を突き合
  わせ、`MB_SDA`(`P1_0`)/`MB_SCL`(`P1_1`)は`FlexComm3`(`LPI2C3`)・Alt2、
  `MB_MOSI`(`P3_20`)/`MB_MISO`(`P3_22`)/`MB_SCK`(`P3_21`)/`MB_CS`(`P3_23`)
  は`FlexComm6`(`LPSPI6`)・Alt3で、4ピンとも統一されていることを確認
- **他ボードに既存パターンあり**: `i2c.cpp`/`r01lib_spi.cpp`を確認したと
  ころ、A156向けの分岐には既に`MB_SDA`/`MB_SCL`・`MB_MOSI`/`MB_SCK`
  （SPI版はMikroBus専用ピンセット）をサポートするコードが存在していた
  （N947だけ未実装だった）。この既存パターンをそのままN947向けに移植
- **実装**: `mcu.cpp`の`init_mcu()`にFlexComm3/FlexComm6のクロック供給
  （`CLOCK_SetClkDiv`+`CLOCK_AttachClk(kFRO12M_to_FLEXCOMMx)`、既存の
  FlexComm1/2と同じ設定）を追加。`i2c.cpp`の`I2C`コンストラクタに
  `MB_SDA`/`MB_SCL`分岐（`unit_base=LPI2C3`）、`r01lib_spi.cpp`の`SPI`
  コンストラクタに`MB_MOSI`/`MB_MISO`/`MB_SCK`/`MB_CS`分岐
  （`unit_base=LPSPI6`）を追加
- **`SPIClass`の構造変更**: 従来`SPIClass`は引数なしの単一インスタンス
  固定（`D10`-`D13`決め打ち、内部の`r01libSPI*`もファイルstatic変数で
  全インスタンス共有）だった。`TwoWire`と同じ設計（コンストラクタで
  ピンを受け取る）に変更——`SPIClass(int mosi=ARD_MOSI, int miso=ARD_MISO,
  int sclk=ARD_SCK, int cs=ARD_CS)`、`r01libSPI*`もインスタンスメンバに
  変更。既存の`SPI`（引数なしデフォルト構築、後方互換）はそのまま、
  新規`SPI1(MB_MOSI, MB_MISO, MB_SCK, MB_CS)`を追加
- **実機確認済み**: `Wire2`は`test_Wire2_MikroBus_N947`（バススキャン、
  デバイス未接続でもハング・クラッシュなし）、`SPI1`は
  `test_SPI1_MikroBus`（`MB_MOSI`-`MB_MISO`ループバック、
  `transfer()`/`transfer16()`）で実機確認——ロジアナ波形・Serial出力の
  両方でOK

### `Serial1`（MikroBus UART、`MB_TX`/`MB_RX`）

ユーザーから「`MB_RX`/`MB_TX`を`Serial1`にできる？」と依頼。D0/D1の
`Serial1`は上記のとおりFlexComm2競合で見送り済みだったが、`MB_RX`/
`MB_TX`（`P1_16`/`P1_17`）は事情が異なると判明——`pin_mux.c`のalt-function
一覧を確認したところ`FC5_P0`/`FC5_P1`（`FlexComm5`、Zephyrのpinctrlヘッダ
でAlt2と確認）という、`Wire2`が使う`FlexComm3`とも`I3C1`（専用ペリフェラ
ルでFlexCommを消費しない）とも別の、**未使用のFlexCommインスタンス**が
使えた。つまりこの2物理ピンは、GPIO・`Wire1`（I3C1）・`Serial1`
（FlexComm5/LPUART5）の3用途を排他的に切り替えて使える。

- **実装**: `mcu.cpp`にFlexComm5クロック分周設定を追加。`Serial.cpp`の
  `s_pinMap[]`（USBTX/USBRX→LPUART4の1エントリのみだった）に`MB_TX`/
  `MB_RX`→`LPUART5`のエントリを追加、`LP_FLEXCOMM5_IRQHandler`も追加。
  `arduino_serial.cpp`/`.h`に、以前D0/D1向けに一度削除した`Serial1`グロー
  バルインスタンスを、今度は`MB_TX`/`MB_RX`向けとして復活
- **実機バグ発見・修正: SOSパニック（`arduino_io.h`のインクルードによる
  ピン値の衝突）**: 実装直後、実機フラッシュしたところSOSのモールス信号
  でLEDが点滅し続ける不具合が発生——過去のA153移植時のD0/D1バグと全く同じ
  症状。原因は`arduino_serial.cpp`が`arduino_io.h`をincludeしていたこと。
  `arduino_io.h`は`MB_TX`/`MB_RX`をArduinoピン番号リナンバリングの対象に
  含んでおり（`#undef`してから小さな連番の`enum`値として再定義する仕組
  み）、`SerialClass Serial1( MB_TX, MB_RX )`の宣言がこのincludeより後にあ
  ったため、`MB_TX`/`MB_RX`が生のr01lib物理ピン値ではなく再番号化された
  値に置き換わっていた。一方`Serial.cpp`の`s_pinMap[]`は生のr01lib
  `io.h`の値と比較する作りのため一致せず、`Serial::resolve_pins()`が
  `_base`を`nullptr`のままにし、コンストラクタ内の`panic()`が`static`初
  期化時（`setup()`実行前）に発火していた。`arduino_io.h`のinclude・
  リナンバリングが効く**前**に`constexpr int SERIAL1_TX_PIN = MB_TX;`等
  で生の値を退避し、`Serial1`の構築にはその退避値を使うよう修正
- **実機確認済み（3用途の排他切り替えを含む）**: `test_Serial1_MikroBus_N947`
  （`MB_TX`-`MB_RX`ループバック）で波形・Serial出力とも問題なしを確認。
  さらに`onboard_temperature_sensor`（`Wire1`でI3Cアクセス）→
  `test_digitalWrite_mikrobus_pins`（プレーンGPIO）→
  `test_Serial1_MikroBus_N947`（UART）の順に実機で連続実行し、同じ2本の
  物理ピンがI3C・GPIO・UARTの3モードを正しく切り替えられることを確認
