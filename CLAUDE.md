# mcx-arduino-core 開発引き継ぎドキュメント

## プロジェクト概要
- **リポジトリ**: https://github.com/teddokano/mcx-arduino-core
- **内容**: NXP FRDM-MCXA153 / FRDM-MCXN947（いずれもCortex-M33）向けのArduino IDEボードサポートパッケージ。
  v0.4.0以降は**ソース配布方式**（プリビルド`.a`は廃止）で、`hardware/nxp/mcx/cores/arduino/`（両ボード共有）と
  `hardware/nxp/mcx/variants/<board>/src/`（ボード固有）が唯一の実体。編集すれば次のビルドにそのまま反映される
- **同梱ライブラリ**: `mcxPinState`（ピン所有状況のデバッグ表示）、`mcxRCServo`（RCサーボ）。
  どちらも別リポジトリが開発の本拠地で、`hardware/nxp/mcx/libraries/`配下はリリース時に同期する取り込みコピー
- **現在のリリース**: **v0.6.0**（2026-09-12）。macOS・Windows・Linuxの3プラットフォームで
  インストール〜ビルド〜アップロード〜IDE内蔵デバッガまで検証済み
- **開発中**: **0.7.0**（`0.7.0-dev`ブランチ）
- **リリースごとの変更点**: [CHANGELOG.md](CHANGELOG.md)
- **各リリースで何をやり、どこで詰まったかの詳細**: [docs/DEVELOPMENT_LOG.md](docs/DEVELOPMENT_LOG.md)
  （v0.1.5〜v0.6.0の全作業記録。自動では読み込まれないので、経緯が必要なときだけ開く）

---

## ドキュメントの地図
どこに何が書いてあるか。**この`CLAUDE.md`は「今この瞬間に効いているルールと状態」だけを持ち、
過去の経緯は`docs/DEVELOPMENT_LOG.md`に置く**という分担にしてある。

| 知りたいこと | 見る場所 |
|---|---|
| 対応APIの一覧・未対応項目 | [API_COMPATIBILITY.md](API_COMPATIBILITY.md) |
| ピン配置（ボード別） | [PIN_MAPPING_A153.md](PIN_MAPPING_A153.md) / [PIN_MAPPING_N947.md](PIN_MAPPING_N947.md) |
| リリースごとの変更点 | [CHANGELOG.md](CHANGELOG.md) |
| 使い方の入門 | [TUTORIAL.md](TUTORIAL.md) / [TUTORIAL.ja.md](TUTORIAL.ja.md) |
| クラス・関数のリファレンス | `docs/api/`（Doxygen生成、リリース前に再生成する） |
| ボードを追加する手順 | [docs/porting_a_new_board.md](docs/porting_a_new_board.md) |
| 上級者向けの個別トピック | `docs/advanced_sdk_tuning.md` / `docs/advanced_r01lib_i3c.md` / `docs/mcxpinstate_guide.md` |
| リリース前の実機チェック手順 | [examples/release_check/README.md](examples/release_check/README.md) |
| 過去の作業記録・バグの切り分け経緯 | [docs/DEVELOPMENT_LOG.md](docs/DEVELOPMENT_LOG.md) |

---

## 繰り返し踏んだ罠と教訓
**複数回踏んだもの・踏むと原因から遠い場所で失敗するものだけ**を挙げる。
詳しい経緯は[docs/DEVELOPMENT_LOG.md](docs/DEVELOPMENT_LOG.md)の該当バージョンの節にある。

### コードを書くとき
- **ブロックコメント内の`*/`でコメントが早期終了する**（4回踏んだ）。`GPIO_Type*/bitmask`・`ARD_D*/ARD_A*`・
  `D0-D19/MB_*/SPI_*/ARD_*`・`release_check/NN_*/`のような表記が原因で、
  **実際のミスとは似ても似つかない場所でビルドエラーになる**。
  現在は`.github/scripts/check_repo_hygiene.py`の`comment-terminator`が毎push検出する
- **ALT（mux）値を`pin_mux.c`のコメント内の位置カウントで導出しない**（3回踏んだ）。
  `CLKOUT`のようにマルチプレクサのスロットを消費しない項目が混ざると1つずれる。
  権威ある情報源は**Zephyrのpinctrlヘッダ**（`modules/hal/nxp/dts/nxp/mcx/MCX*-pinctrl.h`、シリコン正確）。
  **これは2つのcheckoutに存在する**（`~/dev/ArduinoCore/`と`~/dev/slint-zephyr/`）——
  片方だけ見て「無い」と結論しないこと
- **「ペアごとに違う値を定数1つで済ませた」形は、その定数が通る経路が1本しかないと生き残る**。
  A153のI2C（4ピンペアに対しALTが1つ）と`cs_manual_control()`のALT決め打ちが実例で、
  どちらも**到達不能なパスだったため動かしても見つからず**、先回りの全数監査で初めて出た
- **既存ファイルをPythonで書き換えるときは行末コードを確認する**。
  同一ディレクトリ内でもLF/CRLFが混在している（`i3c.h`=LF、`i3c.cpp`=CRLF）。
  `open(p, newline='')`で読み書きするか、そもそも`Edit`ツールを使う
- **`io.h`は5ボード分のブロックが1ファイルに並んでいる**。範囲を指定して読む前に
  必ず`#elif CPU_*`の位置を確認すること（A156のブロックをA153と取り違えて報告した前例あり）

### 原因を切り分けるとき
- **クロックのattachが`mcu.cpp`/`clock_config.c`に無くても「漏れている」とは限らない**（2回踏んだ）。
  **インスタンス単位の設定はドライバ側が持っていることがある**——`Serial.cpp`の`s_pinMap[]`が
  `kFRO12M_to_FLEXCOMM5`/`kFRO12M_to_LPUART2`を持ち、コンストラクタの`_setup_clock()`で適用される。
  `mcu.cpp`をgrepして「無い」を示すのは、探す場所が違えば何も示さない
- **エラーコードは最後の1バイトまで突き合わせる**。`MAKE_STATUS(group,code) = group*100+code`が
  `uint8_t`に切り詰められるので、`220`=`kStatus_I3C_Busy`(7900)、`222`=`kStatus_I3C_Nak`(7902)、
  `134`=`kStatus_LPI2C_Nak`(902)。**「NAKですらなくバスがビジー」と分かれば、コードより先に配線を疑える**
  （実際にジャンパの挿しっぱなしによる短絡だった）
- **レジスタ幅から導いた理論値より実測を信じる**。I3CのI2C_MODE下限を
  ODBAUDのビット幅から10kHzと見積もったが、実機は56kHzで頭打ちだった
- **再現用の引数はドキュメントの記憶からではなく実物のログから取る**。
  gdb-bridgeがIDEから起動できなかった件は、CLAUDE.mdに記録してあった引数の形が実物と違っていたのが原因で、
  決定打はユーザーが貼ったIDEのログそのものだった
- **NXPの回路図PDFは`pdftotext`で読まない**（2カラムレイアウトで無関係な項目が同じ行に混ざる）。
  `Read`ツールでページを画像として開いて視認すること。回路図は`ref/`にある（`.gitignore`対象＝リポジトリ管理外）

### 検証するとき
- **コンパイルが通ったことを動作の証拠にしない**。ソース配布方式へ移行したとき、
  114サンプル×2ボードが通ったあと実機初回でハングした（`LPSPI0`のクロック未供給）
- **回帰スイープは逐次で実行する**。`xargs -P 4`は**偽の失敗を出す**——
  4プロセスが同じ`core.a`ビルドキャッシュを作り合って競合し、
  `hello_world`すら落ちるうえ**実行のたびに失敗する顔ぶれが変わる**。
  CIの`compile_examples.sh`が逐次なのは正しい
- **フローティングピンを読むだけのテストは、機能が壊れていても偶然通る**。
  `INPUT_PULLDOWN`はv0.2.1で「実機確認済み」としながら実際には一度も効いておらず、
  原因は「配線なしのピンが`digitalRead()`でたまたまLOWを返す」テスト設計だった。
  **外から逆方向に駆動してから解放し、内部プルが引き戻すかを見る**形にして初めて実証できた

### リリースするとき
- **リリースzipは単一のトップレベルディレクトリでラップする**（`git archive --prefix=mcx/`）。
  Boards Managerのインストーラーがzip直下に1つだけのラッパーディレクトリを要求する
- **git追跡外のファイルは`git archive`から黙って消える**。しかもローカル開発用symlinkは作業ツリーを
  指しているので**ローカル検証は全部通り、壊れているのは配布物だけ**になる。
  ユーザーのグローバル`~/.config/git/ignore`の`*.exe`でWindows用exeが落ちかけた実例がある。
  現在は`platform-paths`チェックが「存在し、かつ追跡下にある」ことを毎push確認する
- **`update_package_index.yml`は既存エントリのchecksum/sizeを書き換えるだけで、新規エントリを作らない**（2回踏んだ）。
  実行前に`package_nxp_mcx_index.json`へ対象バージョンのプレースホルダーエントリを追加しておくこと。
  なお**既存エントリを上書きせず配列に追加する**（過去バージョンも選べるようにするため）。
  現在は`--release`時の`package-index-entry`チェックが検出する
- **バージョンを上げると`mcxPinState`の`#warning`が再発火する**（設計どおり）。
  `arduino_io.h`が前回の照合時点から変わっていないことを
  `git log <前回コミット>..HEAD -- .../arduino_io.h`が空であることで確認してから、
  `MCXPINSTATE_VERIFIED_AGAINST`を上げる。**バンドル側と上流リポジトリの両方に反映すること**

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

## 0.5以降のロードマップ方針（v0.4.2開発中に策定）
ユーザーから「0.5, 0.6, 0.7, 0.8ぐらいまでのプランで、足回りを固めるとかサポートするボードの数を増やすとか、数バージョン先まで手堅く進めるための順番を考えて」と依頼。以下の順序で合意（0.7以降は「まだ先の話なのであとで考える」として保留）:

- **0.5: CI（実機不要の足回り）＋ 0.4.2で予定していた確認ポイントの統合**。現状workflowは`update_package_index.yml`だけで、全サンプル×両ボードの回帰コンパイルは毎回手動（1セッションで5回手で回すこともある）——これをGitHub Actionsで自動化する。あわせて、これまで繰り返し踏んできた機械的ミス（`package_nxp_mcx_index.json`のプレースホルダー追加忘れ＝2回、`*/`によるコメント早期終了＝4〜5回、`platform.txt`の`version`系3行と`Doxyfile`の`PROJECT_NUMBER`の齟齬）もチェック対象にする。**ボードを増やすと検証コストが2倍→3倍→4倍に効くので、増やす前に自動化するという順序**（ユーザーの提案で、0.4.2の残作業＝`Wire2`のクロック実測・拡張した`test_combined_peripherals`の実機確認も0.5に含める方針）
- **0.6: 先回りの監査＋ポーティング手順の明文化**。`SPI`/`SPI1`/`Wire2`で3回続けて出た「`clock_config.c`が触れていないFlexCommがリセットデフォルトの遅いクロック源のまま残る」を、次のバグ報告を待たずに全ペリフェラル一括で洗う。N947対応の経験を`docs/porting_a_new_board.md`として書き出し、その過程で「ボード追加時に手で直す必要がある箇所」（`arduino_io.h`の`NUM_ANALOG_INPUTS`等のボード分岐、mcxPinStateの`ALIAS_NAMES`/`KNOWN_INSTANCES`、gdb-bridgeのボード別Windows exe）を可視化・削減する
- **0.7以降: 保留**（ボード追加が有力だが、時期が来てから判断）

### ボード追加候補の実態調査（この検討で判明したこと）
- **C444は「半分できている」わけではない**: `r01lib`に`CPU_MCXC444VLH`分岐はあるが、(1) `r01lib.h`で`I3C_SUPPORTED`が定義されない＝I3Cペリフェラル自体が無い、(2) `AnalogIn.h`/`PwmOut.h`にはC444分岐が**一切無い**（A153とN947のみ）。しかもC444はKinetis系でLPADC/FlexPWMではなくADC16/TPMという別ペリフェラルのため、移植ではなく新規実装が必要——`analogRead`/`analogWrite`/`analogWriteFrequency`/`tone`が全部未実装。I2Cも`fsl_lpi2c`ではなく`fsl_i2c`、Cortex-M0+でFPU無し。GPIO/Serial/I2C/SPIの骨組みだけがある状態で、単独で1バージョン分の規模（ユーザーも「その通り．手間が大きい」と同意）
- **オンボードセンサーの違い（ユーザーからの情報）**: C444とN236にはI3Cセンサーが無く、代わりに**加速度センサ**が載っている。したがってこの2ボードでの実機検証は、既存2ボードが使っている「`Wire1`(I3C)経由のP3T1755温度センサー」ではなく、プレーンI2C経由の加速度センサを使う形になる——`test_combined_peripherals`・`release_check/01`等、現状`Wire1`+`P3T1755`前提で書かれているサンプル群の設計に影響するため、0.6のポーティング手順書で扱うべき項目
- **A156はA153の兄弟、N236はN947の兄弟**で流用が効く。3枚目を足すならA156が最も安い（N236は上記の加速度センサの件が追加で乗る）
- **回路図が`ref/`に揃っている**（ユーザーが配置。`ref/`は`.gitignore`対象＝リポジトリ管理外なので、この記録が無いと存在に気づけない）: `FRDM-MCXA153.pdf`・`FRDM-MCXA156.pdf`・`FRDM-MCXC444.pdf`・`FRDM-MCXN236.pdf`・`FRDM-MCXN947SH.pdf`。ボード追加時のピン確認で参照する。**注意: NXPの回路図PDFは2カラムレイアウトのため`pdftotext`でのテキスト抽出は信頼できない**（無関係な項目が同じ行に混ざる）——N947のanalogWriteピン特定時に確立した通り、`Read`ツールでページを画像として直接開いて視認する方式を使うこと。あわせて`ref/r01lib_pin_table.xlsx`（各ピンのavailability一覧）も利用可能

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
- **v0.4.0以降のソース構成**: `MCUXpresso_project/`ディレクトリは廃止（削除済み）。ソースの唯一の実体は`hardware/nxp/mcx/cores/arduino/`（両ボード共有）＋`hardware/nxp/mcx/variants/<board>/src/`（ボード固有）で、プリビルド`.a`のビルド・配置手順も不要になった——編集したソースはそのままarduino-cli/Arduino IDEのビルドに反映される（詳細は[docs/DEVELOPMENT_LOG.md](docs/DEVELOPMENT_LOG.md)の「`cores/arduino/`一本化・`platform.txt`書き換え完了」節）
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

**利点**: 従来の手順だと、`main`にプレースホルダーchecksum付きの新バージョンエントリを一旦pushしてから確定させるまでの間、誰かが`main`経由でインストールを試みると失敗する可能性があった（短時間ではあるが）。ステージングブランチ方式なら、`main`のインデックスには常に検証済みの内容だけが載る状態を保てる。**v0.3.1で初適用・完了**——macOS/Windows/Linux全てでBoards Manager経由インストール〜動作確認まで成功、`main`のchecksumも確定済み。詳細は[docs/DEVELOPMENT_LOG.md](docs/DEVELOPMENT_LOG.md)の「v0.3.1リリース完了」節。念のため、`main`のchecksum確定後にmacOSで改めて本番URL（`.../main/package_nxp_mcx_index.json`）経由のインストールも再検証し、問題ないことを確認済み

---

## 開発ブランチ運用方針（v0.3.2から採用）
ユーザー指示: 「0.3.2の開発版をスタート。このバージョンから開発用ブランチを切って進める。ドキュメント関連の更新はmainブランチで。開発用ブランチは0.3.2-devにする」

- 各バージョンの開発作業（ソースコード変更、r01lib/arduino_layerの修正、サンプル追加等）は**`<version>-dev`という名前の専用ブランチ**（例: `0.3.2-dev`）上で行う。これは`prepare0.3.0`ブランチでのN947対応と同じ「開発用ブランチを切ってから最後に`main`へマージ」というパターンだが、ブランチ命名規則を`prepare<version>`から`<version>-dev`に変更し、以後この命名で統一する
- ローカル開発用symlink（`~/Library/Arduino15/packages/nxp/hardware/mcx/<version>-dev`）も、ブランチ名と同じ`<version>-dev`という命名になるため、今後はgitブランチ名とsymlink名が常に一致する（偶然ではなく意図した対応）

**訂正（同セッション内）**: 当初「ドキュメントのみの更新は`main`に直接コミットする」という方針で着手し、実際に`analogWriteFrequency()`関連のドキュメント更新（`PIN_MAPPING_*.md`/`API_COMPATIBILITY.md`/`CHANGELOG.md`）を`main`にコミット・pushしたが、ユーザーから指摘で撤回: 「今後は作業中のブランチでやる。そうでないとリリース版との整合が取れないから」。`main`はいつでもBoards Manager経由でユーザーが参照しうる「現在のリリース版」の実体であり、未リリースの`0.3.2-dev`の機能を説明するドキュメントを`main`に置くと、その機能がまだ存在しない状態のユーザーに向けて存在するかのような記述を見せてしまう——上記の「利点」は誤りだった。
- **現在の方針: ドキュメント（CLAUDE.mdも含む）も含めて、そのバージョンの開発作業はすべて`<version>-dev`ブランチ上で行い、リリース時に`main`へまとめてマージする**
- `main`にコミット済みだった該当ドキュメント更新（`833f949`）は`git revert`で取り消し（`9c0f9ad`）、同じ内容を`0.3.2-dev`へ`git cherry-pick`で移設（`7e20c89`）

---

## リリース準備チェックリスト（v0.4.0から採用）
v0.4.0の`main`マージ直前、ユーザーから「今回のリリース準備の手順を記録しておく。今後のリリース準備では必ずこれらを行うこと」と指示。`<version>-dev`ブランチでの開発が一区切りつき、`main`へマージする前に**必ず**、以下を順番に実施する（実施済みかどうかに関わらず、リリースのたびに毎回やり直す）:

1. **`CHANGELOG.md`の`[Unreleased]`セクションを補完してから確定**: そのバージョンの開発期間中に`CLAUDE.md`へ記録してきた内容（新機能・変更・実バグ修正）を漏れなく洗い出し、`[Unreleased]`に追記——特にセッション中盤以降に追加された機能（今回で言えば`mcxPinState`・`examples/release_check/`・`docs/`配下の上級者向けガイド等）は、CHANGELOGの初稿作成時点でまだ存在しておらず抜け落ちやすいので要注意。全項目を追記し終えてから`[Unreleased]`→`[<version>] - <日付>`に書き換えて確定する
2. **ドキュメント全体の再監査**: Explore agentに、全`.md`ファイル（README/README.ja/TUTORIAL/TUTORIAL.ja/API_COMPATIBILITY/CHANGELOG/PIN_MAPPING_*/LICENSE/`docs/`配下/`variants/*/README.md`/`examples/release_check/README.md`）を対象にした網羅監査を依頼する。特に開発期間中にリネーム・統合・移動したファイル/サンプル名/パスへの追従漏れ（古い名前の残存、リンク切れ）を機械的に洗い出すのが目的——`CLAUDE.md`自身の過去形の言及（開発日誌としての性質上正しい）は対象外としてよい
3. **`docs/api/`（Doxygen）の再生成**: `doxygen Doxyfile`を実行し、直近のソース変更（コメントを含む）が反映された状態にする。`docs/api/index.html`のタイムスタンプより新しいソースファイルがないか確認してから判断するとよい
4. **ライセンスチェック**: `LICENSE`全文を読み返し、開発期間中に追加した新規ツール/ファイル（外部プロジェクトのソースを読んで挙動を参考にしたもの含む）で、帰属記載が漏れているものがないか確認する。「コードは一切コピーしていないが、他プロジェクトのソースを読んで互換動作を実装した」ケースは、コピーでなくても透明性のため記載する、というこのプロジェクトの既存の判断基準（`upload.sh`のArduinoCore-zephyr参照等）を毎回適用する
5. **その他の機械チェック**: `git status`のクリーンさ、自コードの`TODO`/`FIXME`/`XXX`残存、GitHub Issuesのオープン状態、新規追加した実行ファイル・設定ファイル（今回なら`gdb-bridge`バイナリ群・`boards.txt`の`debug.*`設定）の整合性、`package_nxp_mcx_index.json`の妥当性（新バージョンエントリはまだ追加しない——それは実際のリリース作業段階）、リリースzipサイズの見積り、「暫定」「未確認」「実機確認待ち」等の古い表現が現行ドキュメントに残っていないか
6. **全サンプル×両ボードの回帰コンパイルスイープ**（`examples/Arduino_compatible_API`・`Arduino_incompatible_API`・`release_check`配下の全`.ino`）を、上記1〜5の変更後に最終確認として実行し、新規リグレッションがないことを確認する
7. **`examples/release_check/`を実機で通す（両ボード）**——**コンパイルが通ったことを動作の証拠にしない**。項目6はコンパイルだけで、実行時にしか出ない不具合は一切見ていない（v0.4.0のソース配布移行では114サンプル×2ボードが通ったあと実機初回でハングした）。グループは`examples/release_check/README.md`の表に従い、番号体系は**`0n`=配線も外部部品も不要／`1n`=ジャンパ配線のみ／`2n`=外部ライブラリ・モジュール・ボードが必要**:
   - **`0n`**（`01`〜`05`）: 配線不要。`05`はN947限定
   - **`1n`**（`11`〜`14`）: ジャンパのみ。`11`のSerial1配線は**ボードで違う**（A153=D0-D1、N947=MikroBus `MB_TX`-`MB_RX`）。`14`（`setWireTimeout`）は両ボード共通で`D19`-`D8`＋`D18`-`D7`
   - **`2n`**（`21`〜`23`）: 外部`P3T1755.h`＋MikroBus配線／外部LM75系センサー／`Waveshare_TFT_Touch`の`SDBitmapViewer`。`21`/`22`は**CIスタブではなく実物のライブラリ**を`--library`で指定すること
   - **2枚同時接続での書き込みも各プラットフォームで1回**: A153とN947を両方つなぎ、ポートを切り替えて両方に書き込めること。
     `upload.sh`/`upload.bat`はポートのUSBシリアル番号（`{upload.port.properties.serialNumber}`）を
     LinkServerの`--probe`に渡すので、**Windows/Linuxのポート検出がこの番号を同じ形で返すか**が肝。0.7.0ではmacOSでしか確認していない
   - **IDE内蔵デバッガも各プラットフォームで1回**（`gdb-bridge`の起動経路はOSごとに別物——macOS/Linuxは`launch.sh`から`uname -s`で選ぶ別バイナリ＋別の`findLinkServer()`分岐、Windowsは共有exeを直接起動）

この7項目が終わってはじめて「`main`へのマージ」以降の既存のリリース手順に進む: `main`マージ→リリースzip作成→GitHub Release作成→**ステージングブランチ（`staging-<version>`）でのmacOS/Windows/Linux 3プラットフォーム検証**（v0.3.1から採用、「リリース前クロスプラットフォーム検証」節参照）→問題なければ`main`に対して`update_package_index.yml`を手動実行しchecksum確定。**このステージングブランチでの検証は必ず実施する——スキップしてよい状況は無い**。今回（v0.4.0）は新規追加のIDE内蔵デバッガ（`gdb-bridge`）がmacOSでしか実機確認できていないため、ステージング検証時に「インストール→ビルド→アップロード」の従来チェックに加えて「Windows/LinuxでもIDEのDebugボタン→ブレークポイント→ステップ実行を試す」を追加すること

---

## 残りのPendingタスク
1. ~~Linux対応の実機検証~~ **解消済み（v0.2.2で確定）**: v0.2.1リリース後の実機検証で、ファイル名の大文字小文字ミスマッチ（`arduino.h`/`Arduino.h`、`spi.h`/`SPI.h`）によりLinuxでビルドが失敗することが判明・修正し、v0.2.2としてリリース。Linux実機（Ubuntu系）でBoards Manager経由インストール〜Blinkスケッチのビルド〜書き込み〜実行まで成功を確認済み。README.md/TUTORIAL.md/TUTORIAL.ja.mdの「未検証」表記もすべて「macOS, Windows 11, Linuxで検証済み」に更新済み
2. マルチボード対応（MCXN947, MCXA156, MCXN236）— **N947は`prepare0.3.0`ブランチで完了**。GPIO/Serial/Wire/Wire1/SPI/analogRead/analogWrite/tone・noTone・MikroBusの`SPI1`/`Wire2`/`Serial1`まで実機検証済み、`README.md`の対応ボード表もA153と同じ✅に変更済み（ユーザー判断、2026-08-16）。残るはバージョン番号の更新とリリース手順（下記5）のみ。A156/N236は未着手
3. ~~`examples/tests/GPIO_NXP_Arduino`の不要なgitlinkエントリの整理~~ **解消済み**: `git ls-files --stage`で`160000`（gitlink）エントリが残っているのに`.gitmodules`が存在しないと判明（外部クローンの誤`git add`の名残）。`git rm --cached`でインデックスから除去し、他4つの外部ライブラリクローンと同様`.gitignore`に追加
4. ~~v0.3.0リリース~~ **完了**: 2026-08-16リリース。詳細は[docs/DEVELOPMENT_LOG.md](docs/DEVELOPMENT_LOG.md)の「リリース前最終チェックとv0.3.0リリース完了」節
5. ~~SDライブラリビルド時の`-Waddress-of-packed-member`警告~~ **解消済み（v0.3.1で対応）**: `platform.txt`の`compiler.cpp.flags`に`-Wno-address-of-packed-member`を追加して警告クラス自体を抑制。純粋な診断抑制フラグ（`-W`系）でコード生成には一切影響しないため、プリビルド`.a`の再ビルドや実機再検証は不要と判断——両ボードで`SDBitmapViewer`（`SD`ライブラリ使用）をコンパイルし、警告が完全に消えたことを確認
