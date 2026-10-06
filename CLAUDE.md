# mcx-arduino-core 開発引き継ぎドキュメント

## プロジェクト概要
- **リポジトリ**: https://github.com/teddokano/mcx-arduino-core
- **内容**: NXP FRDM-MCXA153 / FRDM-MCXN947 / FRDM-MCXA156（0.8.0から。いずれもCortex-M33）向けのArduino IDEボードサポートパッケージ。
  v0.4.0以降は**ソース配布方式**（プリビルド`.a`は廃止）で、`hardware/nxp/mcx/cores/arduino/`（全ボード共有）と
  `hardware/nxp/mcx/variants/<board>/src/`（ボード固有）が唯一の実体。編集すれば次のビルドにそのまま反映される
- **同梱ライブラリ**: `mcxPinState`（ピン所有状況のデバッグ表示）、`mcxRCServo`（RCサーボ）、`EEPROM`（0.7.0で追加）。
  前の2つは別リポジトリが開発の本拠地で、`hardware/nxp/mcx/libraries/`配下はリリース時に同期する取り込みコピー。
  `EEPROM`はこのリポジトリが本体（フラッシュの配置がリンカスクリプトと一体なので）
- **現在のリリース**: **v0.8.0**（2026-10-05、FRDM-MCXA156の追加）。macOS・Windows・Linuxの3プラットフォームで
  インストール〜ビルド〜アップロード〜IDE内蔵デバッガまで検証済み
- **開発中**: **0.9.0**（`0.9.0-dev`ブランチ）。**範囲はFRDM-MCXN236の対応と、FRDM-MCXA156のD3/D5/D6/D9での`analogWrite`の2つ**（2026-10-05に決定。ボード追加の順番はA156 → N236 → C444で合意済み）。
  A156の前提と決定は下の「FRDM-MCXA156の前提と決定事項（0.8.0）」節、N236の分は「FRDM-MCXN236の前提と決定事項（0.9.0、作業中）」節
- **リリースごとの変更点**: [CHANGELOG.md](CHANGELOG.md)
- **各リリースで何をやり、どこで詰まったかの詳細**: [docs/DEVELOPMENT_LOG.md](docs/DEVELOPMENT_LOG.md)
  （v0.1.5〜v0.8.0の全作業記録。自動では読み込まれないので、経緯が必要なときだけ開く）

---

## ドキュメントの地図
どこに何が書いてあるか。**この`CLAUDE.md`は「今この瞬間に効いているルールと状態」だけを持ち、
過去の経緯は`docs/DEVELOPMENT_LOG.md`に置く**という分担にしてある。

| 知りたいこと | 見る場所 |
|---|---|
| 対応APIの一覧・未対応項目 | [API_COMPATIBILITY.md](API_COMPATIBILITY.md) |
| ピン配置（ボード別） | [PIN_MAPPING_A153.md](PIN_MAPPING_A153.md) / [PIN_MAPPING_N947.md](PIN_MAPPING_N947.md) / [PIN_MAPPING_A156.md](PIN_MAPPING_A156.md) / [PIN_MAPPING_N236.md](PIN_MAPPING_N236.md)（0.9.0で作業中） |
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
- **arduino-cliのビルドキャッシュはスケッチのパスごとに1つで、ボードは区別されない**（0.7.0で2回踏んだ）。
  A153向けにビルドした直後に`arduino-cli upload -b ...:frdm_mcxn947`すると、**A153のバイナリがN947に書き込まれる**
  （エラーにならず、シリアル出力が無いか、`Wire ACK Fault`が出るだけ）。
  2枚で試すときは`compile -u`でビルドと書き込みを一緒に行うか、`--build-path`をボードごとに分けること
- **A153・N947のバイナリ一致で「何も変わっていない」を確かめるときは、行数の変わる編集に注意する**（0.8.0）。
  `assert()`は`__LINE__`をイメージに埋め込むので、`i3c.cpp`のA156の分岐で1行減らしただけで、I3Cを含む49件が変わった（比べた1件は4バイト違い）。
  違いが出たら`cmp -l`で位置を出し、`addr2line`で関数を見る。比較を崩さないよう、分岐の中の行数は変えないでおく
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

## FRDM-MCXA156の前提と決定事項（0.8.0）
0.8.0（2026-10-05リリース）で対応した。当時の方針のうち、いまも効いている決定と前提だけをここに置く。実機確認と不具合の修正の経緯は[docs/DEVELOPMENT_LOG.md](docs/DEVELOPMENT_LOG.md)のv0.8.0の節。

- **0.8.0の範囲はFRDM-MCXA156の対応のみだった**。3枚目のボード追加の順番は**A156 → N236 → C444**で、それぞれ別のバージョンにする
  （A156はA153と同系統でピン表とクロックが中心。N236はオンボードにI3Cセンサーが無いボードの検証を先に解いておく役。
  C444はCortex-M0+・Kinetis系で、A156の4〜6倍の規模）
- **GPIO治具は入れない**。最近のリリース作業は自動化が大きく進んでいて、治具による改善は手間の削減にそれほど効かない、というユーザー判断
- **実機は5枚とも手元にある**（A153・N947・A156・N236・C444）
- **SDKは`ref/SDK_2_16_000_FRDM-MCXA156.zip`**（ユーザーがSDK Builderから取得）。コアが取り込んでいるSDKと同じ2.16.000で、
  `cores/arduino/sdk/`の共有ドライバはこのzipのものとバイト単位で一致することを確認済み（`semihost_hardfault.c`は元からこのリポジトリ独自）
- **A156のボードは抵抗の付け替えを前提にする**。ピン関連の文書（`PIN_MAPPING_A156.md`、variantのREADMEなど）に必ず明記する:
  - **R59・R60を2-3側へ付け替える**: 工場出荷時は1-2側で、D10=P3_13(PWM1_B2)、D11=P3_15(PWM1_B1)。
    付け替えるとD10=P2_6(LPSPI1_PCS1)、D11=P2_13(LPSPI1_SDO)になり、`SPI`がD10〜D13に出る（D12=P2_16、D13=P2_12は直結）。
    付け替えたボードではD10・D11でPWMは使えない。開発に使うボードは付け替え済み
  - **R75・R76を外す**: 工場出荷時はA4(P1_12)・A5(P1_13)がCANトランシーバ(TJA1057)の`CAN_RXD`・`CAN_TXD`にもつながっていて、
    A4はトランシーバに駆動される。コアはA0〜A5の6本とも使える前提にする。
    開発用の基板（R59・R60は付け替え済み）からR75・R76も外し、A0〜A5の6本とも`analogRead()`が動くことをユーザーが確かめた（2026-10-05）
- **I²Cの割り当て（2026-10-03に決定）**: A156はオンボードのP3T1755が**ArduinoのD18/D19（`LPI2C0`）に直結**している
  （A153・N947のように専用ピンが無い。I3C0もセンサーにはこの2本でしか届かない）。そこで
  **`Wire`=D18/D19（センサーも`0x48`でここ）、`Wire1`=MikroBusの`MB_SDA`/`MB_SCL`（`LPI2C3`、`P3_28`/`P3_27`、ALT2）**とし、I3C0は使わない。
  `Wire1`は0.8.0で実装する。他の2ボードと`Wire1`の意味が違うので、`Wire1`でセンサーを読む既存サンプル（十数本）にはA156の分岐が要る。
  コアは`arduino_i2c.cpp`の「`I3C_SDA`のピンなら`Wire1`（I3C）」と「`MB_SDA`のピンならN947の`Wire2`」の2つの判定をボードで分ける必要がある。
  **テストは`MB_SDA`-`D18`・`MB_SCL`-`D19`をジャンパでつなぐ**: `Wire1`からP3T1755を読める、`Wire`と`Wire1`の間でターゲットモード
  （N947の`LPI2C3`はターゲットとして動かなかったが、A156では両方向を試す）、D18/D19をGPIOでLOWにして`Wire1`のタイムアウト。
  プルアップは2.2kΩ（センサー側）と4.7kΩ（MikroBus側）が並列になる
- **UARTの割り当て（2026-10-03に決定）**: `Serial`=USB（`LPUART0`）、`Serial1`=D0/D1（`LPUART2`、`P2_11` RX/`P2_10` TX、ALT3）、
  **`Serial2`=MikroBusの`MB_RX`/`MB_TX`（`LPUART1`、`P3_20`/`P3_21`、ALT3）を0.8.0で入れる**。`Serial2`はどのボードにも無かった新しい名前で、
  A156だけに置く（宣言・`serialEvent2`の呼び出しもA156の分岐で）。テストは`MB_RX`-`MB_TX`のジャンパでループバック
- **D3/D5/D6/D9（FlexPWM1）での`analogWrite`は0.8.0では見送る**（2026-10-03、ユーザー判断）。
  A156の`analogWrite`はA153と同じ`PWM0`〜`PWM5`（FlexPWM0、J3の`P3_11`〜`P3_6`）だけ。
  **D-ピンのPWMは0.9.0で入れる**（2026-10-05に決定。A156のPWMは`PWM0`〜`PWM5`と合わせて10本、周波数は6系統になる）（D3=`P3_12` PWM1_A2、D5=`P3_14` PWM1_A1、D6=`P3_16` PWM1_A0、D9=`P3_17` PWM1_B0、いずれもALT7。
  D6とD9はsm0を共有するので周波数が連動する）
- **LinkServer 26.9の不具合（Pendingタスク9）はA156には出ない**（2026-10-03に確認）。26.9.130は
  `Flash variant 'MCXA1x6 ...' detected (1MB = 128*8K at 0x0)`と正しく判定し、46KBのイメージの書き込みと`verify`が通った
  （A156のvariantがまだ無いので、A153向けの`hello_world`のELFを`MCXA156:FRDM-MCXA156`として書いた）。
  3か所の判定と「A153だけ」という説明文はそのままでよい
- **開発用A156のオンボードMCU-LINKは、`LinkServer probes`の`Device`・`Board`列が空**（`MCU-LINK on-board (r0E7) CMSIS-DAP V3.128`）。
  A153・N947のプローブはボード名とチップ名を返す。gdb-bridgeは2枚以上つながっているとき`Device`列でプローブを選ぶので、
  そのままでは2枚接続でA156をデバッグできなかった。**gdb-bridgeを直した**（2026-10-03）: チップ名が一致するプローブが無く、
  `Device`列が空のプローブがちょうど1つなら、それを選ぶ。空のプローブが2つ以上ならエラーで止める。
  A156＋A153の2枚接続で、gdbから`gdb-bridge`をそれぞれのDEVICEで起動し、正しいプローブが選ばれることを確認した
  （`0x20010000`がA156では読め、SRAMが32KBのA153では`Cannot access memory`になることで、つながったチップも確かめた）
- **variantとコアの分岐はできている**（2026-10-03）。variantの`include/`・`src/`・`svd/`はSDKのzipから無改変、
  `clock_config.c`・`pin_mux.c`もSDKのテンプレートそのままで、A153のConfig Tools版がやっている分（ポートのクロック、LPI2C・LPSPIのクロック）は
  `mcu.cpp`のA156分岐で行う。実機で確認済み: 起動と`Serial`、`EEPROM`（`test_EEPROM`、書き込みをまたいだ保持も）、`Wire`でP3T1755、
  `Wire`・`Wire1`のターゲットモード（`test_Wire_target_self`、`Wire1`の`LPI2C3`も応答する）。
- **センサーを読むサンプルは、サンプルごとに`#if`で`SENSOR_WIRE`を定義する**（2026-10-03、ユーザー判断。コアに共通の名前は置かない）。
  A153・N947では`Wire1`、A156では`Wire`。マクロにしたのはA153・N947のバイナリを変えないため（全177件の一致で確認）。
  メッセージ中の「Wire1」はそのままで、A156ではそう読み替える旨をヘッダーに書いた。`release_check/01`はA156でALL OK
- **N947はSJ14・SJ15がA側（1-2）の基板を前提にする**。3つとも橋渡しされた基板ではD18/D19が`Wire`とI3C1（P3T1755）の両方につながり、`release_check/01`の`Wire`のターゲットの3項目が調停負け（`137`）でFAILする。N947の`Wire`の確認が予想外に落ちたら、コードより先にどの基板かを確かめる

---

## FRDM-MCXN236の前提と決定事項（0.9.0、作業中）
0.9.0で対応する（0.9.0の範囲はこれとA156のD-ピンのPWMの2つ）。回路図は`ref/FRDM-MCXN236.pdf`（Rev C）。決まったものと、回路図・SDKで確かめた事実だけをここに置く。

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
  開発用のN236（Rev C）からはR25・R67を外した（2026-10-06、ユーザーが実施）。`analogRead()`でA1・A2を読む確認は、ポーティングのあとに行う。
  ピン関連の文書（`PIN_MAPPING_N236.md`、variantのREADMEなど）に必ず明記する
- **UARTは`Serial`（USB、FC4、`P1_8`/`P1_9`）と`Serial1`（D0/D1）の2つ、SPIは`SPI`だけ（2026-10-05に決定）**。
  MikroBusのUART（`MB_RX`/`MB_TX`）はD0/D1と同じピンなので`Serial2`は無い。MikroBusのSPIはArduinoのSPIと同じ線（FC3）で、
  CSが`P1_16`（=D18、`Wire`のSDA）なので、独立した`SPI1`も置かない（`arduino_spi.{h,cpp}`でN236だけ`SPI1`を定義しない）
- **`PWM0`〜`PWM5`は他の3ボードと同じ並び（2026-10-06に決定）**: `PWM0`/`PWM1`がsm2のB/A、`PWM2`/`PWM3`がsm1、`PWM4`/`PWM5`がsm0。
  `PWM0`=`P3_17`(D6)、`PWM1`=`P3_16`、`PWM2`=`P3_15`、`PWM3`=`P3_14`(D9)、`PWM4`=`P2_7`(D5)、`PWM5`=`P3_12`(D3)。回路図にPWM0〜5のシルクは無い。
  **PWMを出せるピンは物理的に6本で、4本はD-ピンと同じピン**（A156の「独立した10本」とは違う）。D2（`P2_0`）も`PWM1_A3`を出せるが、使っていない
- **D3・D5・D6・D9の`analogWrite`は、物理ピンが`PWM0`〜`PWM5`と同じなので、何もしなくてもPWMになる**（`analogWrite()`は物理ピンでPwmOutの表を引く）。
  そろっていない2点——`digitalPinHasPWM()`が`PWM0`〜`PWM5`の範囲しか真にしないこと、`pinMode()`のあとの`analogWrite()`がピンをPWMに戻さないこと——は、
  **A156のD-ピンのPWMの作業でまとめて直す**（2026-10-06、ユーザー判断。それまでN236はこのまま）
- **ポートの作業状態（2026-10-06に開始）**: variant（SDKのボード用テンプレートを無改変）、`boards.txt`、gdb-bridgeの`n236.cfg`、コアのN236分岐、
  `EEPROM`、mcxPinStateの表、サンプルの分岐、CIの対象、`PIN_MAPPING_N236.md`・variantのREADMEまで書いた。`hello_world`で起動・`Serial`・RGB LEDを実機確認。
  2026-10-06に配線なしの実機確認が通った: `release_check/01`・`04`・`06`（2回、書き込みをまたいだ保持も）・`07`・`08`、`test_Wire_target_self`（`Wire`と`Wire1`の両方）。
  確認用のスケッチで、`analogRead(A3)`が`-1`、A0・A1・A2・A4・A5がそれぞれ内部プルアップ／プルダウンに従うこと、`PWM0`〜`PWM5`の1kHzとデューティ比、`analogWriteFrequency`、`tone`（D2）もPDIRを直接読んで確かめた。
  `01`の`isize 0`の項目は、FXLS8974CFが送ったぶんだけレジスタのポインタを進める（P3T1755は進めない）ので、N236だけ直前にポインタを書き直す。
  `11`（D0-D1・D2-D3）と`Serial1`・`Wire1`の同時使用も通った（下の「FC2の共有の実装」）。`12`（D11-D12）・`13`もALL OK（`13`はN236では`PWM0`がD6なので、サーボのパルスをD6-D7のジャンパ経由でD7で測る。D8のジャンパは要らない）。`14`（`Wire`と`Wire1`の両方、400kHzでの上限の切り詰めも）もALL PASS。既知の電圧での`analogRead`も確認した（12ビットで、A0・A1・A2・A4・A5がGNDで0〜1、3V3で4083〜4095。1本ずつ3V3にしたとき、ほかのピンは追従しない）。配線だけで済む確認はこれで全部。`03`（SW2のFALLINGの割り込み3回とLEDの切り替え、`detachInterrupt()`後は押しても反応しない、LOWのレベル割り込み）も通った。IDE（macOS）のDebugボタンで、ブレークポイント・ステップ実行・変数・SVDの表示もすべて確認した（2026-10-06、ユーザー）。`24`はN947との2枚でALL OK（N236はA153と同じ側＝`0x42`・先攻に移した。N947と同じ側だと2枚とも相手を待って進まないため。相手は常にN947）。LinkServer 26.9.130はN236のフラッシュを`MCXNxxx (1024KB)`と正しく判定し、106KBの`01`を書き込めた（書き込んだ`01`はALL OK）、README・CHANGELOG・API_COMPATIBILITYはこれから。mcxPinStateの上流（`~/dev/Arduino/mcxPinState`）はローカルで書き換えただけで、コミット・pushしていない
- **組になるPWMの周期の問題（2026-10-06にN236で発見。A156のD-ピンのPWMの作業と一緒にコードを直す、とユーザーが決定（同日）。周期を組（サブモジュール）ごとに持たせ、ドキュメントの「片方を変えると相手も変わる」を成り立たせる）**: `PwmOut`は周期（`_period_us`）をオブジェクトごとに持つが、周期レジスタはサブモジュールの2本で共有している。
  `analogWriteFrequency(PWM3, 2500)`のあとで相手の`PWM2`に`analogWrite()`すると、共有の周期が`PWM2`の持つ1kHzに戻る。そのあと`PWM3`を書くと周期は2.5kHzに戻るが、`PWM2`のデューティ比は崩れたままになる（0.75のはずが0.46）。
  周期を短くすると、相手のピンのコンペア値が周期を超えてHigh固定になる（`PWM5`を20kHzにしたとき、`PWM4`がそうなった）。
  `PwmOut.cpp`は4ボードとも同じ作りなので、A153・N947・A156でも起きるはず（未確認）。A156のD-ピンのPWM（D6とD9がsm0を共有）にも関わる
- **FC2の共有の実装**: SDKの`LPUART_Init()`・`LPI2C_MasterInit()`・`LPI2C_SlaveInit()`はFlexCommのモードを自分の分だけにし、
  `LPUART_Deinit()`・`LPI2C_MasterDeinit()`はFlexComm全体をリセットする。そこでN236のFC2だけ、初期化のあとで`flexcomm_keep_shared()`（`mcu.h`）が両方のモードに戻し、
  Deinitを呼ばない（`Serial::reinit()`、`I2C::~I2C()`）。割り込みは`Serial.cpp`の`LP_FLEXCOMM2_IRQHandler()`が`Serial1`とSDKのハンドラの両方に渡す。
  D0/D1は`FC2_P3`/`FC2_P2`で、LPUARTがここに出るのは両方のモードのときだけ（回路図のラベル`FC2_UART_RXD`/`TXD`による）。
  2026-10-06に実機で確認: `release_check/11`がALL OK。確認用のスケッチで、`Serial1`（D0-D1のジャンパ）と`Wire1`（WHO_AM_I）の交互の使用、初期化と`end()`の順番の入れ替え、`Serial1.begin(baud, config)`のやり直し、`Wire1.setClock(400k)`、`Serial1`の長い送信中の`Wire1`の転送、`Wire1`のターゲット化がすべて通った
- **加速度センサーのWHO_AM_Iは`0x86`（レジスタ`0x13`）**。2026-10-06にユーザーがデータシートで確認し、以前の別のアプリケーションでもこの値を読んでいる。ZephyrのFXLS8974ドライバと、NXPのレジスタ定義（ユーザーのFXLS89xx_Arduinoライブラリの`fxls896x.h`）とも一致する。
  `release_check/01`はN236でこの値と、静止時に約1gを見る。Wireを`Stream`として使う確認では、P3T1755の`T_LOW`の代わりに`OFF_X`/`OFF_Y`（`0x22`/`0x23`）を書いて戻す

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
| サードパーティライブラリ互換性（ArduinoJson/LiquidCrystal/DHT/NeoPixel/OneWire/Adafruit BusIO/Adafruit Unified Sensor） | ✅ | v0.2.1で最小スケッチのコンパイルを確認。0.7.0で各ライブラリの**同梱サンプル全47本×両ボード**に広げて再確認し、`BitOrder`の本物のenum化とavr-libcの文字列関数・`M_`定数の宣言を追加した。残る失敗8本は、BLE（4本）、ADXL343ドライバ（1本）、Ethernetが要求する`Client.h`（3本、coreにネットワーク基底クラスが無い）。Servoはライブラリ側のアーキテクチャ非対応で不可（既知の限界）。ライブラリはスクラッチパッドに`ARDUINO_DIRECTORIES_USER`で入れたので、`~/Documents/Arduino/libraries`には無い |
| Wire (I2C) | ✅ | |
| Wire1 (I3C, I2Cモード) | ✅ | オンボードP3T1755で確認、重大バグ修正済み。A156の`Wire1`はI3CではなくMikroBusの`LPI2C3`（0.8.0） |
| SPI | ✅ | |
| attachInterrupt | ✅ | |
| detachInterrupt | ✅ | v0.2.1で追加。SW2を使った実機確認済み |
| analogRead | ✅ | LPADC。A153は`A0`-`A3`、N947は`A2`-`A5`、A156は`A0`-`A5`（`A4`・`A5`はR75・R76を外したボードで） |
| analogWrite (PWM) | ✅ | A153・A156はFlexPWM0、N947はFlexPWM1。`PWM0`-`PWM5`のみ |
| millis / micros | ✅ | SysTick(1ms) + DWT |
| delayMicroseconds | ✅ | wait_us()ベース、v0.2.1で追加 |
| tone / noTone | ✅ | CTIMER0, 任意のデジタルピン |
| Serial1（A153・A156はD0/D1、N947はMikroBusの`MB_TX`/`MB_RX`）、Serial2（A156だけ、MikroBus、0.8.0） | ✅ | 入力バッファ有効化・RX割り込み・available()の3バグ修正後、実機ループバックで確認 |
| shiftOut / shiftIn | ✅ | 割り込みベースの相互検証で確認 |
| pulseIn / pulseInLong | ✅ | |
| random / randomSeed | ✅ | |
| UNO R3/R4互換マクロ・定数一式 | ✅ | `release_check/01`の「compat macros」節で実行時に値を確認 |
| String クラス | ✅ | 独自実装（WString移植ではない）。連結・数値変換・検索・置換・大小文字変換・trim等を実機確認、全項目OK |
| EEPROM（0.7.0） | ✅ | 1KB、内蔵フラッシュの末尾。`release_check/06`（API・書き込み・リセット後と書き込み後の保持）と`07`（ウォッチドッグで書き込み中に1000回リセット）で両ボード確認。CLIの`upload`とgdbの`load`、IDE（macOS）の書き込みボタンとDebugボタンで消えないことも確認 |
| Wire.setWireTimeout / getWireTimeoutFlag / clearWireTimeoutFlag（0.7.0） | ✅ | LPI2Cのピンlowタイムアウト。`Wire`・N947の`Wire2`・A156の`Wire1`（A153・N947の`Wire1`はI3Cで無効）。`release_check/14`（ジャンパ）で3ボードとも確認 |
| I2Cターゲット（スレーブ）モード（0.7.0） | ✅ | `Wire`とA156の`Wire1`（A153・N947の`Wire1`とN947の`Wire2`は`begin(address)`で止まる）。自分自身をターゲットにする形（`release_check/01`）と2枚接続（`release_check/24`）で確認 |
| Wire: begin()のオーバーロード・内部プルアップ・Stream化・5引数requestFrom・バッファ上限（0.7.0） | ✅ | `test_Wire_begin_address`と`test_Wire_Stream_requestFrom5`で両ボード確認（後者は`release_check/01`に統合） |
| Serial.begin(baud, config) / end() / serialEvent（0.7.0） | ✅ | 12形式をRXピンからビット単位で読んで確認（`release_check/11`に統合） |
| AVR互換の補助関数（0.7.0） | ✅ | `itoa`/`dtostrf`/`word`/`_BV`/`analogReference`の定数/`HardwareSerial`/avr-libcの文字列関数/`M_`定数/`SDA`・`SCL`/`BitOrder`のenum化。`test_avr_compat_helpers`（`release_check/01`に統合。単体でもA156を含む3ボードでALL PASS） |
| print(double)・String(double)の丸め（0.7.0） | ✅ | `test_print_float_rounding`で3ボード確認（A156は0.9.0の開発開始時。代表的な項目は`release_check/01`にも入っている） |
| panic()のメッセージ・HardFaultの報告・スタック上限（0.7.0） | ✅ | `error: ...`をUSBシリアルへ。`test_fault_report`で8種類のクラッシュを3ボード確認（A156は0.9.0の開発開始時。FPUのあるA156でもPCが正しい行を指す）。スタックはヒープの終わりで`MSPLIM`により止まり、IDEの「ローカル変数で使える」表示はその量と一致する |
| 複数ボード同時接続での書き込み・デバッグ（0.7.0、0.8.0で3枚） | ✅ | macOS（IDEとarduino-cli）、Windows・Linux（IDE、`0.7.0-rc1`のステージング経由）の全てで、2枚つないだままの書き込み（ポートを切り替えてそれぞれ）とデバッグ（1枚ずつ順番に）を確認。0.8.0で、A153・N947・A156の3枚をつないだまま3つのボードを同時にデバッガで動かせることを、macOS・Windows・Linuxの全てで、ステージング（`staging-0.8.0`）と本番の`main`のURLから入れた両方で確認（A156のプローブは`Device`列が空） |
| 上記全機能の同時使用 | ✅ | `test_combined_peripherals.ino`（Serial1込み）で実機確認済み。WARNINGなし、`serial1`ループバック欠落なし |
| ボードマネージャーインストール | ✅ | v0.1.5時点で確認済み。v0.2.0リリース後、実際にGitHubの`package_nxp_mcx_index.json`経由でBoards Managerからインストールし直し、macOS/Windows 11双方でビルド・書き込み・実行まで動作確認済み |

（v0.2.0の機能開発・デバッグ自体はmacOS実機で実施。リリース後のBoards Managerインストール検証はmacOS/Windows 11の両方で実施）

---

## ローカル開発環境
- **OS**: macOS（Saitama, Japan）
- **リポジトリパス**: `~/dev/mcx-arduino-core`
- **v0.4.0以降のソース構成**: `MCUXpresso_project/`ディレクトリは廃止（削除済み）。ソースの唯一の実体は`hardware/nxp/mcx/cores/arduino/`（全ボード共有）＋`hardware/nxp/mcx/variants/<board>/src/`（ボード固有）で、プリビルド`.a`のビルド・配置手順も不要になった——編集したソースはそのままarduino-cli/Arduino IDEのビルドに反映される（詳細は[docs/DEVELOPMENT_LOG.md](docs/DEVELOPMENT_LOG.md)の「`cores/arduino/`一本化・`platform.txt`書き換え完了」節）
- **xPackツールチェーン**: `~/.xpacktools/xpack-arm-none-eabi-gcc-14.2.1-1.1/`（`package_nxp_mcx_index.json`記載のものと同一バイナリ、チェックサム確認済み）
- **ローカルArduino IDE連携**: `~/Library/Arduino15/packages/nxp/hardware/mcx/<version>-dev`（今は`0.9.0-dev`。ブランチ名と同じ。パッチリリースのブランチを並行して進める間は、作業ツリーで切り替えたブランチに合わせてこの名前も付け替える）をこのリポジトリの`hardware/nxp/mcx/`へのシンボリックリンクとして設定済み（編集が即座に反映される）。ツールチェーンも`~/.xpacktools/`への symlink。`-dev`サフィックスにより、Boards Manager経由でインストールする実リリース版とはディレクトリ名が衝突せず共存できる（ただし下の項目のとおり、並んでいるとリリース版が選ばれる）
- **リリース版を入れたままだと`-dev`が使われない**（0.7.0で踏んだ）: `packages/nxp/hardware/mcx/`に
  Boards Managerで入れた`0.6.0`と`0.7.0-dev`のsymlinkが並ぶと、**IDEも既定のarduino-cliもインストール済みの`0.6.0`を選ぶ**。
  開発中の修正がIDEで一切効かず、「直したはずの不具合がIDEでは再現する」形で表に出る。
  実機確認の前に`arduino-cli compile -v`の`Using core ... from platform in folder:`が`-dev`を指しているか確かめること。
  2026-09-24から`0.6.0`は`~/Library/Arduino15/mcx-0.6.0-backup`に退避中（Boards Manager検証で使うときは戻す）。
  0.8.0のリリース確認で本番の`main`のURLから入れた`packages/nxp`（0.8.0とツールチェーン）は
  `~/Library/Arduino15/nxp-0.8.0-release-installed`に退避してある（開発環境は元に戻した）
- **注意（Boards Manager経由の実インストール検証時のハマりどころ）**: 上記symlink環境を無効化する際、`~/Library/Arduino15/packages/nxp`を同じ`packages/`直下で別名（例: `nxp.dev-backup`）にリネームしただけでは不十分 — arduino-cliは`packages/*`配下の全ディレクトリ名をpackager IDとして解釈するため、リネーム後も`nxp.dev-backup:mcx`という別パッケージとして「0.1.9-dev installed」表示が残ってしまう（`arduino-cli core list --all`で再現・特定）。無効化する際は`packages/`の外（例: スクラッチパッド等）に完全に退避すること。v0.2.0リリース後、この手順でBoards Manager経由のGitHubからの実インストールを検証済み

## GitHub Actions
- **`regression_check.yml`**: push・PRごとに全サンプルのコンパイル（fast/fullの2段）とhygieneチェック（`.github/scripts/check_repo_hygiene.py`、タグでは`--release`）
- **`update_package_index.yml`**: GCCのsizeをHEADリクエストで取得、プラットフォームZIPのchecksum/sizeをダウンロードして計算・更新
- **既知の制限**: タグpush（`push: tags: '[0-9]+.[0-9]+.[0-9]+'`）で起動した場合、`actions/checkout`がdetached HEADでチェックアウトするため最後の`git push`が失敗する（過去のv0.1.6〜v0.2.0全リリースで再現）。実際のchecksum確定は、リリース後に`gh workflow run update_package_index.yml --ref main`（または Actions UI の "Run workflow"）で`main`ブランチに対し手動実行する必要がある。**リリース時は「タグpush→(失敗を確認)→mainに対してworkflow_dispatchを手動実行」の2段階が必須の手順**

### リリース前クロスプラットフォーム検証: ステージングブランチ方式（v0.3.1から採用）
これまでは「タグpush→`main`のchecksum確定→ユーザーが各OSで実機インストール検証」という順序で、`main`の`package_nxp_mcx_index.json`が確定してから初めて検証していた。ユーザーから「リリース前に各OSでのインストールを確認する方法はないか」と相談があり、以下の手順を提案・採用が決定:

1. いつも通り`gh release create`でリリースzipを添付（この時点でダウンロードURLは実在・安定する。`main`のインデックスをまだ更新していなくても関係ない）
2. `main`とは別に**ステージング用ブランチ**（例: `staging-0.3.1`）を作り、`package_nxp_mcx_index.json`だけをそこにpush——今作ったリリースのエントリ（url/checksum/sizeは実際の値）を`platforms[]`の末尾に追加したもの（既存エントリは書き換えない。v0.3.1当時は`platforms[0]`を書き換えていたが、0.4以降は過去バージョンも選べるよう追加にしている）
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
   - **節の冒頭に3〜5行の要約（`### Highlights`の見出しと箇条書き。Added/Changed/Fixedと同じレベル）を書く**（0.7.0から）。詳細は正確だが一般ユーザーが読み切るには多いので、詳細を読むかどうかをここで判断できるようにする。
     主な追加・互換性の改善・重要な修正を、1行1項目で平易に書く。GitHub Releaseのノートはこの節をそのまま使うので、要約もそこに載る
2. **ドキュメント全体の再監査**: Explore agentに、全`.md`ファイル（README/README.ja/TUTORIAL/TUTORIAL.ja/API_COMPATIBILITY/CHANGELOG/PIN_MAPPING_*/LICENSE/`docs/`配下/`variants/*/README.md`/`examples/release_check/README.md`）を対象にした網羅監査を依頼する。特に開発期間中にリネーム・統合・移動したファイル/サンプル名/パスへの追従漏れ（古い名前の残存、リンク切れ）を機械的に洗い出すのが目的——`CLAUDE.md`自身の過去形の言及（開発日誌としての性質上正しい）は対象外としてよい
3. **`docs/api/`（Doxygen）の再生成**: `doxygen Doxyfile`を実行し、直近のソース変更（コメントを含む）が反映された状態にする。`docs/api/index.html`のタイムスタンプより新しいソースファイルがないか確認してから判断するとよい
4. **ライセンスチェック**: `LICENSE`全文を読み返し、開発期間中に追加した新規ツール/ファイル（外部プロジェクトのソースを読んで挙動を参考にしたもの含む）で、帰属記載が漏れているものがないか確認する。「コードは一切コピーしていないが、他プロジェクトのソースを読んで互換動作を実装した」ケースは、コピーでなくても透明性のため記載する、というこのプロジェクトの既存の判断基準（`upload.sh`のArduinoCore-zephyr参照等）を毎回適用する
5. **その他の機械チェック**: `git status`のクリーンさ、自コードの`TODO`/`FIXME`/`XXX`残存、GitHub Issuesのオープン状態、新規追加した実行ファイル・設定ファイル（今回なら`gdb-bridge`バイナリ群・`boards.txt`の`debug.*`設定）の整合性、`package_nxp_mcx_index.json`の妥当性（新バージョンエントリはまだ追加しない——それは実際のリリース作業段階）、リリースzipサイズの見積り、「暫定」「未確認」「実機確認待ち」等の古い表現が現行ドキュメントに残っていないか
6. **全サンプル×全ボードの回帰コンパイルスイープ**（`examples/Arduino_compatible_API`・`Arduino_incompatible_API`・`release_check`配下の全`.ino`）を、上記1〜5の変更後に最終確認として実行し、新規リグレッションがないことを確認する
7. **`examples/release_check/`を実機で通す（全ボード）**——**コンパイルが通ったことを動作の証拠にしない**。項目6はコンパイルだけで、実行時にしか出ない不具合は一切見ていない（v0.4.0のソース配布移行では114サンプル×2ボードが通ったあと実機初回でハングした）。グループは`examples/release_check/README.md`の表に従い、番号体系は**`0n`=配線も外部部品も不要／`1n`=ジャンパ配線のみ／`2n`=外部ライブラリ・モジュール・ボードが必要**:
   - **`0n`**（`01`〜`08`）: 配線不要。`08`はD2・D4・D5とA0/A1（N947はA2/A3）を内部プルで動かすので空けておく（`01`はA153のフラッシュを99%使っていて、`08`の分が入らなかった）。`05`はN947限定。`06`（EEPROM）は自分で1回リセットしてから判定し、もう一度書き込むと前回のデータが書き込みをまたいで残ったかも確認する。
     `07`（EEPROMの書き込み中リセット）はウォッチドッグで1000回リセットをかけ、1ボード約6分。EEPROMを上書きするので`06`の2回が済んでから流す
   - **`1n`**（`11`〜`15`）: ジャンパのみ。`11`のSerial1配線は**ボードで違う**（A153・A156=D0-D1、N947=MikroBus `MB_TX`-`MB_RX`）。`14`（`setWireTimeout`）は全ボード共通で`D19`-`D8`＋`D18`-`D7`、N947は`Wire2`用に、A156は`Wire1`用に`MB_SCL`-`MB_PWM`＋`MB_SDA`-`MB_INT`も。`15`はA156限定で`MB_SDA`-`D18`＋`MB_SCL`-`D19`
   - **`2n`**（`21`〜`24`）: 外部`P3T1755.h`＋MikroBus配線／外部LM75系センサー／`Waveshare_TFT_Touch`の`SDBitmapViewerDemo`（`SDBitmapViewer`ではない。A156だけはDemoが`#error`で止まるので、`/PLAYLIST.JSN`を外したカードで`SDBitmapViewer`）／もう1枚のボード（`24`は2枚（0.8.0ではA153とN947、A156とN947）をD18-D18、D19-D19、GND-GNDでつなぎ、両方に書き込む。どちらかが前から同じスケッチを動かしていたら、両方をほぼ同時にリセットしてから始める）。`21`/`22`は**CIスタブではなく実物のライブラリ**を`--library`で指定すること
   - **全ボード同時接続での書き込みも各プラットフォームで1回**: 対応ボードをすべて（0.8.0ではA153・N947・A156の3枚）つなぎ、ポートを切り替えてそれぞれに書き込めること。
     `upload.sh`/`upload.bat`はポートのUSBシリアル番号（`{upload.port.properties.serialNumber}`）を
     LinkServerの`--probe`に渡すので、**Windows/Linuxのポート検出がこの番号を同じ形で返すか**が肝。0.7.0でmacOS・Windows・Linuxの全てで2枚での書き込みを確認した（どれもLinkServerと同じ形の番号を返す）。
     **デバッグも同じく全ボードをつないだまま、各ボードで1回ずつ**（IDEのDebugボタンで。0.8.0では3枚を同時にデバッガで動かせることも確認した）。IDEはデバッグ時にポートを渡さないので、
     gdb-bridgeは`LinkServer probes`の`Device`列のチップ名でプローブを選ぶ。種類の違う2枚は区別できるが、同じ種類の2枚は区別できない（エラーで止まるのが正しい動作）。
     チップ名が一致するプローブが無く、`Device`列が空のプローブがちょうど1つなら、それを選ぶ（A156のプローブが空のため、0.8.0から）
   - **IDE内蔵デバッガも各プラットフォームで1回**（`gdb-bridge`の起動経路はOSごとに別物——macOS/Linuxは`launch.sh`から`uname -s`で選ぶ別バイナリ＋別の`findLinkServer()`分岐、Windowsは共有exeを直接起動）

この7項目が終わってはじめて「`main`へのマージ」以降の既存のリリース手順に進む: `main`マージ→リリースzip作成→GitHub Release作成→**ステージングブランチ（`staging-<version>`）でのmacOS/Windows/Linux 3プラットフォーム検証**（v0.3.1から採用、「リリース前クロスプラットフォーム検証」節参照）→問題なければ`main`に対して`update_package_index.yml`を手動実行しchecksum確定。**このステージングブランチでの検証は必ず実施する——スキップしてよい状況は無い**。各OSで「インストール→ビルド→アップロード」に加えて「IDEのDebugボタン→ブレークポイント→ステップ実行」まで試す（v0.4.0で追加、0.6.0以降は3OSとも毎回確認している）

---

## 残りのPendingタスク
1. ~~Linux対応の実機検証~~ **解消済み（v0.2.2で確定）**: v0.2.1リリース後の実機検証で、ファイル名の大文字小文字ミスマッチ（`arduino.h`/`Arduino.h`、`spi.h`/`SPI.h`）によりLinuxでビルドが失敗することが判明・修正し、v0.2.2としてリリース。Linux実機（Ubuntu系）でBoards Manager経由インストール〜Blinkスケッチのビルド〜書き込み〜実行まで成功を確認済み。README.md/TUTORIAL.md/TUTORIAL.ja.mdの「未検証」表記もすべて「macOS, Windows 11, Linuxで検証済み」に更新済み
2. マルチボード対応（MCXN947, MCXA156, MCXN236）— **N947は`prepare0.3.0`ブランチで完了**。GPIO/Serial/Wire/Wire1/SPI/analogRead/analogWrite/tone・noTone・MikroBusの`SPI1`/`Wire2`/`Serial1`まで実機検証済み、`README.md`の対応ボード表もA153と同じ✅に変更済み（ユーザー判断、2026-08-16）。v0.3.0でリリース済み。
   **A156は0.8.0でリリース済み**（「FRDM-MCXA156の前提と決定事項（0.8.0）」節）。N236・C444は未着手
3. ~~`examples/tests/GPIO_NXP_Arduino`の不要なgitlinkエントリの整理~~ **解消済み**: `git ls-files --stage`で`160000`（gitlink）エントリが残っているのに`.gitmodules`が存在しないと判明（外部クローンの誤`git add`の名残）。`git rm --cached`でインデックスから除去し、他4つの外部ライブラリクローンと同様`.gitignore`に追加
4. ~~v0.3.0リリース~~ **完了**: 2026-08-16リリース。詳細は[docs/DEVELOPMENT_LOG.md](docs/DEVELOPMENT_LOG.md)の「リリース前最終チェックとv0.3.0リリース完了」節
5. ~~SDライブラリビルド時の`-Waddress-of-packed-member`警告~~ **解消済み（v0.3.1で対応）**: `platform.txt`の`compiler.cpp.flags`に`-Wno-address-of-packed-member`を追加して警告クラス自体を抑制。純粋な診断抑制フラグ（`-W`系）でコード生成には一切影響しないため、プリビルド`.a`の再ビルドや実機再検証は不要と判断——両ボードで`SDBitmapViewer`（`SD`ライブラリ使用）をコンパイルし、警告が完全に消えたことを確認
6. **上流`r01lib`（`~/dev/mcuxpresso/r01lib`、github.com/teddokano/r01lib）への反映は保留**（2026-09-26、ユーザー判断）。
   コア同梱の`r01lib`は上流と21ファイル・約1,800行ずれている（コア側だけに`pin_registry.*`・`r01lib_spi.*`、上流側だけに`spi.*`・`semihost_hardfault.c`）。
   上流はA156・N236・C444も対象にしていて、それらはここでは確かめられない。そのため
   **このコアで他のボード（A156等）の対応が揃った時点でまとめて反映する**。それまで上流リポジトリには手を入れない
7. **GPIO治具は作らない**（2026-10-02、ユーザー判断）。0.7.0で保留にしていたが、0.8.0の検討で
   「リリース作業の自動化が大きく進んでいて、治具による改善は手間の削減にそれほど効かない」として見送った。
   組み合わせを設計した`.github/scripts/check_jig_pairing.py`は残っている
8. **ネットワーク基底クラス（`Client`/`Server`/`UDP`/`IPAddress`）は0.8以降**（2026-09-26、ユーザー判断で0.7.0は見送り）。
   無いため`Ethernet`ライブラリが`Client.h`で止まる（ArduinoJsonのEthernet系サンプル3本もこれで失敗）。
   入れるときは、ArduinoCore-APIのソースがLGPLなのでコピーせず自前で書く。
   仮想関数のシグネチャはArduinoCore-APIと完全に一致させる（派生ライブラリがそのままビルドできるように）。
   W5500系シールドなどで**実際に通信できるまでを確認**すること（コンパイルだけでは対応済みにしない）
9. **LinkServer 26.9.130はFRDM-MCXA153のフラッシュを32KBと読む**（2026-09-28に判明、N947自体は26.9で動く）。
   32KBを超えるスケッチの書き込み・デバッグが「Attempt to load into missing flash area」で失敗する。
   0.7.1で、26.9を避けて別の版を使うようにした。**N947も含めて全ボードで避ける**——LinkServerは書き込み後も`redlinkserv`を残し、
   26.9は26.6が起動したそれに再接続すると`Redlink interface error 240`で失敗するので、ボードごとに版を変えられない。避ける版の一覧は
   `upload.sh`の`flash_size_bug()`、`upload.bat`の`:consider`、`gdb-bridge`の`flashSizeBug`の**3か所**にある。
   NXPが直した版を出したら、その版で46KBの`hello_world`を書けることを確かめてから3か所を揃えて更新する。
   原因の詳細はLinkServerのライセンス（使用結果の報告の公開制限）に配慮して、公開リポジトリには症状だけを書く
