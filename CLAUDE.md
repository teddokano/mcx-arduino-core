# mcx-arduino-core 開発引き継ぎドキュメント

## プロジェクト概要
- **リポジトリ**: https://github.com/teddokano/mcx-arduino-core
- **内容**: NXP FRDM-MCXA153 / FRDM-MCXN947 / FRDM-MCXA156（0.8.0から）/ FRDM-MCXN236（0.9.0から。ここまではCortex-M33）/ FRDM-MCXC444（0.10.0から。Cortex-M0+）向けのArduino IDEボードサポートパッケージ。
  v0.4.0以降は**ソース配布方式**（プリビルド`.a`は廃止）で、`hardware/nxp/mcx/cores/arduino/`（全ボード共有）と
  `hardware/nxp/mcx/variants/<board>/src/`（ボード固有）が唯一の実体。編集すれば次のビルドにそのまま反映される
- **同梱ライブラリ**: `mcxPinState`（ピン所有状況のデバッグ表示）、`mcxRCServo`（RCサーボ）、`EEPROM`（0.7.0で追加）、`Servo`（0.10.0で追加、公式のServoライブラリと同じAPI。0.12.0からPWMの無いピンもタイマーの割り込みで）、`MsTimer2`と`TimerOne`（0.12.0で追加、AVR向けの同じ名前のライブラリと同じAPI。TimerOneのPWMの関数は無い）、`mcxTicker`（0.12.0で追加、ESP32・ESP8266のコアの`Ticker`と同じAPI）。
  前の2つは別リポジトリが開発の本拠地で、`hardware/nxp/mcx/libraries/`配下はリリース時に同期する取り込みコピー。
  `EEPROM`はこのリポジトリが本体（フラッシュの配置がリンカスクリプトと一体なので）。`Servo`もこのリポジトリが本体で、コアの`analog_pwm_out()`（`arduino_analog.h`）でピンの`PwmOut`を受け取り、マイクロ秒でパルス幅を設定する（`analogWriteResolution()`に触らない）。PWMの無いピンはタイマーの割り込みでGPIOを上げ下げする（下の`Servo`の項目）
- **現在のリリース**: **v0.12.0**（2026-11-01）。リリースzipはSHA-256 `4bc86783d5f4aea44647e8f74b53c3f4454cbef1e60960e3d70fae3b4d3ba9ff`、18890777 bytes。タグ`0.12.0`は`68ba2fb`。前の版は0.11.0（2026-10-25）、0.10.0（2026-10-18）、0.10.1（Fritzingの部品だけ）。
- **開発中**: なし（次の版は未定。`0.12.0-dev`は`main`と同じ）。
- **リリースごとの変更点**: [CHANGELOG.md](CHANGELOG.md)
- **各リリースで何をやり、どこで詰まったかの詳細**: [docs/DEVELOPMENT_LOG.md](docs/DEVELOPMENT_LOG.md)
  （v0.1.5〜v0.12.0の全作業記録と、CLAUDE.mdから移した記述。自動では読み込まれないので、経緯が必要なときだけ開く）

---

## ドキュメントの地図
どこに何が書いてあるか。**この`CLAUDE.md`は「今この瞬間に効いているルールと状態」だけを持ち、
過去の経緯は`docs/DEVELOPMENT_LOG.md`に置く**という分担にしてある。

| 知りたいこと | 見る場所 |
|---|---|
| 対応APIの一覧・未対応項目 | [API_COMPATIBILITY.md](API_COMPATIBILITY.md) |
| ピン配置（ボード別） | [PIN_MAPPING_A153.md](PIN_MAPPING_A153.md) / [PIN_MAPPING_N947.md](PIN_MAPPING_N947.md) / [PIN_MAPPING_A156.md](PIN_MAPPING_A156.md) / [PIN_MAPPING_N236.md](PIN_MAPPING_N236.md) / [PIN_MAPPING_C444.md](PIN_MAPPING_C444.md) |
| リリースごとの変更点 | [CHANGELOG.md](CHANGELOG.md) |
| 使い方の入門 | [TUTORIAL.md](TUTORIAL.md) / [TUTORIAL.ja.md](TUTORIAL.ja.md) |
| クラス・関数のリファレンス | `docs/api/`（Doxygen生成、リリース前に再生成する） |
| LLM・AIのツール向けの案内 | [docs/llms.txt](docs/llms.txt)（Pagesで`https://teddokano.github.io/mcx-arduino-core/llms.txt`。0.12.0から） |
| ボードを追加する手順 | [docs/porting_a_new_board.md](docs/porting_a_new_board.md) |
| 上級者向けの個別トピック | `docs/advanced_sdk_tuning.md` / `docs/advanced_r01lib_i3c.md` / `docs/mcxpinstate_guide.md` |
| リリース前の実機チェック手順 | [examples/release_check/README.md](examples/release_check/README.md) |
| 過去の作業記録・バグの切り分け経緯 | [docs/DEVELOPMENT_LOG.md](docs/DEVELOPMENT_LOG.md) |
| ボード別の前提・決定（A156 / N236 / C444） | [docs/dev_notes/board_A156.md](docs/dev_notes/board_A156.md) / [board_N236.md](docs/dev_notes/board_N236.md) / [board_C444.md](docs/dev_notes/board_C444.md)（そのボードの分岐を触る前に読む） |
| 各版の経緯、0.12.0の機能（タイマー・`Servo`・`mcxTicker`・`mcx-discovery`など）の仕様と決定、1.0で固定すること | [docs/dev_notes/releases_and_decisions.md](docs/dev_notes/releases_and_decisions.md)（その機能を触る前に読む） |
| ローカル開発環境、GitHub Actions、ステージング方式、ブランチ運用 | [docs/dev_notes/workflow.md](docs/dev_notes/workflow.md) |
| リリース準備チェックリスト（7項目）、索引の構成 | [docs/dev_notes/release_checklist.md](docs/dev_notes/release_checklist.md)（リリース準備に入るとき、全文を読む） |
| Pendingタスクの全文 | [docs/dev_notes/pending_tasks.md](docs/dev_notes/pending_tasks.md) |
|リリース1.0.0にむけてやっておくべきこと|[memo/notes_toward_1.0.0.md](memo/notes_toward_1.0.0.md)|
| チップのレジスタ・ペリフェラルの仕様 | NXPのドキュメントのMCPサーバー**ChipDocs**（2026-10-14に接続）。RMはA153=`MCXAP64M96FS3RM`、A156=`MCXAP100M96FS6RM`、N947=`MCXNP184M150F70RM`（MCX NのRM。N23xは対象外）。N236はデータシート`MCXN23x`とセキュリティRMだけ。C444はChipDocsに無く、ユーザーが`ref/MCXC444-RM.pdf`（RM Rev.1）と`ref/MCXC444-DS.pdf`（データシート Rev.4）を置いた（2026-10-17。RMは1段組みなので`pdftotext -layout`で読める）。ボードの回路図は`ref/`（ページを画像で読む）、ピンのALTはZephyrのpinctrlヘッダで確かめる |

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
- **リセットで途中で切られたフラッシュの消去・書き込みは、読むとバスフォールトになることがある**（N947、0.7.0の`EEPROM`）。
  ECCが合わなくなるためで、失敗は切られた時点ではなく**次の起動で最初にその領域を読んだ所**で起きる
  （当時はHardFaultがシリアルに何も出さずにハングした。0.7.0からは`error: HardFault: ...`とPCが出る）。
  書き込みを伴うフラッシュ領域を起動時に読むコードは、`FAULTMASK`＋`CCR.BFHFNMIGN`で読んで`CFSR`を見る（`EEPROM.cpp`の`read_flash()`）
- **A156はリセット後、ほぼ全ピンの入力バッファ（`PCR.IBE`）が切れている**（0.8.0で実機確認）。
  SDKのテンプレートの`pin_mux.c`はデバッグUARTのピンしか入れないので、そのままだと`digitalRead()`が常に0、
  `Wire`は最初の転送で止まる（LPI2CがSCLのHighを見られない）。A153では起きない。
  ただし**`AnalogIn`は全ボードでアナログピンのIBEを切る**ので、A153・N947でも`analogRead()`のあとの`pinMode()`では`digitalRead()`が0のままだった（0.8.0で発見・修正）。
  いまは`DigitalInOut`のコンストラクタと`pin_mux()`が、MCXの3チップすべてでIBEを入れる。
  新しいボードで「入力だけ読めない」「I2Cが最初の転送で止まる」ときは、まずPCRの値を読む
- **`io.h`は5ボード分のブロックが1ファイルに並んでいる**。範囲を指定して読む前に
  必ず`#elif CPU_*`の位置を確認すること（A156のブロックをA153と取り違えて報告した前例あり）

### 原因を切り分けるとき
- **クロックのattachが`mcu.cpp`/`clock_config.c`に無くても「漏れている」とは限らない**（2回踏んだ）。
  **インスタンス単位の設定はドライバ側が持っていることがある**——`Serial.cpp`の`s_pinMap[]`が
  `kFRO12M_to_FLEXCOMM5`/`kFRO12M_to_LPUART2`を持ち、コンストラクタの`_setup_clock()`で適用される。
  `mcu.cpp`をgrepして「無い」を示すのは、探す場所が違えば何も示さない
- **エラーコードは最後の1バイトまで突き合わせる**（0.10.0まで。0.11.0から`endTransmission()`はAVRの0〜5を返し、
  切り詰める前のSDKのコードは`Wire.lastStatus()`で読める）。`MAKE_STATUS(group,code) = group*100+code`が
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
- **回帰スイープを並列にするときは、プロセスごとにビルドキャッシュを分け、`--build-path`は使わない**。`xargs -P 4`は**偽の失敗を出した**——
  4プロセスが同じ`core.a`ビルドキャッシュを作り合って競合し、
  `hello_world`すら落ちるうえ**実行のたびに失敗する顔ぶれが変わる**。
  一方で`--build-path`を渡すと、arduino-cliはスケッチごとにコアをビルドし直す（2026-10-24に、並列のfullが逐次より遅かった原因）。
  `compile_examples.sh`・`compile_third_party.sh`は、走らせるごとに自分のキャッシュ（`.noindex`のフォルダー）を作り、スケッチごとに`sketches/`だけを消す。
  ローカルでは`.github/scripts/compile_all_boards.sh full --queue 10`が速い（全ボードのビルドを1つの一覧にして10本ずつ流す。fullで約8分。ボードごとの並列は約10.5分、third_partyは約11分）。
  1つのボードの中も`SWEEP_JOBS=N`で並列にできる（`.github/scripts/run_builds.sh`。workerごとにキャッシュを分け、最初の1本で作ったコアのキャッシュを複製する。CIは`nproc`）
- **arduino-cliのビルドキャッシュはスケッチのパスごとに1つで、ボードは区別されない**（0.7.0で2回踏んだ）。
  A153向けにビルドした直後に`arduino-cli upload -b ...:frdm_mcxn947`すると、**A153のバイナリがN947に書き込まれる**
  （エラーにならず、シリアル出力が無いか、`Wire ACK Fault`が出るだけ）。
  2枚で試すときは`compile -u`でビルドと書き込みを一緒に行うか、`--build-path`をボードごとに分けること
- **大量にビルドした作業ディレクトリは、その作業が終わったらすぐゴミ箱（`~/.Trash`）に移す**（2026-10-13）。
  0.9.1の準備でスクラッチパッドが140GBになり、ディスクの空きが19GBまで減った。大半はバイナリ比較の`build/`（全サンプル×全ボード×2つのコアで124GB）で、
  ほかに回帰コンパイルの作業領域（4.5GB）と、実機確認でボードごとに分けた`--build-path`（1つ200〜270MBが約190個）。
  スクラッチパッドはセッションが続くかぎり消えない。比較の結果（一致・不一致の一覧）を記録したら、ビルドディレクトリは残さない。
  `~/Library/Caches/arduino`（arduino-cliとIDEのビルドキャッシュ）も10GBになっていたので一緒に移した（消しても次のビルドで作り直される）。
  ゴミ箱に移しただけでは空きは増えないので、移したらユーザーに空にしてもらう
- **A153・N947のバイナリ一致で「何も変わっていない」を確かめるときは、行数の変わる編集に注意する**（0.8.0）。
  `assert()`は`__LINE__`をイメージに埋め込むので、`i3c.cpp`のA156の分岐で1行減らしただけで、I3Cを含む49件が変わった（比べた1件は4バイト違い）。
  違いが出たら`cmp -l`で位置を出し、`addr2line`で関数を見る。比較を崩さないよう、分岐の中の行数は変えないでおく
- **同じ形の項目がまとめてFAILしたら、コードより先にジャンパを疑う**（0.9.0の実機確認だけで7回）。
  `Serial1`の項目が全部落ちる、`Wire1`のタイムアウトが全部NAK（`134`）で即座に返る、サーボのパルスが測れない、などはどれもジャンパの付け忘れ・挿し違い・前の確認の配線の残りだった。
  挿し直して流し直す前に、その確認がどのピンの組を使うかを`release_check/README.md`の表で確かめる
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
  現在は`update_package_index.yml`の最初の段（hygieneの`--package-index`）が検出する（2026-10-25に`--release`から移した）
- **バージョンを上げると`mcxPinState`の`#warning`が再発火する**（設計どおり）。
  `arduino_io.h`が前回の照合時点から変わっていないことを
  `git log <前回コミット>..HEAD -- .../arduino_io.h`が空であることで確認してから、
  `MCXPINSTATE_VERIFIED_AGAINST`を上げる。**バンドル側と上流リポジトリの両方に反映すること**

---

## 決定の要点（詳細は`docs/dev_notes/releases_and_decisions.md`）
- **1.0までに変えない**: インスタンス名（`Wire1`・`Serial1`・`SPI1`など）は付け替えない。`endTransmission()`の戻り値はAVRの0〜5。公開APIの範囲はAPI_COMPATIBILITYの冒頭の「What stays compatible」。
- **ADCの無いピンへの`analogRead()`をビルド時に検出する案は入れない**（実行時の`panic()`のまま）。
- **0.12.0でやらなかった**: `SoftwareSerial`・USB・r01libの名前空間（次の版は未定）。
- **0.12.0で入れた機能**（`MsTimer2`・`TimerOne`・`mcxTicker`、`Servo`のPWMの無いピン、PWMの周期とパルス幅をtickで持つこと、`mcx-discovery`、デバッガのペリフェラルのレジスタ表示、A4/A5の`Wire.setPins()`）の仕様と決定は、触る前に`releases_and_decisions.md`を読む。

## ボード別の前提（要点。そのボードを触る前に`docs/dev_notes/board_*.md`を読む）
- **共通**: 実機は5枚（A153・N947・A156・N236・C444）が手元にある。センサーを読むサンプルは`SENSOR_WIRE`を`#if`で定義する。N947はSJ14・SJ15がA側（1-2）の基板が前提。
- **A156**: R59・R60を2-3側へ付け替え、R75・R76を外した基板が前提。`Wire`=D18/D19（センサーも`0x48`でここ）、`Wire1`=MikroBus（`LPI2C3`）、`Serial2`=MikroBus。オンボードMCU-LINKは`Device`列が空。
- **N236**: Rev Cで、R25・R67を外した基板が前提。`Wire`=D18/D19（`LPI2C5`）、`Wire1`=`LPI2C2`（加速度センサー）、`analogRead(A3)`は`-1`。FC2は`Serial1`と`Wire1`で共有。
- **C444**: Cortex-M0+でFPU・MPU・`MSPLIM`無し。`Serial`はUSBとD0/D1の両方（`Serial1`なし）、`Wire`=D18/D19、`Wire1`=加速度センサー、FOPTは`0x3B`、D5・D6・D9はTPM0_CH2を共有、スタックはRAMの下端。

## 開発の進め方（要点。詳細は`docs/dev_notes/workflow.md`）
- 開発は`<version>-dev`ブランチで行い、ドキュメントとCLAUDE.mdもそのブランチに入れる。`main`にはリリース時にまとめてマージする。
- IDEとの連携は`~/Library/Arduino15/packages/nxp/hardware/mcx/<version>-dev`（symlink、今は`0.12.0-dev`）。Boards Managerで入れたリリース版が並ぶとそちらが選ばれるので、実機確認の前に`arduino-cli compile -v`の`Using core ... from platform in folder:`を見る。退避と復帰は`script/dev_env_*.command`。
- リリース前は`staging-<version>`ブランチで3OSのインストールと動作を確かめてから、`main`に索引のエントリを入れる。

## リリース準備（`docs/dev_notes/release_checklist.md`に全文。入るときは全部読む）
1. CHANGELOGの`[Unreleased]`を補完して確定（冒頭にHighlights）。2. ドキュメント全体の再監査（`docs/llms.txt`も）。3. `docs/api/`（Doxygen）の再生成。4. ライセンスの確認。5. 機械チェック。6. 全サンプル×全ボードの回帰コンパイル。7. `examples/release_check/`を実機で全ボード（書き込み・デバッグも各OSで1回）。

## Pendingタスク（番号は変えない。全文は`docs/dev_notes/pending_tasks.md`）
- **6**: 上流`r01lib`（`~/dev/mcuxpresso/r01lib`）には手を入れない。反映は保留（0.10.0でも反映しないと決めた）。
- **7**: GPIO治具は作らない。
- **9**: LinkServer 26.9.130はA153のフラッシュを32KBと読む。避ける版の一覧は`upload.sh`の`flash_size_bug()`、`upload.bat`の`:consider`、`gdb-bridge`の`flashSizeBug`の3か所。公開リポジトリには症状だけを書く。
- **10**: A4/A5の`Wire.setPins()`は0.12.0で入れた（解消）。
