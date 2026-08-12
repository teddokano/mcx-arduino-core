# mcx-arduino-core 開発引き継ぎドキュメント

## プロジェクト概要
- **リポジトリ**: https://github.com/teddokano/mcx-arduino-core
- **現在のバージョン**: v0.2.0（`package_nxp_mcx_index.json` 上の最新リリース。`prepare0.1.9`→`main`マージ・GitHub Release作成済み、`f393132`でchecksum自動更新も確定）
- ブランチ名は`0.1.9`のままだがリリースバージョンは`0.2.0`に変更 — analogRead/analogWrite/millis/micros/tone/noToneの追加で基本的なArduino API群が揃ったためマイナーバージョンを上げる判断
- **内容**: NXP FRDM-MCXA153 (Cortex-M33) 向けArduino IDEボードサポートパッケージ
- **作業中バージョン**: v0.2.1（`prepare0.2.1`ブランチ、`main`未マージ・未リリース）

---

## 現在の作業状況（完了済み）

### MacでのGCCインストール ✅
- GCCツールチェーンをARM公式からxPack（`https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack`）に切り替え
- `package_nxp_mcx_index.json` のxPack GCC checksumを全OS分正しく設定
- macOS / Windows ともにボードマネージャーからのインストール・基板動作確認済み

---

## package_nxp_mcx_index.json の現在の構成

| OS | ツール | URL |
|---|---|---|
| macOS arm64 | xPack 14.2.1-1.1 | `xpack-arm-none-eabi-gcc-14.2.1-1.1-darwin-arm64.tar.gz` |
| macOS x86_64 | xPack 14.2.1-1.1 | `xpack-arm-none-eabi-gcc-14.2.1-1.1-darwin-x64.tar.gz` |
| Linux x86_64 | xPack 14.2.1-1.1 | `xpack-arm-none-eabi-gcc-14.2.1-1.1-linux-x64.tar.gz` |
| Linux arm64 | xPack 14.2.1-1.1 | `xpack-arm-none-eabi-gcc-14.2.1-1.1-linux-arm64.tar.gz` |
| Windows | xPack 14.2.1-1.1 | `xpack-arm-none-eabi-gcc-14.2.1-1.1-win32-x64.zip` |

xPack checksums（正しい値）：
- darwin-arm64: `SHA-256:f52ea3760c53b25d726a7345be60a210736293db85f92daa39d1d22d34e2c995`
- darwin-x64:   `SHA-256:b5bf8d5af099fd464d1543e5b8901308fb64116fa7a244426cacf4ff1b882fc7`
- linux-x64:    `SHA-256:ed8c7d207a85d00da22b90cf80ab3b0b2c7600509afadf6b7149644e9d4790a6`
- linux-arm64:  `SHA-256:a1ac95c8d9347020d61e387e644a2c1806556b77162958a494d2f5f3d5fe7053`
- win32-x64:    `SHA-256:0b2d496b383ba578182eb57b3f7d35ff510e36eda56257883b902fa07c3bba55`

---

## v0.1.5 で修正した内容（完了済み）

### arduino_i2c.cpp
- グローバル `I2C i2c(...)` を `begin()` 内の遅延初期化（`new I2C()`）に変更
- Arduino IDEの `--no-whole-archive` リンク方式での初期化順序問題を解消

### arduino_spi.cpp
- グローバル `r01libSPI spi(...)` を `begin()` 内の遅延初期化に変更
- `beginTransaction()` で設定変更があったときのみ `frequency()` / `mode()` を呼ぶよう変更
  → 毎回呼ぶとLPSPI再初期化時にPCSが誤ってアサートされる問題を解消

### arduino_serial.cpp / arduino_serial.h
- `print(double)` を整数演算ベースの実装に変更（`nano.specs`環境での`snprintf`浮動小数点非対応問題を解消）
- `printf()` を自前パーサーに変更（`%f`/`%g`/`%e`を整数演算で処理）
- `print(double, digits=2)` / `println(double, digits=2)` に桁数引数を追加

### arduino_i2c.h
- `begin(int baud=100000)` / `requestFrom(..., size_t, ...)` シグネチャ更新

---

## v0.1.6〜v0.1.8 で修正した内容（完了済み・リリース済み）

### v0.1.6（`rel/v0.1.6`）
- Arduino API命名・互換性の修正（`58f4951 fix: correct Arduino API naming and compatibility issues for v0.1.6`）

### v0.1.7（`prepare0.1.7`, ディレクトリ構成変更含む）
- `db3b01d` Wire動作の修正、`af4c690` NAKによるハングアップ修正
- `e77651f` SPI動作修正（CSアサートがマルチバイト転送終了まで継続するよう修正）
- `a9a2126` Serialインスタンスをstaticインスタンスに変更
- `e9dcc81` ヒープサイズ調整
- `directory_structure_change2` ブランチをマージ：`arduino_serial.cpp/.h` を大幅整理（145行→大幅削減）、`arduino_i2c.cpp`・リンカスクリプト等のパス構成見直し

### v0.1.8（`b1edd25 for 0.1.8 release`）
- `6abc09e fix: return value of "TwoWire::requestFrom"` — 戻り値の不具合修正
- `package_nxp_mcx_index.json` のchecksum更新（GitHub Actionsによる自動更新）

---

## v0.2.0 で作業中の内容（`prepare0.1.9` ブランチ・未リリース・すべてコミット済み）

analogRead/analogWrite/millis/micros/tone/noToneが揃い、基本的なArduino APIが一通り使えるようになった（マイナーバージョンを0.1.9ではなく0.2.0とする判断の理由）。

### I3C/Wire1対応（P3T1755温度センサー、`09b3a5d`）
- オンボードのP3T1755温度センサーをI3Cバス経由でI2Cモードとして使えるように対応
- `TwoWire Wire1( I3C_SDA, I3C_SCL );` を新規追加、`TwoWire::begin()` でSDA/SCLピンが `I3C_SDA`/`I3C_SCL` と一致する場合は `I3C` インスタンスを生成し `mode( I3C::MODE::I2C_MODE )` に設定
- 使用例: `examples/Arduino_compatible_API/onboard_temperature_sensor/onboard_temperature_sensor.ino`（`2206dd2`。旧`test_Wire_P3T1755`はデバッグ経緯の記録として残置）

### analogRead / analogWrite 実装（LPADC・FlexPWM0、`e476f36`）
`/Users/tedd/dev/mcuxpresso/r01lib_prj_generator/` で生成されたFRDM-MCXA153向け `AnalogIn`（LPADC）/ `PwmOut`（FlexPWM0）クラスを移植し、Arduino API化。
- **r01lib本体**: `AnalogIn.h/.cpp`（12bit LPADC、A0-A3対応）、`PwmOut.h/.cpp`（FlexPWM0 sm0-2、PWM0-PWM5対応）を新規追加。`io.h` に `PWM0`〜`PWM5`（P3_6〜P3_11、既存のD0-D19とは物理的に重複しない新規ピン）を追加。依存する `fsl_lpadc.c/h` SDKドライバを `drivers/` に追加
- **Arduinoレイヤー**: `arduino_layer/arduino_analog.cpp/.h` 新規。`analogRead(pin)` は16bit値を10bit（0-1023）に変換、ピンごとに `AnalogIn` を遅延生成。`analogWrite(pin, value)` は0-255のduty値を `PwmOut` に反映、初回生成時に周期1kHzを設定
- PWM0-PWM5の物理コネクタ位置は `examples/Arduino_compatible_API/test_PWM_pin_identify/` で実機確認済み、問題なし

### millis() / micros() 実装（SysTick + DWT、`cef7f8b`）
- DWT->CYCCNTは`wait()`/`delay()`が既に有効化しているが、単体では96MHz時に約45秒でオーバーフローするため、SysTickを1msティックに設定してカウントを32bit ms全体（~49日、本家Arduino相当）まで拡張
- `arduino_main.cpp`（`cores/arduino/`・`arduino_layer/`の両方）に `SysTick_Handler()` をオーバーライドして実装、`millis()`/`micros()`は初回呼び出し時に遅延初期化
- 実機バグ: 初回`micros()`呼び出し時、SysTickの初回ティックがまだ発火していないと`ms_tick_dwt`基準値が0のままで、起動からの生DWTカウントがそのまま返っていた（`while(!Serial)`待ち時間が丸ごと出るなど）。`millis_init()`でDWT有効化直後に基準値を明示的にスナップショットするよう修正（`ddb3714`）

### tone() / noTone() 実装（CTIMER0、`7bb5615` + `ddb3714`）
- analogWriteのFlexPWM0はPWM0-5専用ピンに限定されるため、tone()は任意のデジタルピンに対応する必要があり、未使用だったCTIMER0（このMCUに3系統ある）のMatch割り込みでGPIOをソフトウェアトグルする方式で実装
- 依存ドライバ `fsl_ctimer.c/h` を別プロジェクト(`IchigoJamMcx_GPIO`)から移植。コピー元のSDKバージョンが新しく`MSDK_REG_SECURE_ADDR`マクロ未定義エラーが出たため、非TrustZoneビルドではno-opになるフォールバック定義を追加
- 実機バグ1: CTIMER0のクロックmuxが未アタッチで`CLOCK_GetCTimerClkFreq()`が0を返し、match値がオーバーフローして割り込みが一切発火しなかった → `CLOCK_AttachClk(kFRO12M_to_CTIMER0)`を追加
- 実機バグ2: `CTIMER_StopTimer()`はカウンタ値をリセットしないため、周波数を下げて次のtone()を呼ぶとカウンタが新しいmatch値に到達するまで（最大数分）止まったままになる → `CTIMER_StartTimer()`前に`CTIMER_Reset()`を追加

### 重大バグ: I3C使用時のBusFault（`ddb3714`）
`I2C(sda, scl, no_hw=true)` — I3Cのコンストラクタが委譲するベースクラスコンストラクタ — は `if (no_hw) return;` でハードウェア初期化を完全にスキップするため、`I2C::unit_base` が未初期化のまま残る。I3C自身の `frequency(uint32_t,uint32_t,uint32_t)` はシグネチャが異なるため `I2C::frequency(uint32_t)` を**オーバーライドではなく隠蔽**するだけで、`TwoWire::begin()`内の汎用的な `i2c->frequency(baudrate)` 呼び出しは（`i2c`がI3Cインスタンスでも）`I2C::frequency(uint32_t)`にディスパッチされ、未初期化の`unit_base`経由でBusFaultを起こしていた。
- 発覚経緯: `test_combined_peripherals.ino`でI3C・millis・ADC・PWM・toneを同時使用するスケッチが特定の組み合わせ（I3C + tone）でのみ無応答になる不具合を、フォールトハンドラ（`HardFault_Handler`等）を仕込んでCFSR/BFARレジスタを直接UART経由でダンプして特定（`I3C_ENABLE=1, TONE_ENABLE=1`のみで再現、BFAR=0x0000000F付近＝ほぼNULLポインタ経由のアクセス）
- 修正: `TwoWire::begin()`でI3Cインスタンスの場合は`i3c->frequency(baudrate, 0, 0)`をI3C自身の3引数オーバーロードとして直接呼ぶよう変更
- 実機の`test_combined_peripherals.ino`で全機能同時動作を確認済み

### その他の修正
- **`-lm`リンク漏れ**（`ddb3714`）: `platform.txt`のリンクレシピに`libm`が含まれておらず、`ceilf`/`floorf`等の標準数学関数を使うライブラリが必ずリンクエラーになっていた。`-lc -lm -lgcc`に修正
- **ビルドツールチェーンの不一致**: ここまでの`.a`再ビルドはローカルのARM公式配布ツールチェーンを使っていたが、実際にエンドユーザーへ配布されるのはxPack版14.2.1-1.1。xPack版（`~/.xpacktools/`に展開、チェックサム一致確認済み）でクリーンリビルドし直し、全サンプルの回帰・実機動作を再確認済み
- **README.md**（`d99973f`）: API対応表でanalogRead/analogWrite/millis/micros/tone/noToneを✅に更新
- **`examples/tests/`の扱い**（`0d2daf9`）: 4つの外部ライブラリ（`I2C_device_Arduino`, `LCDDriver_NXP_Arduino`, `LEDDriver_NXP_Arduino`, `TempSensor_NXP_Arduino`、いずれも`github.com/teddokano/...`の独立リポジトリ）を`.gitignore`に追加し、このリポジトリの管理対象外に

### バージョン0.2.0への引き上げ（`ec7d3f4`）
- `package_nxp_mcx_index.json`（`version`/`url`/`archiveFileName`）と`platform.txt`（`version=0.2.0`）を`0.1.8`→`0.2.0`に更新
- checksum/sizeは意図的に古いまま（実際のリリースタグpush時に`update_package_index.yml`が自動計算・上書きする運用のため、この時点では未修正で問題ない）
- ブランチ名`prepare0.1.9`はリネームしない方針を確認済み（当初`0.1.9`予定だったが機能追加量から`0.2.0`が妥当と判断）

### UNO R3/R4互換性の調査・対応（`a0a0bb7`, `2ae1983`）
UNO R3（`ArduinoCore-avr`、ローカルインストール済み）・UNO R4（`ArduinoCore-API`/`ArduinoCore-renesas`、`gh api`で取得）を参照元に、本ボードでスケッチ互換性が壊れる箇所を洗い出して対応。ベースはいずれもLGPL 2.1だが、対応内容はArduino標準APIの定型的な再実装（数学定数の`#define`、`min`/`max`テンプレート等）にとどまるため、ユーザー判断によりMITライセンスの本リポジトリへの追加を許容（「問題ないと判断した」）。
- **`time_t`のあいまいなオーバーロード**（`a0a0bb7`）: `Serial.print(time_t)`が`SerialClass::print`のどのオーバーロードにも一意に決まらずコンパイルエラー（newlib-nanoの`time_t`は32bit ARMでも`__int_least64_t`＝64bit定義のため）。`print`/`println(long long)`・`(unsigned long long)`オーバーロードと`_print_num64`/`_print_unum64`を追加して解消
- **数学定数・互換マクロ一式**（`2ae1983`、`arduino.h`に追加）: `#include <math.h>`・`<cstdlib>`、`PI`/`HALF_PI`/`TWO_PI`/`DEG_TO_RAD`/`RAD_TO_DEG`/`EULER`、`radians()`/`degrees()`、`LSBFIRST`/`MSBFIRST`/`SERIAL`/`DISPLAY`、`boolean`/`byte`/`word`型、`min()`/`max()`テンプレート、`abs()`/`constrain()`/`sq()`、`lowByte()`/`highByte()`/`bitRead()`/`bitSet()`/`bitClear()`/`bitToggle()`/`bitWrite()`/`bit()`、`interrupts()`/`noInterrupts()`（`cpsie`/`cpsid`インラインアセンブラ）、`map()`
- 確認用スケッチ: `examples/Arduino_compatible_API/test_Serial_print_time_t/`, `test_math_constants/`, `test_arduino_compat_macros/`

### shiftOut/shiftIn, pulseIn/pulseInLong, random/randomSeed 実装（`7d7d334`）
- `arduino_io.cpp/.h`（`digitalWrite`/`digitalRead`/`micros()`ベースのソフトウェア実装）に`shiftOut`/`shiftIn`/`pulseIn`/`pulseInLong`を追加
- `random(long)`/`random(long,long)`/`randomSeed(unsigned long)`は`arduino.h`にインライン実装として追加（標準`rand()`/`srand()`ベース）
- shiftInの検証はshiftOutの実出力と組み合わせる方式を採用（物理的に同一ピンを同時にshiftOut/shiftInできない制約があるため、割り込みでクロックエッジを捕捉してデータを再構成するアプローチに合意して実装）
- 確認用スケッチ: `examples/Arduino_compatible_API/test_shiftOut_pulseIn_random/`

### Serial1（D0/D1ハードウェアUART）追加と関連バグ修正（`b215e5a`）
これまで`Serial`はUSBブリッジ経由のみで、D0/D1に直結したハードウェアUART出力が使えなかった。Wire/Wire1の前例に倣い`Serial1`を新規追加（`SerialClass Serial1(arduino_pin_by_number[D1], arduino_pin_by_number[D0]);`）。実装過程で実機テストにより2件の既存バグが判明・修正された。

- **`SerialClass`のコンストラクタをtx/rx pin引数対応に変更**: 従来は引数なし（`USBTX`/`USBRX`固定）だったのを`SerialClass(int tx_pin, int rx_pin)`に変更し、`Serial`/`Serial1`両方をこのクラスで表現できるように
- **バグ1（誤った当初仮説として一旦Alt4に変更→リバート）**: `pin_mux.c`のYAMLコメント順を「リスト位置＝ALT番号」と誤読し、D0/D1のmux値を`kPORT_MuxAlt3`→`kPORT_MuxAlt4`に変更したが改善せず。Zephyrプロジェクトのpinctrlヘッダ（`MCXA344VLH-pinctrl.h`、NXPリファレンスマニュアル同等のシリコン検証済み情報源）で両ピンとも実際は`Alt3`であることを確認し、`Alt3`/`Alt3`に戻して確定。他のCPUターゲット（MCXC444VLH/MCXA156VLL/MCXN236VDF/MCXN947VDF）は、`lpuart_pin_map_t`の`mux`フィールドを`tx_mux`/`rx_mux`に分割する機械的なリファクタのみで、値は一切変更していないことを確認済み
- **バグ2（真因）**: `pin_mux.c`の`BOARD_InitPins()`はUSBTX/USBRXの入力バッファは明示的に有効化しているが、D0/D1は未設定のままリセットデフォルト（入力バッファ無効）だった。`Serial::pin_mux()`はMUXフィールドしか設定していなかったため、`DigitalInOut::input_buffer(bool)`を新設し（`io.h/.cpp`、`PORT_PCR_IBE_MASK`を操作）、`Serial`コンストラクタで`rx_io.input_buffer(true)`を呼ぶよう修正
- **バグ3**: 入力バッファ修正後もRXが大半のバイトを取りこぼす問題が残存。`SerialClass`が`Serial::attach()`を一度も呼んでいなかったため、RX割り込みが有効化されず`readable()`/`getc()`が常に1バイトのハードウェアレジスタを直接ポーリングする方式（`delay()`等のブロッキング処理下でオーバーランしやすい）のままだった。`SerialClass::begin()`内で`attach([]{}, RxIrq)`を呼ぶよう修正
- **`available()`が0/1しか返さない問題**: `Serial::available()`をベースクラスに新設し、RXリングバッファの占有バイト数（`(_rx_head - _rx_tail) & (RX_RING_BUF_SIZE - 1)`）を返すよう実装。`SerialClass::available()`はこれに委譲
- 3件とも実機での逐次検証により確定（ユーザーがシリアルモニタ出力を都度報告、空文字列→部分受信→全文受信の順で切り分け）
- 確認用スケッチ: `examples/Arduino_compatible_API/test_Serial1/`（D1-D0ジャンパでのループバックテスト）

### ドキュメント整備（`LICENSE`/`CHANGELOG.md`新規作成、README拡充）
- **`LICENSE`ファイルが存在しないままREADMEからリンクされていた**（リンク切れ）ことが判明し、新規作成（MIT License, Copyright (c) 2026 Tedd OKANO）
- `LICENSE`に「Third-Party Notices」セクションを追加：UNO R3/R4互換マクロ群（数学定数・`min`/`max`・ビット操作マクロ・`map`・`random`等）がArduinoCore-avr/ArduinoCore-API（いずれもLGPL 2.1）のインターフェースに合わせた独自再実装であり、コードのコピーではない旨を明記。同内容の帰属コメントを`arduino.h`の3箇所の同期コピーにも追加
- **`CHANGELOG.md`を新規作成**（Keep a Changelog形式）。既存のGitHub Releases（v0.1.0〜v0.1.8、`gh release list`で確認・全リリースタグ作成済みと判明）の内容と`CLAUDE.md`の技術詳細を突き合わせて過去分を整理し、v0.2.0をUnreleasedセクションとして記載。README冒頭からリンク
- **README.mdに「Pin Mapping (FRDM-MCXA153)」セクションを新規追加**：`io.h`のピンマクロ定義を実際に確認し、D0-D19/A0-A5/PWM0-PWM5と各ペリフェラル（Serial/Serial1/Wire/Wire1/SPI）の物理ピン対応表を作成。A4/A5はマクロとしては存在するがLPADCチャンネル未配線のため`analogRead`非対応である点も明記
- **`Wire1`が「I3Cペリフェラルを使っているがI2Cモードで動作している」ことの明確化**：README・API対応表の記述だけでは伝わりにくいと判断し、ピン対応表の下に注記を追加（I3C固有機能（動的アドレッシング、IBI、高速クロック等）は一切使用・公開していない旨を明記）
- API対応表に`analogWrite`のPWM周期が1kHz固定（変更不可）である旨のNotesを追加
- Pendingタスクから「IchigoJam-firm GPIO/PWMサポート」を削除（ユーザー指示）。tone/noTone実装セクション内の移植元プロジェクト名（`IchigoJamMcx_GPIO`）への言及は技術的な出典情報として残置

### 複合動作確認（Serial1込み、最終リリース前検証）
`test_combined_peripherals.ino`にSerial1のループバック検証（D1→D0送信、次ループ冒頭で受信・欠落チェック）を追加し、Serial1・I3C(Wire1)・analogRead・analogWrite・tone・millis/microsを同時に動かす実機テストを実施。WARNINGなし、`serial1`ループバック（`hb0`〜`hb5`等）の欠落なし、`temp`/`adc`とも安定、`pwmDuty`も規則通り変化することを確認 — Serial1追加後としては初めての全機能同時動作確認

### v0.2.0リリース完了（`main`マージ・GitHub Release作成・checksum確定）
- `prepare0.1.9`（`cea0e94`）→`main`へfast-forwardマージ・push（分岐なし、18コミット）
- リリースzip（`mcx-arduino-core-0.2.0.zip`、`hardware/nxp/mcx/`をgit archiveしビルド済み`.a`を追加したもの、13,020,412 bytes）を作成し、GitHub Release `0.2.0`（`main`のHEAD `cea0e94`をtarget）にアップロード: https://github.com/teddokano/mcx-arduino-core/releases/tag/0.2.0
- **判明した既知の問題**: タグpushで自動起動する`update_package_index.yml`は、`actions/checkout`がタグをdetached HEADでチェックアウトするため`git push`が失敗する（exit code 128）。過去のv0.1.6〜v0.1.8のタグpush時も同様に全て失敗しており、実際のchecksum確定は毎回リリース後に手動で`workflow_dispatch`を`main`ブランチに対して実行することで行われていたと判明（`gh run list`で過去の成功/失敗パターンを確認して特定）。v0.2.0でも同じ手順（タグpush→失敗を確認→`gh workflow run update_package_index.yml --ref main`で手動実行）でchecksum/sizeを確定（`f393132`、SHA-256:`4beef79ec0def9aff3c9d878336b650ffc758cf9ce8ab1a0b6c3a8f5571f96f7`、ローカルで計算した値と一致確認済み）
- 副次的に判明した軽微な問題（未対応・低優先度）: workflowの`Post Checkout`クリーンアップ時に警告が出る（`fatal: No url found for submodule path 'examples/tests/GPIO_NXP_Arduino' in .gitmodules`）。原因はリポジトリのツリーに`examples/tests/GPIO_NXP_Arduino`へのgitlink（`git ls-files --stage`で`160000`エントリ、コミット`db3b01d`由来）が残っているのに`.gitmodules`ファイル自体が存在しないこと（外部クローンを誤ってそのまま`git add`した名残と推測）。ジョブ自体は成功扱いで実害はないが、いずれ`git rm --cached examples/tests/GPIO_NXP_Arduino`等で整理してもよい

### チュートリアル新規作成（`837d202`）
- `TUTORIAL.md`（英語）・`TUTORIAL.ja.md`（日本語）を新規作成。インストール→オンボードLED点滅→Serial→デジタル入力/割り込み→analogRead→analogWrite→millis/micros→tone/noTone→Wire1（オンボードP3T1755）→SPI→Serial1→shiftOut/shiftIn/pulseIn/random→UNO R3/R4互換マクロ、の順に、それぞれ単体で書き込んで動くスケッチとして構成
- 新規に書き起こしたスニペット（`LED_BUILTIN`点滅、`SPI.endTransaction()`込みの例、`Serial1`単体例）は`arduino-cli compile`で実際にコンパイル確認してから掲載。既存の`examples/`のスケッチをベースにしたセクションはそのまま流用
- README.mdの冒頭からリンク（英語版をメイン、日本語版へのリンクも併記）

---

## v0.2.1 で作業中の内容（`prepare0.2.1` ブランチ・未リリース）

### delayMicroseconds() 実装
- v0.2.0公開後に`delayMicroseconds()`が未対応であることが判明。`delay()`と同じパターンで、r01libに既存の`wait_us()`（`mcu.h`/`mcu.cpp`、SDKの`SDK_DelayAtLeastUs()`ベース）を呼ぶだけの実装として追加
- `arduino.h`（3箇所の同期コピー）に宣言追加、`arduino_main.cpp`（2箇所の同期コピー）に実装追加
- 確認用スケッチ: `examples/Arduino_compatible_API/test_delayMicroseconds/`（`micros()`で前後を計測し、要求値との比較・WARNING表示付き）。実機確認済み
- README.mdのAPI対応表にも追加

### Arduino APIギャップ調査と`String`クラス新規実装
- v0.2.0公開後、Arduinoで標準的とされるAPIのうち未対応のものがないか総点検（Explore agentに依頼して網羅的に調査）。抜けていたもの: `detachInterrupt()`、`yield()`、ctype.h系ラッパー（`isAlpha`等）、`analogReference()`、`analogReadResolution`/`analogWriteResolution`、`String`クラス、`Serial.flush()`/`peek()`/`setTimeout()`/`readBytes()`/`parseInt()`等のStream系ヘルパー。`pow`/`sqrt`/`sin`/`cos`/`tan`は`arduino.h`が`<math.h>`を読み込み済みのため対応不要と判明
- 上記のうち`String`クラスをまず実装。本家ArduinoCore-avr/ArduinoCore-APIの`WString`（LGPL 2.1）を移植する案と、独自にゼロから書く案をユーザーに提示し、**独自実装（MITのまま統一）を選択**（本家移植だとその1ファイルだけLGPL表記を残す必要が出るため）
- 新規ファイル `arduino_string.h`/`arduino_string.cpp`（`arduino_layer/`に新規、`variants/frdm_mcxa153/include/`にはヘッダのみ同期 — `.cpp`実装は元々`hardware/`側には置かない構成のため）
- コンストラクタ（`const char*`, `std::string`, `char`, 数値各種＋基数/小数桁指定）、`+`/`+=`/`concat`、比較演算子、`c_str`/`length`/`isEmpty`、`charAt`/`operator[]`、`indexOf`/`lastIndexOf`/`substring`/`startsWith`/`endsWith`、`replace`/`remove`、`toUpperCase`/`toLowerCase`/`trim`、`toInt`/`toFloat`/`toDouble`を実装。ヒープ確保は`new`＋`_alloc_copy()`で毎回exact-fit再確保する単純な方式（本家のcapacity先読み最適化はなし、実用上は問題ない想定）
- double→文字列変換は`SerialClass::_print_double()`と同じ整数演算ベースの手法（nano.specsの`snprintf`が`%f`非対応のため）をローカルに複製して実装（`dtoa()`ヘルパー）
- `SerialClass::print(const String&)` / `println(const String&)` オーバーロードを追加
- ビルド設定: `MCUXpresso_project/.../Debug/arduino_layer/subdir.mk`に`arduino_string.cpp`をCPP_SRCS/CPP_DEPS/OBJS/cleanターゲットへ追加
- 確認用スケッチ: `examples/Arduino_compatible_API/test_String/`（連結・数値変換・検索・置換・大小文字変換・trim等を`OK`/`FAIL`判定付きで一通り検証）。実機確認済み、全項目OK
- README.mdのAPI対応表にも追加（WStringの移植ではなく独自実装である旨を明記）

### detachInterrupt() 実装
- ギャップ調査で判明した2件目。既存の`attachInterrupt()`はピンごとの管理テーブルを持たず、呼ぶたびに新しい`InterruptIn`をnewしてリークする作りだった（`digitalWrite`/`pinMode`用の`digital_pins[]`に相当するものが割り込み側にはなかった）ため、`detachInterrupt()`の実装にはまずこの管理テーブル追加が前提として必要だった
- `arduino_io.cpp`に`interrupt_pins[]`テーブルを新設（`digital_pins[]`と同じ`MAX_DIGITAL_PINS`サイズ）。`attachInterrupt()`は既存インスタンスがあれば再利用するよう修正（副次的にリークも解消）、`detachInterrupt()`はテーブルから該当ピンの`InterruptIn`を引いて新設の`disable()`を呼ぶ
- r01lib側: `InterruptIn`クラスに`disable()`メソッドを新設（`InterruptIn.h/.cpp`）。`PORT_SetPinInterruptConfig(..., kPORT_InterruptOrDMADisabled)`（`FSL_FEATURE_PORT_HAS_NO_INTERRUPT`環境では`GPIO_SetPinInterruptConfig(..., kGPIO_InterruptStatusFlagDisabled)`）でハードウェアの割り込み設定を無効化し、IRQディスパッチ用の`cb_table[][]`エントリもクリア
- 確認用スケッチ: `examples/Arduino_compatible_API/test_detachInterrupt/`（SW2を3回押すとdetach→3秒後に自動re-attach、という流れ）。実機確認済み — detach中の押下は無反応、re-attach後は正常に再開することを確認
- README.mdのAPI対応表にも追加

### Serial.flush() 実装
- ギャップ調査で判明した3件目。r01libの`Serial`クラスにTXリングバッファ（`_tx_head`/`_tx_tail`、256バイト）はあったが、送信完了を待つ手段がなかった
- `Serial::flush()`をr01lib側に新設（`Serial.h/.cpp`）。まずTXリングバッファが空になるまでスピンウェイトし、その後LPUARTの`kLPUART_TransmissionCompleteFlag`（ソフトウェアバッファではなくハードウェアのシフトレジスタが実際に送信完了したか）が立つまで待つ、という2段階の待ち合わせ。バッファが空でも直近の1バイトはまだ物理的に送信中の可能性があるため
- `SerialClass::flush()`は単純なパススルー
- 確認用スケッチ: `examples/Arduino_compatible_API/test_Serial_flush/`（Serial1を9600bpsで使い、`flush()`が実際にブロックした時間を計測してビット数から計算した理論値と比較）。実機確認済み — 実測24799/24002/23914µs、理論値23958µsとほぼ一致し、ソフトウェアバッファの空きだけでなくハードウェア送信完了まで正しく待っていることを確認
- README.mdのAPI対応表にも追加

### Serial.peek() 実装
- ギャップ調査で判明した4件目。RXリングバッファ（`_rx_head`/`_rx_tail`）から`_rx_tail`を進めずに次の1バイトを覗き見るだけの実装。`SerialClass::begin()`が常に`attach([]{}, RxIrq)`を呼ぶため、`Serial`/`Serial1`では常にリングバッファモードが有効で問題なく使える。コールバック未登録時（このプロジェクトでは実質発生しない経路）は-1を返す
- 確認用スケッチ: `examples/Arduino_compatible_API/test_Serial_peek/`（Serial1ループバックで"AB"を送信し、`peek()`を2回呼んでも`available()`が変化しないこと、`read()`後は次のバイトが見えることを検証）。実機確認済み、全項目OK
- README.mdのAPI対応表にも追加

### Serial Stream系ヘルパー実装（setTimeout/readBytes/readBytesUntil/parseInt/parseFloat/find）
- ギャップ調査で判明した5件目、これでリストの中〜高優先度項目が一通り完了
- Arduino層（`arduino_serial.h/.cpp`）のみで完結する実装。r01lib側の変更は不要 — 既存の`read()`/`peek()`/`millis()`を組み合わせたポーリングベース
- `_timed_read()`/`_timed_peek()`という共通ヘルパーを新設（`millis()`基準のタイムアウト付きでデータを待つ）。`parseInt()`/`parseFloat()`は共通の`_parseNumber(bool allow_decimal)`ヘルパーに集約し重複を回避（先頭の非数字をスキップ→符号→整数部→小数点（許可時）→小数部、という状態遷移）
- `find()`は素朴な逐次一致（KMP等の最適化はなし、実用上問題ない想定）
- ビルド時に`arduino_serial.cpp`で`millis()`が未宣言というエラーが発生 — 従来`arduino_serial.cpp`は`arduino_serial.h`/`arduino_io.h`のみincludeしており`millis()`宣言元の`arduino.h`を直接includeしていなかったため。`arduino_serial.cpp`に`#include "arduino.h"`を追加して解消（include guardがあるため循環includeにはならない）
- 確認用スケッチ: `examples/Arduino_compatible_API/test_Serial_stream_helpers/`（Serial1ループバックで各関数を検証、最後にタイムアウトパス（`setTimeout(500)`未達時の待ち時間）も実測）。実機確認済み、全項目OK — タイムアウト実測値は設定通り500ms
- README.mdのAPI対応表にも追加

### 残りの低優先度ギャップ対応（analogReference / analogRead・WriteResolution / yield / ctype.h系）
- ギャップ調査リストの最後の項目群。これで洗い出した抜けはすべて対応完了
- `analogReference(uint8_t mode)`: no-opスタブ（`arduino_analog.h/.cpp`）。このボードのLPADCリファレンス電圧はハードウェアで固定（`AnalogIn.cpp`で`kLPADC_ReferenceVoltageAlt3`固定）でユーザー切り替え不可のため、互換性のための宣言のみ
- `analogReadResolution(int bits)` / `analogWriteResolution(int bits)`: no-opではなく実際にスケーリングする実装。`adc_resolution_bits`（デフォルト10）/`pwm_resolution_bits`（デフォルト8）を静的変数として保持し、`analogRead()`は`read_u16() >> (16 - adc_resolution_bits)`、`analogWrite()`は`(1 << pwm_resolution_bits) - 1`を上限にスケーリングするよう変更。1-16の範囲にクランプ
- `yield()`: `arduino.h`にinline no-opスタブとして追加。本コアはRTOSなしの単純ループ構成で協調スケジューラが存在しないため、譲る先がない
- ctype.h系ラッパー: `arduino.h`に`<cctype>`をinclude追加し、`isAlpha`/`isAlphaNumeric`/`isAscii`/`isWhitespace`/`isControl`/`isDigit`/`isGraph`/`isLowerCase`/`isPrintable`/`isPunct`/`isSpace`/`isUpperCase`/`isHexadecimalDigit`をinline関数として追加（標準`<cctype>`関数の薄いラッパー、`isAscii`のみ`newlib`の`isascii()`に依存せず`(unsigned)c < 128`で自前実装）
- 確認用スケッチ: `examples/Arduino_compatible_API/test_analog_resolution_and_misc/`（A0の同一信号を10bit/12bitで読み比べ約4倍の関係を確認、ctype.h系は既知の文字で網羅的に検証）。実機確認済み、全項目OK
- README.mdのAPI対応表にも追加

### Linux対応: upload.shのLinkServer探索ロジック強化
- ローカルにクローン済みの`ArduinoCore-zephyr`（`/Users/tedd/dev/ArduinoCore/ArduinoCore-zephyr/`, Apache License 2.0）の`tools/upload_pyocd_or_linkserver.sh`を参照し、`upload.sh`のLinux分岐を強化
- 旧実装: `which LinkServer`を先に試し、無ければ固定パス`/usr/local/LinkServer/LinkServer`にフォールバックするだけだった
- 新実装: 固定パス`/usr/local/LinkServer/LinkServer`を最優先、次にバージョン付きインストールディレクトリ`/usr/local/LinkServer_<version>`（LinkServerのインストーラが実際に使う命名規則、`sort -V`で最新版を選択）、最後に`PATH`（`command -v`）にフォールバック。存在確認も`-f`（ファイル存在）から`-x`（実行可能）に強化
- macOS分岐は変更なし。ローカルの実LinkServerインストール（`/Applications/LinkServer_26.6.137/LinkServer`）で探索ロジックの回帰がないことをdry-runで確認済み
- `LICENSE`のThird-Party Noticesに、この探索ロジックがArduinoCore-zephyrから移植したものである旨を追記
- **注意**: これはコードの改善であり、実際にLinux環境上でBoards Managerインストール〜ビルド〜書き込みまでの一連の流れを検証したわけではない。実機（実OS）での検証は引き続き未実施（残りのPendingタスク#1のまま）

### 2周目のAPI互換性精査と「すぐ対応するもの」実装（Wire/SPI/String/Serial）
- 1周目のギャップ調査（言語リファレンスの関数群）が完了したのを受け、Wire/TwoWire・SPI・Stringクラス・Print系の完成度を2周目として精査（Explore agentに依頼）
- **バグ発見・修正**: `SPI.beginTransaction()`は`SPISettings`の`clock`/`dataMode`はハードウェアに反映していたが、`bitOrder`（MSBFIRST/LSBFIRST）は保存されるだけで一切適用されていなかった。r01lib側の`SPI`クラスに`bit_order()`メソッドを新設（`spi.h/.cpp`、`masterConfig.direction`を書き換えて`LPSPI_MasterInit`し直す、`mode()`と同じパターン）し、`beginTransaction()`から呼ぶよう修正
- **副次的に発見したもう1つの不整合**: `arduino_spi.h`が独自定義していた`enum endian { MSBFIRST=0, LSBFIRST=1 }`が、`arduino.h`側の`#define LSBFIRST 0 / #define MSBFIRST 1`（本家Arduino標準値）と**値が逆**だった。マクロは`arduino.h`内で`arduino_spi.h`のinclude後に定義されるため、スケッチ側で`MSBFIRST`と書くとマクロ経由で値1になる一方、`SPISettings`のデフォルトコンストラクタ内（`arduino_spi.h`自身、マクロ未定義の時点で解決）ではenum経由で値0になる、という食い違いがあった。enumの値を`LSBFIRST=0, MSBFIRST=1`に修正して整合させた
- **SPI**: `end()`（`spi`インスタンスを`delete`）、`transfer16()`（現在の`bitOrder`に応じてMSB/LSBどちらのバイトを先に送るか切り替え）、`usingInterrupt()`/`notUsingInterrupt()`（no-opスタブ）を追加。`beginTransaction()`内の`static`ローカル変数だった`last_clock`/`last_mode`を`SPIClass`のメンバ変数に変更（`transfer16()`から現在の設定を参照する必要があったため）
- **Wire**: `setClock(uint32_t)`を追加。`begin()`内のI3C/I2C分岐ロジック（BusFault修正時に確立したI3Cの3引数`frequency()`呼び出し）をそのまま再利用
- **Serial**: `readString()`/`readStringUntil(char)`を追加（`String`クラスと`_timed_read()`を組み合わせるだけ）
- **String**: `reserve()`（no-op、常にtrue。このクラスはconcat/assign毎にexact-fit再確保する設計のため予約する容量という概念がそもそもない）、`getBytes()`/`toCharArray()`、`startsWith(s, offset)`オーバーロードを追加
- 確認用スケッチ: `test_SPI_bitorder_end_transfer16`（MOSI-MISOループバック配線要）、`test_Wire_setClock`（オンボードP3T1755、配線不要）、`test_Serial_readString`（Serial1 D1-D0ジャンパ要）。3本とも実機確認済み、全項目OK
- README.mdのAPI対応表を全項目更新。**I2Cスレーブモード**（`Wire.begin(address)`, `onReceive`, `onRequest`）は未対応であることを❌付きで明記 — r01lib側にスレーブ用I2C/LPI2Cドライバが一切存在せず、Arduino層だけでは実装不可能で新規の低レベルドライバ開発が必要という規模の大きさから、既知の制限事項として記録

### 中優先度ギャップ対応（SPIレガシーAPI、Stringの64bit対応）
- 2周目の精査で「中優先度」に分類していた残り項目
- **SPI**: `setBitOrder()`/`setDataMode()`/`setClockDivider()`（pre-1.6世代のレガシーAPI、`SPISettings`/`beginTransaction()`を使わず即座にハードウェアへ反映する方式）を追加。`setClockDivider()`はAVRの`SPI_CLOCK_DIVn`定数（レジスタエンコーディングが非線形なので、定数値をそのまま除数として使わずlookup table的にswitch文で分周値へ変換）をサポートするが、除算対象はAVRの`F_CPU`ではなくr01lib SPIの`master_clk_freq`（このボードのSPIペリフェラル入力クロック、新設した`clock_freq()`アクセサ経由で取得）。この違いはREADMEにも明記
- **String**: `long long`/`unsigned long long`のコンストラクタ・`concat`・`operator+=`を追加。実装は既存の`long`/`unsigned long`版と同じパターン（`snprintf`の`%lld`/`%llu`/`%llx`/`%llo`、整数フォーマットなのでnano.specsの浮動小数点非対応制約とは無関係で問題なし）
- 確認用スケッチ: `test_SPI_legacy_api`（MOSI-MISOループバック配線要、legacy API切り替え後もtransferが正常動作することを確認）、`test_String_64bit`（配線不要、32bit longをオーバーフローする値で64bit経路が実際に使われていることを確認）。実機確認済み、全項目OK

### 3周目のAPI互換性精査と3件のバグ修正（BIN基数、Serial.write、attachInterrupt LOW）
- 2周目で見つけたSPIバグの修正パターンに味を占め、3周目としてSerial print系・write系・PROGMEM・attachInterruptモード・`ARDUINO`系マクロを精査（Explore agentに依頼）。3件の実バグと3件の未対応機能ギャップ（PROGMEM/F()、`#define ARDUINO`、`ARDUINO_ARCH_*`系マクロ、いずれもCortex-M機種では実害が小さいと判断し今回は見送り）が判明
- **バグ1: `Serial.print(x, BIN)`が無言で10進数を返す**。`_print_num`/`_print_unum`/`_print_num64`/`_print_unum64`（`arduino_serial.cpp`）はいずれも`DEC`/`HEX`/`OCT`だけをsnprintfの書式指定で分岐し、それ以外（`BIN`含む）は`else`で10進フォーマットにフォールバックしていた（snprintfに`%b`相当がないため）。任意基数（2〜36）対応の`_utoa_radix()`ヘルパーを新設し、この`else`分岐を置き換えて解消
- **同じバグが`String`クラスの数値コンストラクタにも独立して存在**（`arduino_string.cpp`、`String::String(long, base)`等の4つの整数コンストラクタ）。ユーザーの実機テストで`Serial.print(255, BIN)`は直っているのに`String(255, BIN) == "11111111"`が`FAIL`する、という形で発覚 — Serial側だけ直して見落としていた。同じ`_utoa_radix`ヘルパーを`arduino_string.cpp`側にも複製し（別の翻訳単位なので共有不可、既存の`dtoa()`と同じ扱い）、4つのコンストラクタすべてを修正
- **バグ2: `Serial.write()`のオーバーロード不足＋r01lib実装の隠蔽**。`SerialClass`は`write(uint8_t c)`（戻り値`void`）しか宣言しておらず、C++の名前隠蔽ルールにより基底クラス`Serial`の`write(const uint8_t*, size_t)`（一括書き込み）が完全に不可視化されていた。`write(uint8_t)`（戻り値を本家準拠の`size_t`に変更）、`write(const uint8_t*, size_t)`、`write(const char*, size_t)`、`write(const char*)`の4オーバーロードをすべて`SerialClass`自身に明示的に宣言する形で解消（`using`宣言だと基底の`status_t`返り値がそのまま漏れて`size_t`として誤解釈される問題があるため、ラッパーとして書き直した）
- **バグ3: `attachInterrupt(pin, isr, LOW)`が無言でRISING扱いになる**。このプロジェクト独自の定数番号`RISING=0/FALLING=1/CHANGE=2`が、デジタルレベル定数`LOW`（`false`=0）と数値衝突していた（本家Arduinoは`CHANGE=1/FALLING=2/RISING=3`で`LOW=0`を意図的に空けてある）。定数を本家と同じ番号体系に振り直し、r01libに`InterruptIn::low()`（`kPORT_InterruptLogicZero`によるレベルトリガー割り込み）を新設して`case LOW:`を正式サポート
- 確認用スケッチ: `test_Serial_BIN_and_write`（Serial1 D1-D0ジャンパ要、BIN基数の期待値比較＋write系の往復確認、null バイトを含むバッファでcount-basedであることも確認）、`test_Interrupt_LOW`（配線不要、SW2長押しでカウンタが数十万回増えることを確認 — エッジトリガーなら数回で止まるはずが374,533回増加し、レベルトリガーとして機能していることを実証）。実機確認済み、全項目OK
- README.mdのAPI対応表を更新（`attachInterrupt`にLOW追加、`Serial.write`の4オーバーロード明記、BINバグ修正済みである旨明記）

### 4周目のAPI互換性精査と5項目実装（Wire.end、find/findUntil、availableForWrite、INPUT_PULLDOWN、OUTPUT_OPENDRAIN）
- 3周目までで見つかったバグ・機能ギャップがすべて解消したのを受け、4周目としてWire.end/Serial.availableForWrite/Stream系findの拡張オーバーロード/INPUT_PULLDOWN・OUTPUT_OPENDRAIN pinModeを精査（Explore agentに依頼）。EEPROM・Stream::flush()のRXクリア相当は「本家にも存在しない・対応不要」と判明、5項目を実装
- **`Wire.end()`**: `TwoWire`に新設（`arduino_i2c.h/.cpp`）。直近実装した`SPIClass::end()`と同じパターンで`delete i2c; i2c = nullptr;`。r01libの`I2C::~I2C()`/`I3C::~I3C()`は既に`LPI2C_MasterDeinit`/`I3C_MasterDeinit`を呼ぶ実装済みのデストラクタを持っていたため、Arduino層に配線するだけで済んだ
- **`Serial.find(target, length)` / `findUntil(target, terminator)`**: 既存の`find(const char*)`と同じ素朴な逐次一致方式（KMP等の最適化なし）で追加。`findUntil`はtarget側とterminator側の2本のマッチャーを並行して回し、terminatorが先に完成したら`false`で早期終了
- **`Serial.availableForWrite()`**: これまでr01lib `Serial`クラスにはTXバッファの空き状況を返すAPIとして`bool writable()`（1バイト分の空きがあるか）しかなかった。`available()`のRXリングバッファ残量計算（`(_rx_head - _rx_tail) & (RX_RING_BUF_SIZE - 1)`）と同じパターンで`size_t Serial::availableForWrite()`を新設し、使用中バイト数からバッファ容量（255＝256バイトのリングバッファの実効容量、満杯検出のため1バイト分予約）を引いて返すよう実装。`SerialClass`側に`int availableForWrite()`として露出
- **`INPUT_PULLDOWN` / `OUTPUT_OPENDRAIN`**: r01libの`DigitalInOut::PinMode`には元々`PullDown`（0x2）・`OpenDrain`（0x8）が存在していたが、Arduino層の`pinMode()`が`INPUT_PULLUP`しか見ておらず、他のモードは`DigitalInOut::mode()`実装（`PORT_SetPinPullUpDown`/`PORT_SetPinOpenDrain`をビットマスクとして独立に見る作り）まで到達できなかった。`arduino_io.h`に`INPUT_PULLDOWN = 0x20`・`OUTPUT_OPENDRAIN = 0x30`を追加（既存の`INPUT_PULLUP = 0x10`と同様、`OUTPUT=1`と衝突しない値）、`pinMode()`のモード判定を拡張。既存ピン再利用パス（`digital_pins[pin_num] != nullptr`のとき）でも`->mode(pin_mode)`を呼ぶよう変更し、同一ピンに対して`pinMode()`を呼び直した際にpull/open-drain設定も更新されるようにした（従来は方向切り替えのみでpull設定が反映されなかった）
- 確認用スケッチ: `examples/Arduino_compatible_API/test_Wire_end_find_availForWrite_pullmodes/`（オンボードP3T1755でWire1.end()前後の読み取りを比較、Serial1ループバックでfind/findUntilを検証、availableForWriteをバースト書き込み前後で比較、D4フローティングピンでPULLDOWN/PULLUP切り替えを検証、D2-D3ジャンパでOUTPUT_OPENDRAINの真のオープンドレイン特性——D3をPULLDOWNにした状態でD2をHIGHにしても押し上げられずLOWのまま、という push-pull出力とは区別できる挙動——を検証）

### 実機バグ発見・修正: `Wire.end()`（I3C使用時）でBusFault/ハング
上記5項目の実機確認1発目で判明。`Wire1.end()`を呼ぶと「Wire1.end() called」の出力直前で無応答になる不具合が発生
- **原因**: `TwoWire::end()`の`delete i2c`は仮想デストラクタ経由で正しく`I3C::~I3C()`（`I3C_MasterDeinit`）→`I2C::~I2C()`の順にチェーンされるが、`I2C::~I2C()`は無条件に`LPI2C_MasterDeinit( unit_base )`を呼んでいた。I3C経由でI2Cモードとして使う場合、ベースクラス委譲コンストラクタ`I2C(sda, scl, no_hw=true)`は`if (no_hw) return;`でハードウェア初期化を完全にスキップし`unit_base`を未初期化のまま残す（v0.2.0で発覚した`TwoWire::begin()`のBusFaultバグと全く同じ未初期化`unit_base`が根本原因）。今回は仮想デストラクタなので`frequency()`のような隠蔽では回避できず、`I2C::~I2C()`が必ず呼ばれてしまう構造的な問題。これまで`I2C`/`I3C`インスタンスを`delete`するコードパスがコードベース中に一つも存在しなかったため、`Wire.end()`の追加で初めて顕在化した
- **修正**: `I2C`クラスに`bool _no_hw`メンバを新設（`i2c.h`）、コンストラクタで`no_hw`引数の値を保持（`i2c.cpp`）。デストラクタで`_no_hw`が立っていれば`LPI2C_MasterDeinit`/`I2C_MasterDeinit`呼び出しをスキップするよう修正（I3Cの場合は`I3C::~I3C()`が既に`I3C_MasterDeinit`で正しく後始末しているため、ベースクラス側で何もすることがない）
- ヘッダ同期・ライブラリ再ビルド・`test_Wire_end_find_availForWrite_pullmodes`/`test_combined_peripherals`/`test_Wire_setClock`のコンパイル確認済み。実機再検証済み — `Wire1.end()`後もハングせず、`begin()`し直して正常読み取り継続を確認

### 4周目5項目、実機確認完了
`test_Wire_end_find_availForWrite_pullmodes.ino`を実機で実行、全項目OK（`Wire1.end()`前後の温度読み取り、`availableForWrite`のバースト/flush前後の増減、`find(target,length)`/`findUntil()`の早期終了、`INPUT_PULLDOWN`/`INPUT_PULLUP`のフローティングピン読み取り、`OUTPUT_OPENDRAIN`の真のオープンドレイン特性）
- README.mdのAPI対応表を更新（`pinMode`にPULLDOWN/OPENDRAIN追記、`Serial.find(len)`/`findUntil`/`availableForWrite`/`Wire.end`を新規追加）

### 5周目のAPI互換性精査と4項目実装（String operator+数値版、Printable、NOT_AN_INTERRUPT、fast GPIOレジスタアクセス）
- 4周目までの傾向（見つかる項目の優先度が徐々に下がってきている）についてユーザーから「優先度の低いものまで出てきたということか、それとも高優先度の取りこぼしがあったということか」と問われ、1〜3周目は実バグ・Stringクラス丸ごと欠落等の高優先度、4〜5周目はニッチな項目に移ってきている旨を回答。Explore agentへの監査はチェックリスト式で聞いた観点にしか答えないため高優先度の見落としを完全には否定できないとも説明し、次の一手としてサードパーティArduinoライブラリの実地コンパイルテストを提案、ユーザーが承認
- 5周目監査で見つかった6項目のうち、`SPI.transfer(void*,size_t)`は既に実装済みと判明。残り5項目のうち4項目を実装（`Wire.setWireTimeout`系は後述の理由で見送り）
- **`String`の free `operator+`数値版**: `concat()`/`operator+=`は`int`/`unsigned int`/`long`/`unsigned long`/`long long`/`unsigned long long`/`float`/`double`/`F()`まで揃っていたが、対応する free `operator+`（`String + int`等）が`String`/`const char*`/`char`の3種類しかなく、`String s = "x=" + someInt;`のような一般的なイディオムがコンパイルできなかった。既存の`operator+(String lhs, char rhs) { lhs += rhs; return lhs; }`と同じワンライナーパターンで9個追加（数値8種＋`F()`版）
- **`Printable`インターフェース**: このプロジェクトには元々`Print`という抽象基底クラスが存在せず（`SerialClass`が直接の具象クラス）、実質`Print`を名乗れるクラスは`SerialClass`のみだったため、`using Print = SerialClass;`という型エイリアスとして`Print`を新設（`arduino_serial.h`）。`class Printable { virtual size_t printTo(Print&) const = 0; };`を本家と同じ形で追加し、`SerialClass::print`/`println(const Printable&)`を実装（`p.printTo(*this)`を呼ぶだけ）
  - **判明した制約**: 本家ArduinoのPrint系クラスは`print()`/`println()`が全て`size_t`（書き込みバイト数）を返す設計で、サードパーティライブラリの`printTo()`実装は`size_t n = 0; n += p.print(x); ...; return n;`という累算イディオムを多用する。しかしこのプロジェクトの`SerialClass::print()`/`println()`はほぼ全て`void`を返す設計（`write()`のみ`size_t`）のため、このイディオムを使う`printTo()`実装はそのままではコンパイルが通らない。全`print()`/`println()`オーバーロード（约26個）を`size_t`返却に作り直すのは影響範囲が大きすぎるため今回は見送り、既知の制約としてREADMEに明記
- **`NOT_AN_INTERRUPT`**: 定数を`-1`として追加（`arduino_io.h`）。このMCUは有効なGPIOピンなら基本的にどれでも割り込み対応可能なため、`digitalPinToInterrupt()`の実装（`return pin_num;`）自体は変更せず、定数だけ追加（この定数をチェックするスケッチがコンパイルは通るように、という互換性目的）
- **`digitalPinToPort`/`digitalPinToBitMask`/`portOutputRegister`/`portInputRegister`/`portModeRegister`**: NeoPixel系の高速ビットバングライブラリ向け。r01libの`DigitalInOut`が既に内部で保持している`gpio_n`（`GPIO_Type*`）・`gpio_pin`（ビット番号）に`public`アクセサ`gpio_base()`/`gpio_bit()`を新設（`io.h`）。Arduino層は`pinMode()`で既に生成済みの`digital_pins[]`エントリからこれを読み出すだけ（`pinMode()`未実行のピンは`nullptr`/`0`を返す——このプロジェクトは元々`digitalWrite`/`digitalRead`自体が`pinMode()`未実行だと無反応になる設計のため、既存の制約と整合）。レジスタは`GPIO_Type`の`PDOR`（出力、R/W）/`PDIR`（入力、読み取り専用）/`PDDR`（方向、1=OUTPUT）を採用——AVRの`PORTx`/`PINx`/`DDRx`と同じ「単一R/Wレジスタ」方式に対応する自然な選択（`PSOR`/`PCOR`のような書き込み専用atomicセット/クリアレジスタも存在するが、AVR互換の意味論に合わせて`PDOR`を採用）
- **見送った項目**: `Wire.setWireTimeout()`/`clearWireTimeoutFlag()`/`getWireTimeoutFlag()`はユーザーに実装せず見送ることを提案予定（このメッセージ作成時点では未提示）。理由: r01libの`I2C::write_core()`/`read_core()`はSDKの`LPI2C_MasterStart`/`Send`/`Receive`/`Stop`という完全にブロッキングなSDK関数を直接呼んでおり、実際のバス ハング（スレーブがクロックストレッチし続ける等）はほぼ確実にこれらSDK関数の内部ポーリングでスタックする。Arduino層だけでタイムアウト値とフラグを保持する「見せかけの」実装は、ユーザーに「タイムアウトで守られている」という誤った安心感を与えるだけで実際のハングを防げないため、正直な実装のためにはr01libのブロッキング呼び出し自体にデッドライン機構を組み込む必要があり、今回のスコープを超える規模と判断
- 確認用スケッチ: `examples/Arduino_compatible_API/test_String_plus_numeric_Printable_fastGPIO/`（`String + int/long/long long/unsigned long/float/F()`の連結確認、`Printable`を実装したカスタムクラス`Point`を`Serial.println()`に直接渡して視覚確認、`NOT_AN_INTERRUPT`の値と実ピンとの非衝突確認、D2-D3ジャンパでfast GPIOレジスタ直接操作がdigitalWrite相当に振る舞うか——`portOutputRegister`への書き込みが実際にD3で観測できるか、`portInputRegister`が自分自身の状態を正しく反映するか、`portModeRegister`がOUTPUT方向を正しく示すか——を検証）。全examplesの回帰コンパイルも実施、問題なし
- README.mdのAPI対応表を更新（`digitalPinToInterrupt`/`NOT_AN_INTERRUPT`、fast GPIOレジスタ系、`String`の`operator+`数値版、`Printable`インターフェース（`print()`がvoidを返す制約を明記）を追加）

### README.md「Supported Arduino APIs」表をAPI_COMPATIBILITY.mdへ分離
5周目完了後、ユーザーから「表が長くなりすぎていないか」との指摘。46行のフラットな1枚表になっており、Notes列には「v0.2.1で修正」的な経緯説明や`Printable`/`Wire.setWireTimeout`の数文にわたる理由説明まで混在し一覧性が低下していた。`CHANGELOG.md`/`TUTORIAL.md`（`.ja.md`）を既に別ファイルに切り出している本プロジェクトの慣習に合わせ、`API_COMPATIBILITY.md`を新規作成して全表を移設（GPIO/割り込み、Serial、Wire、SPI、タイミング、アナログ、その他デジタルI/O、String/Print、互換マクロの9カテゴリに見出しで分割、内容自体は変更なし）。README.md側は数行の要約＋リンクに圧縮、冒頭のTUTORIAL/CHANGELOGへのリンク行にも同様に追加

### 機能ギャップ埋め（PROGMEM/F()、ARDUINO/ARDUINO_ARCH_*マクロ）
- 3周目で「未対応（バグではなく機能ギャップ）」として見送っていた3項目に対応
- **`ARDUINO`バージョンマクロ・`ARDUINO_ARCH_*`系マクロ**: ソースコード側ではなく`platform.txt`の`compiler.defines`に追加（`-DARDUINO=10819 -DARDUINO_ARCH_MCX -DARDUINO_{build.board}`）。`{build.board}`はarduino-cliが`boards.txt`の`frdm_mcxa153.build.board=FRDM_MCXA153`から自動展開する組み込みプロパティで、`ARDUINO_FRDM_MCXA153`として正しく定義されることを実際にビルドして確認済み。従来これらのマクロが本当に未定義かどうか自体、テストスケッチで`#ifdef`/`#pragma message`を使って実証してから着手した
- **`PROGMEM`/`pgm_read_byte`等/`PSTR`**: `arduino.h`にno-opマクロとして追加。Cortex-Mはvon Neumann構成でフラッシュとRAMが同一アドレス空間のため、AVRのようなpgm_read系の特殊アクセスは本来不要 — ソース互換性のためだけの宣言
- **`F("...")`/`__FlashStringHelper`**: `arduino.h`に`class __FlashStringHelper;`（前方宣言のみ、実体は定義しない、本家と同じ流儀）と`F()`マクロを追加。`arduino_string.h`と`arduino_serial.h`にも同じ前方宣言を重複させて自己完結させ（`arduino.h`が`arduino_string.h`をincludeする順序に依存しないように）、`String(const __FlashStringHelper*)`コンストラクタ・`concat`/`operator+=`、`SerialClass::print`/`println(const __FlashStringHelper*)`を実装。中身は単に`const char*`へreinterpret_castして通常経路に渡すだけ
- 確認用スケッチ: `test_PROGMEM_F_ARDUINO_macros`（配線不要、`pgm_read_byte`の値確認、`F()`をSerial/Stringの両方で使用、`#if ARDUINO >= 100`・`ARDUINO_ARCH_MCX`・`ARDUINO_FRDM_MCXA153`の`#ifdef`確認）。実機確認済み、全項目OK
- README.mdのAPI対応表にも追加。これで3周にわたるAPI互換性精査で見つかった項目はすべて対応完了（唯一の例外はI2Cスレーブモード、既知の制限として明記済み）

---

## 動作確認済み

| API | 状態 | 備考 |
|---|---|---|
| GPIO / digitalWrite / digitalRead | ✅ | |
| Serial | ✅ | |
| Serial.flush() | ✅ | v0.2.1で追加。Serial1@9600bpsで実測値と理論値を比較し実機確認済み |
| Serial.peek() | ✅ | v0.2.1で追加。Serial1ループバックで実機確認済み |
| Serial Stream系（setTimeout/readBytes/readBytesUntil/parseInt/parseFloat/find） | ✅ | v0.2.1で追加。Serial1ループバックで実機確認済み、タイムアウトパスも実測 |
| analogReference / analogRead・WriteResolution / yield / ctype.h系 | ✅ | v0.2.1で追加。実機確認済み（A0の10bit/12bit比較、ctype.h系は既知文字で検証） |
| SPI.end / transfer16 / bitOrderバグ修正 | ✅ | v0.2.1で追加・修正。MOSI-MISOループバックで実機確認済み |
| Wire.setClock | ✅ | v0.2.1で追加。オンボードP3T1755で実機確認済み |
| Serial.readString / readStringUntil、String::reserve/getBytes/toCharArray/startsWith(offset) | ✅ | v0.2.1で追加。Serial1ループバック＋純粋ロジック検証で実機確認済み |
| SPI.setBitOrder/setDataMode/setClockDivider、String 64bit（long long/unsigned long long） | ✅ | v0.2.1で追加。MOSI-MISOループバック＋純粋ロジック検証で実機確認済み |
| Serial print BIN基数バグ修正（Serial・String両方）、Serial.write全オーバーロード、attachInterrupt LOWモード | ✅ | v0.2.1で修正・追加。全項目実機確認済み（LOWモードは長押しでカウンタ374,533回増加を確認） |
| PROGMEM/pgm_read系、F()/String対応、ARDUINO/ARDUINO_ARCH_*マクロ | ✅ | v0.2.1で追加。実機確認済み |
| Wire.end、Serial.find(len)/findUntil、Serial.availableForWrite、INPUT_PULLDOWN、OUTPUT_OPENDRAIN | ✅ | v0.2.1で追加。実機確認済み（`Wire.end()`はI3C使用時のBusFaultバグを修正後に確認） |
| String operator+数値版/F()、Printable、NOT_AN_INTERRUPT、digitalPinToPort/BitMask+portOutput/Input/ModeRegister | ✅ | v0.2.1で追加。実機確認済み（fast GPIOレジスタ直接操作がD2-D3ジャンパで正しく動作、Printableカスタムクラスの出力を視覚確認） |
| Wire (I2C) | ✅ | |
| Wire1 (I3C, I2Cモード) | ✅ | オンボードP3T1755で確認、重大バグ修正済み |
| SPI | ✅ | |
| attachInterrupt | ✅ | |
| detachInterrupt | ✅ | v0.2.1で追加。SW2を使った実機確認済み |
| analogRead | ✅ | LPADC, A0-A3 |
| analogWrite (PWM) | ✅ | FlexPWM0, PWM0-PWM5のみ |
| millis / micros | ✅ | SysTick(1ms) + DWT |
| delayMicroseconds | ✅ | wait_us()ベース、v0.2.1で追加 |
| tone / noTone | ✅ | CTIMER0, 任意のデジタルピン |
| Serial1 (D0/D1ハードウェアUART) | ✅ | 入力バッファ有効化・RX割り込み・available()の3バグ修正後、実機ループバックで確認 |
| shiftOut / shiftIn | ✅ | 割り込みベースの相互検証で確認 |
| pulseIn / pulseInLong | ✅ | |
| random / randomSeed | ✅ | |
| UNO R3/R4互換マクロ・定数一式 | ✅ | コンパイル確認のみ（数値的な動作確認は各マクロの単純さから省略） |
| String クラス | ✅ | 独自実装（WString移植ではない）。連結・数値変換・検索・置換・大小文字変換・trim等を実機確認、全項目OK |
| 上記全機能の同時使用 | ✅ | `test_combined_peripherals.ino`（Serial1込み）で実機確認済み。WARNINGなし、`serial1`ループバック欠落なし |
| ボードマネージャーインストール | ✅ | v0.1.5時点で確認済み。v0.2.0リリース後、実際にGitHubの`package_nxp_mcx_index.json`経由でBoards Managerからインストールし直し、macOS/Windows 11双方でビルド・書き込み・実行まで動作確認済み |

（v0.2.0の機能開発・デバッグ自体はmacOS実機で実施。リリース後のBoards Managerインストール検証はmacOS/Windows 11の両方で実施）

---

## ローカル開発環境
- **OS**: macOS（Saitama, Japan）
- **リポジトリパス**: `~/dev/mcx-arduino-core`
- **MCUXpressoプロジェクト**: `~/dev/mcx-arduino-core/MCUXpresso_project/_r01lib_frdm_mcxa153/`
- **ビルド済み.a**: `~/dev/mcx-arduino-core/MCUXpresso_project/_r01lib_frdm_mcxa153/Debug/lib_r01lib_frdm_mcxa153.a`
- **xPackツールチェーン**: `~/.xpacktools/xpack-arm-none-eabi-gcc-14.2.1-1.1/`（`package_nxp_mcx_index.json`記載のものと同一バイナリ、チェックサム確認済み）
- **ローカルArduino IDE連携**: `~/Library/Arduino15/packages/nxp/hardware/mcx/0.2.0-dev`（v0.2.0リリース後に`0.1.9-dev`から改名）をこのリポジトリの`hardware/nxp/mcx/`へのシンボリックリンクとして設定済み（編集が即座に反映される）。ツールチェーンも`~/.xpacktools/`への symlink。`-dev`サフィックスにより、Boards Manager経由でインストールする実リリース版（`0.2.0`）とはディレクトリ名が衝突せず共存可能
- **注意（Boards Manager経由の実インストール検証時のハマりどころ）**: 上記symlink環境を無効化する際、`~/Library/Arduino15/packages/nxp`を同じ`packages/`直下で別名（例: `nxp.dev-backup`）にリネームしただけでは不十分 — arduino-cliは`packages/*`配下の全ディレクトリ名をpackager IDとして解釈するため、リネーム後も`nxp.dev-backup:mcx`という別パッケージとして「0.1.9-dev installed」表示が残ってしまう（`arduino-cli core list --all`で再現・特定）。無効化する際は`packages/`の外（例: スクラッチパッド等）に完全に退避すること。v0.2.0リリース後、この手順でBoards Manager経由のGitHubからの実インストールを検証済み

## GitHub Actions
- **Workflow**: `.github/workflows/update_package_index.yml`
- **役割**: GCCのsizeをHEADリクエストで取得、プラットフォームZIPのchecksum/sizeをダウンロードして計算・更新
- **既知の制限**: タグpush（`push: tags: '[0-9]+.[0-9]+.[0-9]+'`）で起動した場合、`actions/checkout`がdetached HEADでチェックアウトするため最後の`git push`が失敗する（過去のv0.1.6〜v0.2.0全リリースで再現）。実際のchecksum確定は、リリース後に`gh workflow run update_package_index.yml --ref main`（または Actions UI の "Run workflow"）で`main`ブランチに対し手動実行する必要がある。**リリース時は「タグpush→(失敗を確認)→mainに対してworkflow_dispatchを手動実行」の2段階が必須の手順**

---

## 残りのPendingタスク
1. Linux対応の実機検証：xPack GCC（Linux x86_64/arm64）のchecksumと`upload.sh`のLinux分岐（`uname`判定、`which LinkServer`／`/usr/local/LinkServer/LinkServer`探索）はコード上は用意済みだが、実機でのBoards Managerインストール・ビルド・書き込みは未検証（macOS/Windows 11のみ確認済み）
2. マルチボード対応（MCXN947, MCXA156, MCXN236）
3. （低優先度）`examples/tests/GPIO_NXP_Arduino`の不要なgitlinkエントリの整理
