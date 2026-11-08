# FRDM-MCXC444の前提と決定事項（0.10.0）

CLAUDE.mdが大きくなったので、読むときだけ開くようにここへ移した（2026-11-09、ユーザーが決定）。中身は移した時点のまま（「下の項目」「上の項目」などの参照は元のCLAUDE.mdでの位置のまま）。

## FRDM-MCXC444の前提と決定事項（0.10.0）
0.10.0（2026-10-18リリース）で対応した（2026-10-16に決定）。回路図は`ref/FRDM-MCXC444.pdf`（SCH-93440 Rev A）、SDKは`ref/SDK_2_16_000_FRDM-MCXC444.zip`。
「ビルドが通るだけ」のスパイクの結果は[memo/notes_toward_1.0.0.md](memo/notes_toward_1.0.0.md)の§4-5（`c444-spike`ブランチ、未push）。スパイクのスタブは持ち込まず、`0.10.0-dev`の上で作り直した。

- **Kinetis系でCortex-M0+（ARMv6-M、FPU無し）**、48MHz。DWTのサイクルカウンタ・CFSR/HFSR・`MSPLIM`が無い。周辺回路はLPUART/UART・`fsl_i2c`・`fsl_spi`・ADC16・TPM・PIT/LPTMRで、CTIMER・UTICK・I3C・FlexPWM・LPADCは無い
- **ピンは回路図とZephyrの`MCXC444VLH-pinctrl.h`で確かめた**（2026-10-16）。r01libの`io.h`のC444の表（D0〜D19・A0〜A5）は回路図と一致する。
  D0=PTA1、D1=PTA2、D2=PTD3、D3=PTA12、D4=PTA4、D5=PTA5、D6=PTE29、D7=PTE30、D8=PTA13、D9=PTD2、D10=PTD4、D11=PTD6、D12=PTD7、D13=PTD5、D18(SDA)=PTE0、D19(SCL)=PTE1。
  A0〜A3=PTB0〜PTB3、A4=PTC2、A5=PTC1。LEDは赤=PTE31、緑=D13、青=D6
- **フラッシュ設定のFOPTは`0x3B`にする**（SDKの`startup_mcxc444.cpp`のままだと`0x3F`）。D4（PTA4、SW3）はリセット直後NMIの端子で、LOWのまま起動するとNMIで止まる。
  `0x3B`は`NMI_DIS`を0にしてNMIを常に止める。r01libのC444のテンプレートのプロジェクトと同じ。`FSEC`は`0xFE`（保護なし）。`0x3D`（`BOOTPIN_OPT`）は別の効果で対策にならない
- **`Serial`はUSBのシリアルとD0/D1の両方**（同じLPUART0、2026-10-16にユーザーが決定。UNO R3と同じ）。**`Serial1`は置かない**（2026-10-16にユーザーが決定）。
  MikroBusのUART（LPUART1、D14/D15=PTE0/PTE1）は`Wire`と同じピン、もう1つのUART2（D2/D9）はLPUARTではない旧型のUARTで、後から足すことはできる。
  LPUARTに7ビットのモードが無い（`SERIAL_7xx`は工夫が要る）
- **`Wire`=D18/D19（I2C1、ALT6）、`Wire1`=オンボードの加速度センサーFXLS8974CFのバス（I2C0、PTE24/PTE25、ALT5、4.7kΩのプルアップ、ヘッダには出ていない）**（2026-10-16にユーザーが決定。N236と同じ形）。
  センサーは`0x18`（SA0はR36でGND）、INT1はPTD1、CS/WAKE_UPはR40（0Ω）でD11につながる。`SENSOR_WIRE`は`Wire1`。
  A4/A5（PTC2/PTC1、ALT2）も`Wire`と同じI2C1なので、2つ目のバスにはならない。0.12.0から`Wire.setPins(A4, A5)`で`Wire`がA4/A5へ移れる（`Wire1`は移れない。Pendingタスク10）
- **`SPI`=D10〜D13（SPI1、ALT2）**。MikroBusのSPIも同じ線なので`SPI1`は置かない（N236と同じ）。`MCXC444.h`が`SPI0`/`SPI1`をマクロで定義しているので、Arduinoの`SPI1`とぶつからないようにする
- **PWMはTPM**: D3=TPM1_CH0、D10=TPM0_CH4、**D5・D6・D9は3本ともTPM0_CH2**（同時に別のデューティにできない）。1つのTPMの全チャネルは周期を共有する。
  **D5・D6・D9は最後に`analogWrite()`した1本だけがPWMになり、前のピンはGPIOのLOWに戻す**（2026-10-16にユーザーが決定）。赤のLED（PTE31）はTPM0_CH4でD10と同じチャネル。
  `PWM0`〜`PWM5`のピンは無い
- **`cores/arduino/sdk/`の`fsl_i3c`・`fsl_pwm`・`fsl_spc`・`fsl_utick`・`fsl_inputmux`はMCXの4つのvariantへ移した**（2026-10-16、ユーザーが決定）。
  `cores/arduino/sdk/`の`.c`は全ボードでビルドされるので、C444に無い周辺回路のドライバは置けない。zipと同じファイルを4つのvariantに置いている
- **RMとデータシートで確かめたこと（2026-10-17）**: TPMの分周比（`SC[PS]`）はカウンタを止めたときしか書けない。デューティ100%は`CnV > MOD`で、そのためMODは`0xFFFF`未満にする。
  ADC16の校正は平均32回で、ADCのクロックは4MHz以下が望ましい。16ビットのモードのADCのクロックは2.0〜12.0MHz（データシート）。
  FTFAは4バイト単位で書き、消去せずに同じ場所へもう一度書けない。命令の実行中はそのフラッシュを読めない（SDKのドライバは待ちをRAMで行う）。
  PITとLPUART0/1はバスクロック、I2C0・I2C1・SPI1はシステムクロック、SPI0はバスクロック。デジタルの機能のピン（TPMを含む）は`GPIOx_PDIR`で読める。
  `SIM_COPC`（ウォッチドッグ）はリセット後に一度だけ書ける（最初の書き込みでカウンタが0から数え始める）。カウンタの値は読めない。
  回路図: A4/A5（PTC2/PTC1）にはR26・R27（4.7kΩ）のプルアップがある。D18/D19にはプルアップが無い（`Wire.begin()`の内蔵プルアップで持ち上がる）。SW3（D4）はR15（10kΩ）でプルアップ。**VREFHの端子はJP6が開いていてどこにもつながっていない**（ボードの`board.h`が`BOARD_ADC_USE_ALT_VREF`を立て、SDKのADC16の作例はVALTH＝VDDAを使う。2026-10-17に「JP6で3.3V」と書いたのは誤りで、実機でVREFHを基準にすると全部の入力が最大値になった）
- **C444の確認の記録と、実機で見つけて直した不具合（2026-10-17〜18）**: RM・データシートの確認、`release_check`の結果、`test_bringup_C444`、`07`のCOP、`21`、`test_fault_report`は、DEVELOPMENT_LOGの「CLAUDE.mdから移した記述」に移した。**TPMの`CnV`・`MOD`は周期の終わりまでバッファで待つ**（`pwm_tpm_service()`がSysTickで書く）、ADC16のクロックと基準電圧（VALTH）、SPI1のバスクロックなどもそこにある
- **`Wire`の`setWireTimeout()`とターゲットモード（2026-10-18）**: SDKの`I2C_MasterTransferBlocking()`は待ちに上限が無く（`I2C_RETRY_TIMES`が0）、SCLを押さえられると止まったままになるので、
  コントローラの転送はr01libの`transfer_kinetis()`（`i2c.cpp`）で自前に書いた。タイムアウトはSMBusのSCL low timeout（`SLTH`/`SLTL`、モジュールのクロックの64分の1で数える）で、**SCLだけ**を見る。上限はどの速度でも約87ms。
  コードは`uint8_t`に切り詰めて、タイムアウトが`80`、アドレスのNAKが`81`、データのNAKが`78`、ビジーが`76`。SDAを押さえられたまま始めるとバスがビジーで即座に`76`、SCLを押さえられたまま始めるとタイムアウトになる。
  SLTFはIICIFも立てるので、IICIFが立ったあとにもSLTFを見る（見ないと成功として返った）。
  ターゲットは`arduino_i2c.cpp`がI2Cの割り込みで動かす（SDKの`I2C_SlaveInit()`は同じモジュールのマスターの設定も書き換えるので使わない）。**Kinetisの1つのI2Cはマスターかスレーブのどちらかで、自分のターゲットを呼べない**
  （LPI2Cは両方が独立して動く）。そのため`release_check/01`と`14`のターゲットの節、`test_Wire_target_self`はC444ではSKIPを出し、ターゲットモードは`24`（N947との2枚）だけで確かめる。
  **コントローラの転送の間は`C1[IICIE]`と`FLT[SSIE]`の両方を止める**: SSIEが立っていると自分の転送のSTARTでもIICIFが立ち、アドレスを送り終える前に「1バイト終わった」と読んでいた
  （`24`で、応答の確認だけが通り、データを送る項目が全部FAILした）。`14`（D19-D8・D18-D7、ターゲットの節はSKIP）と`24`（C444が`0x42`・先攻）がALL OK
- **C444の実機で見つけて直した不具合（2026-10-18）・できているもの・全作例のビルド・`analogRead()`の電圧の確認**: ポートのクロック、GPIOのALT（Kinetisは1）、ADCの基準電圧、`delayMicroseconds()`・`delay()`（M0+ではSysTickのカウンタを数える）、I2CのSCLとリピーテッドスタート、SPIのSCK、TPMの`CnV`・`MOD`のバッファと`pwm_tpm_service()`の時刻の見積もりは、DEVELOPMENT_LOGの「CLAUDE.mdから移した記述」に移した
- **スタックあふれの扱い（2026-10-18、ユーザーが決定）**: M0+には`MSPLIM`もMPUも無い。前は、あふれるとヒープ・bss・dataを上書きし、RAMの下端（`0x1FFFE000`）の下の予約領域に書いた所でバスエラーになっていた。
  HardFaultを積む場所が無いのでLOCKUPになり（RMの6.2.2.7）、リセットしてスケッチがまた動き、何も報告されずにリセットを繰り返した。
  いまは**スタックをRAMの下端に置き**（`MCXC444.ld`。data・bss・noinit・ヒープは上端に詰める）、あふれた最初の書き込みで何も上書きせずにLOCKUPのリセットになる。
  **起動時に`RCM_SRS1[LOCKUP]`を見て**（`mcu.cpp`の`start_mcu()`）、`error: HardFault: stack overflow, most likely (the core locked up and reset)`を出してSOSで止まる。
  LOCKUPはHardFaultの処理の中の故障でも起きるので「most likely」。実機で確かめたこと: 予約領域への書き込みはHardFault、MSPを下端にして`push`するとLOCKUPでリセット（`SRS1`=`0x2`。書き込みのあとは`0x4`）、`test_fault_report`のkind 4・2・5、LOCKUPで止まったあとのRESETボタンでスケッチが普通に動く（`SRS0`=`0x40`、`SRS1`=`0`）。
  ほかに試して使えなかった案: GCCの`-fstack-limit-symbol`はM0+では警告なしに何も生成しない。SysTickでの見張りは遅すぎる（再帰なら1msでRAMを何周もする）。
  リンカスクリプトの注意: 空の出力セクションに付けたアドレスは、セクションごと捨てられて効かない（`.stack`は`. +=`で中身の大きさを持たせた）。
  RAMの各セクションは8バイトで始まり8バイトで終わるので、すき間も大きさに入り、IDEの「ローカル変数で使える」がスタックの大きさと一致する（`test_fault_report`で20224バイト）。
  HardFaultの処理で新しいスタックに移る判定は、C444では`_vStackBase`+1KBより下

## 動作確認済み
（APIごとの確認の表は、DEVELOPMENT_LOGの「CLAUDE.mdから移した記述」に移した。対応しているAPI・していないAPIは[API_COMPATIBILITY.md](API_COMPATIBILITY.md)。）
