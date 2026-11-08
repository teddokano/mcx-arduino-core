# FRDM-MCXN236の前提と決定事項（0.9.0）

CLAUDE.mdが大きくなったので、読むときだけ開くようにここへ移した（2026-11-09、ユーザーが決定）。中身は移した時点のまま（「下の項目」「上の項目」などの参照は元のCLAUDE.mdでの位置のまま）。

## FRDM-MCXN236の前提と決定事項（0.9.0）
0.9.0（2026-10-09リリース）で対応した。回路図は`ref/FRDM-MCXN236.pdf`（Rev C）。いまも効いている決定と、回路図・SDKで確かめた事実だけをここに置く。
実機確認とPWMの修正、リリース準備の経緯は[docs/DEVELOPMENT_LOG.md](docs/DEVELOPMENT_LOG.md)のv0.9.0の節、実機確認の結果はvariantのREADME。

- **基板はRev C**（2026-10-05、ユーザーがシルクで確認）。Rev BでD18/D19が`P4_0`/`P4_1`から`P1_16`/`P1_17`に変わった。
  コア同梱の`r01lib`のN236分岐は、`io.h`が`I2C_SDA`/`I2C_SCL`を`A4`/`A5`とし、`i2c.cpp`が「D18/D19だけ」とするなど、新旧の版が混ざっているので、そのまま信用しない
- **SDKは`ref/SDK_2_16_000_FRDM-MCXN236.zip`**（2.16.000）。`cores/arduino/sdk/`の共有ドライバ32ファイルはこのzipのものとバイト単位で一致（2026-10-05に確認）。
  MCUのパッケージは`MCXN236VDFT`なので、ピンのALTはZephyrの`MCXN236VDF-pinctrl.h`で確かめる
- **オンボードMCU-LINKは`LinkServer probes`の`Device`列に`MCXN236`を返す**（`MCU-LINK FRDM-MCXN236 (r0E7) CMSIS-DAP V3.130`）。gdb-bridgeはA156（`Device`列が空）と区別できる
- **I²Cの割り当て（2026-10-05に決定）**: **`Wire`=D18/D19（`LPI2C5`、`P1_16`/`P1_17`）、`Wire1`=`LPI2C2`（`P4_0`/`P4_1`）**。I3Cは使わない（I3Cのセンサーが無く、I3C1はD18/D19でしか出ない）。
  `Wire1`のバスにはオンボードの加速度センサーFXLS8974CF（`0x18`、SA0はR191でGND）とMikroBusの`MB_SCL`/`MB_SDA`がつながる（ほかにFlexIOヘッダ・PMOD・カメラ・PTN5150・CODECにも）。プルアップはどちらのバスも4.7kΩ。
  **FC2は`Serial1`（D0/D1、`P4_3`/`P4_2`）のLPUARTでもあるので、LP_FLEXCOMM2はLPUARTとLPI2Cを同時に使うモード（`LP_FLEXCOMM_PERIPH_LPI2CAndLPUART`）で動かす**。
  `Serial1`と`Wire1`のどちらが先に初期化されても、後のほうが先のほうのモードを消さないこと
- **センサーの検証（2026-10-05に決定）**: `SENSOR_WIRE`はA153・N947と同じく`Wire1`。`release_check/01`のセンサーの項目は、N236ではFXLS8974CFのWHO_AM_I（`0x86`、下の項目）と、
  動作させた静止時の加速度の大きさが約1gになることを見る。P3T1755を読むサンプルは、外付けのP3T1755（`release_check/21`のモジュール）をMikroBus（=`Wire1`）につなげばそのまま動く見込み
- **加速度センサーの割り込み線はMCUにつながっていない**（R13＝INT1-`P0_21`、R14＝INT2-`P0_24`、R18＝WAKEUP-`P0_23`がすべて未実装）。D4（`P0_21`）・D8（`P0_23`）は空いている
- **SPIは付け替え不要**: SJ1・SJ2は工場出荷時に1-2で、D10=`P1_3`、D11=`P1_0`（`LPSPI3`）。D12=`P1_2`、D13=`P1_1`
- **J3のPWM 6本はFlexPWM1で、D-ピンと同じピン**: `P3_12`（D3、A0）・`P2_7`（D5、B0）・`P3_14`（D9、A1）・`P3_15`（B1、SJ1をB側にしたときのD10）・`P3_16`（A2、SJ2をB側にしたときのD11）・`P3_17`（D6、B2）。
  サブモジュールの組はD3とD5、D9と`P3_15`、`P3_16`とD6
- **アナログ入力はA0・A1・A2・A4・A5の5本（2026-10-06に変更。10-05の決定ではA0・A4・A5の3本だった）**。
  A0=`P4_6`、A1=`P4_15`、A2=`P4_16`、A4=`P4_12`、A5=`P4_13`。`NUM_ANALOG_INPUTS`は5。
  A3（`P4_17`）は青色LEDと共用（切り離せない）なので、**デジタルピンとして残し、`analogRead(A3)`は`-1`を返してユーザーに知らせる**
  （NaNの代わり。panicで止めない）。`-1`はどの分解能（最大16ビット）でも正しい読み値にならない。**`-1`を返すのはN236のA3だけ**で、ADCのチャネルが無いピン
  （A153の`A4`/`A5`など）は従来どおりpanicで止まる——N236のA3はチップにはチャネルがあり、ボードの配線の都合で使わないだけ、という違いで書き分ける。
  回路図で確かめたA1・A2の工場出荷時のつながり（2026-10-06）:
  A1はR25（0Ω）でCANトランシーバTJA1057（U11）の`RXD`（出力）につながる。トランシーバは`VCC`=P5V0（R179）、`VIO`=VDD_BOARD（R180）で常に電源が入り、
  `S`はR185（100kΩ）でGNDに引かれてノーマルモードなので、`RXD`はバスが無いとレセッシブ（High）を出し続ける。
  A2はR67でトランシーバの`TXD`（入力、VIOへの内部プルアップがある）につながる。`USB1_OTG_PWR`（NX5P3090の`EN`）との間のR92は未実装（DNP）で、A3と`USB1_OTG_OC`の間のR88も未実装。
  **N236のボードはR25・R67を外すことを前提にする**（2026-10-06に決定。A156のR75・R76と同じ扱い）。外すとA1・A2はトランシーバから切り離され、FlexCANは使えなくなる。
  開発用のN236（Rev C）からはR25・R67を外した（2026-10-06、ユーザーが実施）。A1・A2の`analogRead()`は実機で確認済み。
  ピン関連の文書（`PIN_MAPPING_N236.md`、variantのREADMEなど）に必ず明記する
- **UARTは`Serial`（USB、FC4、`P1_8`/`P1_9`）と`Serial1`（D0/D1）の2つ、SPIは`SPI`だけ（2026-10-05に決定）**。
  MikroBusのUART（`MB_RX`/`MB_TX`）はD0/D1と同じピンなので`Serial2`は無い。MikroBusのSPIはArduinoのSPIと同じ線（FC3）で、
  CSが`P1_16`（=D18、`Wire`のSDA）なので、独立した`SPI1`も置かない（`arduino_spi.{h,cpp}`でN236だけ`SPI1`を定義しない）
- **`PWM0`〜`PWM5`は他の3ボードと同じ並び（2026-10-06に決定）**: `PWM0`/`PWM1`がsm2のB/A、`PWM2`/`PWM3`がsm1、`PWM4`/`PWM5`がsm0。
  `PWM0`=`P3_17`(D6)、`PWM1`=`P3_16`、`PWM2`=`P3_15`、`PWM3`=`P3_14`(D9)、`PWM4`=`P2_7`(D5)、`PWM5`=`P3_12`(D3)。回路図にPWM0〜5のシルクは無い。
  **PWMを出せるピンは物理的に6本で、4本はD-ピンと同じピン**（A156の「独立した10本」とは違う）。D2（`P2_0`）も`PWM1_A3`を出せるが、使っていない
- **D3・D5・D6・D9の`analogWrite`は、物理ピンが`PWM0`〜`PWM5`と同じなので、そのままPWMになる**（`analogWrite()`は物理ピンでPwmOutの表を引く）。
  `digitalPinHasPWM()`もD3/D5/D6/D9で真（0.9.0）
- **`release_check`でのN236の扱い**: `01`の`isize 0`の項目は、FXLS8974CFが送ったぶんだけレジスタのポインタを進める（P3T1755は進めない）ので、N236だけ直前にポインタを書き直す。
  `13`は`PWM0`がD6なので、サーボのパルスをD6-D7のジャンパ経由でD7で測る（D8のジャンパは要らない）。`24`はA153と同じ側（`0x42`・先攻）で、相手は常にN947（同じ側だと2枚とも相手を待って進まない）。
  `21`はオンボードにP3T1755が無いので、N947のオンボードのP3T1755を、N236の`MB_SDA`-N947の`MB_RX`、`MB_SCL`-`MB_TX`、GND-GNDでつないで読む（N947は`hello_world`）。
  N947のセンサーが応答しないときは、N947のリセットでは戻らずUSBの抜き差しで戻った。LinkServer 26.9.130はN236のフラッシュを正しく判定する
- **`PwmOut`の作り（0.9.0で書き直した）**: チップごとの表（FlexPWMのインスタンス・リセット・クロック・ALT・クロック源、各ピンのインスタンス・サブモジュール・チャネル）と1つの本体。
  周期と両チャネルのパルス幅を組（サブモジュール）ごとに持ち、`apply()`が両チャネルを書き直す。相手が動いている組に後から加わるピンは、相手の周期を引き継ぐ（`analogWrite()`の初回も1kHzに戻さない）。
  A156は`D6`/`D9`がFlexPWM1のsm0（A/B）、`D5`がsm1のA、`D3`がsm2のA（ALT7。D3は赤色LEDと共用）。
  `pinMode()`は`analog_pin_to_gpio()`でPwmOutの持ち主の記録を外し、次の`analogWrite()`が`claim_pin()`でPWMに戻して`pinMode()`のGPIOオブジェクトを消す（mcxPinStateで持ち主が1つになる）。
  確認は`release_check/08`（`test_analogWrite_pairs_and_pinMode`を含む）
- **FC2の共有の実装**: SDKの`LPUART_Init()`・`LPI2C_MasterInit()`・`LPI2C_SlaveInit()`はFlexCommのモードを自分の分だけにし、
  `LPUART_Deinit()`・`LPI2C_MasterDeinit()`はFlexComm全体をリセットする。そこでN236のFC2だけ、初期化のあとで`flexcomm_keep_shared()`（`mcu.h`）が両方のモードに戻し、
  Deinitを呼ばない（`Serial::reinit()`、`I2C::~I2C()`）。割り込みは`Serial.cpp`の`LP_FLEXCOMM2_IRQHandler()`が`Serial1`とSDKのハンドラの両方に渡す。
  D0/D1は`FC2_P3`/`FC2_P2`で、LPUARTがここに出るのは両方のモードのときだけ（回路図のラベル`FC2_UART_RXD`/`TXD`による）
- **加速度センサーのWHO_AM_Iは`0x86`（レジスタ`0x13`）**。2026-10-06にユーザーがデータシートで確認し、以前の別のアプリケーションでもこの値を読んでいる。ZephyrのFXLS8974ドライバと、NXPのレジスタ定義（ユーザーのFXLS89xx_Arduinoライブラリの`fxls896x.h`）とも一致する。
  `release_check/01`はN236でこの値と、静止時に約1gを見る。Wireを`Stream`として使う確認では、P3T1755の`T_LOW`の代わりに`OFF_X`/`OFF_Y`（`0x22`/`0x23`）を書いて戻す
