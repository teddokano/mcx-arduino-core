# mcx-arduino-core 開発引き継ぎドキュメント

## プロジェクト概要
- **リポジトリ**: https://github.com/teddokano/mcx-arduino-core
- **現在のバージョン**: v0.3.2（`package_nxp_mcx_index.json` 上の最新リリース。`analogWriteFrequency(pin, hz)`（Teensy互換の非標準拡張、両ボードで実機ロジアナ検証済み）、`FRDM_MCXA153`/`FRDM_MCXN947`ボード識別マクロ新設、`r01lib_I3C`サンプルのSOSパニック修正が主な内容。**このバージョンから「`<version>-dev`ブランチで開発、ドキュメント含め全ての変更をそのブランチ上で進め、リリース時に`main`へまとめてマージする」運用を採用**（詳細は「開発ブランチ運用方針（v0.3.2から採用）」セクション参照）。`0.3.2-dev`→`main`マージ・GitHub Release作成、ステージングブランチ方式でmacOS/Windows/Linux全て検証後`main`のchecksum確定（`bb34ab0`）。2026-08-20リリース。詳細は後述の「v0.3.2 で作業中の内容」セクション参照
- **旧バージョン**: v0.3.1（2026-08-19リリース。SDライブラリの`-Waddress-of-packed-member`警告抑制、MCUXpresso SDK直呼びサンプル、ライブラリ不要のI3C/Wire1生レジスタアクセス例。このリリースから「ステージングブランチ方式」を採用）、v0.3.0（2026-08-16リリース。FRDM-MCXN947ボード対応の追加が主目的。Issue #1/#2/#3はこのリリースで解消）、v0.2.2（2026-08-13リリース。Linux実機での`#include <Arduino.h>`/`<SPI.h>`のファイル名大文字小文字ミスマッチによるビルド失敗を修正するパッチリリース）
- **完了**: `prepare0.3.0`ブランチでのFRDM-MCXN947ボード対応は完了し、v0.3.0としてリリース済み。GPIO/Serial(USB)/Wire1(オンボードI3Cセンサー)/Wire(プレーンI2C、外部LM75系センサーとの実通信も確認済み)/SPI/analogRead/analogWrite(全6チャンネル・独立性込み)/tone・noTone(圧電サウンダで実音確認)は実機検証済み、String/UNO互換マクロはコンパイル確認済み。これでN947の主要機能は一通り実機検証済み（任意項目のanalogRead精度確認も定電圧源で完了、D0-D19/A2-A5/MikroBusヘッダの全GPIO出力も実機確認済み）。**MikroBusヘッダにSPI/I2C/UARTの追加インスタンス`SPI1`/`Wire2`/`Serial1`を新規実装・実機確認済み**（`MB_RX`/`MB_TX`は`Wire1`(I3C)・GPIO・`Serial1`(UART)の3モードを排他切り替え可能）。D0/D1自体は引き続きSerial1に意図的に未対応（FlexComm2資源競合のため、`Wire`と同時に使えない）。**重要な実機バグ4件を修正済み**: (1) `analogWrite`の物理ピンが当初A153流用のP3_6-P3_11（実際は未配線のテストポイント）のままだった → 回路図でP2_2-P2_7/FlexPWM1が正しいピンと判明・修正、(2) その修正時のALT値導出（pin_mux.cのコメント内位置カウント方式）がP2_2/P2_3の2ピンだけ誤っていた → Zephyrのpinctrlヘッダ（シリコン正確）で全ピンAlt5と確定・修正、(3) `pinMode()`がPORT MUXを一切変更しない実装だったため、起動時にI3C用ALT10へ固定されている`MB_RX`/`MB_TX`だけ`digitalWrite`が効かなかった → `pinMode()`で常にALT0(GPIO)へ明示的に再設定するよう修正、(4) MikroBus向け`Serial1`追加時、`arduino_serial.cpp`が`arduino_io.h`をincludeしていたため`MB_TX`/`MB_RX`がArduinoリナンバリング後の値にすり替わりSOSパニックが発生 → include前に生の値を退避するよう修正、(5) A153にもMikroBus対応（`SPI1`のみ、`Wire2`/新規`Serial1`はチップの物理制約——I2Cペリフェラルが1系統のみ・既存Serial1とMikroBus UARTが同じLPUART2——により不可能と判明し見送り）を追加した際、新規使用の`LPSPI0`にクロックが供給されておらずCS以外無反応だった → `mcu.cpp`にクロック設定を追加。方針: 未実装が残っていてもN947が一通り完成した段階でリリースする。詳細は「v0.3.0 で作業中の内容」セクション参照
- **v0.2.1**（前バージョン）: `prepare0.2.1`→`main`fast-forwardマージ・GitHub Release作成済み、2026-08-12リリース
- **重要な学び（リリースzipの構造要件）**: v0.2.1の初回リリース作業で`git archive --format=zip -o ... HEAD:hardware/nxp/mcx`を使ってzipを作成したところ、Arduino IDE経由の実インストールで`Failed to install platform: ... no unique root dir in archive, found '.../cores' and '.../tools'`エラーで失敗。Arduino Boards Managerのインストーラーは**zip直下に単一のラッパーディレクトリが1つだけ**存在することを要求する（インストーラーがそのディレクトリを剥がして`packages/<vendor>/hardware/<arch>/<version>/`に配置する仕組み）。`git archive HEAD:hardware/nxp/mcx`はサブディレクトリの中身を直接展開するため、`boards.txt`/`cores/`/`tools/`/`variants/`等がzip直下に並ぶ「フラットな」構造になってしまい、この要件を満たしていなかった。実際に公開済みのv0.2.0のzipを確認したところ、そちらは`mcx/`という単一のラッパーディレクトリを持つ正しい構造になっており問題なし（0.2.1作成時のみのミス）。**今後リリースzipを作る際は、必ず単一のトップレベルディレクトリ（名前は任意、例: `mcx-arduino-core-<version>/`）でラップすること** — `git archive`で作る場合は一旦別ディレクトリに展開してからラッパーディレクトリごと`zip -r`するか、`--prefix=<name>/`オプションを使う
- **内容**: NXP FRDM-MCXA153 (Cortex-M33) 向けArduino IDEボードサポートパッケージ
- v0.2.1の主な内容: `String`クラス独自実装、`Print`/`Stream`/`Printable`抽象基底クラス新設（サードパーティライブラリ互換性向上）、Serial/Stream系ヘルパー一式、複数の実バグ修正（SPI bitOrder、Serial BIN基数、Serial.writeオーバーロード、attachInterrupt LOW、Wire.end() BusFault）。詳細は本ファイル内の「v0.2.1 で作業中の内容」セクションおよび[CHANGELOG.md](CHANGELOG.md)を参照
- **Linux対応の扱い**: v0.2.1にxPack GCC（Linux x86_64/arm64）・`upload.sh`のLinux分岐を含めたが、実機（実Linux環境）でのBoards Managerインストール〜ビルド〜書き込みは未検証（README.mdに明記済み）。ユーザー方針: このリリース版を使って実際にLinuxマシンで検証し、確認できた時点で正式サポート確定とする（残りのPendingタスク#1）
- **v0.2.1のBoards Managerインストール実機検証**: macOS・Windowsともに実機でインストール・動作確認済み（Windowsはユーザーが別マシンで実施、インストール成功・実行確認まで完了と報告）。Linuxのみ未検証で残っている
- **重要な変更（`package_nxp_mcx_index.json`の構造・過去バージョン対応）**: v0.2.1インストール検証中、Boards Managerで過去バージョン（0.2.0等）を選択できないことが判明。原因は`package_nxp_mcx_index.json`の`platforms`配列が**常にエントリ1つだけ**で、リリースのたびに`version`/`url`/`checksum`等を上書きする方式だったため（0.1.0リリース以来ずっとこの方式）。ユーザー指示で過去バージョンも選択可能にする方針に変更し、以下を実施:
  - `platforms`配列を11エントリ（0.1.0〜0.2.1）に拡張。各バージョンの正しいchecksum/sizeはgit履歴から機械的に抽出（各バージョンが「最新」だった期間の最終コミット時点のスナップショットを採用、手打ちでの転記ミスを回避）し、実際にGitHub Releaseからダウンロードして全11バージョンのchecksum一致を検証済み
  - ツールチェーンは2種類混在（0.1.0〜0.1.4は`arm-none-eabi-gcc 14.2.rel1`＝ARM公式配布、0.1.5〜0.2.1は`14.2.1-1.1`＝xPack）。`tools`配列に両方を保持するよう変更
  - 過去バージョンのzip自体（0.1.0/0.1.5/0.1.8/0.2.0で個別確認）は全て単一トップレベルディレクトリ（`mcx/`）を持つ正しい構造で問題なし——構造バグはv0.2.1の初回zipのみの一過性のミスだったと確定
  - **`.github/workflows/update_package_index.yml`の重大な設計ミスを修正**: 「Download platform ZIP and compute checksum」「Update platform checksum」の2ステップが`platforms[0]`を決め打ちで参照・上書きしていた（エントリが1つしかない前提のコード）。`hardware/nxp/mcx/platform.txt`の`version=`行を読み、`platforms[]`の中から**そのバージョンに一致するエントリだけ**を検索して更新するよう変更。シェル変数をpythonのヒアドキュメントに未クォートで埋め込む脆い書き方だった箇所も、`VERSION="$VERSION" python3 << 'PYEOF'`（環境変数経由・quotedヒアドキュメント）に修正し安全性も向上
  - **今後のリリース手順の変更点**: 新バージョンをリリースする際は、`package_nxp_mcx_index.json`の既存エントリを上書きするのではなく、**`platforms`配列に新しいエントリを追加**すること（`checksum`/`size`は他バージョンと同様に一旦古い値のまま、または適当なプレースホルダーで良い——`update_package_index.yml`のworkflow_dispatch実行時に`platform.txt`のバージョンと一致するエントリを見つけて自動的に正しい値へ更新される）
  - **実機確認済み**: ローカルキャッシュ（`~/Library/Arduino15/package_nxp_mcx_index.json`）削除・Arduino IDE再起動後、Boards Managerのバージョンドロップダウンに0.1.0〜0.2.1の全11バージョンが表示されることを確認

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

## v0.2.2: Linux実機ビルドがファイル名の大文字小文字違いで失敗する問題を修正（`main`上で直接作業・2026-08-13リリース済み）
ユーザーがLinux実機（Ubuntu系、`nxl76485`ユーザー）でv0.2.1をBoards Manager経由インストール→Blinkスケッチのビルドを試したところ「Failed to install platform」は解消していたが、コンパイル自体が`#include <Arduino.h>` の時点で`compilation terminated`エラーで失敗。

- **原因**: このリポジトリの実ファイル名は`arduino.h`（小文字）だが、Arduino IDE/arduino-cliが全スケッチに自動挿入する行は`#include <Arduino.h>`（大文字A）。macOS・Windowsは既定でファイルシステムが大文字小文字を区別しないため今まで気づかれなかったが、Linuxは区別するため、実際に該当ファイルが見つからずビルド不能だった。これはv0.1.0リリース以来ずっと存在していた潜在バグで、今回が初めての実Linux環境でのビルド試行だったため今になって発覚した
- **同種の問題をコードベース全体で機械的に洗い出し**: 全`#include`文とその参照先ファイルの実際の大文字小文字を突き合わせるPythonスクリプトを書いて検査。もう1件、`#include <SPI.h>`（多くのサンプルスケッチが使用）に対し、実ファイルはr01lib低レベルSPIクラスの`spi.h`（小文字、`arduino_spi.h`とは別物）しか存在しないことが判明——macOS/Windowsでは`SPI.h`が（本来意図しない）`spi.h`にたまたま解決されて実害なく動いていたが、Linuxでは同様に見つからず失敗するはずだった
- **修正方針**: 大文字小文字違いの2ファイルを同一ディレクトリに共存させる方式は、macOS等の大文字小文字を区別しないファイルシステム上で開発する際にファイル内容が衝突・破壊されるリスクがある（実際に本セッション中、`SPI.h`を書き込んだ際に既存の`spi.h`の内容を上書きしてしまう事故が発生し、要復旧）ため、この方式は避けた:
  - `arduino.h` → `Arduino.h`にリネーム（真の実体ファイル。内部の`#include "arduino.h"`参照5箇所・サンプルスケッチ7本の`#include "arduino.h"`/`<arduino.h>`もすべて`Arduino.h`表記に統一）
  - r01libの低レベルSPIクラス`source/r01lib/spi.h`/`.cpp` → `r01lib_spi.h`/`.cpp`にリネーム（`r01lib.h`の参照も更新、自己参照していた無意味な`#include "spi.h"`行も削除）してファイル名を空け、そこに新規`SPI.h`（`#include "arduino_spi.h"`一行だけの薄いラッパー）を追加。既存の`arduino_spi.h`（本体）自体はリネームせず、参照箇所を増やさない
  - MCUXpressoのビルド設定（`Debug/source/r01lib/subdir.mk`）も`spi.cpp`→`r01lib_spi.cpp`に追従、`.a`をクリーンビルドし直し（リネーム前の`.o`が`ar`アーカイブに残留し重複していたため一度`.a`ファイル自体を削除してからリビルド）
  - **git側の注意点**: `git config core.ignorecase`がmacOSでは既定で`true`のため、`git mv`ではなく手動コピーで大文字小文字だけ違うパスをリネームすると、`git status`/`git add -A`がリネームとして認識せず「変更」としか見えず、実際には元の小文字パスのままcommitされてしまう罠があった。`git rm --cached`→`mv`（実ファイルをcase-preservingで大文字化）→`git add`で正しく大文字パスとしてindexに反映されることを確認
- **検証**: 通常のmacOS開発機（大文字小文字を区別しない）でのビルド確認だけでは同じ見落としを再現できないため、`hdiutil create -fs "Case-sensitive APFS"`で一時的にcase-sensitiveなAPFSボリュームを作成し、`hardware/nxp/mcx/`一式をコピーした上でxPack GCCで直接`#include <Arduino.h>`/`<SPI.h>`/`<Wire.h>`を使うテストコードをコンパイル、エラーゼロで通ることを実機Linux相当の条件で確認
- 全examples（Arduino_compatible_API・Arduino_incompatible_API）の回帰コンパイルも実施、問題なし
- **v0.2.2としてリリース済み**: ユーザー判断で即パッチリリース。`main`にfast-forward（別ブランチなし、直接main上で作業）、`platforms`配列に0.2.2エントリを新規追加（既存の0.2.1エントリ上書きではなく、CLAUDE.md記載の新方式どおり）、リリースzip作成→GitHub Release作成→タグpush（`gh release create`が自動でタグ作成・push、ローカルの重複annotatedタグは削除して整理）→`update_package_index.yml`手動実行でchecksum確定（`c63ffd4`、SHA-256:`52880c626303ee3deb89da26dcec694ada19eb79f042d7da16445ff904991b47`、ローカル計算値と一致確認済み、全12バージョンのエントリが正しく保持されていることも確認）。2026-08-13リリース。ローカル開発用symlinkも`0.2.2-dev`に更新
- **v0.2.2のBoards Managerインストール実機検証（macOS）**: 開発用symlinkを`packages/`外へ完全退避・ローカルインデックスキャッシュ削除の上でBoards Manager経由インストール実施。`hello_world`(Blink)・`test_combined_peripherals.ino`とも実機で正常動作を確認済み
- **v0.2.2のLinux実機検証: 成功**（今回の修正の本来の目的）。Boards Manager上で0.2.2が表示されない問題が発生したが、ローカルキャッシュのクリアがうまくいっていなかっただけと判明（ユーザー側で再度クリアして解決）。0.2.2をインストールし、Blinkスケッチのビルド〜実機書き込み〜実行まで成功を確認。**これでLinux対応が実機検証済みとなり正式サポート確定**（残りのPendingタスク#1が解消）
- **v0.2.2のWindows実機検証: 成功**。初回試行時はBoards Manager経由のダウンロードがネットワークタイムアウト（`Client.Timeout exceeded while awaiting headers`、GitHub側のURLは`curl`で200確認済みだったためWindows機側の一時的なネットワーク問題と判断）で失敗したが、時間を置いての再試行で解消し、動作確認完了と報告。**これでv0.2.2はmacOS・Windows・Linuxの3プラットフォームすべてで実機検証済み**
- **方針変更: TUTORIAL.md/TUTORIAL.ja.mdからバージョン番号を削除**: ユーザー指示で、タイトル・本文中の「(v0.2.1)」等のバージョン表記をすべて削除（英日両方）。**今後もチュートリアルにバージョン番号を含めない方針** — リリースのたびにタイトルを更新し忘れて古いバージョン表記のまま放置される問題（このセッションで実際に2回発生: 「チュートリアルが(v0.2.0)になってた」の指摘、その後v0.2.1→v0.2.2の際も同じ表記が残っていた）を構造的に無くすための判断。バージョン間の変更点を知りたい場合はCHANGELOG.mdへ誘導する形は維持

## v0.2.1 で作業中の内容（`prepare0.2.1` ブランチ→`main`マージ済み・2026-08-12リリース済み）

### リリース後: TUTORIAL.md/TUTORIAL.ja.mdの更新
ユーザーから「チュートリアルが(v0.2.0)になってた」と指摘。タイトルのバージョン表記だけでなく、本文もv0.2.0時点の内容のままで、v0.2.1で追加された機能のセクションが無いことが判明。「追加された機能はチュートリアルとして必要か」との問いに対し、このチュートリアルは「ペリフェラルを動かしてみる」実践ガイドでAPI網羅が目的のAPI_COMPATIBILITY.mdとは役割が違う、という観点で選別を提案:
- **`String`クラス**: 初心者が最初に書く実用的なスケッチで頻繁に使う基礎的な型のため、新規セクション「2.3. Strings」として追加（Serial出力セクションの直後、以降のセクション番号を2.4〜2.13に振り直し）
- **`INPUT_PULLDOWN`**: 既存の「Digital input and interrupts」セクションに一言追記のみ
- **Serial入力（`parseInt`等）**: 既存の「Serial output」セクションに`Serial.available()`/`parseInt()`の短い例を追記
- **`Print`/`Stream`抽象基底クラス、fast-GPIOレジスタ、`Wire.end()`、`PROGMEM`/`F()`、`Printable`**: ライブラリ作者向け・上級者向け機能のため追加しないと判断（API_COMPATIBILITY.mdで十分カバー済み）

ユーザー承認のうえ実施。英語版・日本語版の両方に同じ変更を反映（セクション番号・TOCアンカーとも整合性確認済み）。新規に書き起こしたStringスニペットは実際に`arduino-cli compile`でコンパイル確認してから掲載（ローカル開発用symlinkをBoards Manager実機検証のため一時退避していたため、`0.2.1-dev`として復元してから実施）。

また、Linux対応に関する古い記述（「Linuxはまだ未対応」「将来のリリースで予定」）も、v0.2.1で実際にLinux対応をリリースに含めた（実機未検証）という現状に合わせて両言語とも修正。

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

### サードパーティArduinoライブラリの実地コンパイルテスト → Print/Stream抽象基底クラス新設
5周目完了後、チェックリスト式監査の限界（聞いた観点にしか答えない）を補うため、ユーザー提案で実際に有名なサードパーティライブラリを`arduino-cli compile`してみる方針に転換。Adafruit NeoPixel/ArduinoJson/OneWire/Adafruit BusIO/Adafruit Unified Sensor/LiquidCrystal/Servo/DHT sensor libraryをインストールし最小スケッチでコンパイルテストした結果、NeoPixel・OneWireは成功したが、**ArduinoJson・LiquidCrystal・DHT（Adafruit_Sensor経由）が同一の根本原因で失敗**することが判明: `fatal error: Print.h: No such file or directory` / `'Stream' was not declared in this scope`。このプロジェクトには本家Arduinoにある抽象基底クラス`Print`/`Stream`が実在せず、`Printable`実装時（前回セッション）に`using Print = SerialClass;`という型エイリアスで済ませていたため、`class Foo : public Print`という一般的なパターン（`LiquidCrystal`、ArduinoJson内部の`StringBuilderPrint`、`Adafruit_Sensor`等）がSerialClassの「TX/RXピン必須コンストラクタ」に阻まれてコンパイルできなかった

- **LGPL移植 vs 独自実装の再検討**: ユーザーから「Print/Streamはハードウェアから独立した層なので本家から持ってこられないか」と提案。本家`Print.cpp`/`Stream.h`はArduinoCore-avr/API由来のLGPL 2.1で、`String`のとき（WString移植 vs 独自実装）と同じ論点だったため、LGPL 2.1の実際の義務（コピー・改変した該当ファイル自体はLGPL 2.1でライセンスし直す必要があり「明示するだけ」では済まない、一方それを使う側はMITのままでよい）を説明。その上で「独自実装では中身の互換性が担保できないのでは」との質問に対し、サードパーティ側が依存するのはクラス階層の形（メソッド名・シグネチャ・virtual dispatch）であって著作権保護される「中身」ではなく、かつ実際の振る舞い（数値print・parseInt等）は既にこのプロジェクトが独自実装済み・実機検証済みのコードであるため、移すだけで互換性は担保できると回答。ユーザー了承のうえ独自実装（MIT継続）で新設する方針に決定

- **新規ファイル**: `arduino_layer/Printable.h`（`Print`前方宣言のみ）、`Print.h`/`Print.cpp`（抽象`Print`基底クラス。`write(uint8_t)`のみ純粋仮想、`write(const uint8_t*,size_t)`はデフォルト実装ありの仮想関数、`print()`/`println()`群・`_print_num`系・`_utoa_radix`ヘルパーは全て旧`SerialClass`から移設し`size_t`返却に変更、DEC/HEX/OCT/BINマクロもここに集約）、`Stream.h`/`Stream.cpp`（`Print`を継承する抽象`Stream`基底クラス。`available()`/`read()`/`peek()`が純粋仮想、`find`/`findUntil`/`parseInt`/`parseFloat`/`readBytes`/`readString`等の`_timed_read`ベース実装も旧`SerialClass`から移設）
- **`arduino_serial.h`/`.cpp`の縮小**: `SerialClass`を`class SerialClass : public Serial, public Stream`に変更。旧来あった`print`/`println`/`find`/`parseInt`等の宣言・実装を全削除し、Stream/Printの必須オーバーライド（`write(uint8_t)`/`write(bulk)`/`available()`/`read()`/`peek()`/`availableForWrite()`、いずれもr01lib `Serial`のハードウェアプリミティブを呼ぶだけ）と`begin()`/`operator bool()`のみに削減。`write()`をオーバーライド宣言すると名前隠蔽でPrintの`write(const char*)`等も隠れてしまうため`using Print::write;`を追加（過去の「Serial.write()オーバーロード欠落バグ」と同じ名前隠蔽パターンだが、今回は戻り値型が両者ともsize_tで一致するため`using`で正しく解決できる）
- **ビルド設定**: `MCUXpresso_project/.../Debug/arduino_layer/subdir.mk`に`Print.cpp`/`Stream.cpp`をCPP_SRCS/CPP_DEPS/OBJS/cleanターゲットへ追加
- **`arduino.h`**: `#include "Printable.h"`/`"Print.h"`/`"Stream.h"`を`arduino_string.h`の直後・`arduino_serial.h`の直前に追加
- **副次的に見つかった2件の小さな互換性ギャップ**（DHT sensor library・Adafruit BusIOの再テストで判明、Print/Stream本体とは無関係）:
  - `microsecondsToClockCycles`未定義（DHTのタイムアウト計算で使用）→ `F_CPU`（このボードは96MHz固定、`board/clock_config.h`の`BOARD_BOOTCLOCKFRO96M_CORE_CLOCK`と一致）と`clockCyclesPerMicrosecond()`/`clockCyclesToMicroseconds()`/`microsecondsToClockCycles()`マクロを`arduino.h`に追加
  - `BitOrder`型が存在しない（Adafruit BusIOの`typedef BitOrder BusIOBitOrder;`で使用）→ 本家は`LSBFIRST`/`MSBFIRST`を`enum BitOrder`の列挙子として定義するが、このプロジェクトは既に`#define LSBFIRST 0`/`#define MSBFIRST 1`マクロを採用済み（`arduino_spi.h`の`enum endian`バグ修正の経緯参照）のため、同名の列挙子を持つ本物のenumは宣言できない（プリプロセッサがトークン置換してしまう）。`typedef uint8_t BitOrder;`という素朴な型エイリアスで対応——ライブラリ側は型名の存在だけを必要としており、列挙子名までは参照していないため実用上問題なし
- **再テスト結果**: ArduinoJson・LiquidCrystal・DHT sensor library・Adafruit BusIOすべてコンパイル成功に転換。NeoPixel・OneWireは引き続き成功。Servoのみ、ライブラリ側が対応アーキテクチャをソース内`#error`でハードコード管理しており`mcx`が入っていないため引き続き失敗——これはAPI不足ではなくライブラリ側の明示的な非対応で、こちら側の修正では直せない既知の限界として記録
- 全examplesの回帰コンパイル（既存test_Serial_BIN_and_write/test_PROGMEM_F_ARDUINO_macros/test_String_plus_numeric_Printable_fastGPIO等、旧`Printable`実装に依存していたスケッチ含む）も問題なし
- 確認用スケッチ: `examples/Arduino_compatible_API/test_Print_Stream_hierarchy/`（`class Foo : public Print`をハードウェア非依存で実装できることを`BufferPrint`で確認、`Stream&`への真の多態性を`Serial1`をベースクラス参照経由で`find()`する形で確認、`print()`/`println()`が実際のバイト数を返すようになったこと確認、`Printable::printTo()`内の`n += p.print(x)`累算イディオムが——以前はvoid返却のため無意味だったが——今回`size_t`化により正しく機能することを確認）。D0-D1ジャンパ要、実機確認はこのあと実施予定
- README.md/API_COMPATIBILITY.mdを更新（`Printable`の「`print()`がvoidを返す」という制約説明を削除し、`Print`/`Stream`の新規行を追加）

### v0.2.1リリース前の最終複合動作確認
タグ`0.2.1`をpush済み・GitHub Release作成前の段階で、ユーザーから「全体の動作テストをやっておいたほうがいいか」との提案。直近のPrint/Streamリファクタ（`SerialClass`の継承階層を作り直す大きな変更）は`test_Print_Stream_hierarchy.ino`単体では実機確認済みだったが、Serial1・I3C・ADC・PWM・tone・millisを同時に動かす`test_combined_peripherals.ino`はこのリファクタ後に未再実行だったため、最終確認として実機で再実行。`millis`/`micros`同期進行、`temp`安定（~25.8℃）、`adc`変動、`pwmDuty`5刻み増加、`serial1`ループバック（`hb0`〜`hb5`）欠落なし——全て正常、Print/Streamリファクタ後も複合動作に回帰なしと確認

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

## v0.3.0 で作業中の内容（`prepare0.3.0` ブランチ・未リリース）

FRDM-MCXN947のボード追加。ユーザー方針: 「C444とN947のどちらか」でN947を選択（既存スキャフォールディングの多さで判断）、「両方ある」（物理的にA153/N947双方所有）とのことだが技術的な理由でN947から着手。ブランチ名は`prepare0.3.0`とし、以前`prepare0.1.9`が最終的に0.2.0としてリリースされた前例に倣い、ブランチ名の数字は最終リリース番号を縛らない（他ボードが追加で乗ることもあり得る）。**リリース方針: 未実装が残っていてもN947が一通り完成した段階でリリースする**（ユーザー確認済み）。

### ソース取り込み
`/Users/tedd/dev/mcuxpresso/r01lib_prj_generator/general_tests_FRDM_MCXN947.zip`（2026-08-11付）の`_r01lib_frdm_mcxn947/source/r01lib/`を、リポジトリに既にあった約107コミット分古いコピーへ丸ごと上書き。

### A153から移植したr01libレベルのバックポート
新しいr01libコピーには以下がまだ無かったため、A153側で確立済みの実装をそのまま移植:
- `Serial::peek()`/`available()`/`availableForWrite()`/`flush()`（リングバッファ状態＋`LPUART_GetStatusFlags`ベース、チップ非依存でそのままコピー可）
- `InterruptIn::low()`/`disable()`（`InterruptIn.cpp`はA153版がN947の`#ifdef CPU_MCXN947VDF`分岐を既に含む上位互換だったため丸ごと差し替え）
- `SPI::bit_order()`・`clock_freq()`アクセサ（`r01lib_spi.h/.cpp`、N947固有の`cs_manual_control()`等は維持したまま追記）
- `DigitalInOut::input_buffer()`/`gpio_base()`/`gpio_bit()`（fast-GPIO、`io.h/.cpp`）

### ファイル名の大文字小文字統一（v0.2.2の教訓を先取り適用）
r01libの`spi.h/.cpp`→`r01lib_spi.h/.cpp`にリネーム（自己参照する無意味な`#include "spi.h"`も削除）、新規`arduino_layer/SPI.h`（`#include "arduino_spi.h"`一行のラッパー）を追加。`arduino_layer/arduino.h`→`Arduino.h`もリネーム。**リネーム作業中、macOSの大文字小文字非区別ファイルシステム上で`cp .../Arduino.h`streaming後に`rm -f arduino.h`を実行し、同一inodeのため直前にコピーしたファイルを消してしまう事故が発生**（`core.ignorecase=true`の既知の罠、v0.2.2作業時と同じパターン）。`git rm --cached`→大文字小文字を保持した`mv`/`cp`→`git add`の順で正しくインデックスに反映されることを再確認。

### NXP SDKドライバファイルの調達（重要な教訓）
`fsl_ctimer`/`fsl_inputmux`（tone/detachInterrupt等で将来必要)が新しいr01libコピーのdriversフォルダに無かったため、**最初A153の同ファイルをそのまま流用しようとしたが、ユーザーから明示的に危険と指摘された**（「driverファイル（fsl_ctimer/fsl_inputmux/fsl_lpadc）をA153からコピーは危険．N947用があるので，それを探して持ってきてください」）。正しい調達手順を確立:
1. ローカルの実N947プロジェクト（`/Users/tedd/dev/mcuxpresso/shasta_demo_prototypes/__n947/`等）から`fsl_inputmux.c`の実物を発見、デバイスヘッダの型（`INPUTMUX_Type`）と一致することを確認
2. `/Users/tedd/dev/mcuxpresso/mcuxide_sdk_mac/`に複数バージョンのNXP SDK zipが存在すると判明。既に正しく存在していた`fsl_gpio.c`とのMD5チェックサム一致で候補7個から`SDK_2_16_000_FRDM-MCXN947.zip`を厳密に特定し、そこから`fsl_ctimer.c/h`・`fsl_inputmux.c/h`を抽出
3. **N947にはLPADCペリフェラルが存在しないと判明**（`MCXN947_cm33_core0.h`に"LPADC"の出現数0、代わりに`ADC0`/`ADC1`＝`ADC_Type`を持つ）。誤って一度A153から`fsl_lpadc.c/h`をコピーしたが、この事実を受けて削除。正しいADCドライバ（`fsl_adc*.c`）はこのSDK zipにも含まれておらず、**未取得のまま**（analogRead/analogWrite/tone/noTone実装は次フェーズへ持ち越し）

### MCUXpresso Debugビルド設定の新規構築
`_r01lib_frdm_mcxn947`にはDebug設定が一度も存在しなかった（IDEでビルドされたことがなかった）。ユーザーが実際にビルド成功済みの`/Users/tedd/dev/mcuxpresso/__a_p_n947/_r01lib_frdm_mcxn947/Debug/`を教示、これをコピーしてベースに採用（絶対パスを`sed`で書き換え）。`arduino_layer/`用の`subdir.mk`は元々存在しなかったため新規作成（A153のものをモデルに、`-DCPU_MCXN947VDF`系defineと`-mfpu=fpv5-sp-d16 -mfloat-abi=hard`を設定、`arduino_analog.cpp`/`arduino_tone.cpp`はCPP_SRCSから除外）。`makefile`/`sources.mk`に`arduino_layer`を追加。最終ビルド: `lib_r01lib_frdm_mcxn947.a`、コンパイルエラーゼロ。

### リンカスクリプト（手書き）
MCUXpresso生成の`app_template_FRDM_MCXN947`プロジェクトの`.ld`断片から実メモリマップを確認: `PROGRAM_FLASH0`(1M, 0x0)+`PROGRAM_FLASH1`(1M, 0x100000, 連続)、`SRAM`(384K, 0x20000000)、`SRAMX`(96K, 0x4000000)、`SRAMH`(32K, 0x20060000, SRAMと連続)、`USB_RAM`(4K)。A153の`MCXA153.ld`のセクション配置ロジック（`SRAMX0`/`SRAMX1`という2つの補助高速RAM領域の枠組み）を踏襲しつつ、PROGRAM_FLASH0/1を単一の2M領域に統合、SRAMX→SRAMX0相当、SRAMH→SRAMX1相当としてマッピング。`USB_RAM`は未使用のため省略。ヒープ/スタックはA153よりRAM余裕があるため`_HeapSize=0x4000`/`_StackSize=0x1000`とやや大きめに設定。`hardware/nxp/mcx/variants/frdm_mcxn947/linker/MCXN947.ld`として新規作成。

### boards.txt/platform.txt: マルチボード対応のための構造変更
1点目、共有ファイル`cores/arduino/Arduino.h`（両ボード共通、`build.core=arduino`）に`F_CPU`が96MHz固定でハードコードされていることが判明（N947は150MHz、`BOARD_BootClockPLL150M()`）。`platform.txt`の`compiler.defines`に`-DF_CPU={build.f_cpu}`を追加し、`#ifndef F_CPU`ガード化。boards.txt側でボードごとに`build.f_cpu`（A153=96000000UL、N947=150000000UL）を設定。同期コピーされている3箇所のArduino.h全てに同じガードを反映。
2点目、`tools/upload.sh`/`upload.bat`が**LinkServerのターゲット文字列`MCXA153:FRDM-MCXA153`を決め打ち**していたと判明（ボードに依らず常にこの文字列でflashしようとしていた——N947書き込み時に見つかっていなければ誤動作していたバグ）。コマンドライン引数化し、`platform.txt`の`tools.linkserver.upload.pattern`から`{build.linkserver_target}`を渡すよう変更。boards.txtに`frdm_mcxn947.build.linkserver_target=MCXN947:FRDM-MCXN947`を追加（ローカルLinkServerの`devices`一覧で実在確認済み）。
3点目、`compiler.cpu_flags`に`{build.fpu_flags}`を追加（N947は`-mfpu=fpv5-sp-d16 -mfloat-abi=hard`、A153は空文字列）。
`frdm_mcxn947.vid.0`/`pid.0`はA153と同じ`0x1FC9`/`0x0143`を仮設定（同系統のMCU-Linkオンボードプローブという推測）→ **実機接続時に`arduino-cli board list`が正しく`nxp:mcx:frdm_mcxn947`を候補に挙げたため、この値で正しいと確認済み**。

### ヘッダ・ライブラリのhardware/側への同期
`variants/frdm_mcxn947/include/`にr01lib・arduino_layer・board・CMSIS・device・drivers・utilities・componentの各ヘッダを集約（A153のパターンを踏襲）。`AnalogIn.h`/`PwmOut.h`は当初「N947未対応だから」と除外したが、これは誤りと判明——`r01lib.h`がCPU無関係に無条件でこの2つをincludeする作りで、実体は`#if defined(CPU_MCXA153VLH)`で丸ごとガードされ他チップでは空ファイルになる安全設計だったため、除外するとビルドが壊れる（実際に`arduino-cli compile`で`AnalogIn.h: No such file`エラーが発生し発覚）。同様に共有`cores/arduino/Arduino.h`が`arduino_analog.h`/`arduino_tone.h`を無条件includeしており、これも中身が関数プロトタイプ宣言のみ（CPUガードなし、実際に呼ぶスケッチだけがリンクエラーになる設計）と判明したためコピーで対応。`lib_r01lib_frdm_mcxn947.a`を`variants/frdm_mcxn947/lib/`へ配置、リンカスクリプトを`linker/`へ配置。

### arduino-cliコンパイルテスト
ローカルにローカル開発用symlink（`~/Library/Arduino15/packages/nxp/hardware/mcx/0.2.2-dev`）を再作成。ただし実インストール済みの`0.2.2`が`packages/`配下に共存していると、semverの事前リリースタグ比較で`0.2.2-dev`が`0.2.2`より低優先度と判定され`arduino-cli`から見えなくなる問題が新たに判明（過去バージョンでは実インストール版を毎回退避していたため気づいていなかった罠）。実インストール版をscratchpadへ完全退避してから`0.2.2-dev`のみが見える状態にして検証。`hello_world`/`test_String`/`test_Serial1`/`test_SPI_bitorder_end_transfer16`/`test_arduino_compat_macros`がコンパイル成功（A153側の回帰もなし）。

### 実機バグ発見・修正: `Serial1`のグローバルコンストラクタがstatic初期化時にpanic()
`hello_world.ino`を実機フラッシュしたところ、setup()の中身に無関係にSOSのモールス信号でLEDが点滅し続ける不具合が発生（panic()自体はRED/GREEN/BLUEを直接操作してSOSを表示する仕組み）。
- **切り分け**: (1)GPIOのみのスケッチ→正常, (2)`Serial.begin()`のみ追加→SOS再現、という2段階の実機テストでSerial関連と特定
- **根本原因特定**: LinkServerの`gdbserver`モードで実機に接続し、`arm-none-eabi-gdb`から`Serial::Serial(int,int,int)`と`panic()`両方にブレークポイントを設定してライブ確認。グローバル`Serial`（USBTX/USBRX、tx=42/rx=41）は正常に`_base`が解決される一方、`arduino_serial.cpp`がA153から丸ごと移植された際に残っていたグローバル`Serial1`（D0/D1、tx=97/rx=98）が`panic("Serial: unsupported TX/RX pin combination")`を呼んでいることを直接確認。N947のr01lib `Serial.cpp`の`s_pinMap[]`はUSBTX/USBRX→LPUART4の1エントリのみで、D0/D1に対応するエントリが存在しないため
- **修正**: `arduino_serial.cpp`のグローバル`Serial1`インスタンス定義と、`arduino_serial.h`の`extern`宣言を削除。`Serial1`を参照するスケッチはコンパイルエラー（`'Serial1' was not declared in this scope`）になるよう変更——以前の「実行時panicで気づく」より診断しやすい失敗モードに変更
- D0/D1（ARD_D0/ARD_D1、物理P4_3/P4_2）は`FC2_P2`/`FC2_P3`のalt機能を持ちLPUART2として使える可能性があるが、クロックアタッチ/リセットのシンボル特定と実機検証が必要な別作業として`variants/frdm_mcxn947/README.md`に記録、Serial1実装は保留（ユーザー確認: 「Serial1は必要なのであとで実装する事にする」）
- **後日、Serial1実装に着手しようとしたところ、根本的な資源競合が判明**: `mcu.cpp`の`init_mcu()`を確認したところ、`Wire`（I2C_SDA/SCL=D18/D19）は既に`LPI2C2`＝FlexComm2を使用していると判明（`/* I2C */ CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2)`）。schematicで確認したD0/D1のFlexCommとしてのalt機能は`FC2_P2`/`FC2_P3`のみ——つまりD0/D1をUARTとして使うにも同じFlexComm2が必要で、`Wire`と`Serial1`は同時に使えない（LP_FLEXCOMMは1インスタンスにつき同時にひとつのモードしか持てないため）。ユーザーに提示し（AskUserQuestionで3択: 排他利用として実装/別ピン再検討/見送り）、**「Serial1は見送る」と決定**。`variants/frdm_mcxn947/README.md`の記述を「未特定・検証中」から「意図的に未対応（FlexComm2資源競合のため）」に更新
- 修正後、`test_Serial1`がN947向けに正しくコンパイルエラーになること、他の全examplesに回帰がないことを再確認

### 実機検証状況（v0.3.0時点、正確に記録）
- **実機フラッシュして動作確認済み**: GPIO（`pinMode`/`digitalWrite`/`delay`）、Serial（USB経由、`Serial.begin()`/`println()`、RGB LED制御と組み合わせた`hello_world.ino`で確認）、SPI・Wire・Wire1・analogRead・analogWrite・tone/noTone（後述の各専用セクション参照）
- **`arduino-cli compile`のみ確認済み（実機フラッシュはまだ）**: `String`、UNO互換マクロ（`test_arduino_compat_macros`）
- **意図的に未対応**: `Serial1`（上記参照）

### 実機バグ発見・修正: Wire/Wire1（I2C/I3C）動作確認とI3C2件の実機バグ
`Serial1`修正後、ユーザーからI2C/Wire実機検証の依頼。ユーザー方針: 「ArduinoコネクタのI2CピンをWireとして，こちらに繋がってる方をWire1とする」（P3T1755オンボードセンサー側＝Wire1）。既存の`arduino_i2c.cpp`（`Wire(I2C_SDA,I2C_SCL)`=D18/D19、`Wire1(I3C_SDA,I3C_SCL)`=MB_RX/MB_TX、I3CをI2C_MODEで使用）はA153から丸ごと移植済みで、コード上は既にこの意図通りだった。schematic（`FRDM-MCXN947SH.pdf`、ユーザー提供）でP3T1755のI2Cアドレスが`0x48`、I3C1_SDA/SCLがP1_16/P1_17（=MB_RX/MB_TX）であることを確認し、外部ライブラリ`P3T1755.h`が無かったため生レジスタアクセスのテストスケッチを新規作成して検証開始。

- **実機バグ1: I3C1のIBE（入力バッファ）が有効化されていない**: `Wire1`で温度センサーのレジスタ読み取りを試すと`requestFrom`が常に失敗（write成功、read失敗）。GDB（LinkServerのgdbserverモード）で`I3C_MasterTransferBlocking()`にブレークポイントを張り、write時は`status=0`、read時は`status=7902`(`kStatus_I3C_Nak`、アドレスフェーズでNAK)であることを直接確認。`pin_mux.c`を調査したところ、I3C1_SDA/SCL(P1_16/P1_17)のIBE設定コードは`BOARD_InitDEBUG_UARTPins()`という関数内にあったが、この関数自体がコードベースのどこからも呼ばれていないと判明（`init_mcu()`は`BOARD_InitBootPins()`/`BOARD_InitBootClocks()`/`BOARD_InitBootPeripherals()`のみ呼ぶ）。`PORT_SetPinMux()`はMUXフィールドしか触らないため、IBEはリセット後デフォルト（無効）のまま——A153のSerial1 D0/D1バグと全く同じ問題パターン。`I3C::I3C()`コンストラクタに`_scl.input_buffer(true)`/`_sda.input_buffer(true)`を追加して解消
- **実機バグ2: I2C_MODEに切り替える前に一度も実I3Cバス動作をしていないと、以降の読み取りが常にNAKする**: バグ1修正後も読み取り失敗が継続。ユーザーが実機のプルアップ回路をschematicで確認し「R51/R52（標準4.7kプルアップ）がDNP、I3C1_PUR(P1_11)というI3Cペリフェラル駆動の動的プルアップに依存」という設計を指摘。この時点でユーザーから「プルアップはチップ内部で自動的に有効化される．外部プルアップは電流ブースト用のオプション．I2Cモードで動いていれば問題ないはず」との指摘があり、内部弱プルアップ（`DigitalInOut::mode(PullUp)`、PORT_PCR_PS/PE経由）を試したが効果なし（ロールバック）。ユーザー提供のNXP公式デモ（`ref/dm-i3c-temperature-sensor-main.zip`内`P3T1755_FRDM_MCXN947_demo_DAA`、同バスでLM75Bへの I2Cアクセス例も含む）を精査したところ、I2C_MODEへの切り替え前に必ず`ccc_broadcast(CCC::BROADCAST_RSTDAA, ...)`を実行していることを発見。GDBのライブ関数呼び出し（`Wire1`の内部`i2c`ポインタを`I3C*`にキャストして`reg_read()`/`ccc_broadcast()`を直接呼ぶ）でA/Bテストを実施——RSTDAA無し:確実にNAK、RSTDAA有り（それ自体は`kStatus_I3C_WriteAbort`で失敗するのが正常、動的アドレス未割当のため）:確実に成功、を複数回再現。ユーザーからのさらなる指摘「A153で必要なかったのなら，これはN947だけで実行されるようにしておいて」を受け、`#ifdef CPU_MCXN947VDF`でN947限定にガード（A153のI3C_SDA/SCLは実プルアップのある汎用I2Cコネクタと共有ピンのため元々不要）。根本原因は未特定（コード内コメントに明記——I3C仕様上はプルアップアシストが常時有効なはずで、`I3C_MasterInit()`だけではステートマシンが起動せず実際のSTART/STOPサイクルを一度経験する必要がある、というSDK/シリコン初期化順序の問題ではないかという推測にとどまる）
- ユーザーからの検証依頼で、RSTDAA行を一時的に削除して再ビルド・再フラッシュ→失敗再現、復元して再ビルド・再フラッシュ→成功再現、という**制御されたA/Bテストを実機で2往復実施**し再現性を確定させてから最終版として確定
- `Wire`（プレーンI2C、D18/D19）はバススキャンスケッチで実機確認済み（ハング・クラッシュなし、外部デバイス未接続のため0件検出）。実デバイスとの実通信は後日ロジアナ入手後に確認予定としていた（後日実施・確認済み、下記「Wireの実機検証: 外部LM75系センサーとの実通信確認完了」セクション参照）
- `variants/frdm_mcxn947/README.md`に「Wire / Wire1 の実機検証済み動作（既知の癖あり）」セクションを新設し、上記2件のバグと回避策を詳細に記録
- 参考資料`ref/dm-i3c-temperature-sensor-main.zip`（NXP公式、ユーザー提供）を`ref/`に配置、`.gitignore`に`ref/`を追加（リポジトリ管理対象外）。ユーザーはさらに`ref/r01lib_pin_table.xlsx`（各ピンのavailability一覧）も配置、今後のピン確認作業で参照する

### SPI実機検証（MOSI-MISOループバック、暫定）
`transfer()`/`transfer16()`/`setBitOrder(LSBFIRST)`/`end()`→`begin()`再初期化まで一通り往復確認OK。ただしテスト設計時に1件つまずいた: 合否表示にLED `GREEN`を使ったところ「何も点灯していない」ように見えた実機報告があり、GDBで停止位置を確認したところ実際には正常に`ok==true`のGREEN点滅ループ内で動作していた（矛盾）。ユーザーの指摘で判明: `GREEN`はD10で、SPIのデフォルトChip Select（`SPI_CS`/`ARD_CS`）と同じ物理ピンだったため、SPIトランザクション中のCS制御とLED表示用`digitalWrite`が同じピンを取り合っていた。`BLUE`に変更して解消、以後BLUE点滅で正常動作を確認。N947のLED定義（`RED=D9`, `GREEN=D10`, `BLUE=D6`）のうちGREENだけがデフォルトSPI CSと重複するため、SPI関連のテストスケッチではLED表示にGREENを使わないことを`variants/frdm_mcxn947/README.md`に記録。**ユーザー方針: この結果も暫定とし、Wireと同様に後日改めて実機で再確認する**

### Serial（USB経由）モニター出力の取得——最終的に解決（原因はこちらのツール側）
SPI検証中、LEDベースの判定に頼らずシリアルモニターで結果を読みたいというユーザー要望から調査。`/dev/cu.usbmodemUENBVJCYVDM5J3`（LinkServerのデバッグポートと同じノード）に対し`stty`でボーレート設定後`cat`で2秒間読み取りを試みたが、`Serial.println()`を300ms間隔で連続出力するテストスケッチを実行してもASCIIテキストは得られず、毎回同じ6バイトのゴミデータのみ取得。
- ユーザー仮説: 「このコードの元にしたr01libのプロジェクトはSemihost設定になっていたため」→ `board.h`を確認したところ`BOARD_DEBUG_UART_TYPE=kSerialPort_Uart`で既にUART指定（semihostではない）と判明
- 別仮説として、`board.c`の`BOARD_InitDebugConsole()`が`DbgConsole_Init()`経由でLPUART4（r01lib `Serial`クラスがUSBTX/USBRX用に既に初期化済みの同一ペリフェラル）を`init_mcu()`内で再初期化しており、グローバルコンストラクタ（`Serial`のハードウェア初期化はここで走る）より後に実行されるため、Serialの設定を上書き・破壊しているのではと推測。`BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL`（既存のオプトアウト用ガード）を定義して`BOARD_InitDebugConsole()`をスキップし実機再テストしたが、**結果は変化せず同じ6バイトのゴミデータ**——この仮説は否定された（A153も同一パターン（`BOARD_InitDebugConsole()`が同じLPUART0をSerialと共有）だが実績上問題が起きていない点とも整合）
- 上記2件とも実機テストの上で棄却。同じ6バイトが再現性高く得られる点から、USB列挙時のノイズかCMSIS-DAPプロトコルの混線であり、そもそもこのプローブがLPUART4のCDCブリッジを別ttyとしてこの環境に公開していない可能性が高いと判断。`mcu.cpp`の変更はロールバック（コミットなし）
- ユーザーから「r01libのprintfを呼んだら出力は出る？」との質問。`fsl_debug_console.h`で`DEBUGCONSOLE_REDIRECT_TO_SDK=1`と確認、`platform.txt`の`-DSDK_DEBUGCONSOLE=0`（0≠1）によりr01lib.hの分岐で`SEMIHOST_OPERATION`が有効になっている（＝`printf`は素のnewlib実装、ARMセミホスティング経由でデバッガ接続時のみ出力される）ことをソースから確認。LinkServerのgdbserverをセミホストポート付き（`--semihost-port 4444`）で起動し`nc`でtelnet接続を試行——ポートバインディングがtarget実行開始後の遅延ありのタイミングでしか有効にならずrace conditionが強く、1回だけ接続成功（セミホスト機構自体は生きている証拠）したが実際の出力バイトは取得できず、以後は接続拒否やハングが再発。ツールチェーン起因の不安定さと判断しこの経路は断念（`crt_emu_cm_redlink`の残存プロセスをkill -9で強制クリーンアップする一幕もあった）
- ユーザーから「A153では`-DSDK_DEBUGCONSOLE=0`になってる？」との確認質問に対し、`platform.txt`は両ボード共有ファイルなので**A153も同一設定**であると回答。ただしここで重要な整理: `printf`/`PRINTF`（セミホスティング分岐の対象）とArduinoの`Serial.println()`（`SerialClass`/`Print`経由、r01lib `Serial`クラス独自実装がLPUART4レジスタを直接操作、`printf`とは完全に別経路）は無関係——A153の`Serial`出力はこの設定下でも長年実機で安定して確認されてきている実績があり、`SDK_DEBUGCONSOLE`/セミホスティングの話は「N947のSerial出力が読めない」という元々の謎の説明には**ならない**と判明
- ユーザーから「N947ではUSBTX, USBRXクラスを指定して入出力が得られていた．このr01libのクラスをそのまま使って出力を確認したい」との指摘を受け、Arduinoの`Print`/`Stream`層を完全にバイパスしてr01lib `Serial`クラス自身の`printf()`メンバメソッド（`Serial.printf(...)`、`SerialClass`が`public Serial`を継承しているため直接呼び出し可能）を使うテストスケッチを作成。実機書き込み後、ユーザーに**実際のシリアルターミナルアプリで確認してもらったところ、正常に読めることが判明**（`=== raw r01lib Serial::printf test ===`に続き`raw hello 0/1/2...`が正しく表示）——**ハードウェア・ファームウェアは最初から一貫して正常だった**
- 原因はこちらの検証環境（sandboxed bashからの`stty`/`cat`）側にあると判明。`stty -f <port> 115200`でボーレートを設定しても、直後に`stty -f <port> -a`で確認すると**常に`9600`のまま変化しない**ことを発見（このUSB CDCデバイスのmacOSドライバがtermios経由のボーレート変更を反映しない挙動があると推測）。これまでの「6バイトのゴミデータ」「ボーレートを変えても同じ崩れ方」は、実際には常に9600固定で読んでいたことによる典型的なボーレート不一致の症状だったと解釈するのが最も整合的。`screen`コマンド（`-Logfile`オプション未対応の古いバージョン）・`arduino-cli monitor`もこの環境では出力を取得できず、同根の問題と推測
- **結論・今後の方針**: Serial出力自体は完全に正常に動作している。この session環境（sandboxed bash）からの自動読み取りは技術的制限で断念し、Serial出力の確認が必要な場面は引き続きユーザーに実際のターミナルアプリで見てもらう運用とする。自動化された検証は引き続きLED/GDBベースを基本とする

### analogWrite実装（PWM0/FlexPWM、実機検証待ち）
残りの作業候補「6. analogRead/analogWrite」の後半に着手。A153の`PwmOut`クラス（FlexPWM0ベース）を参考にN947版を新規実装。

- **重大な名前衝突バグを発見・修正**: N947のSDKは`PWM0`というマクロをFlexPWMペリフェラルのインスタンス自体（`(PWM_Type*)PWM0_BASE`）として既に定義している。当初io.hの論理ピン名も`PWM0`-`PWM5`（A153と同じ命名）にしたところ、`PwmOut.cpp`内の`PWM_Init(PWM0,...)`等のSDK呼び出しがことごとく「ピン番号→`PWM_Type*`への変換不可」でコンパイルエラーになった（マクロ展開でピン番号の`PWM0`が優先されてしまうため）。A153はSDKのインスタンスマクロが`FLEXPWM0`だったためこの衝突が起きなかった、というN947固有の落とし穴。論理ピン名を`PWM_0`-`PWM_5`（アンダースコア区切り）にリネームして解消。io.h・arduino_io.h・PwmOut.h/.cppの該当箇所を機械的に置換（Python正規表現で単語境界`\bPWM([0-5])\b`のみを対象にし、`PWM0_A1`等のSDK側alt-function名は巻き込まないよう注意）
- **NXP公式のFRDM-MCXN947 FlexPWM driver example**（同SDK zip内）を参照して確認: 特別なクロックアタッチは不要（`CLOCK_EnableClock(kCLOCK_Pwm0_SmX)`のみで足り、A153と同様`CLOCK_GetFreq(kCLOCK_MainClk)`を周波数ソースとして使う）。インスタンス名は`FLEXPWM0`ではなく`PWM0`/`PWM1`（`PWM_Type*`）、リセットシンボルは`kPWM0_RST_SHIFT_RSTn`
- **ピン・サブモジュール・ALT値の導出**: A153と同じ6物理ピン（P3_6..P3_11、モーターコントロール/PWM専用コネクタ由来と見られる、D/A番号と重複しない専用ピン群）を採用したが、サブモジュール番号がA153と異なる（A153=sm0/1/2、N947=sm1/2/3——`pin_mux.c`のpin_signal文字列、例えばP3_6の`.../CT4_MAT2/PWM0_A1/...`のA/B後の数字がサブモジュール番号というNXPの命名規則から確認）。ALT mux値も6ピンで不均一（Alt4とAlt5が混在）——各ピンのalt-function列内でPWM0_Ax/Bxが出現する位置が異なるため（`WUU0_INxx`等が途中に挟まるピンとそうでないピンがある）、I3C1_SDAのAlt10確認で確立した「pin_signal文字列内の位置を数える」方式で個別に導出
- `arduino_analog.cpp`の`analogWrite()`をA153と同じロジック（`PwmOut`をpin単位でnew、`period_us(1000)`で1kHz固定、`write(duty)`で0.0-1.0のduty設定）に差し替え、`panic()`スタブを置き換え
- ビルド設定: `Debug/drivers/subdir.mk`は変更不要（fsl_pwm.cは既存の共通ドライバとして既にビルド対象済み）
- `arduino-cli compile`で確認、hello_world/A153側の回帰なしも確認。実機フラッシュ・動作確認スケッチ（PWM_0のduty比を0→255→0でランプさせ、テスターかLEDで確認する想定）も用意したが、**ユーザー方針でこの実機検証は後日に持ち越し**（Wire/SPIと同様、実装は完了しているが未検証の状態として記録）

### tone/noTone実装（CTIMER0、実機検証待ち）
残りの作業候補「7」に着手。A153の`arduino_tone.cpp`を確認したところ、この1ファイルはCPU依存の`#ifdef`分岐が一切ない完全にチップ非依存な実装だった（`DigitalInOut`/`CTIMER0`/`CLOCK_AttachClk`など全て汎用r01lib API経由）。そのままN947にコピーして試したところ、クロック分周設定の1行だけがコンパイルエラーになった。

- **N947固有のシンボル差異**: 分周器のマクロ名がA153（`kCLOCK_DivCTIMER0`、全て大文字）とN947（`kCLOCK_DivCtimer0Clk`、大文字小文字混在＋"Clk"サフィックス付き）で異なる。さらに分周設定関数自体の名前も違う（A153=`CLOCK_SetClockDiv`、N947=`CLOCK_SetClkDiv`）。NXP公式のFRDM-MCXN947 CTIMER driver example（`boards/frdmmcxn947/driver_examples/ctimer/simple_match_interrupt`、同SDK zip内）で実際の呼び出し方を確認して修正。`CTIMER0`インスタンス・`CTIMER0_IRQn`・`kFRO12M_to_CTIMER0`・`CLOCK_GetCTimerClkFreq()`はA153と共通の名前でそのまま使えた
- 以前この作業の準備として`drivers/`に`fsl_ctimer.c/h`をコピー済みだった（version-matched SDK由来）が、Debugビルド設定（`drivers/subdir.mk`）には未登録のままだったと判明——今回追加してビルド対象に含めた。`arduino_layer/subdir.mk`にも`arduino_tone.cpp`を追加
- `Arduino.h`の`#include "arduino_tone.h"`を有効化（これまでコメントアウトしていた最後の未対応機能）
- `arduino-cli compile`で確認、hello_world/A153の回帰なしも確認。特筆すべき点: これまでtone()依存でN947向けにリンクエラーになっていた既存example（`test_tone`、`test_shiftOut_pulseIn_random`）が、この実装により両方ともコンパイル成功に転じたことを確認——地味だが「後方互換的に既存exampleが直る」というポジティブな副作用
- 確認用スケッチ（D3で440Hz→880Hzを繰り返す、ピエゾブザー/スコープ想定）を実機フラッシュまで実施したが、**ユーザー方針でこの実機検証も後日に持ち越し**（Wire/SPI/analogWriteと同じ扱い）

### ドキュメント整備
残りの作業候補「13〜15」＋ライセンスチェックに着手。
- **`CHANGELOG.md`**: `[Unreleased]`セクションを新設し、N947ボード対応・`Serial1`非対応の理由・`boards.txt`のper-board化・`upload.sh`/`upload.bat`のLinkServerターゲット決め打ちバグ修正を記載
- **`API_COMPATIBILITY.md`**: これまでA153単独の記述だったのを両ボード対応に更新。差分がある行（`Serial1`＝N947非対応、`Wire1`＝ボードごとに物理ピンが違う、`analogRead`＝A153はA0-A3・N947はA2-A5、`analogWrite`＝ピン名が`PWM0-5`(A153)/`PWM_0-5`(N947)で異なる、`ARDUINO_FRDM_MCXA153`/`ARDUINO_FRDM_MCXN947`）に個別に注記
- **`README.md`の"Supported Boards"表**: N947のステータス表示（🔜→✅）についてユーザーに確認したところ、**「🔜のままにする」と回答**——基本機能は実装済みだが一部（Wire実デバイス・SPI再確認・analogWrite・tone実機検証）が未検証のため、正式リリースまでは据え置く判断。ピンマッピング等のN947詳細情報は引き続き`variants/frdm_mcxn947/README.md`側にのみ記載し、トップレベルREADME/TUTORIALには今回は追加しない方針
- **`boards.txt`最終レビュー**: `build.pyocd_target`プロパティが実は`platform.txt`のどのレシピからも参照されていない（LinkServerのみ実装済みでpyOCDアップロードは未実装）ことに気づき、ユーザーに確認。**「コメントを追加」を選択**——A153・N947両方に「現在未使用、将来pyOCD対応する際のための先行メタデータ」という趣旨のコメントを追加（プロパティ自体は削除せず維持）。VID/PID（`0x1FC9`/`0x0143`）は実機接続時の`arduino-cli board list`自動検出で確認済みである旨のコメントも追加
- **ライセンスチェック**（ユーザーからの明示的なリマインダー対応）: `LICENSE`のThird-Party Noticesセクションで`hardware/nxp/mcx/cores/arduino/arduino.h`という**古いパス参照**（v0.2.2で`Arduino.h`にリネーム済みだが未追従だった）を発見・修正。また、今回追加したNXP SDKドライバファイル（`fsl_lpadc.c/h`等）のcopyrightヘッダーが正しく保持されていることを確認した上で、**NXP MCUXpresso SDK由来のdriverファイル群についてLICENSEに一切言及がない**（A153の頃からの既存ギャップ）ことをユーザーに提示。**「LICENSEに追記」を選択**——BSD-3-Clauseである旨とリポジトリURLをThird-Party Noticesに新規追加

### push時に発覚: `.a`肥大化によるGitHub拒否と履歴の一本化
ドキュメント整備完了後、ユーザーから「まだpushしてない？」との確認を受けpushを試行したところ、GitHubに**reject**された——`hardware/nxp/mcx/variants/frdm_mcxn947/lib/lib_r01lib_frdm_mcxn947.a`が複数コミットで100MB超（最大224MB）になっており、GitHubのファイルサイズ上限（100MB）に抵触。

- **原因調査**: A153の`.a`（約50MB）と比較したところ、N947は(1) オブジェクト数がA153の54個に対し84個と多く、(2) 同じ`AnalogIn.cpp`ファイルでもN947のオブジェクトがA153の約2.6倍のサイズ（3.06MB vs 1.16MB）——2点の原因を特定
  1. **`r01device`配下の無関係なドライバがビルドに紛れ込んでいた**: `Debug/sources.mk`/`makefile`が、セッション冒頭でコピー元にした`__a_p_n947`プロジェクト（このN947 Debugビルド設定の取得元）のフル機能テストスイート設定をそのまま引き継いでおり、RTC（PCF2131等）・LCD・LEDドライバ・ADC（NAFE33352）・EEPROM等、Arduino層が一切使わない約27個の`source/r01device/*`オブジェクトがアーカイブに含まれていた。A153の`sources.mk`/`makefile`と比較して該当サブディレクトリを完全に除外
  2. **デバッグシンボル（`-g3 -gdwarf-4`）がN947のSDKヘッダの複雑さで異常に肥大化**: 上記1を修正した後も142MBと依然超過。個別オブジェクトを調査したところ、どのファイルも一律約3MBというデバッグ情報由来と思われる肥大化を確認。`arm-none-eabi-strip --strip-debug`で配布用`.a`のデバッグシンボルを除去したところ**142MB→約500KB**まで縮小。リンク・動作に必要なコード/シンボルテーブルは無傷（実際に複数のテストスケッチで再コンパイル・リンク確認済み）。プレビルドライブラリはリンク用途のみで、エンドユーザーがソースレベルデバッグする用途はそもそも想定していないため、デバッグシンボルの同梱自体が無意味だったと判断
- **履歴の一本化**: この時点で`.a`を含む問題のあるコミットが5つ（初期ポート・Wire/I3Cバグ修正・analogRead・analogWrite・tone実装）に渡って存在し、pushはまだ一度も成功していなかった（originに`prepare0.3.0`は未作成）ため、履歴を書き換えても安全と判断。ユーザーに「各コミットを個別修正」か「履歴を一本化」かを確認し、**「履歴を一本化」を選択**——`git reset --soft`でmainとの分岐点（`17b4959`）まで戻し、修正済みの軽量`.a`を含む全体を単一コミット`23846ce`として再コミット。分岐点からのブロブサイズを全チェックし50MB超のオブジェクトが皆無であることを確認してからpush、成功
- A153側の`.a`（既にリリース済み、50MBで問題なし）はこのブランチでは触れず、スコープを最小限に維持
- 今後の教訓としてCLAUDE.mdに記録: プレビルド`.a`は配布前に必ず`--strip-debug`すること、他プロジェクトのDebug設定をコピーする際は`sources.mk`/`makefile`のSUBDIRSがArduino層に本当に必要なものだけかを確認すること

### `PIN_MAPPING.md`の新設 → ボードごとに`PIN_MAPPING_A153.md`/`PIN_MAPPING_N947.md`に分割
ユーザーから「READMEのpin mappingの項、A153だけしかない。N947も入れないといけない。でもそれでは大きくなりすぎるので、別ファイルにする？」との提案。`API_COMPATIBILITY.md`/`CHANGELOG.md`/`TUTORIAL.md`も同じ理由（README肥大化）で過去に分離した前例があり、同じパターンを踏襲することで合意・実施。まず単一の`PIN_MAPPING.md`にA153（既存表を移設）＋N947（新規表、`io.h`から機械的に抽出: D0-D19、A0/A1非対応・A2-A5対応、`PWM_0`-`PWM_5`、`USBTX`/`USBRX`、`I3C_SDA`/`I3C_SCL`(`MB_RX`/`MB_TX`)、`SW2`/`SW3`——`SW2`が`A5`とピン共有している点、`Serial1`非対応の理由も注記）の2セクションを同居させる形で作成。
- 直後にユーザーから「PIN_MAPPING.mdはボードで分けるべきではない？」と再提案。API_COMPATIBILITY.mdは「共通部分＋一部差分」の性質でまとめる合理性があったが、ピンマッピングは物理ピンがボード間でほぼ完全に別物（比較のメリットが薄い）で、かつ実ユーザーは通常どちらか一方のボードしか持たないため、1ファイルにまとめるより探しやすいという判断で分割に合意
- ユーザーから「`variants/*/README.md`は深いところにあって見つけにくくないか」との確認があり、分割後のファイルは`variants/`配下ではなくリポジトリ直下（`README.md`と同階層）に置く方針であることを明確化してから実施
- `PIN_MAPPING.md`を削除し、`PIN_MAPPING_A153.md`/`PIN_MAPPING_N947.md`をリポジトリ直下に新設（各ファイル末尾に相互リンク）。`README.md`冒頭のリンク一覧と「## Pin Mapping」セクションの参照を両ファイルへのリンクに更新

### コミット
`prepare0.3.0`ブランチは`git reset --soft`による履歴一本化後、単一コミット`23846ce`（"Add FRDM-MCXN947 board support"）としてpush済み（`origin/prepare0.3.0`）。以降の作業（`PIN_MAPPING.md`新設→ボード別分割等）は別途追加コミットとして重ねている。

### Serial1: FlexComm2の資源競合により見送り決定
`Serial1`実装に着手しようとしたところ、D0/D1のFlexCommとしてのalt機能はFC2_P2/FC2_P3のみで、これは`Wire`（I2C_SDA/SCL=D18/D19、`LPI2C2`＝FlexComm2）が既に専用で使っているのと同じFlexComm2インスタンスだと判明。LP_FLEXCOMMは1インスタンスにつき同時にひとつのモード（UART/I2C/SPI）しか持てないため、`Wire`と`Serial1`は同じスケッチで同時に使えない。AskUserQuestionで3択（排他利用として実装/別ピン再検討/見送り）を提示し、「Serial1は見送る」と決定。`variants/frdm_mcxn947/README.md`を「未特定・検証中」から「意図的に未対応（FlexComm2資源競合のため）」に更新

### analogRead実装（LPADC/ADC0、実機検証済み）
残りの作業候補リストから「6. analogRead/analogWrite」に着手。**重大な訂正が判明**: 以前「N947にはLPADCが存在せず`ADC0`/`ADC1`(`ADC_Type`)を持つため正しいADCドライバが未取得」と誤って判断し`fsl_lpadc.c/h`を削除していたが、これは誤りだった。version-matched SDK zip（`SDK_2_16_000_FRDM-MCXN947.zip`）を再確認したところ、`devices/MCXN947/drivers/`には`fsl_lpadc.c/h`しか存在せず、しかもその関数群（`LPADC_Init(ADC_Type *base, ...)`等）は`ADC_Type*`を引数に取ると判明。A153の`fsl_lpadc.h`も同様に`ADC_Type*`を使っており（A153の`ADC0`マクロも`(ADC_Type*)ADC0_BASE`）——NXPは両チップともこのペリフェラルの構造体型名を`ADC_Type`と呼んでいるだけで、ドライバ自体（LPADC API）は共通。「device headerに"LPADC"という文字列がない」＝「LPADCが存在しない」という以前の判断が誤りだったと確定し、`fsl_lpadc.c/h`をSDK zipから復元した

- **NXP公式のFRDM-MCXN947 LPADC pollingサンプル**（同SDK zip内）と突き合わせてA153との相違点を特定:
  1. **VREF初期化が追加で必要**: `SPC_EnableActiveModeAnalogModules(SPC0, kSPC_controlVref)` → `VREF_Init(VREF0, ...)`（LPADCのバイアス電流供給用、A153では不要だったステップ）。`fsl_vref.c/h`を同SDK zipから新規取得
  2. **FIFOインデックス引数が必要**: `FSL_FEATURE_LPADC_FIFO_COUNT`がN947では`2`（A153は`1`）のため、`LPADC_GetConvResult()`/`LPADC_DoResetFIFO()`はFIFO index引数付きの版（`LPADC_DoResetFIFO0()`、`LPADC_GetConvResult(base,&result,0U)`）が必要
  3. **A/B面の明示指定が必要**: N947のA2-A5はpin_mux.cのschematicコメントから channel_id が重複するペア構成と判明（A2=ch14/Bside, A3=ch14/Aside, A4=ch15/Bside, A5=ch15/Aside）。A153は各A-pinが別々のチャンネル番号だったため意識せずに済んでいた差異。`lpadc_conv_command_config_t.sampleChannelMode`に`kLPADC_SampleChannelSingleEndSideA`/`SideB`を明示
  4. **`port_pin_config_t`の位置指定初期化がコンパイルエラー**: A153のコードは`{val1, val2, ...}`という位置指定の集成体初期化を使っていたが、この構造体はビットフィールドで、実際に存在するフィールドは`FSL_FEATURE_PORT_HAS_*`マクロの組み合わせでチップごとに変わる（N947には`driveStrength1`フィールドが存在せず`FSL_FEATURE_PORT_HAS_DRIVE_STRENGTH1=(0)`）。フィールド名を明示する初期化（`cfg.pullSelect = ...`等）に書き換えて解消——今後同種の構造体を他チップに移植する際は位置指定初期化を避けるべき教訓
- io.hのA0/A1はN947では`DISABLED_PIN`（配線なし）と確認済み、A2-A5のみ対応
- ビルド設定: `Debug/drivers/subdir.mk`に`fsl_lpadc.c`/`fsl_vref.c`を追加、`Debug/arduino_layer/subdir.mk`に`arduino_analog.cpp`を追加。`arduino_analog.h`はA153から無変更でコピー、`arduino_analog.cpp`は新規作成（`analogRead()`は実装、`analogWrite()`は宣言のみで呼ぶと`panic()`——FlexPWMのピンマッピングという別作業が必要なため未実装であることを明示）。`Arduino.h`に`#include "arduino_analog.h"`を追加（`arduino_tone.h`は引き続き未追加）
- 確認用スケッチ: A3をLEDのBLINK回数（読み取り値/100）に変換するテストをArduino-cli経由で実機フラッシュ。**ユーザーがA3をGND/3.3Vにショートさせ、点滅回数が実際に変化することを確認** — analogRead()が実機で正しく動作することを確認済み
- ユーザーからのリマインダー: リリース前にライセンス関連のチェックを行うこと（後日実施、下記参照）

### analogRead関連の追加コードのライセンスチェック（実施済み）
上記のTODOへの対応。ユーザーから「これらのコードはどこかからのコピーだったりして、ライセンスに触れることはないか」と確認があり、`analogRead`実装で新規追加・復元した以下2種類のコードを実際に確認した:
- **ドライバファイル自体**（`fsl_vref.c/h`、復元した`fsl_lpadc.c/h`、および先行して取り込み済みの`fsl_ctimer.c/h`/`fsl_inputmux.c/h`）: 全ファイルでNXP/Freescaleのオリジナル`SPDX-License-Identifier: BSD-3-Clause`ヘッダーが無改変で残っていることを確認。`drivers/`配下・`fsl_*`接頭辞というLICENSEの既存の包括的な記述（「NXP MCUXpresso SDK由来、BSD-3-Clause」）でそのままカバーされており、追記不要と判断
- **Arduino層のコード**（`AnalogIn.cpp`/`PwmOut.cpp`のN947分岐、`arduino_tone.cpp`のクロック設定）: 実際の中身を確認したところ、ピンテーブル（`s_pins[]`等）はschematicから独自導出した独自データ、初期化の呼び出し順序はNXP公式サンプル（LPADC polling example、CTIMER example）を参照して「正しいAPI呼び出し順序・シンボル名」を確認しただけと判明。これはAPIの標準的な使用パターン（関数呼び出し順序）であって著作権保護される創作的表現のコピーではなく、コード中のコメントにも参照元サンプル名を明記済み（透明性確保）。仮に厳しく解釈しても参照元サンプル自体が同じSDK内のBSD-3-Clauseのため問題にならない
- 結論: LICENSE追記不要、問題なしと確定

### SPIの実機検証: ロジックアナライザによる最終確認完了
ユーザーが「ロジアナと定電圧源を用意した」と報告、実機検証フェーズを開始。まずSPIから着手（選択肢として analogWrite/SPI再検証/tone/analogRead精度確認をAskUserQuestionで提示し、ユーザーが直接「SPI」と指定）。

- `test_SPI_bitorder_end_transfer16.ino`をコンパイル→N947実機（`arduino-cli upload`、LinkServer経由、ポート`/dev/cu.usbmodemUENBVJCYVDM5J3`）に書き込み。arduino-cliの`board list`はA153/N947がVID/PID共通のため区別できず候補が複数出るが、`--fqbn nxp:mcx:frdm_mcxn947`を明示すれば正しいボードプロファイルで書き込める（ユーザーに接続中のボードがN947であることは口頭で確認）
- ユーザー要望で「ロジアナでの波形を見やすくするため、各テストごとにprint/printlnせず、先に全SPIトランザクションを連続実行してから結果をまとめて表示する」構成に変更。判定用の`bool`/`uint16_t`変数にいったん結果を保持し、4回のトランザクションを`Serial`呼び出しなしで連続実行、最後に`check()`をまとめて呼ぶ形にリファクタ（`examples/Arduino_compatible_API/test_SPI_bitorder_end_transfer16/test_SPI_bitorder_end_transfer16.ino`）
- MOSI(D11)-MISO(D12)ループバック配線＋ロジアナをD10(CS)/D11(MOSI)/D12(MISO)/D13(SCLK)にプローブしてキャプチャ。`transfer(0xA5)`・`transfer16(0x1234, MSBFIRST)`は素直に期待値どおりの波形（16bit転送はCSが2バイト分LOWを維持する1トランザクションとして見える）
- `transfer16(0x5678, LSBFIRST)`の波形で、ロジアナのSPIデコーダ（デフォルトMSBファースト解釈）が`0x1E`→`0x6A`という一見不可解な値を表示し、ユーザーから確認依頼。計算で説明: `0x78`（下位バイト）をビット反転すると`0x1E`、`0x56`（上位バイト）をビット反転すると`0x6A`——`LSBFIRST`時は(1)バイト内のビット順がハードウェアレベルで反転され、(2)`transfer16()`実装が下位バイトを先に送る、という2つの仕様がそのまま波形に表れていると判明。ユーザーがロジアナ側のデコーダ設定を「Bit order: LSB first」に切り替えて再キャプチャしたところ`0x78`→`0x56`と正しい値・順序で表示され、実装が正しいことを実機で最終確認
- `variants/frdm_mcxn947/README.md`の「SPI の実機検証（暫定・要再確認）」を「SPI の実機検証済み動作」に更新し、上記ロジアナ確認の詳細を追記

### Wireの実機検証: 外部LM75系センサーとの実通信確認完了
SPI検証完了後、ユーザーから次の対象として「Wire実デバイス通信。Arduino I²C端子にLM75B温度センサを接続。アドレス0x48に対して温度データを読んでこれるか」と指定。

- プロジェクトに既存のI2C温度センサー用サンプルは`P3T1755.h`（外部ライブラリ、`.gitignore`対象）に依存する高レベルAPIのみだったため、レジスタに直接アクセスする新規テストスケッチ`examples/Arduino_compatible_API/test_Wire_LM75B/test_Wire_LM75B.ino`を作成。`Wire.beginTransmission`→レジスタポインタ0x00へ`write`→`endTransmission(false)`（リピートスタート）→`requestFrom(addr, 2)`→2バイトread、という素朴な実装。LM75/P3T1755系の温度レジスタフォーマット（2バイトMSBファースト、11-bit二の補数値が16bit中の上位11bit(bit15:5)に左詰め、bit5が0.125℃刻み）に基づき、`(int16_t)((msb<<8)|lsb) >> 5`のシフト＋`×0.125`で℃に変換
- N947実機へ`arduino-cli upload --fqbn nxp:mcx:frdm_mcxn947`で書き込み（SPI検証と同じ手順、ボードは口頭確認）
- ユーザーは手元のLM75Bの代わりに**P3T1035xUK-ARD**（NXP製、同じ温度レジスタフォーマット系列）を使用、実機のアドレスに合わせてスケッチ内の定数を`0x48`→`0x72`に変更（ユーザー自身が直接編集）。この変更を尊重し、コメント・定数名（`LM75B_ADDR`→`SENSOR_ADDR`）をLM75B固定ではなく「LM75系センサー全般、テストはP3T1035xUK-ARD@0x72で実施」という表現に更新
- ロジアナでD0(SDA)/D1(SCL)相当のI2Cバスをキャプチャし、`Write[0x72]+ACK`→レジスタポインタ`0x00+ACK`→リピートスタート`Read[0x72]+ACK`→データ2バイト（最終バイトはI2Cの作法どおりNAK）という正しい読み出しシーケンスを確認。Serial出力は`raw = 0x1b70  temp = 27.375 degC`を安定して3回連続出力——室温として妥当な値
- これで`Wire`（プレーンI2C、D18/D19）はバススキャンだけでなく実デバイスとの実通信（write/read）まで実機確認完了。`variants/frdm_mcxn947/README.md`の該当セクションを更新

### analogWriteの実機検証: 物理ピン・ALT値の2件の実機バグを発見・修正して確認完了
Wire検証完了後、ユーザーから「analogWrite」を指定。「どのピンが対象になる？」との質問に対し、当初io.hに実装済みだった`PWM_0`=`P3_11`（A153と同じ物理ピン番号を流用）と回答し、確認用スケッチ`test_analogWrite_duty_N947.ino`（`PWM_0`のduty比を0/25/50/75/100%で3秒ごとに変化）を新規作成・実機書き込みまで進めたところ、ユーザーから**「P3_11はA153．N947なのでP2_2からP2_7まで？」**と根本的な疑問が提示され、これが実機バグの発見につながった。

- **実機バグ1: 物理ピンが誤り（未配線のテストポイント）**: `pin_mux.c`のラベルを確認したところ、`P3_6`-`P3_11`はいずれも`TPxx`（テストポイント）としか繋がっておらず、`J3`/`J12`のような実コネクタには出ていないと判明——コンパイル・リンクは通り、チップの端子自体には正しい波形が出ていたはずだが、基板上のどこからも触れられないピンだった。ユーザーが自ら回路図を確認して疑問を提示してくれたおかげで、実機での「無反応」を待たずに事前に気づけた形
- ユーザーが`ref/FRDM-MCXN947SH.pdf`（回路図PDF、既に`ref/`に配置済み・`.gitignore`対象）を提供。`pdftotext`でのテキスト抽出はNXPの2カラムレイアウトの回路図では信頼できないと判明（無関係な項目が同じ行に混在して読める）ため、`Read`ツールでページを直接画像として開いて視認する方式に切り替え。12ページ目の"ARDUINO SHIELD COMPATIBLE HEADERS"シートで、`P2_2`-`P2_7`が実際のPWM対応ヘッダピンであり、ペリフェラルも`FlexPWM0`ではなく`FlexPWM1`であることを確認・提示
- ユーザーから直接、物理ピンとPWM_0-PWM_5の対応指定があった: `P2_3=PWM0, P2_2=PWM1, P2_5=PWM2, P2_4=PWM3, P2_7=PWM4, P2_6=PWM5`——これは回路図のヘッダに直接印字された"PWM0"-"PWM5"のシルク印刷ラベルをそのまま反映したもので、物理ピン順にはなっていない
- `io.h`（`PWM_0`-`PWM_5`マクロ）、`PwmOut.cpp`（ピンテーブル、`PWM0`→`PWM1`インスタンス、クロック`kCLOCK_Pwm0_SmX`→`kCLOCK_Pwm1_SmX`、リセット`kPWM0_RST_SHIFT_RSTn`→`kPWM1_RST_SHIFT_RSTn`）、`PwmOut.h`（ドキュメント表・経緯コメント）を修正。`PWM1`もSDK側でペリフェラルインスタンスマクロと衝突するため（`PWM0`と同じ問題）、論理ピン名は既存の`PWM_0`-`PWM_5`のまま流用できた
- サブモジュール/チャンネル/ALT値は、まず既存の「pin_mux.cのpin_signal文字列内での位置カウント」方式（I3C1_SDAのAlt10確認で確立済み、`P3_6`/`P3_11`の旧テーブルでも検証再現できていた）で導出（`P2_2`=Alt6, `P2_3`=Alt4, `P2_4`-`P2_7`=Alt5）
- ライブラリ再ビルド（`arm-none-eabi-strip --strip-debug`込み、`.a`肥大化の教訓を最初から適用）・実機書き込みまで完了させ、ユーザーに再検証を依頼

- **実機バグ2: ALT値の誤り（`P2_2`/`P2_3`のみ）**: ロジアナを全6ピンに接続した状態で確認したところ、ユーザーから**「どのピンにも何も出てない」**と報告——`PWM_0`（修正後のはずの`P2_3`）single動作ですら無反応。位置カウント方式の結果を疑い、ローカルにクローン済みのZephyrプロジェクト内`modules/hal/nxp/dts/nxp/mcx/MCXN947VDF-pinctrl.h`（NXP公式データから生成されたシリコン正確なpinctrlヘッダ、`N9X_MUX(port,pin,mux)`マクロ形式）で全6ピンを突き合わせたところ、**全ピン共通でAlt5**という単純な答えが判明——`P2_2`のAlt6、`P2_3`のAlt4は誤りだった。原因は`P2_2`のalt-function一覧に他ピンには無い`CLKOUT`という項目が余分に挟まっており、これがマルチプレクサのスロットを消費しない特殊項目だったため位置カウントが1つずれていたこと（`P2_3`も同様の理由でずれ）。`PwmOut.cpp`のALT値を全て5に統一・`PwmOut.h`のドキュメントに教訓（「位置カウント方式は便利だが単独では信頼しきれない、可能なら権威あるpinctrlソースと突き合わせ、最終的には必ず実機確認」）を追記し、再ビルド・再書き込み
- ユーザーが再度ロジアナで確認し**「10msごとの変化とした．正常」**（`delay(3000)`を`delay(10)`にユーザー自身が編集して確認速度を上げた）と報告、`PWM_0`単体の動作を確認
- 追加でユーザーから「全チャンネルの出力と，独立性が保たれているかを検証」との依頼。新規テストスケッチ`test_analogWrite_all_channels_N947.ino`を作成——(A) 6チャンネル同時に異なる固定duty比（`PWM_0`≈10%〜`PWM_4`≈90%）を8秒間出力し全チャンネルの個別動作を確認、(B) `PWM_0`/`PWM_1`（sm2共有）・`PWM_2`/`PWM_3`（sm1共有）・`PWM_4`/`PWM_5`（sm0共有）の3ペアそれぞれで片方を50%固定しもう片方を0→25→50→75→100%で掃引し、周期共有・duty独立というFlexPWMサブモジュールの設計どおり固定側が影響を受けないことを検証する2フェーズ構成。実機書き込み後、ユーザーが**「確認できた」**と報告——全6チャンネルの出力とチャンネル間の独立性を実機で確認完了
- `PIN_MAPPING_N947.md`・`variants/frdm_mcxn947/README.md`の`analogWrite`セクションを、正しい`P2_2`-`P2_7`/`FlexPWM1`ベースの情報および2件の実機バグの経緯に全面更新

### tone/noToneの実機検証: 圧電サウンダで実音確認完了
analogWrite検証完了後、ユーザーから「tone/noTone」を指定。既存の`examples/Arduino_compatible_API/test_tone`（D13、"Twinkle Twinkle Little Star"のメロディを262Hz-440Hzの範囲で演奏、`tone()`/`noTone()`の呼び出しペアも含む）をそのままN947向けにコンパイル・実機書き込み。`tone()`はanalogWriteのような固定ピンテーブルを持たず、任意のデジタルピンを汎用GPIOトグルで駆動する完全にチップ非依存な実装（CTIMER0の周波数分周設定のみN947固有のシンボル名に対応済み）だったため、analogWriteで発生したような「ピン自体が間違っている」「ALT値が違う」という類のバグが起こりうる構造ではなく、一度の書き込みでユーザーが圧電サウンダを接続し**「圧電サウンダでメロディがなっていることを確認できた」**と実機確認完了を報告——音として実際に聞こえる形での確認のため、ロジアナのキャプチャよりもさらに直接的な実機検証となった。`variants/frdm_mcxn947/README.md`の該当セクションを実機検証済みに更新。これでN947の主要ペリフェラル（GPIO/Serial/Wire/Wire1/SPI/analogRead/analogWrite/tone・noTone）は全て実機検証済みとなり、残るのは任意項目のanalogRead精度確認のみ

### analogReadの実機検証: 定電圧源による精度確認完了
tone/noTone検証完了後、ユーザーから最後の任意項目「analogRead」を指定。新規テストスケッチ`examples/Arduino_compatible_API/test_analogRead_precision_N947/test_analogRead_precision_N947.ino`を作成——`A2`-`A5`の4チャンネルを毎ループ読み取り、10bit生値と`raw*3.3/1023`で計算した電圧をSerialに出力する単純な実装。ユーザーが`A2`（`P0_14`）に定電圧源を接続できることを確認したうえで実機書き込み。

- ユーザーが`A2`-`A5`の4チャンネル全てに既知の電圧（0-3.3V範囲）を順に与え、**「それぞれに与えた電圧を正確に測定できた」**と報告——全チャンネルの精度を確認完了
- `variants/frdm_mcxn947/README.md`の`analogRead`セクションに、当初のGND/3.3Vショート確認（A3のみ、変化の有無だけ確認）に加えて、この定電圧源による4チャンネル精度確認（実際の電圧値との一致）を追記

### 全GPIOピンの出力確認と`pinMode()`のバグ発見・修正
analogRead精度確認完了後、ユーザーから「サポートしてる全てのGPIOで出力が出ることを確認したい」との依頼。`D0`-`D19`（16本、A0/A1は`DISABLED_PIN`のため対象外）を1本ずつ200msのHIGHパルスで順番に光らせる「歩くビット」パターンのテストスケッチ`test_digitalWrite_all_pins_N947`を新規作成・実機確認——**正常動作**。

- ユーザーから「MicroBusのピンはどう？A153ではサポートしてなかった？」と追加の指摘。調査したところ、`io.h`/`arduino_io.h`に`MB_AN`/`MB_RST`/`MB_CS`/`MB_SCK`/`MB_MISO`/`MB_MOSI`/`MB_PWM`/`MB_INT`/`MB_RX`/`MB_TX`/`MB_SCL`/`MB_SDA`というMikroBusピンマクロがA153・N947両方に既に存在していたが、**どちらのボードでも一度もテスト・ドキュメント化されていない**（`PIN_MAPPING_*.md`未掲載）ことが判明。`pin_mux.c`で実配線を確認したところ（`P3_6`-`P3_11`のときのようなテストポイントではなく）`J5`/`J6`（Mikro Busコネクタ）に実際に配線されていることを確認——回路図の"Mikro Bus"セクションのラベルとも一致。ユーザー承認のうえ、同じ「歩くビット」パターンのテストスケッチ`test_digitalWrite_mikrobus_pins_N947`（`MB_AN`除く11本）を新規作成・実機確認
- **実機バグ発見: `MB_RX`/`MB_TX`だけ出力が出ない**: 全11本のうち`MB_RX`/`MB_TX`（`P1_16`/`P1_17`）だけロジアナに何も出ないとユーザーから報告。ユーザー自身が「ああ、I2C(I3C)になってるのか」とほぼ即座に原因を言い当てた——`pin_mux.c`の`BOARD_InitPins()`がオンボードP3T1755センサー用にこの2ピンを起動時にALT10（I3C1_SDA/SCL）へ固定しており、`DigitalInOut`の`pinMode()`実装がPORT MUXフィールドを一切触らず（ピンが既にGPIOである前提でGPIOレジスタのみ操作）、ALT10のまま残っていたことが根本原因
- ユーザーから「D18, D19がI²CとGPIOで切り替えれるように，MB_RX, MB_TXも切り替えできるように」と修正依頼。調査の結果、`I2C`/`I3C`クラスの`begin()`は`pin_mux()`を明示的に呼んでALT変更する一方、GPIO側に戻す経路がコード上どこにも存在しないと判明——`D18`/`D19`も含め全ピンが本来同じ欠陥を抱えていたが、他のピンはブート時デフォルトが偶然ALT0（GPIO）だったため今まで表面化していなかっただけ、と特定
- `arduino_layer/arduino_io.cpp`の`pinMode()`を修正——新規ピン生成時・既存ピン再設定時のどちらでも`->pin_mux( 0 )`（ALT0=GPIO）を明示的に呼ぶよう変更。これにより`pinMode()`は直前にどの周辺機能で使われていたかに関わらず、確実にGPIOとして再取得するようになった。ライブラリ再ビルド（strip込み）・関連スケッチの回帰コンパイル確認（`onboard_temperature_sensor`は外部ライブラリ`P3T1755.h`未インストールによる無関係な既存エラーのみ、回帰なしと確認）・実機書き込み
- **双方向切り替えの実機確認完了**: ユーザーが`onboard_temperature_sensor.ino`（`Wire1`経由でI3Cモードのオンボードセンサーにアクセス）→`test_digitalWrite_mikrobus_pins_N947.ino`（同じ`MB_RX`/`MB_TX`をプレーンGPIOとして駆動）の順に実機で連続実行し、**「GPIOとI²Cのどちらでも切り替えて動かせることを確認できた」**と報告
- 最後にユーザーから「A2〜A5は使える？」との質問。`arduino_io.h`でA2-A5がD-pin/MikroBusピンと同じenumに属することを確認して「使える」と回答、同じ「歩くビット」パターンのテストスケッチ`test_digitalWrite_analog_pins_N947`を新規作成・実機確認——**正常動作**
- `PIN_MAPPING_N947.md`にMikroBusピン表を新規追加、`variants/frdm_mcxn947/README.md`に「全GPIOピンの出力確認と`pinMode()`のバグ修正」セクションを新設し、上記の経緯・バグ・修正内容を記録

### MikroBusのSPI/I2C/UART追加: `Wire2`/`SPI1`/`Serial1`
全GPIO出力確認完了後、ユーザーから「MikroBus上のSPIとI²C(MB_SCL/MB_SDA)を使えるようにできる？」と依頼。調査の結果、`I2C`/`SPI`クラスのコンストラクタは渡されたピンに関わらず`unit_base`（実際に叩くLPI2C/LPSPIレジスタ）がコンパイル時に単一マクロへ固定されている作りで（`Wire`は常に`LPI2C2`、`SPI`は常に`LPSPI1`）、そのままMikroBusピンを渡してもMUXだけ切り替わり中身は既存ペリフェラルのままという不整合になると判明。`i2c.cpp`/`r01lib_spi.cpp`を確認したところ、**A156向けの分岐には既にMikroBus対応コードが存在していた**（N947だけ未実装）ため、既存パターンを移植する方針でユーザー承認のうえ着手。

- ピン・ペリフェラルの特定はZephyrの`MCXN947VDF-pinctrl.h`を最初から採用（位置カウント方式の過去の失敗を教訓化）——`MB_SDA`(`P1_0`)/`MB_SCL`(`P1_1`)は`FlexComm3`(`LPI2C3`)・Alt2、`MB_MOSI`/`MB_MISO`/`MB_SCK`/`MB_CS`(`P3_20`/`22`/`21`/`23`)は`FlexComm6`(`LPSPI6`)・Alt3、4ピンとも統一
- `mcu.cpp`にFlexComm3/6のクロック供給を追加、`i2c.cpp`の`I2C`コンストラクタに`MB_SDA`/`MB_SCL`分岐、`r01lib_spi.cpp`の`SPI`コンストラクタに`MB_MOSI`/`MB_MISO`/`MB_SCK`/`MB_CS`分岐を追加（`RESET_ReleasePeripheralReset`は`kFC3_RST_SHIFT_RSTn`/`kFC6_RST_SHIFT_RSTn`——FlexCommインターフェース単位のリセットシンボル、LPI2Cn/LPSPIn単位の専用シンボルは存在しないと判明）
- **`SPIClass`の構造変更**: 従来引数なしの単一インスタンス固定（`D10`-`D13`決め打ち、内部の`r01libSPI*`もファイルstatic変数で全インスタンス共有）だったのを、`TwoWire`と同じ設計（コンストラクタでピンを受け取る、`spi`ポインタもインスタンスメンバ化）に変更。既存の`SPI`（デフォルト引数で後方互換）はそのまま、新規`SPI1(MB_MOSI, MB_MISO, MB_SCK, MB_CS)`を追加。`TwoWire`側は元々コンストラクタがピンを受け取る設計だったため、`Wire2(MB_SDA, MB_SCL)`をそのまま追加するだけで済んだ
- ライブラリ再ビルド・回帰コンパイル確認（無問題）を経て、新規テストスケッチ`test_Wire2_MikroBus_N947`（バススキャン）・`test_SPI1_MikroBus_N947`（`MB_MOSI`-`MB_MISO`ループバック）を作成・実機確認——**両方とも実機確認済み**（ロジアナ波形・Serial出力とも問題なし）。ユーザーが独自にGPIOへの切り替えも試し、`pinMode()`修正が新ペリフェラルにも一貫して効くことを確認

### `Serial1`をMikroBusヘッダ（`MB_TX`/`MB_RX`）向けに追加
ユーザーから「MB_RX/MB_TXをSerial1にできる？」と質問。D0/D1の`Serial1`はFlexComm2競合で既に見送り済みだったが、`MB_RX`/`MB_TX`は事情が異なると判明——`pin_mux.c`のalt-function一覧に`FC5_P0`/`FC5_P1`（`FlexComm5`、Zephyrのpinctrlヘッダで両ピンAlt2と確認）という、`Wire2`が使う`FlexComm3`とも`I3C1`（専用ペリフェラルでFlexCommを消費しない）とも別の**未使用のFlexCommインスタンス**が使えることを発見。つまりこの2物理ピンはGPIO・`Wire1`(I3C1)・`Serial1`(FlexComm5/LPUART5)の3用途を排他的に切り替えて使える、D0/D1とは根本的に異なる状況だと判明し、ユーザー承認のうえ実装に着手。

- `mcu.cpp`にFlexComm5クロック分周設定を追加。`Serial.cpp`の`s_pinMap[]`（従来USBTX/USBRX→LPUART4の1エントリのみ）に`MB_TX`/`MB_RX`→`LPUART5`のエントリと`LP_FLEXCOMM5_IRQHandler`を追加。`arduino_serial.cpp`/`.h`に、以前D0/D1向けに一度削除した`Serial1`グローバルインスタンスを、今度は`MB_TX`/`MB_RX`向けとして復活
- **実機バグ発見・修正: SOSパニック**: 実装直後の実機フラッシュでSOSモールス信号のLED点滅が発生——過去のA153移植時のD0/D1バグと全く同じ症状。原因を調査したところ、`arduino_serial.cpp`が`arduino_io.h`をincludeしており、`arduino_io.h`は`MB_TX`/`MB_RX`をArduinoピン番号リナンバリングの対象に含んでいた（`#undef`後に小さな連番の`enum`値として再定義）ため、`SerialClass Serial1( MB_TX, MB_RX )`宣言の時点で`MB_TX`/`MB_RX`が生のr01lib物理ピン値ではなく再番号化された値にすり替わっていたと判明。`Serial.cpp`の`s_pinMap[]`は生のr01lib値と比較する作りのため不一致となり、`Serial::resolve_pins()`が`_base`を`nullptr`のままにして`panic()`が`static`初期化時（`setup()`実行前）に発火していた。`arduino_io.h`のinclude・リナンバリングが効く**前**に`constexpr int SERIAL1_TX_PIN = MB_TX;`等で生の値を退避し、`Serial1`の構築にはその退避値を使うよう修正
- ライブラリ再ビルド・回帰コンパイル確認（無問題）を経て再フラッシュ、新規テストスケッチ`test_Serial1_MikroBus_N947`（`MB_TX`-`MB_RX`ループバック）で**実機確認済み**（波形・Serial出力とも問題なし、とユーザー報告）
- **3モード排他切り替えの実機確認完了**: ユーザーが`onboard_temperature_sensor`（`Wire1`でI3Cアクセス）→`test_digitalWrite_mikrobus_pins_N947`（プレーンGPIO）→`test_Serial1_MikroBus_N947`（UART）→再度GPIO、の順に実機で連続実行し、同じ2物理ピンがI3C・GPIO・UARTの3モードを正しく切り替えられることを確認
- `PIN_MAPPING_N947.md`・`variants/frdm_mcxn947/README.md`を更新（セクション名を「MikroBusのSPI/I2C/UART」に拡張、D0/D1の`Serial1`見送り注記にもMikroBus側で実現した旨を追記）

### GitHub Issue対応: #1「標準MOSI/MISO/SCKマクロが未定義」
ユーザーから「githubに来ているIssueを確認して」と依頼。`gh issue list`で2件のオープンIssue（両方ともリポジトリオーナー本人が、別プロジェクト`Waveshare_TFT_Touch_Arduino`のCI設定中に発見・報告したもの）を確認・要約して提示し、ユーザーが「#1に対応する」と指定。

- **Issue #1の内容**: 各ボードの`arduino_io.h`は`SPI_MOSI`/`ARD_MOSI`/`MB_MOSI`等の接頭辞付き名前しか定義しておらず、AVR/SAMD/ESP32/Renesas UNO R4など他の全Arduinoコアが標準で提供する裸の`MOSI`/`MISO`/`SCK`が無いため、それらを直接参照するサードパーティライブラリ（例: 公式`SD`ライブラリ）のコンパイルが`'MOSI' was not declared in this scope`等で失敗する、という報告
- **対応**: A153・N947（このリポジトリで実際に配布している2ボードのみ、A156/N236/C444はまだ未配布のため対象外）の`arduino_io.h`に、`ARDUINO_PIN_RENUMBERING`のenum定義が閉じた直後（`LED_BUILTIN`と同様の「既存ピンへのエイリアス」パターン）に`#define MOSI ARD_MOSI` / `#define MISO ARD_MISO` / `#define SCK ARD_SCK`を追加。`ARD_MOSI`等は既に`SPI`グローバルインスタンスが使うデフォルトSPIピン（A153/N947ともD11/D12/D13）なので、素直なエイリアスとして機能する
- 両ボードのライブラリを再ビルド・再配置。**ついでにA153の配布用`.a`も初めてstrip-debugした**（従来49.5MBのunstrippedのままだったが、GitHub制限内で問題にならなかったため手つかずだった。今回コード変更に伴いどのみち差し替えが必要だったため、N947で確立した教訓を適用し440KBまで圧縮）
- 実際にIssue記載の再現手順（`arduino-cli lib install SD`→`#include <SD.h>`するスケッチをビルド）で検証——**両ボードとも`MOSI`/`MISO`/`SCK`関連のエラーは解消したことを確認**。ただし`SD`ライブラリ自体は`setWriteError`/`clearWriteError`/`getWriteError`という別の未実装API（`Print`/`Stream`周りのギャップ、Issue #1の範囲外）でコンパイルが通らない状態は残る——これは新規Issueとして別途起票すべき事項だが、今回はスコープ外として深追いせず
- 確認用スケッチ`test_MOSI_MISO_SCK_macros`（`MOSI==ARD_MOSI`等を検証、配線不要）を新規作成、両ボードでコンパイル確認。`API_COMPATIBILITY.md`のSPIセクション・`CHANGELOG.md`のUnreleasedセクションに追記

### N947版`test_combined_peripherals`の新規作成
ユーザーから「N947版のtest_combined_peripheralsを作って」と依頼。既存のA153版（`Wire1`のI3Cセンサー・`analogRead`・`analogWrite`・`tone`・`millis`/`micros`・`Serial1`ループバックを1ループ内で同時に回す最終統合確認スケッチ）を参考に、`examples/Arduino_compatible_API/test_combined_peripherals_N947`を新規作成。

- **A153版との構成上の違い**: `Serial1`を含めなかった。N947では`Serial1`（`MB_TX`/`MB_RX`）が`Wire1`のI3Cと全く同じ物理ピンを共有しており（MikroBusのSPI/I2C/UART追加作業で確立済みの制約）、両方を同一ループ内で同時使用するテストは「独立ペリフェラルの同時ストレステスト」にならないため除外——コメントで理由を明記し、代わりに`Wire2`/`SPI1`（こちらは物理的に独立、競合なし）を将来追加する余地があることも記載
- ピン名の差し替え: `PWM0`→`PWM_0`、`ADC_PIN`を`A0`→`A2`（N947はA0/A1非対応）
- 依存する`P3T1755.h`（外部ライブラリ、この開発環境には未インストール）のためこちらの環境ではコンパイル確認不可——ユーザー環境でのビルド・実機確認を依頼し、**「動作してる」と実機確認完了の報告あり**

### A153のMikroBus対応: `SPI1`とGPIO（実機検証済み）
N947のMikroBus対応が一段落したところで、ユーザーから「A153のMikroBus対応も行う」と依頼。着手前にA153の物理ペリフェラル構成を調査したところ、N947とは大きく事情が異なると判明。

- **調査結果**: A153のデバイスヘッダを確認したところ、`LPI2C0`が**チップ全体で1系統のみ**（独立した`Wire2`は物理的に不可能——MikroBusの`MB_SDA`/`MB_SCL`もこの唯一のI2Cペリフェラルへの別ピンルートに過ぎない）。UARTは3系統（`LPUART0`-`2`）あるが、既存の`Serial1`（D0/D1）が`Serial.cpp`の`s_pinMap[]`で既に`LPUART2`を使用しており、Zephyrの`MCXA153VLH-pinctrl.h`で確認したところMikroBusの`MB_TX`/`MB_RX`も同じ`LPUART2`にしか接続されていない（独立した新規`Serial1`も不可能）。SPIのみ`LPSPI0`/`LPSPI1`の2系統があり、`SPI`（Arduinoヘッダ）は`LPSPI1`のみ使用中——**未使用の`LPSPI0`を使った本物の独立`SPI1`は実現可能**と判明。この分析結果をユーザーに提示し、「SPI1とGPIOだけ対応にする」との回答を得た
- **実装**: `arduino_io.cpp`の`pinMode()`にN947と同じALT0再取得の修正を適用（A153ではI3Cのような起動時ALT10固定ピンは無いため未発覚の潜在バグだったが、予防的に適用）。`r01lib_spi.cpp`の`SPI`コンストラクタに`MB_MOSI`/`MB_MISO`/`MB_SCK`/`MB_CS`分岐を追加（`unit_base=LPSPI0`）。ALT値はZephyrのpinctrlヘッダで確認したところ、MikroBus用`LPSPI0`・既存`SPI`用`LPSPI1`ともに偶然Alt2で一致していたため分岐不要だった。`arduino_spi.h`/`.cpp`の`SPIClass`もN947と同じ構造変更（コンストラクタでピンを受け取る、`spi`ポインタをインスタンスメンバ化）を適用し、新規`SPI1(MB_MOSI, MB_MISO, MB_SCK, MB_CS)`を追加
- **実機バグ発見・修正: `LPSPI0`にクロックが供給されていなかった**: 確認用スケッチ`test_SPI1_MikroBus_A153`（`test_SPI1_MikroBus_N947`と同構成）を実機フラッシュしたところ、ユーザーから「CS以外出てない」と報告。`mcu.cpp`を確認したところ、既存の`SPI`用`LPSPI1`にはクロック分周・アタッチの設定があったが、新規`LPSPI0`向けの設定が丸ごと漏れていた——`LPSPI0`は元々このコードベースで一度も使われたことがないペリフェラルだったため、誰も配線したことがなかった。`CLOCK_SetClockDiv(kCLOCK_DivLPSPI0,1u)`＋`CLOCK_AttachClk(kFRO12M_to_LPSPI0)`を追加して解消。ライブラリ再ビルド・回帰コンパイル確認（無問題）を経て再フラッシュ、ユーザーが**「ループバック，波形ともにOK」**と実機確認完了
- GPIO側は`test_digitalWrite_mikrobus_pins_A153`（全12ピン、`MB_AN`はN947と違い`DISABLED_PIN`ではなく実ピンのため含む）で実機確認済み——**「完璧」**とユーザー報告
- `PIN_MAPPING_A153.md`・`variants/frdm_mcxa153/README.md`を更新（MikroBusピン表・`SPI1`の技術詳細・Wire2/Serial1が物理的に不可能である理由を記録）
- ユーザーから「test_combined_peripheralsを更新してtest_combined_peripherals_A153とする」と依頼。既存の`test_combined_peripherals`（N947版作成時にA153版の存在に気づき参考にしていたもの）を`test_combined_peripherals_A153`へリネームし、新規`SPI1`（`MB_MOSI`-`MB_MISO`ループバック、`transfer16()`を毎ループ実行）を追加——`Wire1`(I3C0)・`Serial1`(LPUART2)・`analogRead`(ADC)・`analogWrite`(FlexPWM0)・`tone`(CTIMER0)のどれとも衝突しない独立ペリフェラルのため、既存のストレステストにそのまま統合できた。`P3T1755.h`依存のためこの環境では直接コンパイルできず、一時的にセンサー呼び出しをスタブ化したスクラッチコピーで構文面のみ確認（本番ファイルは変更せず）。ユーザー環境でのビルド・実機確認を依頼し「OK」の報告あり
- コミット時、複数ファイルを1回の`git add`にまとめた際、存在しないパスが1つ混じっていたためコマンド全体が失敗し、他の正当なファイルも巻き込まれて未ステージのまま最初のコミット（`2cb3783`）に含まれていなかったことが判明（push後に`git status`で発覚）。フォローアップコミット（`3e9d6e1`）で残りのファイルを追加して解消——教訓として、`git add`に複数パスを渡す際は個々の存在を事前確認するか、コミット直後に`git status`で漏れがないか確認する習慣が必要

### GitHub Issue対応: #3「Printクラスにwrite error追跡APIが無い」
Issue #1対応時に見つかっていた`SD`ライブラリの別の未解決エラー（`setWriteError`/`clearWriteError`/`getWriteError`）について、ユーザーが正式にIssueとして起票し（本人がIssue #1と同じ経緯・別プロジェクトのCI設定中に発見）、「issue #3に対応する」と依頼。

- **Issue #3の内容**: `Print`クラスに標準Arduino APIの`setWriteError()`（`protected`、派生クラスが内部で呼ぶ）・`getWriteError()`/`clearWriteError()`（`public`）が無く、これらを使う`SD`ライブラリの`SdFile`/`File`クラスがコンパイルできない、という報告。Issue本文に実装方針まで具体的に記載されていた（`private: int write_error = 0;` / `protected: void setWriteError(int err=1)` / `public: int getWriteError()` / `void clearWriteError()`）
- **対応**: A153・N947両方の`Print.h`（`arduino_layer/`と`hardware/variants/*/include/`の計4箇所）に、Issue記載のシグネチャ・アクセス指定子どおりに追加。`getWriteError()`/`clearWriteError()`は既存の`availableForWrite()`の直後（`public`セクション）に、`setWriteError()`は既存の`private:`セクション直前に新規`protected:`セクションとして追加し、`_write_error`メンバは既存の`private:`ヘルパー群と一緒に配置
- ライブラリ再ビルド・両ボードの回帰コンパイル確認（無問題）を経て、実際にIssue記載の再現手順（`SD`ライブラリをインストールし`#include <SD.h>`するスケッチをビルド）で検証——**両ボードとも完全にコンパイル成功**（`-Waddress-of-packed-member`警告のみ残るが、Issue本文に「これは無関係・無害なので無視してよい」と明記済みのもの）。これでIssue #1・#3両方が原因だった`SD`ライブラリのビルド不能が解消
- 確認用スケッチ`test_Print_writeError`（`FlakyPrint`という`Print`派生クラスを自作し、`setWriteError()`呼び出し前後で`getWriteError()`/`clearWriteError()`が正しく追跡・リセットされるかを検証、配線不要）を新規作成、A153実機で4項目とも「全てOK」の実機確認済み。`API_COMPATIBILITY.md`・`CHANGELOG.md`のUnreleasedセクションに追記

### 実プロジェクト（Waveshare_TFT_Touch_Arduino）での動作確認と、実機バグ2件・Issue #2の発見・修正
Issue #1・#3が対応済みになったことを受け、これらのIssueの発端となった実プロジェクト`Waveshare_TFT_Touch_Arduino`（ユーザー自身のリポジトリ、ローカルに`~/dev/Arduino/libraries/Waveshare_TFT_Touch/`としてクローン済み）で実際にexampleがコンパイル・動作するかを検証する依頼。当初`examples/SDBitmapViewer/`のみを指定されたが、ユーザー自身が「間違い」と訂正し、ライブラリ全体（親ディレクトリ）を指定し直した。`arduino-cli compile --library <path> ...`（サンプルが標準ライブラリ検索パス外にある場合の正しい指定方法、`upload`単体では`--library`フラグが存在せず`compile --upload`と組み合わせる必要があると判明）で`SDBitmapViewer`（LCD+SDカード、SPIバス共有・別CS）が両ボードともコンパイル成功——Issue #1・#3が実際にこのプロジェクトのビルド不能を解消したことを確認

#### 実機バグ発見1: LCD画像の乱れ（SPIのCSピンがハードウェアPCS機能に強制mux）
ユーザーが実機（A153）で`SDBitmapViewer`を実行したところ、「R3, Minimaでは正常に表示できたbmpファイルの画像が乱れて表示される．しかも描画が非常に遅い」と報告（写真添付、NXPロゴが斜めの帯状ノイズで乱れる）。ユーザーから「以前の検討ではissue #2は描画速度との直接の関係はなさそうという結論になった．そのため保留にしてある．それよりも画像の乱れを優先してfix」との明確な優先順位指示。

- **切り分け**: SDの読み取り(`f.read`)とLCDへの書き込み(`tft.startWrite/writePixels/endWrite`)を完全に分離した診断スケッチ（Phase A: SD読み取りのみでチェックサム比較、Phase B: `tft.*`を使った合成カラーバーパターンのみ）を作成し実機確認。Phase Aは2回とも同一チェックサムで安定・SHORT READ皆無（SD読み取り自体は正しい）、Phase Bはカラーバーが正しく表示（LCD書き込み自体も正しい）——にも関わらず**Phase A（`tft.*`を一切呼んでいない）実行中に画面に斜めの黒い点が走る**という一見矛盾した結果が得られ、これが決定的な手がかりとなった
- **根本原因特定**: `r01lib_spi.cpp`の`SPI`コンストラクタが、渡された`cs`引数のピンを無条件で`_cs.pin_mux(2)`によりLPSPIハードウェアのPCS（Peripheral Chip Select）機能へmuxしていた。これにより(1) `ST7789`ライブラリが`pinMode`/`digitalWrite`で行う手動CS制御が`SPI.begin()`実行後は完全に無効化される（ピンがGPIOでなくなるため）、(2) **さらに深刻なのは、同じSPIバスを共有する別デバイス（SDカード、CS=D5）の`SPI.transfer()`呼び出しのたびに、LPSPIハードウェアがLCDのCS(D10)を自動でパルスしていた**ため、SD向けの意味のないデータがLCDのコマンド/データレジスタに誤って取り込まれていた。`chip_select`メンバをコンストラクタ内で確認したところ、N947側には既に未使用の`cs_manual_control()`という手動CS切り替えメソッドが存在していた（A153には存在せず）ことも判明——おそらく過去のIssue #2調査時の未完成の対策と推測
- **修正**: A153・N947とも、コンストラクタでCSピンをLPSPIハードウェアPCS機能へmuxするのをやめ、常にGPIO（スケッチ側の`digitalWrite`制御下）のままにするよう変更。これは本家Arduinoの標準的なSPI流儀（CSは常にスケッチ側の責任）と一致する。N947は既存の`cs_manual_control(true)`をコンストラクタから呼ぶ形に、A153は該当ピンのmux呼び出し自体を削除する形で対応。両ボードともライブラリ再ビルド・strip・配置、回帰コンパイル、実プロジェクトの`SDBitmapViewer`のコンパイル確認を経て、実機A153に書き込み直し——**ユーザーが画像乱れの解消を確認**（「画像の乱れは解消された．でもまだ遅い」）

#### Issue #2の修正（画像の乱れとは無関係と判明も、Issueとしては正当な修正）
画像乱れ解消後、ユーザーから「まだ遅い．Issue #2の可能性が高いのでこれもfix」との依頼。`gh issue view 2`で内容確認: `SPI::frequency()`/`mode()`が設定変更のたびに`LPSPI_Deinit()`（クロックゲート停止＋全レジスタリセット）+`LPSPI_MasterInit()`（CFGR1・FIFOウォーターマーク・ダミーデータまで含めた全再構築）という重い処理を行っており、2台のSPIデバイスが毎ループ異なる`SPISettings`で交互動作する構成（例: タッチ+LCD）で無駄なコストを払うという指摘

- **修正**: NXP SDK（`fsl_lpspi.c`）のソースを精査し、`LPSPI_MasterInit()`内部で実際に何が行われているかを確認。`LPSPI_MasterSetBaudRate()`（ボーレート、モジュール無効化のみ必要）・TCRレジスタへの直接書き込み（CPOL/CPHA/LSBF/PRESCALE、`LPSPI_MasterTransferBlocking()`自身がCONT/CONTC/RXMSK/PCS/WIDTH以外のTCRフィールドをtransfer毎に触らないことを確認済み）・`LPSPI_MasterSetDelayTimes()`を使い、「無効化→変更が必要なレジスタだけ直接書き換え→再有効化」という軽量な方式に`frequency()`/`mode()`/`bit_order()`を置き換え（`LPSPI_Deinit()`/`LPSPI_MasterInit()`を使わない）。両ボードとも`-Wall`警告ゼロでビルド確認、実機書き込みして確認を依頼したが、**ユーザーから「速度は変わらない」と報告**——Issue #2のメカニズム自体は正しく修正されたが、このワークロード（`SDBitmapViewer`）における実際のボトルネックではなかったと判明

#### 実機バグ発見2: SDライブラリのスカラーバイト転送が真のボトルネックと判明
ユーザーから追加情報「R3で3秒の描画にA153では11秒」（約3.7倍遅い）。単発の再初期化コストでは説明しきれない規模のため、実際の`drawBmp()`と同じチャンク単位でSD読み取り/LCD書き込みを別々に`micros()`計測する診断スケッチを新規作成・実機書き込み。結果:
```
chunkCount=2400  totalUs=10467071
sdTotalUs=8998921 (85.9%)  avg SD read per chunk(us)=3749
lcdTotalUs=1280173 (12.2%)  avg LCD write per chunk(us)=533
```
SD読み取りが全体の85.9%を占め、96バイトのチャンク読み取りに平均3749µsもかかっている（同程度のバイト数を送るLCD書き込みの7倍以上）ことが判明。

- **原因**: 標準`SD`ライブラリの`spiRec()`は`SPI.transfer(0xFF)`という**1バイトずつのスカラー転送**をループで呼ぶ実装（`Sd2Card.cpp`で確認）。このプロジェクトの`SPIClass::transfer(uint8_t)`は、LCDの一括転送（`SPI.transfer(buf,size)`）と全く同じ`spi->write()`→`LPSPI_MasterTransferBlocking()`という重い汎用パスを1バイトのためだけに通っていた。`fsl_lpspi.c`のソースを精査したところ、この関数は毎回「無効化→引数チェック→FIFOフラッシュ→ステータスフラグクリア→再有効化→TCR書き込み(TX FIFO空待ち込み)→もう一度TCR書き込み→FIFO経由でのバイト転送」という、大量バイト・複雑な転送を安全に扱うための防御的な処理を1バイトのためだけに毎回フルで行っており、実際のビットクロック時間（4MHzで約2µs）に対し約37µsもの固定オーバーヘッドが乗っていた
- **修正**: `SPI`クラスに`transfer_byte(uint8_t)`という軽量パスを新設（A153/N947両方の`r01lib_spi.h/.cpp`）。TCRの動的フィールド（PCS/CONT/CONTC/RXMSK/TXMSK）を「単発・非continuous・TX+RX有効」の固定値へ都度書き込み（FIFO空待ち不要——このパスでは呼び出し間で必ずモジュールがアイドル状態に戻っていることを利用）、あとはSDKの`LPSPI_WriteData()`/`LPSPI_ReadData()`とステータスフラグ（`kLPSPI_TxDataRequestFlag`/`kLPSPI_RxDataReadyFlag`、SDK自身が内部で待っているのと同じフラグ）を直接ポーリングしてTDR/RDRを操作するだけの実装に置き換え、`LPSPI_MasterTransferBlocking()`の防御的な毎回リセットを完全にバイパス。CPOL/CPHA/PRESCALE/LSBF/FRAMESZは`frequency()`/`mode()`/`bit_order()`が既に正しく維持している値をそのまま使うため触れない。`SPIClass::transfer(uint8_t)`（Arduino層）をこの新パスに接続
- 両ボードとも`-Wall`警告ゼロでビルド確認、回帰コンパイル問題なし。診断スケッチを再書き込みしてユーザーが確認、**「良くなりました」**と改善を確認（正確な計測値・R3比の最終確認は今後のフォローアップ）

### ボード表示名の変更、ピン配置図の追加、リリース前チェック
- **ボード表示名の変更**: Arduino IDEのボード選択ダイアログで両ボードとも`(NXP Cortex-M33)`と表示され、Zephyr版など同名ボードを提供する他のパッケージと区別が付かないという指摘。`boards.txt`の`frdm_mcxa153.name`/`frdm_mcxn947.name`を`FRDM-MCXA153 (mcx-arduino-core)`/`FRDM-MCXN947 (mcx-arduino-core)`に変更（`arduino-cli board listall`で反映確認済み）。`TUTORIAL.md`/`.ja.md`のボード選択手順の記載も追従
- **ピン配置図の追加**: ユーザーがN947向けの手書きピン配置図（`img/pins-FRDM-MCXN947.png`）を新規作成、`PIN_MAPPING_N947.md`から参照。A153の既存図（`img/pins-FRDM-MCXA153.png`、単一ファイル）もArduinoヘッダ用（`-ard.png`）とMikroBusヘッダ用（`-mb.png`）の2枚に分割し、`PIN_MAPPING_A153.md`が両方参照、`TUTORIAL.md`/`.ja.md`はArduinoヘッダ図のみ参照。README.mdの単一図埋め込みは削除（各ボードの図は各`PIN_MAPPING_*.md`側に集約する方針に統一）。ユーザーが図の中身を確認・修正した後、`PIN_MAPPING_N947.md`のalt text誤り（`![pins-FRDM-MCXA153](img/pins-FRDM-MCXN947.png)`——N947の画像なのにalt textがA153のままだった）を発見・修正
- **チュートリアルにボード限定の注記を追加**: N947対応が増えたことで「このチュートリアルはどのボード向けか」が曖昧になりうるため、`TUTORIAL.md`/`.ja.md`冒頭に「このチュートリアルの例はFRDM-MCXA153を使用しています」という注記を追加
- **リリース前チェック依頼への対応**: ユーザーから「リリース前に確認しておくことは？」と依頼を受け、作業ツリーの未コミット差分（なし）・`.DS_Store`混入（なし）・`ref/`のgitignore状態（正常）・GitHub Issue #1/#2/#3のopen状態（方針通り）・全50サンプルスケッチの両ボード回帰コンパイルを実施。既知の制約（外部ライブラリ`P3T1755.h`未インストール、6本）を除き、**3本のサンプル（`test_Analog_read_write`/`test_PWM_pin_identify`/`test_analog_resolution_and_misc`）がN947でのみコンパイルエラー**になることを新規発見——これらはA153向けの古い汎用サンプルで、素の`PWM0`-`PWM5`（アンダースコアなし）を使っており、当時N947側は名前衝突回避のため`PWM_0`-`PWM_5`（アンダースコア付き）という別名にしていたため
- また、git追跡外のローカル空ディレクトリ`examples/Arduino_compatible_API/test_String_reserve_getBytes/`（`.ino`ファイルが存在しない）も発見したが、リポジトリに一切影響しないためユーザー判断で放置可とした

### N947の`analogWrite`ピン名を`PWM_0`-`PWM_5`から`PWM0`-`PWM5`に統一（A153と共通化）
上記チェックで見つかったPWM0/PWM_0問題について、ユーザーから「マクロは共通でPWM0が使えるようにしたい」と依頼。「A153ではどう解決したか」との質問に対し、**A153はこの問題自体が発生していない**（A153のSDKはFlexPWMインスタンス名を`FLEXPWM0`という別文字列で定義しており、`PWM0`というマクロ自体を定義しないため、最初から素の`PWM0`-`PWM5`が自由に使えていた）と回答。N947だけがSDK自体が`PWM0`/`PWM1`をFlexPWMペリフェラルインスタンス（`(PWM_Type*)PWM0_BASE`等）として定義しているため、本物の名前衝突が存在すると説明。

- **既存の前例を発見**: `source/r01device/led/LEDDriver.h`（別のLEDドライバコード、PCA995x系）が、全く同じ「SDKの`PWM0`と自前の`PWM0`（LED enum用）が衝突する」問題を`#undef PWM0`で解決していることを発見——同じ技法をio.hに適用できると判断
- **PWM1固有の制約**: `PwmOut.cpp`のN947実装は、SDKの`PWM1`（`FlexPWM1`インスタンスへのポインタ）をドライバ呼び出し内で直接使用（`PWM_Init(PWM1,...)`等、計9箇所）。単純に`io.h`で`PWM1`を`#undef`+再定義すると、同じ翻訳単位内で`PwmOut.cpp`自身がSDKの意味を失いビルドが壊れる。`PWM2`-`PWM5`はSDKが定義していないため無条件に安全、`PWM0`はSDKの意味をどこも使っていないため安全、`PWM1`だけがこの制約を受けると特定
- **実装**:
  - `io.h`: `PWM0`/`PWM1`を`#undef`してから、6ピン全てを`PWM0`-`PWM5`（アンダースコアなし）として再定義
  - `PwmOut.cpp`（N947分岐）: `#include "PwmOut.h"`（io.hを間接include）の**前**に`static PWM_Type * const FLEXPWM1 = PWM1;`でSDKの元の意味を退避してから使うよう変更、ドライバ呼び出し9箇所を全て`FLEXPWM1`に置き換え。`s_pins[]`テーブル自体は`PwmOut.h`include**後**に評価されるため、`{ PWM0, ... }`等の記述はio.hが再定義したArduinoピン値を正しく指す（A153の`s_pins[]`と同じ挙動）
  - `arduino_layer/arduino_io.h`: Arduinoピンリナンバリング機構（生のr01lib値を捕捉する配列→`#undef`→小さい連番へ再定義する`enum`という毎ピン共通の仕組み）内の`PWM_0`-`PWM_5`を`PWM0`-`PWM5`に統一
  - `PwmOut.h`のドキュメント表・経緯コメント、`examples/Arduino_compatible_API/test_analogWrite_*_N947`/`test_combined_peripherals_N947`の3スケッチ、`PIN_MAPPING_N947.md`・`API_COMPATIBILITY.md`・`variants/frdm_mcxn947/README.md`も全て新しい命名に追従（`variants/frdm_mcxn947/README.md`は一括置換で「以前は`PWM_0`という名前にしていた」という過去形の説明文まで書き換わってしまう事故が一瞬発生し、手動で修正）
- ライブラリ再ビルド（`-Wall`警告ゼロ）・ヘッダ同期・全50サンプルの両ボード回帰コンパイルを実施、チェックで見つかった3本を含め問題なくコンパイル成功することを確認
- **実機検証済み**: `test_analogWrite_all_channels_N947`（リネーム後の`PWM0`-`PWM5`で書き直し）をN947実機に書き込み、ロジックアナライザで動作確認——SDKマクロ衝突回避の核心部分である`PWM0`/`PWM1`ペア（sm2共有）を含む全6チャンネルの出力、およびペアごとの独立性（片方固定・片方掃引）が正常動作することを確認

### リリース前最終チェックとv0.3.0リリース完了
ユーザーから「リリース前に他にやっておくべきことは？」との依頼を受け、作業ツリーのクリーンさ・全50サンプルの両ボード回帰コンパイル・md間の内部リンク切れ・variant READMEの古い「暫定」表記の有無を確認、いずれも問題なし。CHANGELOG.mdの`[Unreleased]`→`[0.3.0]`変換とリリース実作業自体は「実際にリリースする際に判断」として一旦保留を提案したところ、ユーザーから「リリース作業を行う」と明確な指示。

- **`prepare0.3.0`→`main`マージ**: `main`が分岐なしの祖先だったため`git merge --ff-only`でfast-forward（`17b4959`→`3137057`、24コミット）。push成功
- **CHANGELOG.md確定**: `[Unreleased]`を`[0.3.0] - 2026-08-16`に変更してコミット（`ce7c648`）
- **リリースzip作成**: `git archive --format=zip --prefix=mcx/ HEAD:hardware/nxp/mcx`で単一トップレベルディレクトリ構造を確保。**A153の`.a`は`.gitignore`対象のため`git archive`に含まれない**ことを確認し、直近ビルド済みのstrip済み`.a`を手動でzipに追加（N947の`.a`は`hardware/`配下にコミット済みのため`git archive`だけで含まれる——A153/N947で扱いが違う点に注意）
- **リリース前ローカル検証（今回新規に実施）**: 過去のv0.2.1（zip構造バグ）・v0.2.2（Linux大文字小文字バグ）がいずれも「ローカル開発用symlink環境では再現せず、実際に公開されたzipをインストールして初めて発覚」した教訓を踏まえ、今回は**実際に作成したリリースzipをローカルで展開し、開発用symlinkを一時退避した上で本物のBoards Manager相当のインストール状態を再現**（`~/Library/Arduino15/packages/nxp/hardware/mcx/0.3.0`に実ファイルとしてコピー）。この状態で`hello_world`・SPI CS修正回帰・PWM0/PWM1修正回帰を含む4スケッチを両ボードでコンパイル確認——全て成功。検証後、開発用symlinkを復元
- **GitHub Release作成**: `gh release create 0.3.0`でzip添付・CHANGELOG該当セクションをリリースノートとして使用。タグpush時の自動`update_package_index.yml`実行は既知のdetached HEAD問題で失敗することを確認（想定通り）、`gh workflow run update_package_index.yml --ref main`で手動実行しchecksum確定（`de9c8bd`、SHA-256:`f253a06284b78a41f5709dc2dd7d6abf75d56ea9905c8d4d8c1f2eff54510772`、ローカルで計算した値と完全一致確認済み）
- **Issue #1/#2/#3**: `main`マージ時、各コミットメッセージの`fixes #N`表記によりGitHubが自動的にclose——手動でcloseする前に気づき、代わりに各Issueへ「Shipped in 0.3.0」コメントを追加してリリースへのリンクを残した
- **Arduino IDE Boards Manager経由インストールの実機検証**: ユーザー依頼で開発用symlink（`0.3.0-dev`）を一時退避し、ローカルのpackage indexキャッシュ（0.2.2時点のまま古かった）も削除した上でユーザー自身がArduino IDEを再起動しBoards Manager経由でインストール。**macOSでインストール〜ビルド〜アップロード〜動作までN947で確認完了**と報告。検証後、開発用symlinkを復元
- **Windows/Linuxでも実機検証完了**: 後日、ユーザーから「WindowsとLinuxの両方で正常に動作．インストールからビルド，アップロード，動作までをN947で確認した」と報告。**これでv0.3.0はmacOS・Windows・Linuxの3プラットフォームすべてでBoards Manager経由インストール〜実機動作まで検証済み**（v0.2.2までの前例と同じ検証範囲）

---

## v0.3.1 で作業中の内容（未リリース）

### MCUXpresso SDK APIを直接呼ぶサンプルの追加: `test_GPIO_toggle_speed_SDK_API`
v0.3.0リリース後、`platform.txt`を`0.3.1`に更新しローカル開発用symlinkも`0.3.1-dev`に改名して次の開発サイクルを開始（ユーザー指示）。最初の作業として、ユーザーから「MCUXpresso SDKのAPIをスケッチから呼んで使う例を作って。GPIO出力のHIGHとLOWを交互に繰り返すIO速度計測のためのテストで、GPIOの設定までをArduino SDKで、HIGH/LOW出力をMCUXpresso SDKのAPIでやる」という依頼。

- **構成**: `pinMode()`（Arduino API）でGPIOの方向・PORT muxを設定した後、`digitalPinToPort()`/`digitalPinToBitMask()`（この プロジェクト独自のarduino_layer——fast-GPIO/ビットバンギング用に元々用意されていたヘルパー）で生の`GPIO_Type*`とビットマスクを取得し、それをMCUXpresso SDK純正の`GPIO_PortSet()`/`GPIO_PortClear()`（`fsl_gpio.h`）に直接渡してHIGH/LOW出力を行う、という3層（arduino_layer→r01lib→SDK）を跨ぐ構成。`digitalWrite()`ベースのループと速度比較する形にした
- **実装時のミス**: Doxygen風コメント内で`GPIO_Type*/bitmask`と書いたところ、`*/`がブロックコメントの終端として解釈されコンパイルエラーになった——コメント内で`*/`という文字列を書く際の典型的な罠。「GPIO_Type pointer and bitmask」と書き換えて解消
- **ループアンロールの追加**: ユーザーから「ループのオーバーヘッドが大きくなるので、アンロールしてその影響を軽減」との指摘。特にSDK直接呼び出し側は1回のトグルがインライン展開されたレジスタ書き込み2命令程度で終わるため、ループのcompare/increment/branchオーバーヘッドが相対的に無視できなくなる。`DW_TOGGLE`/`SDK_TOGGLE`マクロを定義し、ループ本体に10個ずつ並べる形（UNROLL=10）で対応
- **実機検証済み（A153・N947両方）**:

| | A153 (96MHz) | N947 (150MHz) | 比率 |
|---|---|---|---|
| `digitalWrite()` | 784.7ns/toggle (1.274MHz) | 488.8ns/toggle (2.045MHz) | 1.61x |
| SDK直接 (`GPIO_PortSet`/`Clear`) | 22.9ns/toggle (43.620MHz) | 14.67ns/toggle (68.166MHz) | 1.56x |
| Speedup | 34.23x | 33.32x | — |

  両ボードの速度差比率（1.56〜1.61x）が、クロック周波数比（150MHz/96MHz = 1.5625x）にほぼ一致——両パスとも実行サイクル数自体はボード間でほぼ同じで、実測時間の差は純粋にクロック周波数の違いで説明できることが確認でき、ベンチマークとして筋が通っている結果と判断
- サンプルは`examples/Arduino_compatible_API/test_GPIO_toggle_speed_SDK_API/`としてコミット・push済み

### 調査: Arduino IDEの「Go to Definition」が効かない問題（対処は見送り、既知の問題として記録）
ユーザーからv0.3.1に入れたい機能としてもう1件、「Arduino IDEでマクロ文字列をハイライトしてGo to Definitionを選んでも定義元ファイルにジャンプできない」という報告。

- **切り分け1（マクロの種類）**: AskUserQuestionで具体的にどのシンボルで試したか確認。「D2/A0等のピン名（`#define`→`#undef`→`enum`の2段階リナンバリング）」「`LED_BUILTIN`等の単純な1回きりの`#define`」「`DEC`/`HEX`/`BIN`等Print.h内の定数マクロ」の3つすべてで発生、との回答——2段階リナンバリング特有の問題ではなく、マクロというシンボル種別全般の問題らしいと推測
- **切り分け2（決定的な手がかり）**: ユーザーが実際にArduino IDEで`pinMode`（マクロではなく実在する関数）にGo to Definitionを試したスクリーンショットを提供。**"No definition found for 'pinMode'"** — マクロだけでなく関数もダメだと判明し、仮説が根本から変わった
- **根本原因の特定**: このプロジェクトは「プリビルド`.a`配布方式」を採用しており、配布物（`hardware/nxp/mcx/variants/*/include/`）に含まれるのはヘッダ（宣言）のみ。`pinMode()`等の実装本体（`arduino_layer/*.cpp`）はエンドユーザーの手元に一切配布されておらず、コンパイル済み`.a`（デバッグシンボルもstrip済み）に固められているだけ。ジャンプ先のソース自体が存在しないため、これは実装バグではなく配布方式そのものに起因する構造的な制約
- **比較検証**: ユーザーの依頼で、ローカルにインストール済みの純正`arduino:renesas_uno`コア（`~/Library/Arduino15/packages/arduino/hardware/renesas_uno/`）の実際のディレクトリ構成を調査。`cores/arduino/`配下に`digital.cpp`等114個の`.h`・31個の`.c`・30個の`.cpp`という**実装ソースそのもの**が配置されており、プリビルドの`.a`は各variant配下の`libfsp.a`（Renesas純正SDK＝FSPのみ）だけだと確認。つまり純正コアは「Arduino層はソース配布、下位のベンダーSDKだけプリビルド」という構成で、これがGo to Definitionが機能する理由だと確定
- **対処方針の検討**: 理屈上は`arduino_layer/*.cpp`を`cores/arduino/`配下にソースのまま配置し、`platform.txt`のビルドレシピを標準的な逐次コンパイル方式に変更すれば（NXP SDKドライバ部分はrenesas_unoの`libfsp.a`同様プリビルドのまま残す想定）、同様にGo to Definitionが機能するようになるはずと判明。ただしこれはビルドレシピの構造自体の変更・ボード共通/固有コードの再整理・ビルド時間への影響・パイプライン変更に伴う実機再検証の必要性を伴う、v0.3.1のパッチ規模を明らかに超える独立したアーキテクチャ変更と判断し、**今回は対処を見送り**
- ユーザー指示で、CLAUDE.md（このセクション）とREADME.mdの両方に既知の問題として記録。当初README.mdに独立した「## Known Issues」セクションを新設したが、ユーザーから「それほど大きな問題ではないのでもう少し柔らかい注意点として書いておけないか、編集前に案を出して」と指摘。案A（見出しをやわらかく変更）・案B（独立見出しを作らず`## Architecture`セクションの説明に溶け込ませる）・案C（最小限の1〜2文）の3案を提示し、ユーザーが案Bを選択。独立見出しを削除し、`## Architecture`のプリビルドライブラリ説明の直後に「副作用として...」という一文＋実装ソースの参照先（`MCUXpresso_project/*/arduino_layer/`等）を追記する形に変更

### SDライブラリの`-Waddress-of-packed-member`警告を抑制（v0.3.1）
前セッションで「次のリリースで解決する」と約束していた件に対応。`platform.txt`の`compiler.cpp.flags`に`-Wno-address-of-packed-member`を追加。この変更は純粋な診断（警告）抑制フラグで、コード生成には一切影響しないこと（`-W`系フラグは`-O`/`-f`系と違いコードを変えない）、かつ`SD`ライブラリはスケッチビルド時にのみコンパイルされ、プリビルド`.a`（r01lib/arduino_layer/drivers）には一切関与しないことを確認済みだったため、対応前にユーザーへ「全機能の実機確認は不要なレベル」と説明し合意を得た上で着手。両ボードで`hello_world`のコンパイル確認と、`SD`ライブラリを使う`SDBitmapViewer`のコンパイルで警告が完全に消えたことを確認（修正前は複数の`-Waddress-of-packed-member`警告が出ていた）

### `Arduino_incompatible_API`カテゴリのI3Cサンプルで発覚したSOSパニックを修正
`examples/Arduino_incompatible_API/r01lib_I3C/`（未コミットのローカルファイル、以前から存在）を実行するとSOSパニックになるとユーザーから報告。原因はMikroBus向け`Serial1`実装時に確立済みの「Arduinoリナンバリングと生のr01lib値の食い違い」バグと全く同じパターン——`I3C i3c(I3C_SDA, I3C_SCL)`は`<Arduino.h>`をincludeしているため`I3C_SDA`/`I3C_SCL`がArduinoリナンバリング後の小さい整数に解決される一方、`I3C`クラスのコンストラクタ内部（`i3c.cpp`、生のr01lib値の世界）のピン検証`panic()`ガードはそれと一致せず落ちていた。既存の姉妹サンプル`r01lib_API.ino`が確立している「シンボル名ではなく生の物理ピン名（`P3_13`等）を使う」という規約に倣い、`I3C_SDA`/`I3C_SCL`をA153での生のr01lib値`P0_16`/`P0_17`に置き換えて解消。`arduino-cli compile`で確認済み（このサンプル自体は元々gitで未追跡のローカルファイルのため、コミットは未実施）

### v0.3.1リリース完了
- **リリースzip作成**: `git archive --format=zip --prefix=mcx/ HEAD:hardware/nxp/mcx`で単一トップレベル構造を確保した上で、A153の`.a`（`.gitignore`対象のため`git archive`に含まれない）を正しい相対パス（`mcx/variants/frdm_mcxa153/lib/`）に手動追加——一度`zip -j0`で誤ってトップレベル直下に追加してしまうミスをした後、展開→ファイル追加→再zipという安全な手順に切り替えて修正。最終zip: SHA-256 `909cfa2c29be58bad21465caf5773d113bf545d716849adef8ea507a78066e69`、1519992 bytes
- **GitHub Release作成**: `gh release create 0.3.1`でzip添付・CHANGELOG該当セクションをリリースノートとして使用。ダウンロード後のchecksum再計算でも同じ値と一致することを確認
- **ステージングブランチ方式を初適用**: `staging-0.3.1`ブランチを作成し、`package_nxp_mcx_index.json`の0.3.1エントリだけを検証済みchecksumで上書きしてpush。ユーザーがこのブランチのraw URLをArduino IDEのAdditional Boards Manager URLsに一時設定し、開発用symlink（`0.3.1-dev`）を`packages/`外へ完全退避・ローカルインデックスキャッシュ削除の上でBoards Manager経由インストールを実施
- **macOS・Windows・Linuxの3プラットフォームすべてでインストール〜動作確認完了**とユーザー報告
- 検証完了後、`main`に対して`update_package_index.yml`を`gh workflow run --ref main`で手動実行しchecksum確定（`e74604c`、ステージングブランチで検証済みの値と完全一致）。ステージングブランチは役目を終えたためリモート・ローカルとも削除（ローカル削除は`git branch -D`——ステージング用コミット自体は`main`にマージされておらず、内容はworkflow生成のコミットで実質的に置き換えられているため強制削除で問題ない）
- 開発用symlinkは検証後に復元（このセッションの一時ディレクトリがターン間でクリアされ、退避先のsymlinkファイル自体は失われたが、シンボリックリンク自体は`ln -s`で再作成するだけで実害なし）

---

## v0.3.2 で作業中の内容（`0.3.2-dev` ブランチ・未リリース）

v0.3.1リリース完了後、次バージョンから「`<version>-dev`ブランチで開発、リリース時に`main`へマージ」という運用に変更（詳細は「開発ブランチ運用方針（v0.3.2から採用）」セクション参照）。`platform.txt`のバージョンを`0.3.2`に、ローカル開発用symlinkも`0.3.2-dev`に更新して開発サイクル開始。

### `r01lib_I3C`サンプルのSOSパニック修正をコミット、N947対応も追加
v0.3.1セッション末で修正済みだった（未コミットのまま残っていた）`examples/Arduino_incompatible_API/r01lib_I3C/r01lib_I3C.ino`のSOSパニック修正を`0.3.2-dev`にコミット。続けてユーザーから「A153とN947ではピン指定を変えないといけない」との指摘を受け、N947向けの生のr01lib I3Cピン値（`I3C_SDA`/`I3C_SCL` → `MB_RX`/`MB_TX` → `P1_16`/`P1_17`、A153の`P0_16`/`P0_17`と同じ導出パターン）を調べ、`#if defined(...)`でボードごとに分岐するよう拡張。両ボードでコンパイル確認済み

### `FRDM_MCXA153`/`FRDM_MCXN947`ボード識別マクロを新設
ユーザーから「ターゲットの種別を検出するマクロに`CPU_MCXN947VDF`と`CPU_MCXA153VLH`を使うようになってる。これを`FRDM_MCXN947`と`FRDM_MCXA153`でも使えるようにして、I3Cのサンプルにも反映させる」と依頼。`boards.txt`の`build.board_defines`に`-DFRDM_MCXA153`/`-DFRDM_MCXN947`を追加（既存の`ARDUINO_FRDM_MCXA153`/`ARDUINO_FRDM_MCXN947`——`platform.txt`の`-DARDUINO_{build.board}`由来——とは別に、`ARDUINO_`プレフィックスなしのボード名マクロとして新設）。`r01lib_I3C`サンプルの`CPU_MCXxxx`分岐を`FRDM_MCXxxx`に置き換え。`boards.txt`は共有ファイルのため全examplesの回帰コンパイル（両ボード、計112ケース）を実施——失敗13件は全て既知の想定内失敗（外部ライブラリ`P3T1755.h`未インストール、または`_A153`/`_N947`専用サンプルを別ボードでコンパイルした際の想定内エラー）で新規リグレッションなしと確認

### `analogWriteFrequency(pin, hz)`を追加（実機検証済み）
残作業候補として提示していた「`analogWrite`のPWM周期を可変にする」に対応。

- **設計方針の検討**: 公式Arduino APIには存在しない拡張機能（AVR系ボードのタイマー制約により、公式は周波数設定APIを標準化していない）と説明した上で、TeensyのAPI（`analogWriteFrequency(pin, frequency)`、ピン単位）とRaspberry Pi Pico/arduino-picoのAPI（`analogWriteFreq(freq)`、グローバル）のどちらに寄せるか相談。Teensy方式を推奨——このプロジェクトの`analogWrite(pin, value)`と一貫性がある、かつFlexPWMは全ピン共有ではなくサブモジュール単位（`PWM0`/`PWM1`, `PWM2`/`PWM3`, `PWM4`/`PWM5`が各ペアで周期共有）という中間的な構造のため、むしろTeensy方式の方が実態に近いと判断。ペア共有の制約はドキュメントに明記する方針で合意
- **実現可能な周波数範囲の検討**: 既存の`PwmOut::apply()`（プリスケーラ0-7自動選択＋16bitカウンタ）がそのまま使える設計だったため新規ロジックは不要と判明。下限はプリスケーラ÷128・カウンタ最大65535の組み合わせで決まるハード上の下限（A153で約11.4Hz、N947で約17.9Hz）、上限は明確なハード上限はなくduty分解能とのトレードオフ（8bit相当の分解能なら A153で約375kHz、N947で約586kHz程度が目安）
- **実装**: `arduino_analog.h`/`.cpp`（両ボード）に`analogWriteFrequency(pin_num, frequency)`を追加。`analogWrite()`と同じ遅延生成パターン（`pwm_out_pins[]`に無ければ`new PwmOut`）で、既存の`PwmOut::period_us()`を呼ぶだけ。duty比ではなく絶対パルス幅が周期変更をまたいで保持される既存の`PwmOut::period()`の仕様をそのまま踏襲するため、「`analogWriteFrequency()`を先に呼んでから`analogWrite()`でdutyを設定する」が正しい呼び出し順であることをコメント・ドキュメントに明記
- 両ボードのプリビルド`.a`を再ビルド・strip・再配置。確認用サンプル`test_analogWriteFrequency`（`PWM0`を1kHz/50Hz/20Hz/5kHzで巡回、都度Serialへ周期を出力）を新規作成、両ボードでコンパイル確認、PWM関連サンプルの回帰コンパイルも問題なし
- **実機検証完了**: ユーザーがロジックアナライザで確認し、**A153・N947の両方で指定通りの周波数となることを確認**したと報告

### ドキュメント運用方針の訂正（同セッション内）
上記`analogWriteFrequency()`関連のドキュメント更新（`PIN_MAPPING_A153.md`/`PIN_MAPPING_N947.md`/`API_COMPATIBILITY.md`/`CHANGELOG.md`）を、当初の方針どおり`main`に直接コミット・pushしたが、ユーザーから訂正: 「今後は作業中のブランチでやる。そうでないとリリース版との整合が取れないから」。`main`にコミット済みだった該当ドキュメント更新は`git revert`で取り消し（`9c0f9ad`）、同じ内容を`0.3.2-dev`へ`git cherry-pick`で移設（`7e20c89`）。詳細・訂正後の方針は「開発ブランチ運用方針（v0.3.2から採用）」セクション参照

### v0.3.2リリース完了
- `0.3.2-dev`（`b504ae4`）→`main`へ通常マージ（`main`が分岐点の祖先ではなくなっていたため`--ff-only`不可、`git merge`でマージコミット`56c83f9`を作成。ドキュメントのrevert/cherry-pick分は内容的に打ち消し合っていたため無衝突）
- `CHANGELOG.md`の`[Unreleased]`を`[0.3.2] - 2026-08-20`に確定、`package_nxp_mcx_index.json`に新規0.3.2エントリを追加してコミット（`609e984`、「リリース作業」コミット）
- リリースzip作成: `git archive --format=zip --prefix=mcx/ HEAD:hardware/nxp/mcx`＋A153の`.a`（gitignore対象）を正しい相対パスへ手動追加。SHA-256 `b9b63d87a199bcf47b116fde60aebfbd4956ffaaf15e09c43a8ddedad23feaad`、1521162 bytes。ダウンロード後の再計算でも一致確認済み
- `gh release create 0.3.2`でGitHub Release作成
- **ステージングブランチ方式を継続適用**: `staging-0.3.2`ブランチで検証済みchecksumを一時push→macOS/Windows/Linuxの3プラットフォームすべてでBoards Manager経由インストール〜動作確認完了とユーザー報告→`main`に対し`update_package_index.yml`を手動実行してchecksum確定（`bb34ab0`、ステージングブランチの値と完全一致）
- ステージングブランチ・`0.3.2-dev`ブランチともマージ済み・役目終了のためリモート・ローカルとも削除

---

## v0.4.0 で作業中の内容（`0.4.0-dev` ブランチ・未リリース）

### ソース配布方式への移行（着手中）
ユーザーから「Arduino IDEのGo to Definitionを使えるようにしたい」という要望を受け、現在のプリビルド`.a`配布方式（`arduino_layer`/`r01lib`の実装ソースがエンドユーザーに一切配布されない）が原因と特定。対処法として「`arduino_layer`だけソース化・NXP SDKドライバはプリビルド維持」という部分案と、「全部ソース化し標準的なArduinoのcoreビルドキャッシュ機構に任せる」という全部案を比較検討。

- Arduinoのビルドシステムは`cores/`配下のソースをFQBN（ボード＋オプション）単位でキャッシュする（renesas_uno等、ソース配布している他コアと同じ挙動）ため、「初回ビルドだけ時間がかかり以降は速い」「A153↔N947を行き来してもそれぞれのキャッシュが独立して保持される」ことを確認済み。デバッガ対応（Arduino IDE 2のデバッグ機能）は今回の範囲外——別途`platform.txt`への`debug.*`設定追加が必要（LinkServerのgdbserverモードが土台になりそう）と整理し、後回しにする方針
- ユーザー判断: 「全部ソース化を進める」。`0.4.0-dev`ブランチを`main`から作成、`platform.txt`のバージョンを`0.4.0`に更新

### r01libソースの重複・ドリフト監査と統合（進行中）
`arduino_layer`/`source/r01lib`配下のファイルは、A153用・N947用としてMCUXpressoプロジェクトディレクトリに別々にコピーされ手動同期されてきた（多くはCPUマクロで分岐する単一ソースとして設計されているにも関わらず）。`cores/arduino/`への一本化に先立ち、両ボードの同名ファイルを全diffし、コメント/typoの差分か、実際の機能的ドリフトかを仕分け。

- **ほぼ全ファイルがコメント修正程度のトリビアルな差分**（N947側が後から整理されており、そちらが正しい版）と判明。`AnalogIn.cpp/h`、`PwmOut.cpp/h`、`i3c.cpp/h`、`io.h`、`mcu.cpp/h`、`Ticker.h`、`InterruptIn.h`、`BusInOut.h`、`irq.h`、`r01lib.h`はこのパターン
- **`Serial.cpp/h`**: 両ボードが独立して別々の改良を積んでいたことが判明。A153側は`tx_mux`/`rx_mux`フィールド分割＋全ボード共通の`rx_io.input_buffer(true)`修正、N947側はMikroBus Serial1（LPUART5）対応——を統合。A153ベースにN947のLPUART5対応を追加する形でマージ
- **`i2c.cpp/h`**: 統合作業中に**実際に稼働中のバグを発見**——N947の`i2c.cpp`には、以前A153の`Wire.end()`BusFault修正で追加した`_no_hw`デストラクタガードが入っておらず、N947で`Wire1.end()`を呼ぶと同種のクラッシュが起きる状態だったと判明。N947のMikroBus/Wire2対応をA153ベース（`_no_hw`修正込み）に統合する形で解消
- **`r01lib_spi.cpp/h`**: 単なる差分ではなく、CS制御の設計そのものが両ボードで異なっていた——A153は「CSピンのmuxを一切触らない」、N947は「`cs_manual_control()`で明示的にGPIO/PCS切り替え」という別アプローチ。ユーザーに相談し、**N947方式（`cs_manual_control()`）に統一**する方針で合意。A153の独自MikroBus SPI1（LPSPI0）対応もこの統合後の構造に追加
- 統合後、両ボードの`.a`を再ビルド・strip・配置。全114サンプルの回帰スイープで新規リグレッションなし（既知の13件のみ）を確認

### ピンリナンバリングとr01libの生ピン値の食い違いを解消
ユーザーから「r01libとArduino.hで違っているピン番号を統一できないか」という提案。調査の結果、`arduino_i2c.cpp`自体は`arduino_io.h`をincludeしないため`Wire`/`Wire1`の内部動作自体は元々安全で、問題は**スケッチ側**が`<Arduino.h>`経由でリナンバリングされた名前を使って直接r01libクラスを構築する場合（`r01lib_I3C.ino`のケース）に限られると特定。

- 完全統一（r01lib自身の生ピンエンコーディングをArduinoの連番に合わせる）は、全ペリフェラルクラスの書き換えを要し、かつ`Arduino_incompatible_API`が売りにする物理ピン透明性（`P3_13`等）を損なうため見送り。ユーザーが提案した「r01libのio.cpp/hの配列順を入れ替える」も同様の理由で根本解決にならないと判断
- 代わりに**「除外リスト方式」**を採用: `I3C_SDA`/`I3C_SCL`/`I2C_SDA`/`I2C_SCL`/`SPI_CS`/`SPI_MOSI`/`SPI_MISO`/`SPI_SCLK`/`ARD_CS`/`ARD_MOSI`/`ARD_MISO`/`ARD_SCK`の12個を`arduino_io.h`のリナンバリング対象から除外（`USBTX`/`USBRX`が元々受けていたのと同じ扱い）。全examplesを検索し、これらが`pinMode`/`digitalWrite`等の裸引数として使われていないことを確認してから実施——安全に「r01lib内部でもArduino層でも常に同じ値」にできた
- 検証として`r01lib_I3C.ino`を、回避策だった生の物理ピン名（`P0_16`等）指定から、素直な`I3C_SDA`/`I3C_SCL`直書きに戻し、両ボードでコンパイル成功・全114サンプルの回帰スイープで新規リグレッションなしを確認
- **実機バグ再発見の顛末**: この一連の統合作業中、一時的に`hardware/nxp/mcx/cores/arduino/r01lib/`にマージ中間ファイルを置いていたところ、`platform.txt`を一切変更していないにも関わらずArduinoのデフォルトcoreビルド機構がこのディレクトリを自動検出してコンパイルしてしまい、`variants/*/include/`の未更新ヘッダとの不整合で大量のビルド失敗（N947側64件）を引き起こす事故が発生。原因究明後、作業用ファイルを`_migration_staging/`（`hardware/nxp/mcx/`の外）に退避して解消——今後cores/への本格移行を進める際は、この「置いた瞬間からビルドに巻き込まれる」という挙動を踏まえて進める必要がある

### 実機検証: `Wire1.end()`修正（N947）
`test_Wire1_onboard_sensor_raw.ino`に`Wire1.end(); delay(100); Wire1.begin();`を一時的に追加してN947実機で確認依頼。**ユーザーが「問題ないことを確認」と報告**——`i2c.cpp`統合で見つけた実バグ（N947の`Wire1.end()`が`_no_hw`修正漏れでクラッシュしうる状態だった件）が正しく解消されたことを実機で確認できた

### 実機検証: SPI（A153）
`test_SPI_bitorder_end_transfer16`をA153実機で確認依頼——`r01lib_spi.cpp/h`統合でCS制御方式を「一切触らない」から`cs_manual_control()`ベースに変更した箇所。**ユーザーが「OK」と報告**、問題なし

### 実機検証: 一度壊れて直った「除外リスト方式」の完成
`r01lib_I3C.ino`実機検証（項目3）で、最初の除外リスト実装（`I3C_SDA`等をリナンバリングテーブルの自分のスロットから外すだけ）に不備が見つかった——A153ではOKだったがN947ではSOSパニックが再発。

- **原因**: `I3C_SDA`はN947では`#define I3C_SDA MB_RX`という**別名（トークン置換）**であって独立した値ではない。自分のスロットだけをリナンバリング対象から外しても、参照先の`MB_RX`自体は依然リナンバリングされるため、`I3C_SDA`は`MB_RX`の新しい（リナンバリング後の）値を暗黙に引き継いでしまう。A153では`I3C_SDA`が生の物理ピンマクロ`P0_16`を直接指すため問題が表面化しなかっただけ（たまたま安全だった）
- **修正**: `arduino_io.h`の他の`#undef`が走る**前**に`constexpr`で現在の値（＝生の値）を退避し、他の名前が全部リナンバリングされた**後**で、退避した生の値に固定的に再定義する方式に変更。これで別名チェーンを断ち切った
- **副次的な誤り発覚**: 同じ最初の実装で`SPI_CS`/`SPI_MOSI`/`SPI_MISO`/`SPI_SCLK`/`ARD_CS`/`ARD_MOSI`/`ARD_MISO`/`ARD_SCK`も除外リストに入れてしまっていたが、これらは`MOSI`/`MISO`/`SCK`裸マクロ（Issue #1、SDライブラリ互換性）の土台であり、スケッチから`pinMode`/`digitalWrite`の通常の引数として使われる必要があるため、リナンバリング対象から外してはいけないものだった。両方とも修正しリナンバリングテーブルに復元
- **もう1件のケアレスミス**: 修正時に書いたコメント文字列`D0-D19/MB_*/SPI_*/ARD_*`自体に`*/`という部分文字列が含まれており、ブロックコメントを予定より早く閉じてしまうバグ（`test_GPIO_toggle_speed_SDK_API`の時と同じパターン）も併発、これも修正
- 修正後、両ボードで`r01lib_I3C.ino`が`I3C_SDA`/`I3C_SCL`直書きでコンパイル成功、全114サンプルの回帰スイープもクリーン

### 実機検証: `r01lib_I3C.ino`のI3C通信そのもの（ピン修正とは別件の発見）
ピン修正確認のため`r01lib_I3C.ino`を両ボードで実機フラッシュしたところ、ピンの問題ではない別の発見があった。

- SOSパニックは解消したが、N947では温度が常に0.00——`P1_16`/`P1_17`直書きでも同じ結果で、ピン定義の問題ではないと確定
- ステータスコード出力を追加して調査した結果、N947では**最初のRSTDAAブロードキャストから一貫して`kStatus_I3C_Nak`(7902)**——アドレスフェーズでNAK、実際にMicroBusピンにも信号が出ていないとユーザーが実機で確認
- ユーザーの指摘で「A153はなぜ動いてる？」と問われ、実際にA153で試したところ**A153は完全に正常動作**（RSTDAA/SETDASA/write/read全てstatus=0、実温度27℃前後）——これによりネイティブI3C SDR経路自体が壊れているわけではなく、N947固有の何かだと判明
- 「I2C_MODEに切り替えてから通信すれば動くのでは」という仮説を試したところ、**A153側が逆に壊れた**（write status=7903 WriteAbort、read status=0だがraw=0xFFFFという実質壊れた値）——実験を戻して確認したところA153は元通り正常動作。この実験自体が誤りだったと確定
- 続けてユーザーが`test_Wire1_onboard_sensor_raw.ino`（`Wire1`経由のI2C_MODE、静的アドレス0x48）をA153で試したところ**endTransmission failed, error=222**——一見新しいバグに見えたが、222は7902(kStatus_I3C_Nak)をuint8_tに切り詰めた値と一致し、**I3Cの仕様通りの正しい挙動**と判明: 直前の`r01lib_I3C.ino`実行でSETDASAにより動的アドレス0x08が割り当てられたため、センサーは静的アドレス0x48では応答しなくなっていた。ユーザーが電源再投入（センサーのリセット）したところ`test_Wire1_onboard_sensor_raw.ino`は正常動作に復帰、この理解が正しいと確認された
- **現状の整理**:
  - A153: `r01lib_I3C.ino`（ネイティブI3C SDR + SETDASA動的アドレス割り当て）は完全に実機検証済み・正常動作
  - N947: `Wire1`（I2C_MODE、静的アドレス）は正常動作するが、`r01lib_I3C.ino`のネイティブI3C SDRでのCCCブロードキャスト（RSTDAA）自体が、最初の1回から一貫してNAKし、物理ピンに信号すら出ない。原因未特定
- **今回のセッションの本題（ピンリナンバリング修正）としては、A153側で完全に実証されたことで検証完了**——N947のネイティブI3C SDR不具合は、今回のピン修正とは無関係の別問題と切り分け済み。ユーザー判断で**既知の課題として保留**し、ソース化作業（`cores/arduino/`一本化）に戻ることに決定

### 実機検証: N947のSerial1、両ボードの複合動作
- `test_Serial1_MikroBus_N947`（MB_TX-MB_RXループバック）をN947実機で確認——**正常動作**（送信した文字列を`Serial1.available()`/受信内容とも正しく取得）。`Serial.cpp`統合で新たに無条件適用されるようになった`rx_io.input_buffer(true)`が、N947でも問題なく動作することを確認
- `test_combined_peripherals_A153`/`test_combined_peripherals_N947`を両ボード実機で確認——**両方とも問題なし**

これでr01libソース統合・ピンリナンバリング修正に関する実機確認チェックリスト（5項目）が全て完了

### `cores/arduino/`一本化・`platform.txt`書き換え完了（プリビルド`.a`配布方式の廃止）
実機確認完了を受けてソース化の本題に復帰。事前の統合作業（`Serial.cpp`/`i2c.cpp`/`r01lib_spi.cpp`統合、ピンリナンバリング修正）で両ボードのファイルはほぼ全て一致していたので、残りの「確認済み・コピー待ち」ファイル（`AnalogIn`/`PwmOut`/`i3c`/`io.h`/`mcu`/`Ticker`/`InterruptIn`/`BusInOut`/`irq`/`r01lib.h`）を実際にN947側からA153へコピーし、`arduino_tone.cpp`（CLOCK_SetClockDiv/CLOCK_SetClkDivのシンボル名差異、どちらのコピーも分岐すらしていなかった）・`arduino_analog.h`/`Arduino.h`（ボード名を直書きしたコメント）を`#if defined`ガード付きで統合。`component/*`・`utilities/*`・`drivers/`の7ペア（fsl_common/fsl_common_arm/fsl_gpio/fsl_i3c/fsl_pwm/fsl_spc/fsl_utick）が両ボードで完全に同一内容であることも確認。これで`arduino_layer`/`r01lib`配下の全20ファイルが両ボードでbyte-for-byte同一に。

- **構成**: `cores/arduino/`（両ボード共有、フラット構成——`variants/*/include/`が元々`#include`をサブディレクトリ修飾なしのフラット参照前提で書かれているため、同じ慣習を踏襲）に、統合済みの`arduino_layer`/`r01lib`ソース＋`component`/`utilities`＋共有driver 9ペア（上記7つ＋`fsl_ctimer.h`/`fsl_inputmux.c/h`、後から見落としに気づき追加）を配置。`variants/<board>/src/`（新設）にボード固有ソース（`board/`・`device/`・`startup/`・チップごとに実際に内容が異なるdriver、`fsl_clock`/`fsl_ctimer.c`/`fsl_lpadc`/`fsl_lpi2c`/`fsl_lpspi`/`fsl_lpuart`/`fsl_reset`＋N947固有の`fsl_lpflexcomm`/`fsl_vref`）を配置
- **`cores/arduino/irq.c`の重複削除**: 従来この場所には「`--whole-archive`でcore.aに強制リンクさせ、startup fileの弱いシンボルを確実に上書きするため」という理由で作られた**独自の重複ファイル**が置かれていた（本物は`source/r01lib/irq.c`としてプリビルド`.a`側にあった）。今回の移行で本物の`irq.c`自体が`cores/arduino/`（＝常に`--whole-archive`の対象）に入るため、この重複ワークアラウンドは不要になり削除
- **`platform.txt`の変更は最小限で済んだ**: Arduinoの標準ビルド機構は`cores/arduino/`配下を最初から自動検出・コンパイルしてcore.aへアーカイブする仕組み（`recipe.ar.pattern`）が既に有効になっていた（従来は`arduino_main.cpp`等ごく少数のファイルしか対象がなかっただけ）ため、ファイルをそこに置くだけで自動的にビルド対象になった。実際に変更したのは`recipe.c.combine.pattern`から`-L{build.variant.path}/lib`と`-l{build.variant_lib}`（プリビルド`.a`のリンク）を削除しただけ。`-Wl,--whole-archive "{archive_file_path}" -Wl,--no-whole-archive`（core.a全体を強制リンク）はそのまま維持——弱いシンボルの上書きが引き続き機能する
- **`boards.txt`**: 両ボードの`build.variant_lib=...`プロパティを削除（もう参照されない）
- **`variants/*/lib/`ディレクトリごと削除**: N947の`.a`は`git rm`、A153の`.a`（gitignore対象）はディスクから削除、`.gitignore`の該当行も削除
- **移行中に発見した実機コンパイルエラー**: `cores/arduino/`へのフラット化作業中、統合済みのつもりだった`arduino_i2c.cpp/h`・`arduino_serial.cpp/h`（Arduino層の`Wire`/`Serial`グローバル宣言）が、実は一度も統合されていなかったことが判明——以前統合したのは同名だが別物の`r01lib`側`i2c.cpp`/`Serial.cpp`だけだった。回帰スイープで`test_Wire2_MikroBus_N947`がN947自身でも失敗するという形で発覚（`Wire2`が存在しないというエラー——A153のコピーがそのまま両ボードに使われていたため）。`Wire2`宣言・定義とN947のRSTDAA priming処理を`#ifdef CPU_MCXN947VDF`で正しく統合、`Serial1`のD0/D1 vs MikroBusルーティングの差異も同様に統合して解消
- **検証**: `hello_world`が両ボードとも一発でコンパイル成功（A153: 43636 bytes、プリビルド`.a`時代の45816 bytesより小さい——プリビルド`.a`のアーカイブメンバ単位のリンクよりも、フルソースの関数単位リンク(`-ffunction-sections`+`--gc-sections`)の方が不要コードをより細かく削れるため）。全114サンプルの回帰スイープも両ボードでクリーン（既知の13件のみ）
- **`MCUXpresso_project/`ディレクトリの扱い**: ユーザーに確認したところ「必要ないなら消す」と回答。`git rm -r`でリポジトリから削除（`_r01lib_frdm_mcxa153`/`_r01lib_frdm_mcxn947`の他、未着手ボード用の`_r01lib_frdm_mcxa156`/`_r01lib_frdm_mcxn236`、各種テスト/トライアルプロジェクトも含め約632MB分）。さらにgit未追跡だったMCUXpresso IDEのワークスペースメタデータ（`.metadata/`等）もディスクから削除。README.mdの「Architecture」節（プリビルドライブラリ前提の説明、Go to Definitionが効かないという注記——今回のソース化で解消済みのため）と「Building the Prebuilt Library」節（丸ごと削除）を、新しいソース配布構成に合わせて更新
- まだ未実施: 実機での書き込み・動作確認（今回はコンパイルレベルの検証のみ）、Arduino IDEでの実際のGo to Definition動作確認

### 実機検証: A153でLPSPI0クロック未供給の実バグを発見・修正、N947は問題なし
上記のコンパイルレベル検証（114サンプル×2ボード）を経て、ユーザーが実際に`test_combined_peripherals_A153`/`_N947`を実機フラッシュ。**`hello_world`は両ボードとも正常動作**を確認した一方、`test_combined_peripherals_A153`は`setup()`終了時のバナー行を1行出力した直後に停止（`loop()`内のどこかでハング）、`test_combined_peripherals_N947`はSOSパニックという、コンパイル検証だけでは検出できない実行時のみのリグレッションが発覚——ソースコード配布方式への移行という大規模なビルドシステム変更が、実機での動作を保証しないことを示す教訓となった。

- **切り分け**: `loop()`内の各ステップ直後に`Serial.println()`+`Serial.flush()`のチェックポイントを挿入して再フラッシュ・再確認する方式で二分探索。当初「I3C(`sensor.temp()`)が怪しい」という仮説を立てコメントアウトを依頼したが、ユーザーが「1行だけで止まる」と報告し否定——的外れな仮説だったと判明。改めて全ステップにチェックポイントを入れ直したところ、**`cp4`（`analogWrite`後）は出力されるが`cp5`（`SPI1.transfer16()`後）が出ない**ことが判明——`SPI1`（MikroBusのSPI、A153では`LPSPI0`）のブロッキング転送内で無限待ちしていると特定
- **根本原因**: `cores/arduino/mcu.cpp`の`init_mcu()`内、実際にA153でビルドされる`#elif CPU_MCXA153VLH`分岐に、`LPSPI0`（MikroBus SPI1用）のクロックアタッチ（`CLOCK_SetClockDiv(kCLOCK_DivLPSPI0,1u); CLOCK_AttachClk(kFRO12M_to_LPSPI0);`）が存在しなかった——このコードはA153のMikroBus SPI1対応作業時に実機バグとして発見・追加されたはずのものだが（このドキュメントの「A153のMikroBus対応: `SPI1`とGPIO」セクション参照）、r01libソース統合作業中に、なぜかA153の実ビルドには使われない`#elif CPU_MCXA156VLL`分岐（未リリースの別チップ向け）の方にだけこのクロック設定が残り、A153自身の分岐からは消えてしまっていた。クロック未供給のペリフェラルに対して`LPSPI_MasterTransferBlocking()`（SDK関数、内部でTX/RXステータスフラグをポーリング）を呼ぶと、そのフラグが永久に立たないため無限ループ＝ハングする、という典型的な症状と完全に一致
- **修正**: `CPU_MCXA153VLH`分岐に`LPSPI0`のクロックアタッチを追加し直した（[mcu.cpp](hardware/nxp/mcx/cores/arduino/mcu.cpp)）。ソース配布方式になったため、`.a`の再ビルド・再配置は不要——ソースを直すだけで次回ビルドに反映される
- ユーザーが再フラッシュし、**A153で正常動作を確認**（`test_combined_peripherals_A153`が全ステップ完走）。チェックポイント計装は削除しスケッチを元の内容に復元
- N947についても同様にチェックポイント計装を入れて確認を依頼したところ、ユーザーから「A153のスケッチを動かしていた」という誤操作の訂正があり、**改めてN947でも問題なく動作することを確認**——N947側には実機バグは無かった（チェックポイント計装も削除・復元済み）

### Go to Definitionの実機（IDE）検証: ローカル開発環境固有の症状と判明、原因究明・解消
実機検証完了を受けてもう一つの検証項目に着手。ユーザーがArduino IDE再起動後・実際にVerifyも実行した上で`Serial.println`/`analogWrite`にGo to Definitionを試したが、**"No definition found"** のまま——ソース化そのものは成功しているはずなのに、当初の目的（Go to Definitionを効かせる）が達成できていない状態だった。

- **調査**: `ps aux`でArduino IDEが実際に起動しているclangdプロセスの引数を確認したところ、`-query-driver=/Users/tedd/Library/Arduino15/packages/**` というフラグが付いていた——clangdはクロスコンパイラ（`arm-none-eabi-c++`）の実体を問い合わせてターゲット情報（system include path等）を取得する際、このglobに一致するパスのドライバしか信頼しない仕組み。Arduino IDEのバックグラウンドclangdバイナリを直接`--check`モードで実行し、実際に使われている`compile_commands.json`（`/var/folders/.../arduino-language-server*/build/compile_commands.json`）を読ませたところ、`the clang compiler does not support '-mcpu=cortex-m33'` → `CreateTargetInfo() return null` → プリアンブル（AST）構築が完全に失敗、という状態を直接再現
- **原因特定**: `compile_commands.json`内のコンパイラパスが `/Users/tedd/.xpacktools/xpack-arm-none-eabi-gcc-14.2.1-1.1/...` という**シンボリックリンクの解決後の実体パス**になっており、これは`-query-driver`のglob（`.../Library/Arduino15/packages/**`）に一致しないため「信頼できないドライバ」としてクロスターゲット情報の取得が拒否され、clangdはmacOSネイティブ（arm64-apple-macosx）としてこのファイルをパースしようとして`-mcpu=cortex-m33`を理解できず失敗していた。このシンボリックリンクは、このプロジェクトのローカル開発環境で「ツールチェーンを二重に持たずに済ませる」ために`~/Library/Arduino15/packages/nxp/tools/arm-none-eabi-gcc/14.2.1-1.1`から`~/.xpacktools/xpack-arm-none-eabi-gcc-14.2.1-1.1`へ張っていたもの（本プロジェクトのCLAUDE.mdの「ローカル開発環境」セクションに記載の運用）
- **ビルド用と言語サーバ用で挙動が異なっていた理由**: 実際に「検証(Verify)」を実行した際に生成される`compile_commands.json`（`.../fullbuild/`）はシンボリックリンクのパスをそのまま保持しておりglobに一致していたため、ビルド自体は問題なく成功していた。一方、Go to Definition等の対話的機能が参照する常駐clangdプロセス用の`compile_commands.json`（`.../build/`）は同じツールチェーンパスをシンボリックリンクの実体まで解決した状態で書き込まれており、この2つの生成経路の違いが「ビルドは通るのにGo to Definitionだけ動かない」という一見矛盾した症状を生んでいた
- **重要な判断**: これは`mcx-arduino-core`パッケージ自体のバグではなく、**このユーザーのローカル開発環境固有のシンボリックリンク運用に起因する症状**と判断——実際にBoards Manager経由でインストールするエンドユーザーの環境ではツールチェーンはシンボリックリンクではなく実ファイルとして配置されるため、この問題自体が発生しない
- **検証**: ユーザーの了承を得て、ローカルの`~/Library/Arduino15/packages/nxp/tools/arm-none-eabi-gcc/14.2.1-1.1`シンボリックリンクを`~/.xpacktools/xpack-arm-none-eabi-gcc-14.2.1-1.1`の実体コピー（`cp -R`、約1GB）に置き換え。Arduino IDEのバックグラウンドプロセスを再起動させて`compile_commands.json`を再生成させたところ、`build`用・`fullbuild`用の両方が同一の（実ファイルの）パスを指すようになり、clangdの`--check`モードでプリアンブル構築が成功することを確認。その後ユーザー自身がArduino IDEで実際にGo to Definitionを試し、**「動作に問題ないことが確認できた」**と報告——ソース化によるGo to Definition対応が設計通り機能することが実証された
- これでv0.4.0のソース化移行に関する実機検証（両ボードのコンパイル・実行・Go to Definition）がすべて完了

### Arduino IDE内蔵デバッガ対応（`gdb-bridge`、実機検証済み）
ユーザーから「Arduino IDE内でデバッガを使えるようにしたい」と依頼。

- **調査で判明した構造的な壁**: Arduino IDE 2 / `arduino-cli`のデバッグ機能は`debug.server`プロパティが`openocd`の場合しかサポートしていない（`arduino-cli`本体のソース——`commands/service_debug_config.go`・`commands/service_debug.go`——を直接確認、`switch server { case "openocd": ... default: return error }`とハードコードされており、他のgdbserver名を直接指定する余地がない）。一方、本家OpenOCD（0.12.0、GitHub上のmasterブランチも確認）にはNXP MCXシリーズ（MCXA153/MCXN947）のtarget定義（フラッシュ書き込みアルゴリズム込み）が一切存在しない——新しいチップファミリのため誰もupstreamしていない状態
- **突破口**: `debug.server`の値そのものは`"openocd"`固定だが、実際に呼び出す実行ファイルのパス（`debug.server.openocd.path`）は`boards.txt`/`platform.txt`側で完全に自由に指定できる。つまり「`openocd`を名乗るが中身はLinkServer自身のgdbserverを起動してgdbとの通信を中継するだけの小さな橋渡しプログラム」を用意すれば、`arduino-cli`は本物のOpenOCDだと思って呼び出すが、実際のデバッグはLinkServer（このチップに対して確実にサポートが効いている本物のツール、README.mdでも必須要件として案内済み）が担当する
- **`arduino-cli`が実際にどう起動するかを実機のgdbで検証**: `getDebugCommandLine()`のソースを読み、`target extended-remote | "<ServerPath>" -c "gdb_port pipe" -c "telnet_port 0"`という1本のgdb `-ex`コマンド文字列を組み立てていることを確認。実際に`arm-none-eabi-gdb`でこの文字列を試験的なプローブスクリプト相手に実行し、gdbが本当にシェル経由でこの文字列を正しくクォート解釈し、`["-c", "gdb_port pipe", "-c", "telnet_port 0"]`という素直なargvでサブプロセスを起動することを確認してから実装に着手（推測で進めず、まず機構を実証）
- **`gdb-bridge`の実装**（`hardware/nxp/mcx/tools/gdb-bridge/`）: Go製の小さなプログラム（`src/main.go`）。`LinkServer gdbserver <DEVICE> --gdb-port <port> --semihost-port -1`をバックグラウンドで起動（このコアのI/OはSerial経由でありセミホスティングは使わないため無効化）、そのTCPポートへの接続を確立できるまでリトライで待機し、確立後は自分自身の標準入出力（gdbがパイプしている側）とそのTCP接続の間で双方向にバイトを中継するだけ。LinkServer自身の標準出力・標準エラーは自分の標準エラーへ逃がす（gdbは標準出力の全バイトをGDB Remote Serial Protocolの一部として解釈するため、混ぜると通信が壊れる）。LinkServer実行ファイルの探索ロジックは`tools/upload.sh`/`upload.bat`と同じ規則（macOS: `/Applications/LinkServer*`最新版、Linux: `/usr/local/LinkServer/`優先→`/usr/local/LinkServer_*`最新版→PATH、Windows: `C:\NXP\LinkServer*`最新版）を独自に複製し常に同じインストールを見つけるようにした（バージョン比較は`sort -V`相当を自前実装、単純な文字列比較だと桁数の違うバージョン番号を誤って比較してしまうため）
- **ビルド・配布**: ローカルにGoツールチェーンが無かったため`brew install go`で導入。`GOOS`/`GOARCH`のクロスコンパイルで、このプロジェクトが既に配布しているツールチェーンと同じ5構成（darwin-arm64/amd64, linux-amd64/arm64, windows-amd64）向けに静的バイナリをビルド（`-ldflags="-s -w" -trimpath`、各2.6〜2.9MB）。ボードごとに接続先デバイス文字列（`MCXA153:FRDM-MCXA153` / `MCXN947:FRDM-MCXN947`、`boards.txt`の`build.linkserver_target`と同じ値）が異なるため、共有の`gdb-bridge-<os>-<arch>`バイナリをOSごとに選んで起動し、ボード別のデバイス文字列を第1引数として渡すだけの薄いランチャースクリプト（`launch-a153.sh`/`.bat`、`launch-n947.sh`/`.bat`）を新設。`boards.txt`にボードごとの`debug.server.openocd.path`（base=Unix向け`.sh`、`.windows`サフィックス=`.bat`）としてこのランチャーを指定、`platform.txt`に共通の`debug.executable`/`debug.toolchain=gcc`/`debug.toolchain.path`/`debug.toolchain.prefix=arm-none-eabi-`/`debug.server=openocd`を追加
- **実機での初回end-to-endテスト**: `arduino-cli debug --info`でプロパティ解決が意図通りであることを確認後、実機接続中のFRDM-MCXN947に対し`arduino-cli debug`をgdbコマンドスクリプトのstdinパイプ経由で実行。1回目は`monitor reset halt`が`Failed to reset to initial execution state - Ec(06). No image address available for soft reset.`で失敗し`continue`がブレークポイントに到達せずハング——LinkServerのgdbserverは素のアタッチ状態だと「どのイメージにリセットすべきか」を知らないため。gdbの`load`コマンド（ELFをターゲットへ転送＝GDB標準のフラッシュ書き込み手順）を先に実行する構成に変更したところ、`load`がLinkServerのフラッシュドライバ経由で正しく書き込み（46608 bytes, 60KB/sec）・PCがリセットベクタ（`0x318 <ResetISR>`）に正しく着地・`break loop`→`continue`で実際に`loop()`のブレークポイントにヒット・`pc`が期待通り`0x38bc <loop()>`を指すことを確認——ソースレベルデバッグ（`.ino`のファイル名・行番号解決込み）が実機で完全に機能することを実証。この結果から、Arduino IDE 2側の「デバッグ開始」操作は通常（他のCortex-M系ボード同様）内部で`load`相当の操作を自動的に行うはずと判断
- **Arduino IDE 2の「デバッグ」ボタンから実際に試したところ2段階のエラーが判明・解消**（`arduino-cli debug`のCLI直接実行とは別の、IDE組み込みのcortex-debug拡張機能——Arduino IDE 2にバンドルされている実体を`/Applications/Arduino IDE.app/.../plugins/cortex-debug/`で直接確認、`Marus/cortex-debug`のフォーク、実際にインストールされているバージョンで検証——を経由する、全く別の起動経路だったため）:
  1. **「At least one OpenOCD Configuration File must be specified.」**: IDEの起動前チェック（`arduino-cli`側ではなくIDEのフロントエンド側の検証）で、`debug.server.openocd.script`（またはscripts）が最低1つ設定されていないと弾かれる。`gdb-bridge`は引数を全く読まないため中身は無意味だが、プレースホルダーファイル`dummy-openocd.cfg`を新設し`platform.txt`の`debug.server.openocd.scripts_dir`/`.script`に設定して解消
  2. **「Failed to launch OpenOCD GDB Server: Timeout.」**: 実際にIDE組み込みのcortex-debug（バンドルされた`dist/debugadapter.js`を直接grep調査）のソースを読んで判明——`arduino-cli debug`のCLIコマンドが使うgdbのパイプ起動（`target extended-remote | "<path>" ...`）とは全く別の起動方式で、IDEは`serverArguments()`で本物のOpenOCD形式の引数（`-c "gdb_port <IDE が選んだポート番号>" -c "tcl_port ..." -c "telnet_port ..." -s <dir> -f <script>...`）を構築して`<ServerPath>`をサーバーとして直接起動し、その標準出力が`initMatch()`の正規表現`/Info\s:[^\n]*Listening on port \d+ for gdb connection/i`（実際の値も直接grepで確認）にマッチするまで待ってから、別途gdbをそのポートへTCP接続する、という設計だった。当初の実装（gdbのパイプ経由の中継のみ）はこの起動方式に非対応だったため、`gdb-bridge`を書き直し: 引数から`-c "gdb_port N"`を検出したら「サーバーモード」に切り替え、指定されたポート番号で自分自身がリッスンし、`Info : Listening on port N for gdb connections`という一致する行を出力してから、以降に来る接続をLinkServerの実際のgdbserverへ中継する
  - **書き直し中に発見した実機バグ**: サーバーモード実装の初期案では、LinkServerのgdbserverの起動確認を「一度TCP接続してすぐ切断する」プローブ方式で行っていたが、これを実機で試したところ**LinkServerのgdbserverは最初のTCPクライアントが切断した時点で（プローブによる切断であっても）デバッグセッション全体を終了してしまう**ことが判明（実機ログで`Wc: GDB stub ... terminating - GDB protocol problem: Pipe has been closed by GDB.` / `Gdbserver on port ... has closed`を確認）——その後IDE役の本物のgdbが接続しようとした時にはLinkServerは既に終了しており失敗する、という形で発覚。起動確認の方式をTCPプローブから「LinkServer自身の標準出力を監視し`GDB server listening on port`という行を検出する」方式に変更して解消（`cmd.StdoutPipe()`＋`bufio.Scanner`で行ごとに監視、検出後もログ行は継続して自分の標準エラーへ転送）
  - 修正後、`arm-none-eabi-gdb`から`target extended-remote localhost:<port>`でTCP接続する実機テスト（cortex-debugが実際に行うのと同じ接続方式）を実施——`load`（フラッシュ書き込み）→リセットベクタ着地→`break loop`→`continue`→実際にブレークポイントヒットまで完全動作を確認
  - 両起動経路（`arduino-cli debug`のパイプ方式、Arduino IDE 2 / cortex-debugのTCPサーバー方式）とも同一の`gdb-bridge`バイナリが自動判別（`-c "gdb_port N"`の有無で分岐）して両対応することを実機で確認済み
- まだ未実施: 実際にArduino IDE 2の「デバッグ」ボタンを最後まで押し切っての動作確認（ブレークポイント設定・ステップ実行・変数閲覧等、UIを通した一連の操作）——上記2件のエラーはいずれもユーザーからのスクリーンショット報告を受けて解消したもので、エラーが解消した後の実際のフルセッションはこれから確認予定

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
| Print/Stream抽象基底クラス（新設） | ✅ | v0.2.1で追加。実機確認済み——ハードウェア非依存のPrint派生クラス、Stream&への多態性、print()/println()の実バイト数返却、Printableのn+=p.print(x)イディオムすべて動作確認 |
| サードパーティライブラリ互換性（ArduinoJson/LiquidCrystal/DHT/NeoPixel/OneWire/Adafruit BusIO） | ✅ | Print/Stream新設・BitOrder型・microsecondsToClockCycles追加によりarduino-cli compile成功。Servoのみライブラリ側のアーキテクチャ非対応で不可（既知の限界） |
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
- **v0.4.0以降のソース構成**: `MCUXpresso_project/`ディレクトリは廃止（削除済み）。ソースの唯一の実体は`hardware/nxp/mcx/cores/arduino/`（両ボード共有）＋`hardware/nxp/mcx/variants/<board>/src/`（ボード固有）で、プリビルド`.a`のビルド・配置手順も不要になった——編集したソースはそのままarduino-cli/Arduino IDEのビルドに反映される（詳細は「`cores/arduino/`一本化・`platform.txt`書き換え完了」セクション参照）
- **xPackツールチェーン**: `~/.xpacktools/xpack-arm-none-eabi-gcc-14.2.1-1.1/`（`package_nxp_mcx_index.json`記載のものと同一バイナリ、チェックサム確認済み）
- **ローカルArduino IDE連携**: `~/Library/Arduino15/packages/nxp/hardware/mcx/0.2.0-dev`（v0.2.0リリース後に`0.1.9-dev`から改名）をこのリポジトリの`hardware/nxp/mcx/`へのシンボリックリンクとして設定済み（編集が即座に反映される）。ツールチェーンも`~/.xpacktools/`への symlink。`-dev`サフィックスにより、Boards Manager経由でインストールする実リリース版（`0.2.0`）とはディレクトリ名が衝突せず共存可能
- **注意（Boards Manager経由の実インストール検証時のハマりどころ）**: 上記symlink環境を無効化する際、`~/Library/Arduino15/packages/nxp`を同じ`packages/`直下で別名（例: `nxp.dev-backup`）にリネームしただけでは不十分 — arduino-cliは`packages/*`配下の全ディレクトリ名をpackager IDとして解釈するため、リネーム後も`nxp.dev-backup:mcx`という別パッケージとして「0.1.9-dev installed」表示が残ってしまう（`arduino-cli core list --all`で再現・特定）。無効化する際は`packages/`の外（例: スクラッチパッド等）に完全に退避すること。v0.2.0リリース後、この手順でBoards Manager経由のGitHubからの実インストールを検証済み

## GitHub Actions
- **Workflow**: `.github/workflows/update_package_index.yml`
- **役割**: GCCのsizeをHEADリクエストで取得、プラットフォームZIPのchecksum/sizeをダウンロードして計算・更新
- **既知の制限**: タグpush（`push: tags: '[0-9]+.[0-9]+.[0-9]+'`）で起動した場合、`actions/checkout`がdetached HEADでチェックアウトするため最後の`git push`が失敗する（過去のv0.1.6〜v0.2.0全リリースで再現）。実際のchecksum確定は、リリース後に`gh workflow run update_package_index.yml --ref main`（または Actions UI の "Run workflow"）で`main`ブランチに対し手動実行する必要がある。**リリース時は「タグpush→(失敗を確認)→mainに対してworkflow_dispatchを手動実行」の2段階が必須の手順**

### リリース前クロスプラットフォーム検証: ステージングブランチ方式（v0.3.1から採用）
これまでは「タグpush→`main`のchecksum確定→ユーザーが各OSで実機インストール検証」という順序で、`main`の`package_nxp_mcx_index.json`が確定してから初めて検証していた。ユーザーから「リリース前に各OSでのインストールを確認する方法はないか」と相談があり、以下の手順を提案・採用が決定:

1. いつも通り`gh release create`でリリースzipを添付（この時点でダウンロードURLは実在・安定する。`main`のインデックスをまだ更新していなくても関係ない）
2. `main`とは別に**ステージング用ブランチ**（例: `staging-0.3.1`）を作り、`package_nxp_mcx_index.json`だけをそこにpush——`platforms[0]`のurl/checksumを、今作ったリリースの実際の値に書き換えたもの
3. 各OS（macOS/Windows/Linux）で、Arduino IDEの**Additional Boards Manager URLsを一時的にこのステージングブランチのraw URL**に切り替えてインストール検証
4. 全OSで問題なければ、いつも通り`main`に対して`update_package_index.yml`を手動実行してchecksum確定

**利点**: 従来の手順だと、`main`にプレースホルダーchecksum付きの新バージョンエントリを一旦pushしてから確定させるまでの間、誰かが`main`経由でインストールを試みると失敗する可能性があった（短時間ではあるが）。ステージングブランチ方式なら、`main`のインデックスには常に検証済みの内容だけが載る状態を保てる。**v0.3.1で初適用・完了**——macOS/Windows/Linux全てでBoards Manager経由インストール〜動作確認まで成功、`main`のchecksumも確定済み。詳細は「v0.3.1リリース完了」セクション参照。念のため、`main`のchecksum確定後にmacOSで改めて本番URL（`.../main/package_nxp_mcx_index.json`）経由のインストールも再検証し、問題ないことを確認済み

---

## 開発ブランチ運用方針（v0.3.2から採用）
ユーザー指示: 「0.3.2の開発版をスタート。このバージョンから開発用ブランチを切って進める。ドキュメント関連の更新はmainブランチで。開発用ブランチは0.3.2-devにする」

- 各バージョンの開発作業（ソースコード変更、r01lib/arduino_layerの修正、サンプル追加等）は**`<version>-dev`という名前の専用ブランチ**（例: `0.3.2-dev`）上で行う。これは`prepare0.3.0`ブランチでのN947対応と同じ「開発用ブランチを切ってから最後に`main`へマージ」というパターンだが、ブランチ命名規則を`prepare<version>`から`<version>-dev`に変更し、以後この命名で統一する
- ローカル開発用symlink（`~/Library/Arduino15/packages/nxp/hardware/mcx/<version>-dev`）も、ブランチ名と同じ`<version>-dev`という命名になるため、今後はgitブランチ名とsymlink名が常に一致する（偶然ではなく意図した対応）

**訂正（同セッション内）**: 当初「ドキュメントのみの更新は`main`に直接コミットする」という方針で着手し、実際に`analogWriteFrequency()`関連のドキュメント更新（`PIN_MAPPING_*.md`/`API_COMPATIBILITY.md`/`CHANGELOG.md`）を`main`にコミット・pushしたが、ユーザーから指摘で撤回: 「今後は作業中のブランチでやる。そうでないとリリース版との整合が取れないから」。`main`はいつでもBoards Manager経由でユーザーが参照しうる「現在のリリース版」の実体であり、未リリースの`0.3.2-dev`の機能を説明するドキュメントを`main`に置くと、その機能がまだ存在しない状態のユーザーに向けて存在するかのような記述を見せてしまう——上記の「利点」は誤りだった。
- **現在の方針: ドキュメント（CLAUDE.mdも含む）も含めて、そのバージョンの開発作業はすべて`<version>-dev`ブランチ上で行い、リリース時に`main`へまとめてマージする**
- `main`にコミット済みだった該当ドキュメント更新（`833f949`）は`git revert`で取り消し（`9c0f9ad`）、同じ内容を`0.3.2-dev`へ`git cherry-pick`で移設（`7e20c89`）

---

---

## 残りのPendingタスク
1. ~~Linux対応の実機検証~~ **解消済み（v0.2.2で確定）**: v0.2.1リリース後の実機検証で、ファイル名の大文字小文字ミスマッチ（`arduino.h`/`Arduino.h`、`spi.h`/`SPI.h`）によりLinuxでビルドが失敗することが判明・修正し、v0.2.2としてリリース。Linux実機（Ubuntu系）でBoards Manager経由インストール〜Blinkスケッチのビルド〜書き込み〜実行まで成功を確認済み。README.md/TUTORIAL.md/TUTORIAL.ja.mdの「未検証」表記もすべて「macOS, Windows 11, Linuxで検証済み」に更新済み
2. マルチボード対応（MCXN947, MCXA156, MCXN236）— **N947は`prepare0.3.0`ブランチで完了**。GPIO/Serial/Wire/Wire1/SPI/analogRead/analogWrite/tone・noTone・MikroBusの`SPI1`/`Wire2`/`Serial1`まで実機検証済み、`README.md`の対応ボード表もA153と同じ✅に変更済み（ユーザー判断、2026-08-16）。残るはバージョン番号の更新とリリース手順（下記5）のみ。A156/N236は未着手
3. ~~`examples/tests/GPIO_NXP_Arduino`の不要なgitlinkエントリの整理~~ **解消済み**: `git ls-files --stage`で`160000`（gitlink）エントリが残っているのに`.gitmodules`が存在しないと判明（外部クローンの誤`git add`の名残）。`git rm --cached`でインデックスから除去し、他4つの外部ライブラリクローンと同様`.gitignore`に追加
4. ~~v0.3.0リリース~~ **完了**: 2026-08-16リリース。詳細は「リリース前最終チェックとv0.3.0リリース完了」セクション参照
5. ~~SDライブラリビルド時の`-Waddress-of-packed-member`警告~~ **解消済み（v0.3.1で対応）**: `platform.txt`の`compiler.cpp.flags`に`-Wno-address-of-packed-member`を追加して警告クラス自体を抑制。純粋な診断抑制フラグ（`-W`系）でコード生成には一切影響しないため、プリビルド`.a`の再ビルドや実機再検証は不要と判断——両ボードで`SDBitmapViewer`（`SD`ライブラリ使用）をコンパイルし、警告が完全に消えたことを確認
