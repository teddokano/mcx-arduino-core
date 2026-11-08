# リリース準備チェックリスト・package_nxp_mcx_index.jsonの構成

CLAUDE.mdが大きくなったので、読むときだけ開くようにここへ移した（2026-11-09、ユーザーが決定）。中身は移した時点のまま（「下の項目」「上の項目」などの参照は元のCLAUDE.mdでの位置のまま）。

## リリース準備チェックリスト（v0.4.0から採用）
v0.4.0の`main`マージ直前、ユーザーから「今回のリリース準備の手順を記録しておく。今後のリリース準備では必ずこれらを行うこと」と指示。`<version>-dev`ブランチでの開発が一区切りつき、`main`へマージする前に**必ず**、以下を順番に実施する（実施済みかどうかに関わらず、リリースのたびに毎回やり直す）:

1. **`CHANGELOG.md`の`[Unreleased]`セクションを補完してから確定**: そのバージョンの開発期間中に`CLAUDE.md`へ記録してきた内容（新機能・変更・実バグ修正）を漏れなく洗い出し、`[Unreleased]`に追記——特にセッション中盤以降に追加された機能（今回で言えば`mcxPinState`・`examples/release_check/`・`docs/`配下の上級者向けガイド等）は、CHANGELOGの初稿作成時点でまだ存在しておらず抜け落ちやすいので要注意。全項目を追記し終えてから`[Unreleased]`→`[<version>] - <日付>`に書き換えて確定する
   - **節の冒頭に3〜5行の要約（`### Highlights`の見出しと箇条書き。Added/Changed/Fixedと同じレベル）を書く**（0.7.0から）。詳細は正確だが一般ユーザーが読み切るには多いので、詳細を読むかどうかをここで判断できるようにする。
     主な追加・互換性の改善・重要な修正を、1行1項目で平易に書く。GitHub Releaseのノートはこの節をそのまま使うので、要約もそこに載る
2. **ドキュメント全体の再監査**: Explore agentに、全`.md`ファイル（README/README.ja/TUTORIAL/TUTORIAL.ja/API_COMPATIBILITY/CHANGELOG/PIN_MAPPING_*/LICENSE/THIRD_PARTY_NOTICES/`docs/`配下/`variants/*/README.md`/`examples/release_check/README.md`）を対象にした網羅監査を依頼する。特に開発期間中にリネーム・統合・移動したファイル/サンプル名/パスへの追従漏れ（古い名前の残存、リンク切れ）を機械的に洗い出すのが目的——`CLAUDE.md`自身の過去形の言及（開発日誌としての性質上正しい）は対象外としてよい
   - **`docs/llms.txt`も対象にする**（0.12.0から、2026-10-26にユーザーが決定）: 冒頭の「Points to keep in mind」（FQBN、インスタンス名、Ethernet、同梱ライブラリなど）がその版の内容と合っているか、新しい文書やライブラリを足したならリストに載せたかを見る。リンクが追跡下のファイルを指していることと、`PIN_MAPPING_*.md`が全部載っていることは、hygieneの`llms-links`が毎push確かめる。リンクは`main`（raw）とPagesの絶対URLなので、公開されるのは`main`にマージしてから。版の番号は本文に書かない。llms-full.txt（全文をまとめたもの）は作らない
3. **`docs/api/`（Doxygen）の再生成**: `doxygen Doxyfile`を実行し、直近のソース変更（コメントを含む）が反映された状態にする。`docs/api/index.html`のタイムスタンプより新しいソースファイルがないか確認してから判断するとよい
   **`main`の`docs/`はGitHub Pagesで公開する**（0.11.0から、2026-10-25にユーザーが決定）: 公開先は`https://teddokano.github.io/mcx-arduino-core/api/`で、READMEからリンクしている。`docs/.nojekyll`を消さないこと（無いとJekyllが`_`で始まるファイルを公開せず、Doxygenのページの多くが404になる）。Pagesの設定（Settings → Pages、`main`の`/docs`）は**0.11.0を`main`にマージしたあとで、ユーザーが一度だけ行う**（`.nojekyll`より先に有効にすると、`_`のファイルが抜けたものが公開される）
4. **ライセンスチェック**: `LICENSE`（MITの本文だけ）と`THIRD_PARTY_NOTICES.md`（0.12.0で`LICENSE`の「Third-Party Notices」から分けた。GitHubがライセンスをMITと判定できるように）の全文を読み返し（`hardware/nxp/mcx/`の2つは写しで、リリースzipに入れるためのもの。直すのは直下のほうで、`cp`で写す。hygieneの`license-copies`が同じ中身かを毎push見る。0.12.0から）、開発期間中に追加した新規ツール/ファイル（外部プロジェクトのソースを読んで挙動を参考にしたもの含む）で、帰属記載が漏れているものがないか確認する。「コードは一切コピーしていないが、他プロジェクトのソースを読んで互換動作を実装した」ケースは、コピーでなくても透明性のため記載する、というこのプロジェクトの既存の判断基準（`upload.sh`のArduinoCore-zephyr参照等）を毎回適用する
5. **その他の機械チェック**: `git status`のクリーンさ、自コードの`TODO`/`FIXME`/`XXX`残存、GitHub Issuesのオープン状態、新規追加した実行ファイル・設定ファイル（今回なら`gdb-bridge`バイナリ群・`boards.txt`の`debug.*`設定）の整合性、`package_nxp_mcx_index.json`の妥当性（新バージョンエントリはまだ追加しない——それは実際のリリース作業段階）、リリースzipサイズの見積り、「暫定」「未確認」「実機確認待ち」等の古い表現が現行ドキュメントに残っていないか
6. **全サンプル×全ボードの回帰コンパイルスイープ**（`examples/Arduino_compatible_API`・`Arduino_incompatible_API`・`release_check`配下の全`.ino`）を、上記1〜5の変更後に最終確認として実行し、新規リグレッションがないことを確認する
7. **`examples/release_check/`を実機で通す（全ボード）**——**コンパイルが通ったことを動作の証拠にしない**。項目6はコンパイルだけで、実行時にしか出ない不具合は一切見ていない（v0.4.0のソース配布移行では114サンプル×2ボードが通ったあと実機初回でハングした）。グループは`examples/release_check/README.md`の表に従い、番号体系は**`0n`=配線も外部部品も不要／`1n`=ジャンパ配線のみ／`2n`=外部ライブラリ・モジュール・ボードが必要**:
   - **`0n`**（`01`〜`09`）: 配線不要。`08`はD2・D4・D5とA0/A1（N947はA2/A3）を内部プルで動かすので空けておく（`01`はA153のフラッシュを99%使っていて、`08`の分が入らなかった）。`09`は周辺機能を使わないソフトウェアの確認で、0.9.0で`01`から分けた（`01`がA153のフラッシュの残り40バイトまで来たため）。`05`はN947限定。`06`（EEPROM）は自分で1回リセットしてから判定し、もう一度書き込むと前回のデータが書き込みをまたいで残ったかも確認する。
     `07`（EEPROMの書き込み中リセット）はウォッチドッグで1000回リセットをかけ、1ボード約6分。EEPROMを上書きするので`06`の2回が済んでから流す
   - **`1n`**（`11`〜`15`）: ジャンパのみ。`11`のSerial1配線は**ボードで違う**（A153・A156=D0-D1、N947=MikroBus `MB_TX`-`MB_RX`）。`14`（`setWireTimeout`）は全ボード共通で`D19`-`D8`＋`D18`-`D7`、N947は`Wire2`用に、A156は`Wire1`用に`MB_SCL`-`MB_PWM`＋`MB_SDA`-`MB_INT`も。`15`はA156限定で`MB_SDA`-`D18`＋`MB_SCL`-`D19`
   - **`2n`**（`21`〜`26`。`25`はN947だけ、`26`（W5500）は**オプション**で、流さなくてよい（2026-10-25、ユーザーが決定））: 外部`P3T1755.h`＋MikroBus配線／外部LM75系センサー／`Waveshare_TFT_Touch`の`SDBitmapViewer`（ライブラリの1.3.1以降。0.9.0から全ボードでこれを使う（2026-10-09、ユーザー判断）。1.3.1で互換性が改善され、`SDBitmapViewerDemo`用の`/PLAYLIST.JSN`のままのカードでも動く。0.8.0までは`SDBitmapViewerDemo`だった）／もう1枚のボード（`24`は2枚（0.8.0ではA153とN947、A156とN947）をD18-D18、D19-D19、GND-GNDでつなぎ、両方に書き込む。どちらかが前から同じスケッチを動かしていたら、両方をほぼ同時にリセットしてから始める）。`21`/`22`は**CIスタブではなく実物のライブラリ**を`--library`で指定すること（`~/dev/Arduino/libraries/TempSensor_NXP_Arduino`と、それが使う`~/dev/Arduino/libraries/I2C_device_Arduino`の2つ）
   - **全ボード同時接続での書き込みも各プラットフォームで1回**: 対応ボードをすべて（0.8.0ではA153・N947・A156の3枚）つなぎ、ポートを切り替えてそれぞれに書き込めること。
     `upload.sh`/`upload.bat`はポートのUSBシリアル番号（`{upload.port.properties.serialNumber}`）を
     LinkServerの`--probe`に渡すので、**Windows/Linuxのポート検出がこの番号を同じ形で返すか**が肝。0.7.0でmacOS・Windows・Linuxの全てで2枚での書き込みを確認した（どれもLinkServerと同じ形の番号を返す）。
     **デバッグも同じく全ボードをつないだまま、各ボードで1回ずつ**（IDEのDebugボタンで。0.8.0では3枚を同時にデバッガで動かせることも確認した）。IDEはデバッグ時にポートを渡さないので、
     gdb-bridgeは`LinkServer probes`の`Device`列のチップ名でプローブを選ぶ。種類の違う2枚は区別できるが、同じ種類の2枚は区別できない（エラーで止まるのが正しい動作）。
     チップ名が一致するプローブが無く、`Device`列が空のプローブがちょうど1つなら、それを選ぶ（A156のプローブが空のため、0.8.0から）
   - **実機チェックの進め方（0.12.0）**: 5ボードを3枚と2枚にまとめて流せる（`1n`は配線の組ごとに付け替える。②`14`・③`16`・④`15`は同じピンを使うので同時には付けない）。`25`の前に、macOSの「インターネット共有」の対象にUSB-LANアダプタが入っているかを見る（挿し直すと名前が変わる）。複数のボードのUSBが同時に消えたら（`02`の途中で4枚）ハブか電源を疑い、1枚ずつつなぐ。
   - **2枚つなぎの`24`と、実機チェック中のUSBの脱落**（0.10.0）: `24`は相手を待つスケッチなので、どちらかが別のスケッチを動かしていた直後は、1回目に相手が答えずFAILすることがある（A156とN947で1回、そのまま流し直してALL OK）。
     両方に書き込み直すか、近い時刻にリセットしてから見る。実機チェックの間に、N236・C444・N947が1回ずつUSBから消えた（`/dev/cu.usbmodem*`と`LinkServer probes`の両方から消え、書き込みは「Multiple probes」や「ポートが無い」で失敗する）。
     挿し直せば戻る。原因は調べていない。ボードを1枚ずつ流す作業の前に、`ls /dev/cu.usbmodem*`で5枚が出ているかを見るとよい。5枚をつないだままの書き込みは、`--build-path`をボードごとに分けたarduino-cliで、ボード名を1回出すだけのスケッチを順番に書いても確かめられる（0.10.0では順方向と逆方向の2周）
   - **IDE内蔵デバッガも各プラットフォームで1回**（`gdb-bridge`の起動経路はOSごとに別物——macOS/Linuxは`launch.sh`から`uname -s`で選ぶ別バイナリ＋別の`findLinkServer()`分岐、Windowsは共有exeを直接起動）
   - **パッチリリース（`x.y.Z`）では、実機で流す範囲をバイナリ比較で決める**（2026-10-11、0.9.1の準備でユーザー判断）。
     前のリリースのタグのコアと、リリース候補のコアで、全サンプル×全ボードをビルドして`.bin`を比べる。
     **バイナリがバイト単位で同じ`release_check`は実機で流さない**（書き込むものが前のリリースと同じなので、流しても新しいことは分からない）。
     違いが出たものは全ボードで流す。違いが出た理由が変更から説明できないものは、`cmp -l`と`addr2line`で関数を確かめてから先に進む。
     全ボードをつないだままの書き込みとデバッグは、書き込み・デバッグの経路（`upload.sh`/`upload.bat`、`gdb-bridge`、`boards.txt`・`platform.txt`の書き込み・デバッグの設定）が前のリリースから変わっていなければ省く。
     **ステージングでの3OSの確認は省かない**（バイナリ比較はリリースzipそのものを見ていない）。比べ方:
     - 両方のコアを**同じパス**に置いてビルドする（r01libの`assert()`が`__FILE__`でソースのパスをイメージに埋め込むので、パスが違うと全部違って見える）。
       `ARDUINO_DIRECTORIES_DATA`をスクラッチパッドのディレクトリにし、その`packages/nxp/hardware/mcx/9.9.9`にコアを置き（`git archive <前のタグ> hardware/nxp/mcx`、終わったら作業ツリーの写しと入れ替える）、
       `packages/nxp/tools`は本物へのsymlinkにする。バージョンのディレクトリ名は`x.y.z`の形でないとarduino-cliが読まない（`0.9.x`は「no patch version found」で止まる）
     - サンプルは**両方とも今のブランチのもの**を使う（コアの違いだけを見るため）。新しく足したサンプルは前のコアで通らないことがある（0.9.1の`test_function_local_static`）
     - ビルドは逐次で、`ARDUINO_BUILD_CACHE_PATH`はコアごとに分ける。`.elf`ではなく`.bin`を比べる（`.elf`はビルドのパスを含むデバッグ情報が入る）
     - `platform.txt`の`version`が上がるので、`MCX_ARDUINO_CORE_VERSION`を使うもの（`mcxPinState`）は違って当然

この7項目が終わってはじめて「`main`へのマージ」以降の既存のリリース手順に進む: `main`マージ→リリースzip作成→GitHub Release作成→**ステージングブランチ（`staging-<version>`）でのmacOS/Windows/Linux 3プラットフォーム検証**（v0.3.1から採用、「リリース前クロスプラットフォーム検証」節参照）→問題なければ`main`に対して`update_package_index.yml`を手動実行しchecksum確定。**このステージングブランチでの検証は必ず実施する——スキップしてよい状況は無い**。各OSで「インストール→ビルド→アップロード」に加えて「IDEのDebugボタン→ブレークポイント→ステップ実行」まで試す（v0.4.0で追加、0.6.0以降は3OSとも毎回確認している）

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
