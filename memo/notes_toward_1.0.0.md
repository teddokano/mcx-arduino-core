# 1.0.0に向けた検討メモ（未決定）

- 作成日: 2026-10-08
- 前提: `main`（v0.8.0）と、r01lib単体リポジトリの2026-08-11時点のコミットを読んだ上での検討
- **位置づけ: このファイルは提案と論点の一覧であり、決定事項ではない。** 採用したものは`CLAUDE.md`か`docs/DEVELOPMENT_LOG.md`へ移し、ここからは消す
- 「未確認」と書いた項目は、コードを読んだだけで実機でもビルドでも確かめていない
- 2026-10-08: §5（A4/A5のI²C）を調べた結果で書き直し、決まった2点（時期と入口の形）は`CLAUDE.md`へ移した
- 2026-10-14: §6（N947のEthernetで基底クラスを確かめる案）を足した。§6-3のlwIPの回し方と、§6-5のC444との順番（1.0前にEthernet、C444は1.x）はユーザーが決めた。§7（`Servo.h`の互換ライブラリの案と、タイマーの割り当て）を足した。§7-2の割り当てはユーザーが決めた。未決の問いの一覧を§8にした。§2-3（A156のメニューは設けない）と§3（pyOCDは検討から外す）もユーザーが決めた
- 2026-10-16: 次の版を0.9.2から0.10.0に変え、C444も0.10.0に入れることをユーザーが決めた（§4-1・§6-5・§9）。C444のスパイクの結果を§4-5に書いた
- 2026-10-16: §7-1の`Servo`の範囲（0.9.2はPWMのピンだけ、PWMの無いピンではpanic）をユーザーが決めた
- 2026-10-15: §9（1.0までの版の割り振り）を足した。順番と、次の版を0.9.2にすることはユーザーが決めた。§5・§6・§7-1・§8をそれに合わせた
- 2026-10-18: §5の`setPins`を0.10.0から外した（版は未定）。§6の「N947での確認でPendingタスク8を満たすとみなすか」は、みなす（クラスは0.10.0で出す）。`mcx_sketch_sets_cop`は利用者向けには載せない。上流r01libへの反映は0.10.0ではしない。いずれもユーザーが決めた（§5・§6・§8・§9）。ウォッチドッグのAPIを§8の問いに足した
- 2026-10-30: 0.12.0の範囲を見直した（§9の⑧は0.12.0で調べる、⑩は既知の制限として確定、§10の名前空間は0.12.0ではやらない。ユーザーが決定）。§5（A4/A5のI²C）の作りが決まり、0.12.0で入れた。決まったことは`CLAUDE.md`へ移し、§5は指し示すだけにした（§2-5・§8・§9も合わせた）
- 2026-11-20: 0.13.0の範囲を検討した。§10の名前空間はやらない、上流r01libへの反映（Pendingタスク6）は1.0の直前にやる、ウォッチドッグのAPIは0.13.0で入れる、ほかの大域インスタンス（`Serial1`・`SPI1`など）を使ったときだけリンクする案はやらない。いずれもユーザーが決定（§8・§9・§10）
- 2026-11-20: §3のprobe-rsの記述を直した（最新の版v0.32.0にA153・A156のターゲットはすでに入っていた。10-31の「まだ入っていない」は誤り）
- 2026-11-26: §3のM33の時間の元を検討した（A・B・Cの3案を比べ、Bに決めた。ユーザーが決定。0.14.0で入れる）
- 2026-11-26: 済んだのに残っていた3か所を直した（§3の`Wire2`は既知の制限として確定済み、§9の⑨はCIに入れ済み、§8のprobe-rsのWindows・Linuxの確認は0.13.0で済んだ）
- 2026-11-01: §5の「実機の確認はまだ」を直し（`release_check/16`が5ボードでALL OK）、§9の⑧を済みにした（2026-10-31に調べ、コードは変えない）
- 2026-10-30: 複数のインスタンスを持てる`Ticker`を`mcxTicker`として0.12.0で入れた（APIなどはユーザーが決定、`CLAUDE.md`）
- 2026-10-30: §7（`Servo`）のPWMの無いピンを0.12.0で入れた。タイマーの割り当てはTimerOneが`CTIMER1`を使うので§7-2から変えた（ユーザーが決定、`CLAUDE.md`）。§8・§9を合わせた
- 2026-10-29: §10（r01libを名前空間に入れる案）を足した。調べた事実・見積もり・おすすめだけで、決まったことは無い
- 2026-10-08に`0.9.0-dev`の状態に合わせた。0.9.0で決まった2件（N236を1.0前に入れる、A156のD3/D5/D6/D9の`analogWrite`）は`CLAUDE.md`のN236・A156の節にあるので、ここから消した

---

## 1. 1.0.0の考え方

1.0.0はSemVer上「公開APIを壊さない約束」になる。したがって1.0前の作業の中心は機能追加ではなく、**後から変えると破壊的変更になるものを決め切ること**。追加だけで済むもの（ボード、USBなど）は1.xへ回せる。

## 2. 1.0前に決めるべきこと（後から直せない）

| # | 論点 | 現状 | 選択肢 |
|---|---|---|---|
| 2-1 | インスタンス名の意味がボードで違う | `Wire1`はA153/N947ではI3C経由、A156ではMikroBusのI2C。MikroBusのI2CはN947では`Wire2`。`Serial2`はA156だけ | (a) 現状のまま固定して文書で明示 (b) `WireMB`のようなボード非依存の別名を足す (c) 名前を付け替える（破壊的。やるなら1.0前のみ） |
| 2-2 | `endTransmission()`の戻り値 | SDKのステータス由来（`138`、`132`など）で、AVRの0〜5と違う | (a) 現状で固定 (b) AVR互換の0〜5に寄せる。戻り値で分岐する既存ライブラリへの影響を見て決める |
| 2-3 | A156の抵抗付け替え前提 | R59/R60の付け替えとR75/R76の除去が前提。出荷状態では`SPI`（D10〜D13）とA4/A5が使えない | ~~Toolsメニューで「出荷状態/改造済み」を選べるようにする案~~ → **メニューは設けない**（2026-10-14、ユーザー判断）。出荷状態でも壊れることはなく、期待した動作をしないだけ（コアは`SPI`を`P2_6`・`P2_13`に出すが、それはヘッダのD10・D11につながっていないので届かないだけで、ヘッダ側の`P3_13`・`P3_15`には触らない。A4・A5はCANトランシーバの信号を読むだけ）。`PIN_MAPPING_A156.md`の冒頭に出荷状態と改造後の違いが表で書いてある。メニューを作らなければ、あとで既定を変える破壊的変更の心配もない。出荷状態でA4を`OUTPUT`にするとトランシーバの出力とぶつかる可能性はあるが、トランシーバに常に電源が入っているかは未確認 |
| 2-4 | 「公開API」の範囲 | 明文化されていない | ピン名、ピン割り当て、`ARDUINO_FRDM_*`マクロ、同梱r01libの直接利用、`mcxPinState`の出力形式のどこまでを互換対象にするかを文書にする |<br>→ 2026-11-26に見直した（ユーザーが決定）: `mcxRCServo`と`mcxPinState`の`PinState`の関数（表は除く）、FQBNのボード名とToolsメニューの選択肢の名前（`opt`・`eeprom`・`upload_method`）、この文書のマクロを足した。メニューの既定、道具の版、フラッシュ・RAMの量、書いていない時間は含めない
| 2-5 | A4/A5のI²C（§5） | 0.12.0で`setPins`を入れた（2026-10-30。`CLAUDE.md`のPendingタスク10） | スケッチ向けの入口は`setPins`。`TwoWire`のコンストラクタは生のピン値を取るコア用のもので、0.9.1で説明を直した（§5-2） |

### 2-1・2-2・2-4の判断の材料（2026-10-23にまとめた。同じ日に3つともおすすめのとおりに決まり、`CLAUDE.md`に移した）

**2-1 インスタンス名**: 5ボードの今の割り当て（`CLAUDE.md`の各ボードの節とAPI_COMPATIBILITYより）

| 名前 | A153 | N947 | A156 | N236 | C444 |
|---|---|---|---|---|---|
| `Wire` | D18/D19 | D18/D19 | D18/D19（オンボードのP3T1755も） | D18/D19 | D18/D19 |
| `Wire1` | I3C、オンボードのP3T1755 | I3C、オンボードのP3T1755（`MB_RX`/`MB_TX`のピン） | MikroBusのI2C | 加速度センサーとMikroBusのI2C | 加速度センサー（ヘッダに出ていない） |
| `Wire2` | なし | MikroBusのI2C | なし | なし | なし |
| `Serial1` | D0/D1 | MikroBus | D0/D1 | D0/D1 | なし（`Serial`がD0/D1） |
| `Serial2` | なし | なし | MikroBus | なし | なし |
| `SPI1` | MikroBus | MikroBus | MikroBus | なし | なし |

- 「オンボードのセンサーのバス」はA156だけ`Wire`で、ほかは`Wire1`。「MikroBusのI2C」は`Wire1`・`Wire2`・無しに分かれる。作例はボードごとの`#if`で`SENSOR_WIRE`を決めている
- 別名（例: センサーのバスの`WireSensor`、MikroBusの`WireMB`）を足すのは今の名前を壊さないので、1.0のあとでもできる。1.0の前に決めなければならないのは「今の名前を付け替えるか」だけ
- おすすめ: 付け替えない（(a)）。名前の意味がボードで違うことは、PIN_MAPPINGの各ファイルとAPI_COMPATIBILITYに表で書く。別名は要望があれば1.xで足す

**2-2 `endTransmission()`の戻り値**: 今はSDKの状態コードを`uint8_t`に切り詰めた値（`arduino_i2c.cpp`の`endTransmission()`が`i2c->write()`の戻り値をそのまま返す）

- 例: LPI2CのNAKは`134`、I3CのNAKは`222`、ビジーは`220`、C444はアドレスのNAKが`81`、データのNAKが`78`、ビジーが`76`、タイムアウトが`80`。成功が`0`なのはAVRと同じ
- AVRの文書の値は、0が成功、1がデータが長すぎる、2がアドレスのNAK、3がデータのNAK、4がその他、5がタイムアウト
- 利用者向けの文書（API_COMPATIBILITY・README・TUTORIAL）には、今の値を書いていない。リポジトリの作例では17か所が今の値（`CODE_NAK`など）を見ている
- `!= 0`で失敗を見るライブラリ（多くはそうしていると見ている。全部は調べていない）はどちらでも動く。値で分けるスケッチ（I2Cスキャナーの`error == 4`など）は、AVRの値でないと意図どおりにならない
- LPI2CのSDKの関数はアドレスのNAKとデータのNAKを同じ値で返すので、2と3を分けるには、何バイト送れたかを見るなどの工夫が要る（調べていない）
- おすすめ: AVRの値に合わせる（(b)）。1.0のあとに変えると壊す変更になる。元のコードが要る人のためには、標準に無い関数（例: `Wire.lastStatus()`）で残せる

**2-4 「公開API」の範囲**: 1.xで壊さないと約束するもの

- 含める候補: API_COMPATIBILITYに載っているArduinoの関数とその動き、ピン名（`D*`・`A*`・`PWM*`・`MB_*`・LEDとボタンの名前）、ボードのマクロ（`ARDUINO_FRDM_*`・`FRDM_*`）、インスタンス名（2-1）、同梱ライブラリのAPI（`EEPROM`・`Servo`・`MCXEthernet`）、標準に無い追加（`analogWriteFrequency()`など）
- 外す候補: 同梱r01libを直接使うこと、SDKの関数を直接呼ぶこと（上級者向けの文書の内容）、ピン名の数値、mcxPinStateの出力の形（人が読むためのもの）、2-2で(b)にしたときの元のコード
- 置き場所の候補: API_COMPATIBILITYの冒頭の短い節（利用者が最初に見る場所）か、別の文書（`COMPATIBILITY.md`など）。おすすめはAPI_COMPATIBILITYの冒頭

## 3. 1.0前に潰したい弱点

- **LinkServer依存**: A153は26.9.130単独だと書き込めず、旧版（26.6.137など）の併存が必要。旧版はNXPのページに載っていない。初めてのユーザーが最初に踏む場所なので、根本対処か、NXPへの報告と回避の自動化のどちらかが要る。
  **LinkServer以外の書き込み手段のうちpyOCDは検討から外す**（以前に、pyOCDにMCXのターゲットが無く、作る工数が大きいとして却下。2026-10-14に再確認して状況が変わっていなかったので、ユーザー判断で外した）。
  再確認の内容: 最新のv0.45.1（7月21日）までの直近5回のリリースノートにMCX・MCXA・MCXNの記述は無い。本体にMCXのターゲットは無く、対応するにはNXPのCMSIS-Pack（DFP）を読み込ませるしかない。
  2026年2月のphoenix-rtos-testsのPR #446に、pyOCDはMCXN947を正式には対応しておらず、コミュニティの方法は必ずしも正しく動かない、という趣旨の記述がある。
  なお、probe-rsはv0.32.0の変更履歴に「MCXN947の書き込みの問題を直した」とある。前の検討で対象にしたかは記録に無い（検討から外すかは未決）。
  2026-11-20に調べ直した（2026-10-31の「最新の版v0.32.0にはまだ入っていない」は誤りだった。`master`の`MCXA1.yaml`だけを見て、v0.32.0の`MCXA.yaml`を見落としていた）:
  **最新の版v0.32.0（2026-07-22、11-20の時点でもこれが最新）に、A153・A156・N947・N236のターゲットが入っている**。
  `probe-rs/targets/MCXA.yaml`にMCXA142〜MCXA577があり、MCXA153はフラッシュ`0x0`〜`0x20000`（128KB）で書き込みのアルゴリズムは`mcxa15x_128`、MCXA156は1MBで`mcxa15x_1024`。N947は`MCXN947.yaml`（`mcxn9xx_2048`）、N236は`MCXN236.yaml`。
  2026-10-23に`master`へ入ったPR #4533（Add all remaining MCXA targets）は、これを`MCXA1.yaml`などに分けてMCXA132・MCXA133などを足したもので、v0.32.0のあとの`master`（434コミット）にあり、まだ版になっていない。
  C444（MCXC444）はv0.32.0にも`master`にも無い。
  分かったのはターゲットの定義があることだけで、MCU-LINKから実際に書き込めるか・デバッグできるかは確かめていない。
  **2026-11-20にA153の実機で試した**（probe-rs 0.32.0を公式のインストーラーで`~/.cargo/bin`に入れた。MCU-LINKのファームウェアはCMSIS-DAP V3.128）:
  - `probe-rs list`でMCU-LINKが見え、`--chip MCXA153`を付ければ読み書き・リセットできる。`probe-rs info`は`--chip`を無視してチップを自動で見分けようとし、見分けられずに失敗する（この版の制限）
  - **32KBを超えるスケッチも書ける**: `hello_world`（39.7KB）と`release_check/01`（71.7KB）を`probe-rs download`で書き込み（4.3秒・7.6秒）、`probe-rs verify`も通った。LinkServer 26.9.130が失敗する大きさ（Pendingタスク9）
  - **probe-rsはリセットのあと切り離すときに`DEMCR`を0に戻す**（TRCENAが消え、DWTの`CYCCNT`が止まる）。Cortex-M33の4ボードの`millis()`・`micros()`・`delay()`は`CYCCNT`で数えるので、`delay()`が終わらなくなった（`release_check/01`が「millis/micros」で止まった。LinkServerで書いた同じバイナリはALL OK）。コアのSysTickの割り込みで毎回TRCENAを入れ直すようにして（`arduino_main.cpp`、0.13.0）、probe-rsで書き込み・リセットしても`01`がALL OKになった（割り込みを切っているときの穴は、下の「このとき`DEMCR`も0になる」の項目で埋めた）
  - **デバッグ（gdb）も動いた**（同じ日、A153）: `probe-rs gdb --chip MCXA153 --reset-halt`（GDBサーバー、`127.0.0.1:1337`）にツールチェーンの`arm-none-eabi-gdb`（15.2）でつなぎ、Sketch → Optimize for Debugging（`-Og`）でビルドした`hello_world`で、リセット直後で止まる、`setup()`・`loop()`のブレークポイント（フラッシュなので自動でハードウェアのブレークポイント）、`bt`、`next`、`step`で`digitalWrite()`に入って引数が見える、`finish`、`delay(100)`を`next`でまたぐ（`millis()`の元の`ms_ticks`がちょうど100ずつ進んだ）、`detach`のあとリセットして動く、を確かめた。つないだ直後に表示される停止位置の関数名がPCと合わない（PCは`ResetISR`なのに`I3C_MasterTransferBlocking`と出る。`--gc-sections`が残すデバッグ情報の古い行の表示で、動作には影響しない）。gdbからは`DEMCR`などのシステムの領域（`0xE000xxxx`）が読めない（probe-rsのメモリマップの外）
  - **4ボードで試した**（2026-11-20、macOS、4枚を同時につないで。C444はUSBに出てこず未確認、probe-rsの対象外）。0.32.0では:
    - A153・A156は、arduino-cliの書き込み（`upload_method=probe_rs`、ポートのシリアル番号でプローブを選ぶ）とgdb-bridge経由のデバッグ（`monitor reset halt`・`load`・ブレークポイント）が安定して動く
    - **N947は、スケッチが動いている状態からの「リセットして止める」が毎回失敗し、その直後は必ず成功する**（10回で成功と失敗が交互。AIRCRの読み出しにFAULT）。`DEMCR`の修正を外しても、先に`DHCSR`でコアを止めても同じで、`--connect-under-reset`は毎回つながらない。やり直しで書けても、そのあとコアが止まったまま残ることがあった
    - **N236は、`probe-rs download`は通るが、gdbからの`load`が失敗する**（毎回「unknown variant」の警告）
    - 0.32.0向けの回避として、書き込みは`download --reset`（同じ接続でリセット。別の`reset`は起動中のチップに当たってFAULTになった）で、失敗したら1回やり直す（2026-11-21に、1秒あけて2回までやり直すように変えた。IDEでデバッグしたあとのN947で、Uploadボタンの書き込みが2回とも失敗したため。そのあとこちらで試した1回目も2回とも失敗したが、それからは続けて10回とデバッグのあとの2回のどれも、1回のやり直しで通り、2回続けて失敗するのは再現できていない）。probe-rsは端末でなくても色・太字の制御文字を付け、すべてを標準エラー出力に出すので、IDEでは成功の「Finished」まで`[m[1mFinished[0m`のようにエラーに見えた。`upload_probe-rs.sh`は出力を標準出力に回して`sed`で制御文字を取り、失敗したときだけ`ERROR:`の行を標準エラー出力に出す（`.bat`は`NO_COLOR=1`と`2>&1`だけで、太字の制御文字は残る。Windowsでは未確認）。さらに、成功した書き込みでも1回目の失敗のエラーが出て紛らわしかったので、失敗した回の出力はためておいて3回とも失敗したときだけ出し、やり直して通ったときは「Written on try 2 of 3 (the first try failed; what it said is left out)」の1行だけを出すようにした（2026-11-21）。IDEからN947へ6回書き込んで、どれも2回目で通った（ユーザーが確認）。N236も1回目で失敗して2回目で通ることがあった。IDEで2回とも失敗した件（2回目は`Flash content verification failed`）は、ターミナルからは20回続けても再現しなかった
  - **SWDのクロックを4MHzにした**（2026-11-21）: probe-rsの既定は1MHzで、Blinkの書き込みと照合にA153・A156で約8秒（LinkServerは約3秒）かかっていた。`--speed`で測ると（照合あり、3回ずつ）、A153は8.4→3.9秒（10MHzで3.5秒）、A156は8.1→3.7秒（3.2秒）、N947は3.5秒（3.3秒）、N236は3.4→1.7秒（1.5秒）。24MHzにしても10MHzと変わらない（プローブ側の上限とみられる）。10MHzの上乗せは小さいので、余裕を見て4MHzにした。`upload_probe-rs.sh`/`.bat`とgdb-bridgeが`PROBE_RS_SPEED`を設定し（probe-rsが自分で読む環境変数）、ユーザーが設定していればそちらを使う
  - **4MHzにするとN947のリセットの失敗がほぼ消えた**: 1MHzでは8回中4回（交互）失敗したが、4MHzでは16回続けて1回目で通った（クロックを変えた直後の2回と、別の測定の最初の1回だけ失敗）。デバッグでも、1MHzでは立ち上げ直しが1回要ったのが、4MHzでは2回とも要らなかった。やり直しの仕組みは残す。デバッグのloadは、A153・A156で8→18KB/s（`load`からloop()に着くまで約5.5→3秒）、N947で13→21KB/s。IDEでも書き込みが速くなり、デバッガの操作の反応も速くなったように感じる、とユーザーが確かめた（2026-11-21）
  - **probe-rs 0.32.0の`download`・`gdb`のオプションを見直した**（2026-11-21）:
    - 入れた: `--speed`（上の4MHz）。`--reset`・`--verify`・`--disable-progressbars`は前から。標準入力を閉じる（`</dev/null`、`.bat`は`<nul`）: プローブが複数で`--probe`が無いと、probe-rsは番号の入力を待ち、`--non-interactive`を付けても待つ（0.32.0の`download`では効かない）。閉じておけばすぐエラーになり、スクリプトの「ポートを選んで」が出る。gdb-bridgeから起動する`probe-rs gdb`は、Goの`exec.Command`の標準入力がもともと`/dev/null`
    - 照合（`--verify`）は、4MHzで約4秒のうち約1.8秒（A153・A156。無しで2.2秒、LinkServerは約3秒）。IDEの「書き込み後にコードを照合する」（`arduino.upload.verify`、IDE 2の既定はオフ。`bundle.js`の設定の定義で確かめた）に従うことにした（2026-11-21、ユーザーが決めた）: `platform.txt`の`tools.probe_rs.upload.params.verify=verify`/`noverify=noverify`を`{upload.verify}`で7番目の引数として渡し、`noverify`なら`--verify`を外す（引数が無ければ照合する。手で動かすとき）。arduino-cliで、`--verify`なしは「Finished in 2.19s」、ありは3.95sになった。照合はIDEで1回、`Flash content verification failed`を捕まえているので、既定がオフだとそういう失敗は「成功」と出る
    - 入れない: `--preverify`（内容が同じなら書き込みを飛ばすが、読むのに書くのと同じ約2.2秒かかり、変えたスケッチでは遅くなるだけ）。`--chip-erase`・`--allow-erase-all`（EEPROMの中身が消える。後者は保護も外す）。`--connect-under-reset`（N947で毎回「Connecting to the chip was unsuccessful」）。`--restore-unwritten`（スケッチの最後のセクターの残りを戻すだけで、要るものが無い）。`--disable-double-buffering`（タイムアウトのときの逃げ道で、起きていない）。`--protocol`（既定がSWD）。`--cycle-power`。`--preset`（設定ファイル）
    - EEPROMの中身は、probe-rsで書き込んでも残る: EEPROMに書くスケッチを入れて、EEPROMの場所を`probe-rs read`で読み、probe-rsでBlinkを書き込んでもう一度読んで比べた。A153・A156・N947・N236とも同じ（LinkServerはv0.7.0で確認済み。CHANGELOGの「uploads leave the storage alone」）gdb-bridgeはGDBサーバーを`--reset-halt`で立ち上げ、立ち上がらなければ1回立ち上げ直す。これでN947も書き込みとデバッグが通るが、N236のデバッグは直らない
  - **Arduino IDE 2（macOS）でも動いた**（2026-11-20、ユーザーが確認）: Tools → Reload Board Dataで「Optimize」「EEPROM」「Upload method」のメニューが出て（boards.txtを書き換えたあとは、読み直すまで出ない）、Upload method → probe-rsでA153に書き込めた。A153のデバッガも、IDEのデバッグボタンからprobe-rsで動いた。2026-11-21にA156・N947のデバッガもIDEから動いた。N236は0.32.0の`load`の失敗で、GDBのセッションが理由を出さずに閉じた（ターミナルには`unknown variant`の警告だけ）
  - **N236＋probe-rs 0.32.0のデバッグは、gdb-bridgeがメッセージを出して止める**（2026-11-21）: `n236-probe-rs.cfg`の`gdb-bridge-probe-rs-debug-broken-up-to: 0.32.0`の行で、`probe-rs --version`がその版以下なら「probe-rs 0.32.0 can't debug the FRDM-MCXN236 (uploading works). To debug it, choose Tools > Upload method > LinkServer」と出して終わる。LinkServerへ黙って切り替える案は採らなかった（LinkServerが無い人には別の分かりにくいエラーになる）。開発版も`--version`は`0.32.0`と出すが、開発版ではチップ名が`MCXN236`に変わっていて今のcfgでは使えないので区別しない。同梱を上げるときにこの行を消す。IDEのデバッグボタンからも、このメッセージが出て止まることをユーザーが確かめた（2026-11-21）
  - **probe-rsの`master`（`187f19a8c`、2026-11-19。ソースからスクラッチパッドにビルド）ではN947・N236とも直っている**: N947は書き込み10回中9回続けて成功（失敗は最初の1回、0.32.0で残した状態から）、デバッグも立ち上げ直しなしで続けて通る。N236は`load`が通り、書き込みも6回続けて成功。A153・A156は書き込み・デバッグとも変わらず通る。効いたとみられるのは「Add support for all MCX N variants」（#4628）とMCX Nのリセットの処理の作り直し
  - **次のprobe-rsの版で同梱を上げるときに要る変更**: `download`の`--reset`が無くなり`--start`になった（`upload_probe-rs.sh`/`.bat`）。N236の名前が`MCXN236VDF`から`MCXN236`になった（`boards.txt`の`build.probe_rs_chip`と`n236-probe-rs.cfg`）。`n236-probe-rs.cfg`の`gdb-bridge-probe-rs-debug-broken-up-to:`の行を消す。0.32.0向けのやり直しは要らなくなる見込み。そのうえでN236の既定もprobe-rsにするかを決める（デバッグが通るなら、`boards.txt`でN236の`probe_rs`の行を`linkserver`の前に移す）
  - **A153・A156・N947の既定をprobe-rsにした**（2026-11-21、ユーザーが決定）: Upload methodのメニューは`boards.txt`で最初に書いた選択肢が既定になるので、`menu.upload_method.probe_rs`の行を`linkserver`より前に移した（`arduino-cli board details`で✔がprobe-rsに付き、`upload.tool`と`debug.server.openocd.script`がprobe-rsのものになることを確かめた）。パッケージを入れれば、ほかに何も入れずに書き込みとデバッグができる。LinkServer 26.9がA153を32KBと読む件（`CLAUDE.md`のPendingタスク9）にも、既定のままなら当たらない。N236は0.32.0でデバッグできないのでLinkServerのまま、C444はLinkServerだけ。当初は「次のprobe-rsの版で決める」としていたが、3ボードは0.32.0で書き込みもデバッグも通り（N947はやり直しつき）、前倒しした
  - **probe-rsで書き込んだ（リセットした）A153・A156は、スケッチの起動から約100msでCPUが4〜8ms止まる**（2026-11-21、`test_Servo_all_pins`のD0の失敗から見つけた）: probe-rsのMCX Aのデバッグシーケンス（MCXのシーケンスの`reset_system`と`wait_for_stop_after_reset`）は、ブートROMの出口で止めるウォッチポイント（`0xE0001020`=`0x40091036`/`0xF0000412`、`0xE0001030`=`0x40091040`/`0xF0000403`）を置いてSYSRESETREQでリセットし、100ms待ってから「念のため」`C_HALT`で止め、ウォッチポイントを消す。A153・A156ではウォッチポイントで止まらず、スケッチが走り出してから止められる。SysTickも止まるので`micros()`の飛びとして測れる（CTIMER2は進み続ける）。0.32.0も開発版（`187f19a8c`）も同じ。N947・N236、LinkServer、電源投入・RESETボタンでは起きない。LinkServerは`0x40091040`への読み出しのウォッチポイント1つでROMの出口に止め（ログの`Boot ROM stalled accessing address 0x40091040`）、そこから走らせるので止まらない（LinkServerは書き込みの前にRESETのピンでリセットするので、古いスケッチが一度起動し直す。SRS=`0x110`）。避ける方法は0.32.0のCLIに無かった: `--reset`を外すと書き込みのあとロックアップのリセット（SRS=`0x8010`）で起動し、そのあと`probe-rs write b32 0xE000ED0C 0x05FA0004`でSWリセット（SRS=`0x4010`）すると止まらないが、スケッチが2回起動する。probe-rsはA15xにつなぐたびに（`read`だけでも）CPUを約6msずつ止めるので、ロックアップで起動した直後につなぐと失敗することもあった。`probe-rs run`はRTTのためにつながったまま終わらない。影響は書き込み直後の1回だけなので、そのままにし、CHANGELOG・API_COMPATIBILITY・`upload_probe-rs.sh`に書き、`test_Servo_all_pins`は始めに300ms待つようにした（ユーザーが決定）。直すならprobe-rsの側（ウォッチポイントの設定）
  - **このとき`DEMCR`も0になる**（同じ2026-11-21、release_checkで見つけた）。SysTickのたびにTRCENAを入れ直すだけでは、割り込みを切っているあいだは働かず、`SDK_DelayAtLeastUs()`（M33の`delay()`と`delayMicroseconds()`）の待ちのループでCYCCNTが止まったまま抜けなくなった（A153に`test_micros_noInterrupts`を書き込むと19回中5回止まった。gdbで見るとDEMCR=0、CYCCNTが目標の497サイクル手前で止まっていた）。割り込みを切った`micros()`も止まったままになる。最初は待ちのループの中で入れ直したが、`micros()`が漏れ、DEMCRを毎回読み書きするとデバッガがほかのビットに書いた値を消しうるので、2026-11-22に**CYCCNTを読むのを`MSDK_GetCpuCycleCount()`（`fsl_common_arm.c`、`mcx-arduino-core:`の印）の1か所にまとめ、TRCENAが落ちていたときだけ入れ直す**形に作り直した（`micros()`・SysTickもこれを通す。SDKのドライバの待ちも同じ関数を通る）。`test_micros_noInterrupts`に、割り込みを切ってスケッチ自身がDEMCRを0にし、`micros()`が進むかを見る項目を足した（前のコアではN947で`micros()`が0usのまま止まることを確かめた。`delay()`・`delayMicroseconds()`は始めにTRCENAを入れるので、待ちの途中で消される形はスケッチからは作れない）。`release_check/09`の`millis()`の判定は一度150以上に緩めたが、直したあとは常に200だったので200に戻し、値を出す行だけ足した
  - **DEMCRを0にするのはprobe-rsの共通の`debug_core_stop()`**（`architecture/arm/sequences.rs`。CMSIS-PackのDebugCoreStopの既定と同じで、手を離すときにDHCSRとDEMCRを0にする）。MCX Aの手順はTRCENAを入れる側。不具合と言えるのは、`reset_system()`のウォッチポイントでA153・A156が止まらないことのほう。2026-11-22にpyOCD（`-t cortex_m`、`write32`でワード単位に書く。probe-rsのgdbサーバー経由ではDWTへの書き込みが効かなかった）でDWTを直接書き、SYSRESETREQのあと300msでDHCSRと比較器のMATCHEDを見た:

    | 設定 | A153 | A156 |
    |---|---|---|
    | probe-rsのMCX A（`0x40091036`/`0xF0000412`、`0x40091040`/`0xF0000403`＝命令の番地の範囲） | 止まらない | 止まらない |
    | データの読み出し`0x40091040`（FUNCTION=`0x816`） | ROMの`0x0300331A`で止まる | 止まらない |
    | データの読み出し`0x4009123C`（同じ） | — | ROMの`0x030033AA`で止まる |
    | 命令の番地の範囲`0x0`〜`0x00FFFFFF`（probe-rsのMCX Nと同じ形） | スケッチの最初の命令（`0x1D6`）で止まる | 同じく（`0x1FA`）で止まる |
    | 何も置かない | 止まらない | 止まらない |

    LinkServerの`devices/MCX/MCXA/*.json`の`bootromstall`は、A153・A152・A143・A142が`0x40091040`、A156・A155・A154などが`0x4009123C`、A45x・A5xxなどが`0x5009123C`。probe-rsは`0x40091040`がデータの読み出しで止める番地なのに、命令の範囲の比較器（MATCH=`0b0010`/`0b0011`）に入れていて、さらにA1（A156など）にもA0と同じ番地を使っている。probe-rsのmain（2026-11-17の`f39123d21`）も同じ値。N236はprobe-rsのN0（N947・N526）に入らず、既定のフラッシュの範囲（`0x0`〜`0x000FFFFF`）で止まるので起きない。直すなら、MCX Aもフラッシュの範囲にする（チップごとの番地が要らない）か、チップごとの番地をデータの読み出しで置く。2026-11-22にprobe-rsへ報告した（probe-rs/probe-rs#4757、https://github.com/probe-rs/probe-rs/issues/4757 ）。直った版が出たら、同梱のprobe-rsを上げて、A153・A156の書き込み直後の停止が消えたかを確かめる
  - **1.0に向けて: M33の時間の元をデバッグ用でないタイマーに移すかを決める**（2026-11-22、ユーザーが決定。0.13.0は上の1か所での入れ直しで済ませた）。CYCCNTはデバッガが自由に書き換えてよいDWTのものなので、入れ直しても、消されてから次に読むまでの時間は失われる。候補はOSTIMER0（A153・A156・N947・N236の4つともあり、`clk_1m`で1MHz、42ビット。コアはどこでも使っていない）。C444はSysTickで数えていて、この問題が無いので変えない。調べること: カウンタのグレイコードの読み替え（ハードウェアで読めるか）、下位と上位を食い違いなく読む手順、`clk_1m`の元のクロックの精度（いまはコアのクロックの精度）、デバッガで止めているあいだも数えるか。ふるまいの変化: `micros()`の刻みが約10nsから1µs（AVRは4µs）、ブレークポイントで止めているあいだも時間が進みうる、`millis()`の精度。SysTickは`MsTimer2`・`mcxTicker`の1msの割り込みのために残す。割り込みを1ms以上切っても時間を取り戻す0.11.0の働きは、数え続けるタイマーならそのまま保てる。見積もりは2〜3日（調べる半日、作る半日〜1日、確かめる1日＝ロジックアナライザでの測定を含む、書く半日）。時間に関わるrelease_checkはやり直しになるので、それだけの版で行う
    - **2026-11-26に検討し、Bに決めた（ユーザーが決定）**。0.14.0で入れる。調べた事実:
      - CYCCNTを読むのは`MSDK_GetCpuCycleCount()`だけで、使うのは`micros()`、SysTickの割り込み（何ms進んだか）、`SDK_DelayAtLeastUs()`の3つ。最後のものは、M33の`delay()`・`delayMicroseconds()`（`wait()`・`wait_us()`経由）と、SDKのドライバー（`fsl_vref.c`・`fsl_flash.c`の数µs〜5ms、`i2c.cpp`の`wait_us()`）が使う
      - **TRCENAの入れ直しで残る弱点は、止まった時間だけではない**。デバッガやIDEがCYCCNTを0に書き戻すと（サイクル数を測る機能など）、SysTickの割り込みの`( CYCCNT - ms_tick_dwt ) / cycles_per_ms`が32ビットで回り込み、`millis()`が一度に最大で約44秒（96MHz）・約28秒（150MHz）進む。確かめてはいない
      - OSTIMER0は4ボードともあり、`FSL_FEATURE_OSTIMER_HAS_BINARY_ENCODED_COUNTER`も`FSL_FEATURE_SYSCTRL_HAS_CODE_GRAY`も無い（SDK 25.12・26.06の`*_features.h`）。カウンタはグレイコードで、変換はソフトウェア（64ビットのXORとシフトを6回）。2進の下位のビットは上位のビットにもよるので、`EVTIMERH`・`EVTIMERL`・`EVTIMERH`と読んで食い違いを避ける手順が要る（SDKの`OSTIMER_GetCurrentTimerRawValue()`はL・Hを1回ずつ読むだけ）。クロックは`clk_1m`（A15xは`kCLK_1M_to_OSTIMER`、N94x・N23xは`CM_OSTIMERCLKSEL`の2）
      - 4ボードともコアクロックはFROが元で（A15xはFRO_HF、N947・N236のPLL0もFRO_HFの48MHzから）、水晶ではない。`clk_1m`はFRO12Mを12で割ったもの。FRO12MとFRO_HFの精度の差は、データシートで確かめていない（ChipDocsが使えなかった）
      - C444は、`micros()`を`ms_ticks`と`SysTick->VAL`（とPENDSTSET）で読み、`delay()`・`delayMicroseconds()`も`SysTick->VAL`を数える。`release_check/09`の「割り込みを700µs止めてSysTickを遅らせる」項目もC444は1008〜1013µsで通る
    - 案と比べ:

      | | A: いまのまま（CYCCNT＋TRCENAの入れ直し） | B: SysTickだけで数える（C444と同じ作り） | C: OSTIMER0 |
      |---|---|---|---|
      | デバッガの影響 | TRCENAを消されると最大1msを1回失う。CYCCNTを書き戻されると`millis()`が数十秒進む | 受けない | 受けない |
      | 割り込みを1ms以上切ったとき（EEPROMの消去など） | 取り戻す（0.11.0から） | 取りこぼす（AVRと同じ） | 取り戻す |
      | `micros()`の刻み | 約10ns（コアクロック） | 同じ | 1µs（AVRは4µs） |
      | ブレークポイントで止めている間 | 止まる | 止まる | 進む（未確認） |
      | 精度の元 | FRO_HF | 同じ | FRO12M（差は未確認） |
      | 5ボードの作り | M33とC444で2通り | 1通りにまとまる | M33とC444で2通り |
      | ほかの効き目 | — | M33の`delayMicroseconds()`が`wait_us()`の浮動小数点の計算をやめ、A153・A156で約14µs長く待つ件も消える | — |
      | 手間 | 0 | 約1日（C444の分岐をM33にも使う。`release_check`の`01`・`09`と`test_micros_noInterrupts`を4ボードで） | 2〜3日（上の見積もり） |

    - **Bを確かめている途中で、probe-rsのもっと重い件が見つかった**（2026-11-26）: probe-rsで書き込んだA153で`release_check/01`の`delay(200)`が8回中3回、実時間（OSTIMER0で測った）で約90msで戻った。レジスタ・RAMは壊れておらず、ITブロックの条件が成り立たないのに中の命令が実行されていた（A153で10回中7回、A156で6回中2回。LinkServerのA153とprobe-rsのN947では0）。probe-rsが起動から約100msでコアを止めて離すときに、xPSRのITの状態が消える。`delay()`のループの`itet hi`の途中で止まると経過がマイナスに回り込み、その場で戻っていた。古いコアの`delay()`（CYCCNT）はたまたま当たりにくかった。対策（ユーザーが決定）: `start_mcu()`で、`DHCSR.C_DEBUGEN`が消えるまでITブロックの無いアセンブラのループで待つ（1回り7サイクル、最長250ms）。直したあと、ITのテストはA153・A156で10回ずつ0、`delay(200)`は199.993〜200.004ms、`01`は8回ずつALL OK、probe-rsのgdbのデバッグも動いた。ITの状態が消える件は、probe-rs/probe-rs#4757にコメントで書き足した（2026-11-26）
    - おすすめ（2026-11-26）: **B**。デバッガの影響を受けなくなり、5ボードの作りが1つになり、手間も小さい。失うのは「割り込みを1ms以上切っても取り戻す」働きだけで、これはAVRでも取りこぼす。EEPROMの消去のように長く切る所は、取りこぼす時間をAPI_COMPATIBILITYに書く。取り戻す働きを残したいならC。SDKのドライバーが使う`SDK_DelayAtLeastUs()`はCYCCNTのまま残す（短い待ちで、TRCENAの入れ直しが効く）
- ~~**N947の`Wire2`がターゲットとして応答しない**~~ → 既知の制限として確定した（2026-10-30、ユーザーが決定。§9の⑩）。原因は分からないまま（A156の同じLPI2C3は応答する）。`API_COMPATIBILITY.md`のI2Cのターゲットモードの行に、`Wire2`は❌と書いてある
- ~~**ネットワーク基底クラスが無い**~~ → 0.10.0で入れた（§6の段階A）
- **OneWireは「フォールバック」で動く**（2026-10-23に調べた。OneWire 2.3.8）: `util/OneWire_direct_gpio.h`はチップやアーキテクチャのマクロで場合分けし、このコアはどれにも当たらないので、
  最後の`#else`で1ビットごとに`pinMode()`・`digitalWrite()`・`digitalRead()`を呼ぶ（ビルドすると`#warning "OneWire. Fallback mode. ..."`）。
  1-Wireはマイクロ秒単位のタイミングなので、`pinMode()`が重いと動かない。どのボードで間に合うかは、DS18B20などで実機で確かめるまで分からない。
  対策の案: OneWireに`ARDUINO_ARCH_MCX`の分岐を足す（上流への提案。このリポジトリではできない）。
  このコアの`portOutputRegister()`は`PDOR`を返し、そこからの`PSOR`(+1)・`PCOR`(+2)・`PDIR`(+4)・`PDDR`(+5)の並び（32ビット単位）はMCXの4チップ（`PDOR`が0x40）とC444（0x0）で同じなので、
  SAMD21の分岐と同じ形（`PIN_TO_BASEREG`に`portOutputRegister(digitalPinToPort(pin))`、`PIN_TO_BITMASK`に`digitalPinToBitMask(pin)`）で5ボードとも書ける。`util/OneWire_direct_regtype.h`にも同じ分岐が要る
  → **DS18B20での実機の確認は、いまはしない**（2026-10-25、ユーザーが決定）。1-Wireの互換性が必要になったときにやる
- **Adafruit NeoPixelはビルドが通るが何も送らない**（2026-10-23に調べた。1.15.5）: `show()`はARMのチップごとに`#if`/`#elif`で信号を出すコードを選び、その並びは`#else`の無いまま`#endif // ARM`で終わる。
  このコアはどれにも当たらないので、`show()`は何もしない（エラーも警告も出ない）。API_COMPATIBILITYの「Third-party libraries」に書いた。
  直すには、NeoPixelの上流に分岐を足してもらう。Renesas（UNO R4）の分岐はDWTのサイクルカウンタでタイミングを取っていて、MCXの4ボード（Cortex-M33）なら同じ形が使えそう。C444（M0+）にはDWTのカウンタが無いので別の形が要る（調べていない）
  → **実機の確認は、いまはしない**（2026-10-25、ユーザーが決定。OneWireと同じく、必要になったときにやる）
- **サードパーティライブラリの回帰**: 定番ライブラリ（Adafruit系など）をCIのコンパイル対象に入れ、「互換」の根拠にする。→ 2026-10-23にCIへ入れた（`.github/third_party/`。0.7.0と0.10.0で手で確かめた9つ、§9の⑨）

## 4. ボード追加の扱い

### 4-1. 方針案
- **C444**: 1.0の必須条件にしない。追加自体は非破壊なので1.xでよい。ただし1.0前に**「ビルドが通るだけ」のスパイク**を一度やり、M0+や別系統IPのせいで公開側（§2）の変更が要らないことだけ確認する。
  **2026-10-14にユーザーがこの方針で決定**（N947のEthernet（§6）と比べて、1.0前はEthernetを入れ、C444は1.xに回す順番。理由は§6-5）。
  **2026-10-16に変更: C444は0.10.0で入れる（ユーザーが決定）**。スパイク（§4-5）で、既存のボードに壊す変更が要らないと分かった。
  ネットワーク基底クラス（Ethernetの段階A）が0.10.0に入るので、Ethernetの残り（ENET・lwIP）の利点はN947の利用者に限られる。
  0.10.0は機能の追加だけで急ぐ修正が無く、リリースチェック（全ボードの実機と3OSのステージング）を1回にまとめられる。Ethernetの残りは0.11.0
- **USB**: 1.xの追加機能

### 4-2. C444を入れる意味
- RAM 32KBでArduinoCore-zephyrでは載らない領域。ネイティブコアの存在意義が最も出る
- MCXで唯一のCortex-M0+（ARMv6-M）。コアに紛れたM33前提の実装が出る
- Kinetis KL系の旧世代IP（`fsl_i2c`、`fsl_spi`、TPM、ADC16、MCG-Lite）。抽象層が特定IPに依存していないことの検証になる
- A/N/Cが揃うことによるラインナップ上の説得力
- セグメントLCDは入門デモの題材になる
- 割り引く点: I3Cが無い、フラッシュ256KB/RAM 32KBで今後の拡張に制約、工数が大きい

### 4-3. r01libのC444対応状況（2026-08-11時点のコミット）

| クラス | 状況 |
|---|---|
| GPIO（`io`） | 対応。Alt番号が1つずれる件（Alt1=GPIO）は吸収済み |
| Serial | 対応。LPUART0（USBTX/RX = D1/D0）とLPUART1（MB_TX/RX）、割り込み駆動 |
| I2C | 対応（`fsl_i2c`）。I2C1固定。D18/D19（Alt6）またはA4/A5（Alt2） |
| SPI | 対応（`fsl_spi`）。SPI1固定 |
| InterruptIn | 対応。PORTA/C/Dのみ（ハード制約） |
| Ticker | 未対応。UTICKが無く、クラスごと`#ifndef`で除外 |
| AnalogIn / PwmOut | 未対応（A153限定にガード） |
| I3C | 非対応（ハードに無い） |

補足:
- `PORT_SetPinOpenDrain`相当はC444では空実装。GPIOのオープンドレイン出力は使えない
- `wait()`が`double`演算。FPUの無いM0+ではソフトフロートになる（コードサイズと短い待ち時間の精度）

### 4-4. C444の工数
- `CLAUDE.md`の「A156の4〜6倍」が妥当。コアはアナログ系を自前実装しているので、ADC16/TPMによる`analogRead`/`analogWrite`/`analogWriteFrequency`/`tone`の新規実装はC444固有の工数になる
- GPIO/Serial/Wire/SPI/`attachInterrupt`だけならr01libがそのまま使え、差分ポート＋M0+対応に近い
- C444固有で残るもの: M0+のコンパイルフラグ・スタートアップ・リンカスクリプト、Ticker相当（LPTMRかPITで代替）、アナログ系のバックエンド、RAM 32KBに合わせたバッファ既定値、ピン割り込みのポート制約の扱い

### 4-5. スパイクの結果（2026-10-16、`c444-spike`ブランチの`e4cd50c`）

`0.9.2-dev`（いまの`0.10.0-dev`）から切ったブランチで、SDK 2.16.000の`FRDM-MCXC444`から`variants/frdm_mcxc444/`を作り、`boards.txt`に足して、
コアのC444で通らない所を`SPIKE`と印を付けたスタブやガードで通した。`hello_world`がリンクまで通り（フラッシュ51KB、RAM 13KB）、全106本の作例のうち76本が通った。
スタブは本番に持ち込まず、0.10.0では`0.10.0-dev`の上で作り直す。実機では動かしていない。

- **既存のボードの公開APIを壊す変更は要らない**。見つかった公開APIの問題は、0.10.0（旧0.9.2）で入れた`pgm_read_word`/`dword`が揃っていないアドレスでM0+のフォールトになることだけで、
  `memcpy()`で読む形に直した（`6d4a31d`、2バイト以上を読むもの全部。M33では同じ1回の読み出しのまま）
- **ボードの起動（FOPT）**: D4（PTA4、SW3）はリセット直後NMIの端子で、LOWのまま起動するとNMIで止まる。r01libのC444のテンプレートのプロジェクトと同じく、
  フラッシュ設定の`FOPT`を`0x3B`（`NMI_DIS`を0にしてNMIを常に止める）にした（SDKのままだと`0x3F`）。`FSEC`は`0xFE`（保護なし）のまま。`0x3D`（`BOOTPIN_OPT`）は別の効果で、対策にならない
- **名前**: `MCXC444.h`が`SPI0`/`SPI1`をマクロで定義していて、Arduinoの`SPI1`とぶつかる。C444はMikroBusのSPIが`SPI`と同じ線なので、N236と同じく`SPI1`を置かない。
  マクロがスケッチから見えると`SPI1.begin()`が分かりにくいエラーになるので、コアで隠す方法を考える。`PWM0`〜`PWM5`・`I3C_SDA`/`I3C_SCL`はC444に無い（ピン番号の表の後ろの名前なので、外しても他のボードの番号は変わらない）
- **Serial**: USBのシリアルとD0/D1は同じUART（LPUART0）なので、両方とも`Serial`にする（2026-10-16、ユーザーが決定。UNO R3と同じ）。`Serial1`は置かない（2026-10-16、ユーザーが決定）。
  スパイクではMikroBusのTX/RX（D18/D19、`Wire`と同じピン）に仮に置いていた。LPUARTに7ビットのモードが無く、`SERIAL_7xx`は工夫が要る
- **M33前提の作り**: `micros()`のDWT（スパイクではSysTickで代えた）、フォールトの報告のCFSR/HFSR/BFAR/MMFAR、`semihost_hardfault.c`のThumb-2命令とHFSRの読み出し、
  スタックの上限（`MSPLIM`）がM0+に無い（スタックがあふれても止まらない。0.10.0でスタックをRAMの下端に置き、LOCKUPのリセットを起動時に報告する形にした。2026-10-18）
- **周辺回路**: CTIMERが無い（`tone()`はTPMかPIT。§7-2のServoのCTIMER1もC444では別のタイマーにする）、ADC16（`analogRead`）、TPM（`analogWrite`・`analogWriteFrequency`）、
  Kinetisの`fsl_i2c`（`Wire`のターゲットモードと`setWireTimeout()`）、`SPI::bit_order()`がr01libに無い、PORTに`IBE`が無い、EEPROM（FTFA、消去の単位が1KB）
- **構造**: `cores/arduino/sdk/`の`fsl_i3c.c`・`fsl_pwm.c`・`fsl_spc.c`・`fsl_utick.c`・`fsl_inputmux.c`は全ボードでビルドされ、C444で通らない。
  使うボードのvariantへ移すか、ファイルをガードで包むかを決める
- **周り**: `Wire1`は加速度センサーのI2C0にする（2026-10-16、ユーザーが決定。A4/A5は`Wire`と同じI2C1なので§5の`setPins`で扱う）、ピン表を回路図と突き合わせる（r01libのC444の分は未確認）、
  作例・`release_check`（I3Cのセンサーが無く加速度センサー）・mcxPinState・Servo・mcxRCServoの作例（`PWM0`）・gdb-bridge・文書

## 5. A4/A5でI²Cを使えるようにする

**2026-10-30に作りが決まり、0.12.0で入れた**（ユーザーが決定。`release_check/16`が実機の5ボードでALL OK、2026-10-30）。決まったことは`CLAUDE.md`の残りのPendingタスク10、経緯は`docs/DEVELOPMENT_LOG.md`のv0.12.0の節に移した。2026-10-08に調べた事実と論点（ボードごとのペリフェラル、`TwoWire`のコンストラクタの件、ESP32・RP2040・STM32の形）は、この節の2026-10-29までの版（gitの履歴）にある

## 6. N947のEthernetで基底クラスを確かめる案（2026-10-14）

§3のネットワーク基底クラスを、W5500のシールドの代わりにFRDM-MCXN947のオンボードEthernetで、実際の通信まで確かめる案。
公式の`Ethernet`ライブラリはWIZnetのチップ（W5x00）のSPIドライバで、TCP/IPはチップのハードウェアが処理する。N947にあるのはMAC（ENET）とPHYだけなので、
このライブラリはN947のEthernetを使えない。lwIPとENETのドライバを使う、別のライブラリを作ることになる。
これで確かめられるのは基底クラスが実際の通信で動くことまでで、**公式の`Ethernet`ライブラリ＋W5500はビルドの確認止まり**（Pendingタスク8の「W5500系シールドなど」はこれで満たすとみなす。2026-10-18、ユーザーが決定）。
→ 2026-10-24に、ユーザーがW5500 Lite（モジュール）を手に入れ、A153で公式の作例12本、A156・N236・C444・N947で4本ずつが動いた（DEVELOPMENT_LOGの「公式`Ethernet`をW5500で」）。

### 6-1. ボードの事実（回路図`ref/FRDM-MCXN947SH.pdf`の8ページ（ETHERNET PHY）と3ページ（SOC PORT0~2）、Zephyrの`frdm_mcxn947-pinctrl.dtsi`・`frdm_mcxn947.dtsi`で確認）

- PHYはMicrochip LAN8741A（U9）、RMII。RJ45はJ16。RMIIの50MHzの基準クロックはボード上の発振器（Y3）から`P1_4`（`ENET_TXCLK`）に入る
- ピン: `P1_4`〜`P1_7`（TXCLK・TXEN・TXD0・TXD1）、`P1_13`〜`P1_15`（RXDV・RXD0・RXD1）、`P1_20`（MDC）、`P1_21`（MDIO）。
  `P1_4`はSJ27（工場出荷時は1-2＝Ethernet側）、`P1_5`〜`P1_7`はR160〜R162（0Ω）でカメラの線と分かれている
- **`P1_21`（MDIO）はArduinoの`D5`と同じ配線で、R166（0Ω）でつながる**。MDIOはPHYの横でR155（1.5kΩ）により`EPHY_VDDIO`へ引き上げられている。
  **R166は実装されている**（2026-10-24、実機で確認。Ethernetを使わないスケッチで`D5`を`LOW`に駆動してから`INPUT_PULLDOWN`にしても`HIGH`と読めた。比べた`D6`は`LOW`に落ちた）。
  そのため、Ethernetを使っている間は`D5`を使えず、使わないときも`D5`に1.5kΩの引き上げがかかる
- ほかのArduinoのピンとは重ならない（同梱r01libの`io.h`のN947のブロックで、`#define`があるのは`D5`=`P1_21`だけ）
- MACアドレスを持つEEPROMは無い（ZephyrはMACアドレスを毎回ランダムに作っている）
- Zephyrのピン設定の名前は`pinmux_enet_qos`。SDKのドライバは`fsl_enet`（`devices/MCXN947/drivers/`）で、PHYは`components/phy/device/phylan8741/`（2026-10-23に`ref/SDK_2_16_000_FRDM-MCXN947.zip`で確認）。
  ピンは10本ともALT9。SDKの作例（`driver_examples/enet/txrx_rxpoll`）は、RMIIの基準クロックを外から入れる`kNONE_to_ENETRMII`、`kCLOCK_Enet`、`PRESETCTRL2`のENETのリセット、PHYのアドレス0
- SDKの作例はMACアドレスを`SILICONID_ConvertToMacAddr()`（`components/silicon_id/`）で作る。NXPのOUI（`54:27:8D`）とチップのIDの3バイト
- 今のコアにENETのドライバもlwIPも無い。`ref/`のN947のSDKのzipにはlwIPが入っていない。SDK Builderで、lwIPを含めて取り直す（2026-10-23、ユーザーが決定）
- ユーザーが`ref/SDK_2_16_000_FRDM-MCXN947_network.zip`を置いた（2026-10-23）。lwIPは2.2.1（`middleware/lwip/`、BSD-3）。
  `devices/MCXN947/drivers/`のうちコアにある21本と`fsl_enet`は、前のzip・コアとバイト単位で一致する。
  ENETのportは`ethernetif.c`・`enet_ethernetif.c`（`ethernetif_get_enet_base()`だけ）・`enet_ethernetif_lpc.c`（N947の`FSL_FEATURE_SOC_MCX_ENET_COUNT`はこちら）。
  RTOSなしの作例（`lwip_examples/lwip_dhcp/bm`）はメインのループで`ethernetif_input()`と`sys_check_timeouts()`を回す。
  portはRTOSなしでもENETの送信完了の割り込みを有効にし、その中でpbufを解放する（`LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT`）。`sys_arch.c`の`sys_check_core_locking()`は割り込みの中からの呼び出しをassertで止める
- スパイク（2026-10-23、スクラッチパッド、ビルドだけ）: lwIP・port・`fsl_enet`・`fsl_phylan8741`を仮のライブラリにまとめ、`sys_now()`は`millis()`、`sys_check_core_locking()`は何もしない自前のものにした。
  ENETの割り込みはNVICで止め、CTIMER2の1msの割り込みで`ETHERNET_DriverIRQHandler()`（送信の回収）・`ethernetif_input()`・`sys_check_timeouts()`を呼ぶ。
  作例の`lwipopts.h`（`MEM_SIZE`22KB、TCPあり）のままで、DHCPのスケッチはフラッシュ108,768バイト・RAM 72,276バイト。`Serial`だけのスケッチ（48,712・22,264）より約60KB・約50KB多い。
  portはこのほか`fsl_adapter_gpio.h`（型だけ）と`ethernetif_mmac.h`を読み込む

### 6-2. 作業の段階

| 段階 | 中身 | 規模 |
|---|---|---|
| A. 基底クラス | `Client`・`Server`・`UDP`・`IPAddress`を自前で書く（ArduinoCore-APIはLGPLなのでコピーしない）。全ボードで公式`Ethernet`とArduinoJsonのEthernet系サンプル3本のビルドを確かめる | 小 |
| B. ドライバとlwIP | N947のSDK（lwIP込み）から、ENET・PHY（LAN8741）のドライバとlwIPを入れる。`NO_SYS=1`、時刻は`millis()` | 大 |
| C. Arduino向けのクラス | 公式`Ethernet`と同じ使い方: `begin(mac)`（DHCP）、固定IP、`EthernetClient`・`EthernetServer`・`EthernetUDP`、DNS、`linkStatus()`、`maintain()` | 中 |
| D. 確かめ方と文書 | `release_check`の`2n`（LANケーブルとPC）、ピン配置、API_COMPATIBILITY、LICENSE（lwIPとSDKはBSD-3） | 中 |

- **2026-10-23の時点**: B・Cは`libraries/MCXEthernet/`にでき（`37df4bc`）、公式`Ethernet`の作例13本が`#include`の差し替えだけでビルドできる。Dは文書（LICENSE・ライブラリのREADME・PIN_MAPPING_N947・API_COMPATIBILITY・CHANGELOG・README）と`release_check/25_ethernet_pc_N947`（ボードのスケッチとPC側の`pc_check.py`）を書いた。どれも実機ではまだ動かしていない
- 全体でボード1枚の追加と同じくらい（1つの版）。Aだけを先に別の版で出すこともできる（追加だけで、今のスケッチを壊さない）。
  **Aは0.10.0で先に出し、B〜Dは0.11.0で入れる**（2026-10-15に決定、版の番号は2026-10-16に変更、§9）
- B・Cはコアではなく同梱ライブラリ（`libraries/`）にする案。使うスケッチのときだけlwIPがビルドされ、ほかのボード・スケッチのビルド時間とバイナリは変わらない
- lwIPを入れたときのフラッシュ・RAMの使用量は、ビルドして測るまで分からない
- 確かめる環境: N947をDHCPのあるLANにつなぎ、同じLANのPCとの間でTCPのクライアント・サーバ、UDP、DHCP、DNSを見る

### 6-3. lwIPの回し方（2026-10-14、ユーザーが決定）

**1msのタイマー割り込みでlwIPを回す。受信もENETの割り込みを使わず、そのタイマーで取りに行く。
スケッチ側からlwIPや受信データに触る所は、すべてそのタイマーの割り込みだけを`NVIC_DisableIRQ()`/`NVIC_EnableIRQ()`で止めて行う**
（SysTick（`millis()`）やシリアルの割り込みは止めない）。

決めた理由:
- UNO R3＋Ethernetシールドでは、TCP/IPをW5x00のハードウェアが処理し、公式`Ethernet`ライブラリには裏で動く処理が無い
  （`available()`・`read()`は呼ばれたときにSPIでチップのレジスタを読むだけ。スケッチが定期的に呼ぶのは、DHCPのリース更新の`Ethernet.maintain()`だけ）。
  R3のスケッチをそのまま動かすには、スケッチが何も呼ばなくてもlwIPが回り続ける必要がある。`loop()`のあとや`delay()`の中で回すと、スケッチが処理を止めている間にTCPの応答が遅れる
- STM32の`STM32Ethernet`（MCU内蔵のEthernet＋lwIP、RTOSなし）は、1msのハードウェアタイマー（既定はTIM14）の割り込みで、受信の取り込み・500msごとのリンク確認・`sys_check_timeouts()`・DHCPを回している（初期化とDNSの待ちでも呼ぶ）
- ただし`STM32Ethernet`には排他が無い（`src/utility/stm32_eth.cpp`・`src/EthernetClient.cpp`・`src/lwipopts_default.h`を読んで確認）。
  割り込みの中で受信のコールバックが`pbuf`の鎖につなぎ足し、スケッチ側の`read()`が同じ鎖を外して解放する。`connect()`・`write()`・`flush()`は`tcp_connect`・`tcp_write`・`tcp_output`を直接呼ぶ。
  どこにも割り込みを止める処理や鍵が無く、`NO_SYS=1`・`SYS_LIGHTWEIGHT_PROT=0`。lwIPは`NO_SYS=1`のとき、すべての呼び出しを1つの流れから行う前提なので、その前提を守っていない
  （実際に不具合として報告されているかは、Issueを見ていないので不明）
- `SYS_LIGHTWEIGHT_PROT=1`が守るのはメモリの確保・解放だけで、`tcp_write`などの呼び出しは守らない。そのため割り込みを止める形にする
- 止めている時間はlwIPの関数1回分の見込み。実際の長さは作ってから測る

### 6-4. 残る論点

- ~~使うタイマー~~ → CTIMER2に決定（2026-10-14、§7-2。MRT0でもよい）
- ~~同梱ライブラリとクラスの名前~~ → **ライブラリのフォルダーとヘッダは`MCXEthernet`（`#include <MCXEthernet.h>`）、クラスとグローバルの名前は公式と同じ（`Ethernet`・`EthernetClient`・`EthernetServer`・`EthernetUDP`）**（2026-10-23、ユーザーが決定）。
  理由: arduino-cliはヘッダと同じ名前のフォルダーのライブラリを選び、`architectures`がボードに合うもの（`mcx`）を`*`（公式の`Ethernet`）より優先する（0.10.0の`Servo`で確認）ので、`Ethernet`という名前にすると5ボードすべてで公式の`Ethernet`（W5500）を覆い隠す。
  クラスの名前を公式と同じにすると、公式`Ethernet`のスケッチは`#include`の1行を変えるだけで動く（STM32の`STM32Ethernet`と同じ形）。
  1つのスケッチで公式の`Ethernet.h`と一緒に読み込むと名前がぶつかる（オンボードとW5500を同時に使う場合だけ）。N947以外のボードでは`#error`で止める
- ~~MACアドレス~~ → **公式と同じ`begin(mac)`と、引数なしの`begin()`の両方**（2026-10-23、ユーザーが決定）。引数なしのときはチップ固有のIDから作る。
  作るアドレスはNXPのOUIではなくローカル管理のアドレス（先頭のバイトが`0x02`の形）にし、IDの全バイトから作る（SDKの作例はNXPのOUIとIDの3バイトで、NXPのOUIの下のアドレスをこちらで割り振ることになり、3バイトではぶつからない保証も無い）。
  チップのIDを読めるか（SDKの`SILICONID_GetID()`がN947で何を読むか）は実機で確かめる
- `D5`: ~~R166の有無~~（実装されている、2026-10-24）と、Ethernetを使う間の`D5`の扱いの書き方
- 見直し（2026-10-23）で直さずに残した点:
  - `EthernetClient`はソケットの番号だけを持つので、1つの接続の写しの片方が`stop()`したあと、その番号を次の接続が使うと、もう片方の写しが別の接続を指す（公式`Ethernet`も同じ）
  - チップのUUIDが読めないと、引数なしの`begin()`のMACアドレスがどのボードでも同じになる（読めるかは実機で確かめる）
  - ~~lwIPの乱数~~ → **ELSのPRNGで種を作り、TCPの初期シーケンス番号も乱数にした**（2026-10-25、ユーザーが決定（案A））。実機で、種が起動ごとに同じで、送り元のポートが24回とも同じだった（`micros()`が最初の呼び出しから数えるコアの不具合も重なって0だった）。ELSは、PRNGが用意できていなければ鍵の無いスロットを1つKDELETEする（SDKの`mcux_els.c`の`ELS_PowerDownWakeupInit()`と同じ。鍵のあるスロットには触らない）ので、下の「ROMと共有するか」の心配は当たらない。以下は当時の記録
  - lwIPの乱数（DNSの問い合わせのIDなど）は起動時の`micros()`とMACアドレスから作るxorshiftで、起動ごとに似た値になる。N947のハードウェアの乱数発生器を使えるかはRMで確かめる。
    SDKで分かったこと（2026-10-23）: N947にはELS（`S50_Type`、`ELS_BASE`）の中にPRNGがあり、`mcuxClEls_Prng_GetRandomWord()`は`ELS_PRNG_DATOUT`を読むだけ。
    作例（`components/els_pkc/examples/mcuxClEls/mcuxClEls_Rng_Prng_Get_Random_example.c`）は、ELSを有効にし、空の鍵のスロットを消す命令でPRNGに種を入れてから読む。
    `els_pkc`を丸ごと取り込まずにレジスタで書けそうだが、ELSの状態をROMやほかの機能と共有するかは確かめていない
  - lwIPのヒープ（`MEM_SIZE` 22KB）は、8本の接続がそれぞれ受信の窓（2920バイト）と送信のバッファ（8760バイト）を使い切ると足りない。足りないときは送信が待ち、受信はlwIPが渡し直すので止まりはしない。大きさは実機で使用量を見て決める
    → 実機で測って決めた（2026-10-25、ユーザーが決定）: ヒープ64KB、受信の窓4×MSS、送信のバッファ12×MSS、ENETの受信の記述子8・送信の記述子32、UDPの受信の待ち行列16。RAMは約79KBから約133KB。経緯はDEVELOPMENT_LOGの「lwIPのメモリの調整」

### 6-4a. 公式`Ethernet`をW5500で実機で動かすときの配線（2026-10-23）

- 秋月で買えるUNOの形のW5500のシールドは「Arduino イーサーネットシールド2」（販売コード114380、W5500とmicroSD）
- **FRDMのボードにはICSPのヘッダーが無い**（2026-10-23、ユーザーが確認）。UNOの形のイーサネットシールドの多くはSPIをICSPの6ピン（1=MISO、3=SCK、4=MOSI）からとり、シールドのD11〜D13はW5500につながっていない（シールド2でもそうかは、シールドの回路図で確かめていない）。
  そのため、ICSPの1・3・4番をボードのD12・D13・D11へ線でつなぐ。CSは`D10`（このコアの`PIN_SPI_SS`）、シールドのSDカードのCSは`D4`
- A156はR59・R60を付け替えた基板でないとSPIがD10〜D13に出ない。N947の`D5`（R166でMDIOとつながりうる）はシールドが使わない
- **W5500 Lite（モジュール）の配線**（2026-10-24。A153・A156・N236・C444・N947で同じ。N947だけRSTを`D8`にした。N947の`D9`は赤のLEDと共用で、作例がRSTを浮かせるとLEDの回路を通して中途半端な電圧がかかりうるため。`D10`も緑のLEDと共用で、CSに合わせてちらつく）: GND・3.3V、SPIは`D11`（MOSI）・`D12`（MISO）・`D13`（SCLK）、CSは`D10`、RSTは`D9`、INTはつながない（公式のライブラリは使わない）。
  公式の作例はRSTに触らないが、`D9`をリセット後の入力のままにしても（RSTは浮く）W5500は動いた。モジュールにプルアップがあるかは確かめていない
- **2026-10-25からRSTは5ボードとも`D8`**（ユーザーの提案。`release_check/26`を作るときに、配線を1通りにするため）。`D8`はどのボードでもほかの機能と共用していない（A153ではMikroBusの`MB_TX`と同じピン）。上の2026-10-24の確認（A153・A156・N236・C444は`D9`）は、公式の作例がRSTに触らないので、そのまま有効

### 6-5. C444との順番（2026-10-14、ユーザーが決定）

**1.0前にN947のEthernetを入れ、C444は1.0の条件にせず1.xで入れる。C444は1.0前に「ビルドが通るだけ」のスパイクを一度やる（§4-1）。**

**2026-10-16に変更: C444を先にし、0.10.0で入れる。Ethernetの残り（段階B〜D）は0.11.0で、1.0の前に入れることは変えない**（ユーザーが決定。理由は§4-1）。
下は2026-10-14に比べたときの判断

比べたときの判断（ユーザーにとってのメリット）:
- どちらを望むユーザーが多いかのデータは無い。GitHubのIssueは5件ともAPIの不具合の報告で、どちらの要望も無い。リリースごとのダウンロード数は10〜18件で、こちらの確認作業の分も含む
- Ethernetの段階Aは全ボードに効く（4枚ともW5500で公式`Ethernet`が使え、`Client.h`で止まっているネットワーク系のライブラリもビルドが通る見込み）。
  ネットワークはArduinoでよく使われる用途で、今のコアではどのボードでもできない。N947ならボード上のRJ45で、部品を足さずに使える
- 規模はEthernetが1つの版くらい、C444はA156の4〜6倍（§4-4）
- C444の強みは、RAM 32KBでArduinoCore-zephyrでは載らず、このコアでないとArduinoで使えないこと（§4-2）。N947はZephyrのコアでも選べる。
  弱みは、メリットがC444を持つ人に限られることと、I3Cが無くフラッシュ256KB・RAM 32KBでできることが少ないこと
- C444を足すこと自体は今のAPIを壊さないので、1.0前のスパイクで公開APIの変更が要らないことを確かめておけば1.xで足せる

## 7. `Servo.h`の互換ライブラリとタイマーの割り当て（2026-10-14）

### 7-1. `Servo.h`の互換ライブラリ

- いま`#include <Servo.h>`はA153で「`Servo.h`が見つからない」で止まる（2026-10-14にarduino-cliで確認）。IDEに入っている公式のServoライブラリは、このコアのアーキテクチャに対応していない
- 案: 公式と同じ使い方（`attach()`・`write()`・`writeMicroseconds()`・`read()`・`attached()`・`detach()`）の同梱ライブラリを足し、IDEの作例（Sweep・Knob）やほかの人のスケッチをそのまま動かす
- 同梱の`mcxRCServo`はサーボの信号を`analogWrite()`で出す（`src/mcxRCServo.h`）。その上に作るとPWMのピンだけになる:
  A153・N947・N236は`PWM0`〜`PWM5`の6台（N236は4本がD3/D5/D6/D9と同じピン）、A156は`PWM0`〜`PWM5`とD3/D5/D6/D9の10台。
  組の2チャンネルは周期を共有するので、組の相手を`analogWrite()`で50Hz以外にするとぶつかる
- AVRの公式Servoは、16ビットのタイマー1つの割り込みでパルスを1本ずつ順に作り、どのデジタルピンでも最大12台まで使える（UNOではそのタイマーのためD9・D10の`analogWrite()`が使えなくなる）。
  同じ作り（タイマー割り込みでGPIOを上げ下げ）にすれば、どのピンでも使える。ほかの割り込みが重なるとパルス幅がわずかにぶれる（AVRでも起きる。ハードウェアのPWMならぶれない）
- 提案: PWMのピンではハードウェアのPWM、それ以外のピンではタイマー割り込み、の使い分け。最初はPWMのピンだけで出し、ほかのピンを後から足す分け方もある（どちらにするかは未決）
- **0.10.0で入れる**（2026-10-15に0.9.2と決め、2026-10-16に0.10.0へ。§9）
- **0.10.0はPWMのピンだけで出し、PWMの無いピンへ`attach()`したら`panic()`で止める。PWMの無いピンへ広げるのは1.x**（2026-10-16、ユーザーが決定、§9の⑫）。
  panicにする理由: 公式のServoはピンを理由に失敗しないので、`attach()`の戻り値を見るスケッチはまず無く、`INVALID_SERVO`を返すだけでは作例（D9を使う）をA153・N947に書き込んでも何も起きずに終わる。
  使えないピンで止めるのは、`analogRead()`や`analogWriteFrequency()`と同じこのコアの扱い。
  PWMの無いピンへ後から広げても、今のスケッチは壊れない（今panicするピンが動くようになるだけ）ので、1.0の前に入れる必要はない
- 作り（`0.10.0-dev`（旧`0.9.2-dev`）の`fa368f1`）: `mcxRCServo`の上ではなく、同梱の`libraries/Servo/`として一から書いた。`mcxRCServo`はコンストラクタで`analogWriteResolution()`を変え、ピンをコンストラクタで受け取るため。
  コアの`analog_pwm_out()`でピンの`PwmOut`を受け取り、周期20msとパルス幅をマイクロ秒で設定する。実機は未確認

### 7-2. タイマーの割り当て（2026-10-14、ユーザーが決定）

**`Servo`のタイマー割り込みは全ボードでCTIMER1、N947のEthernet（§6-3のlwIPを回す1ms）はCTIMER2（MRT0でもよい）。UTICK0は使わない。**

コアが今使っているもの（2026-10-14に`cores/arduino`をgrepして確認）:

| タイマー | 用途 |
|---|---|
| SysTick | `millis()`（1ms） |
| DWTのサイクルカウンタ | `micros()` |
| CTIMER0 | `tone()`/`noTone()` |
| UTICK0 | `mcu.cpp`が起動時に初期化するだけ。r01libの`Ticker`クラスが使い、ArduinoのAPIからは使っていない。スケッチが`Ticker`を直接使う可能性があるので候補から外す |
| FlexPWM | `analogWrite()`（PWM専用） |

各チップのタイマー（variantのデバイスヘッダの`#define`で確認）と、上の割り当てのあとに残るもの:

| ボード | タイマー | 割り当てのあとの空き |
|---|---|---|
| A153 | CTIMER0〜2、LPTMR0、OSTIMER0、UTICK0、WWDT0 | CTIMER2、LPTMR0、OSTIMER0 |
| A156 | CTIMER0〜4、LPTMR0、OSTIMER0、UTICK0、WWDT0 | CTIMER2〜4、LPTMR0、OSTIMER0 |
| N236 | CTIMER0〜4、LPTMR0・1、MRT0、OSTIMER0、UTICK0、WWDT0・1 | CTIMER2〜4、LPTMR0・1、MRT0、OSTIMER0 |
| N947 | CTIMER0〜4、LPTMR0・1、MRT0、OSTIMER0、SCT0、UTICK0、WWDT0・1 | CTIMER3・4、LPTMR0・1、MRT0、OSTIMER0、SCT0 |

**2026-10-30に見直した（ユーザーが決定）**: 0.12.0で`TimerOne`が`CTIMER1`を使うことになったので、`Servo`はA153・A156・N236が`CTIMER2`、N947が`CTIMER3`、C444が`TPM2`にした。0.12.0で入れた（作りと確認は`CLAUDE.md`）

## 8. 未決の問い（一覧）

- [x] §2-1: インスタンス名をどうするか → 付け替えない（2026-10-23、`CLAUDE.md`）
- [x] §2-2: `endTransmission()`の戻り値をAVR互換にするか → AVRの0〜5にした、元のコードは`Wire.lastStatus()`（2026-10-23、`CLAUDE.md`）
- [x] §2-3: A156の出荷状態をメニューで選べるようにするか → 設けない（2026-10-14）
- [x] §2-4: 互換性ポリシーの文書をどこに置くか → API_COMPATIBILITYの冒頭（2026-10-23、`CLAUDE.md`）
- [x] §3: LinkServer 26.9の件の落としどころ（pyOCDは2026-10-14に検討から外した） → 26.9だけで回避する方法は無く、コードは変えない（2026-10-31、0.12.0で調べた。ユーザーが決定。`CLAUDE.md`のPendingタスク9）
- [x] §3: probe-rsを検討するか → 入れた。0.13.0でTools → Upload methodでLinkServerとprobe-rsを選べるようにし（C444以外の4ボード）、probe-rs 0.32.0をパッケージに同梱した。既定はA153・A156・N947がprobe-rs、N236がLinkServer（2026-11-21、ユーザーが決定。§3）。N236の既定はLinkServerのまま（同梱の0.32.0でデバッグできないため）。Windows・Linuxでも、0.13.0のステージングと本番の両方で書き込みとデバッグを確かめた（2026-11-22、ユーザーが確認）
- [x] §3: CIに入れるサードパーティライブラリの選定 → 0.7.0と0.10.0で手で確かめた9つを、2026-10-23にCIへ入れた（`.github/third_party/libraries.txt`。§9の⑨）
- [x] §4-1: C444のスパイクをどのバージョンでやるか → 2026-10-16にやった（§4-5）。C444そのものを0.10.0で入れる
- [x] §4-5: C444の`Serial1`をどこに置くか（置かないか） → 置かない。`Serial`はUSBとD0/D1の両方（2026-10-16、`CLAUDE.md`のC444の節）
- [x] §4-5: `cores/arduino/sdk/`の全ボードでビルドされるファイルを、variantへ移すか、ガードで包むか → MCXの4つのvariantへ移した（2026-10-16）
- [x] §4-5: C444でデバイスヘッダの`SPI0`/`SPI1`マクロをスケッチから隠す方法 → `Arduino.h`の最後で`#undef`する（コアのSPIドライバは`Arduino.h`を読まないので使える、2026-10-16）
- [x] §5-2: `TwoWire`のコンストラクタが受け付けるのを、スケッチのピン名に統一するか（コアの`Wire`/`Wire1`の作り方も変わる）、注意書きだけにするか → 今のまま、コア用（2026-10-30、`CLAUDE.md`のPendingタスク10）
- [x] §5-4: N236で`setPins(A4, A5)`を`Wire1`だけに許すか、`Wire`にも許すか（許すなら、そのあいだ`Wire1`は使えない） → 両方に許す。同時には動かせず、`begin()`がpanicで止める（2026-10-30）
- [x] §5-4: 各ボードのA4/A5に基板上のプルアップがあるか → A156・N236は無し（N236のR298・R299は未実装）、C444は4.7kΩ（2026-10-30に回路図で確認）
- [x] §5-4: 判定用マクロ（`WIRE_HAS_A4A5`など）を置くか → 置かない。AVRと同じ`WIRE_HAS_END`は足した（2026-10-30）
- [x] §6: 段階Aを先に別の版で出すか（N947のEthernetを1.0前に入れることは2026-10-14に決定、§6-5） → 0.10.0で先に出す（2026-10-15に決定、版の番号は2026-10-16に変更、§9）
- [x] §6: N947での確認で、Pendingタスク8の「W5500系シールドなどで実際に通信できるまで」を満たすとみなすか → みなす。クラスは0.10.0で出し、実際の通信は0.11.0で確かめる（2026-10-18）
- [x] §5: A4/A5の`setPins`をどの版で入れるか（2026-10-18に0.10.0から外した。2026-10-25に、0.11.0にも入れないと決めた） → 0.12.0（2026-10-30）
- [x] ウォッチドッグのAPI → **0.13.0で入れる**（2026-11-20、ユーザーが決定。APIの形と各ボードでの作り方はこれから決める）。もとの問い: ウォッチドッグのAPI（AVRの`<avr/wdt.h>`の`wdt_enable()`/`wdt_reset()`の互換など）を出すか、出すならどの版か。C444のCOPは`SIM_COPC`がリセット後1回しか書けず、今はコアが起動時に止めている（スケッチが`mcx_sketch_sets_cop`を定義したときだけ任せる。この口はテスト用で、利用者向けには載せない。2026-10-18に決定）ので、実行時に`wdt_enable()`で設定できる形にするには起動時の扱いから考え直す
- [x] §6-1: R166が実装されているか（`D5`とMDIOの共用） → 実装されている（2026-10-24、実機で確認）
- [x] §6-4: 同梱ライブラリとクラスの名前、MACアドレスの与え方（タイマーは§7-2で決定） → `MCXEthernet.h`、クラスの名前は公式と同じ、`begin(mac)`と引数なしの`begin()`（チップのIDからローカル管理のアドレス）の両方（2026-10-23）
- [x] §7-1: `Servo`をPWMのピンだけで出すか、PWMの無いピンのタイマー割り込みまで最初から入れるか（版は0.10.0、§9） → 0.10.0はPWMのピンだけ、PWMの無いピンではpanic。広げるのは1.x（2026-10-16）
- [x] §10: r01libを`namespace r01lib`に入れるか → **入れない**（2026-11-20、ユーザーが決定。やっても効果が薄い）。上流r01libへの反映（Pendingタスク6）は**1.0の直前にやる**（同日、ユーザーが決定）。もとの問い: r01libを`namespace r01lib`に入れるか。入れるならどの版か、上流のr01libへの反映（`CLAUDE.md`のPendingタスク6）とどう揃えるか、ピンの名前をどう扱うか（§10-3）、r01libを直接使うスケッチをどうするか（§10-4）
- [x] §9: 0.9.2はパッチの番号で機能を足す。リリース準備の「パッチリリースは実機で流す範囲をバイナリ比較で決める」（`CLAUDE.md`）をそのまま当てるか → 0.10.0にしたので当てない（2026-10-16）

## 9. 1.0までの版の割り振り（2026-10-15に決定、2026-10-16に変更）

ユーザーにとってのメリットで検討した候補（2026-10-14）を含めて、この順番で進める。

| 時期 | 中身 | 理由 |
|---|---|---|
| **0.10.0**（2026-10-16に0.9.2から変更） | ① ネットワーク基底クラス（§6の段階A）<br>② `Servo.h`（§7-1）<br>~~③ A4/A5の`setPins`（§5）~~（2026-10-18に外した。0.12.0で入れた）<br>④ `<avr/pgmspace.h>`（2026-10-15に追加）<br>⑤ C444（§4） | ・①〜④は規模が小さく、全ボードに効き、今のスケッチを壊さない。①②は今は「ヘッダが見つからない」でビルドが止まっているもの<br>・機能の追加だけなのでパッチではなくマイナーの版にする<br>・⑤はスパイクで既存のボードを壊さないと分かり、リリースチェックを1回にまとめるため同じ版に入れた（§4-1）<br>・③はA4/A5にI²CのあるA156・N236・C444をそろえて決められる |
| **0.11.0** | ⑥ N947のEthernet（§6の段階B〜D） | ・規模が大きく、lwIPの大きさや1msの割り込みの時間など作ってみないと分からないことがあるので単独の版にする<br>・確かめ方（LANとPC）もほかと違う |
| **0.13.0**（2026-11-20に範囲を決めた） | ⑭ リンクをnewlib-nanoにする（最初のコミットからfull newlibで、スケッチが15〜55KB大きかった）<br>⑮ CIでスケッチのサイズを基準と比べる（`.github/sizes/`）<br>⑯ `Wire1`のI3Cを、`Wire1`を使うスケッチにだけリンクする<br>⑰ ウォッチドッグ（`<avr/wdt.h>`と同梱の`WDT`、§11） | ・⑭〜⑯はRTC_NXP_Arduinoの作例がA153のフラッシュに入らなかった件の調査から。r01libとの差の大半はlibcで、残りの大きなものがI3C<br>・⑮は⑭が誰にも気づかれなかったことへの対策<br>・⑰は1.0向けの未決の問い（§8）から、この版で入れると決めた |
| **1.0の前**（どの版でもよい） | ~~⑦ §2-1・§2-2・§2-4の判断~~（2026-10-23に決定。`CLAUDE.md`）<br>~~⑧ LinkServerの件の落としどころ（§3）~~（2026-10-31に0.12.0で調べた。26.9だけで回避する方法は無く、コードは変えない。ユーザーが決定。`CLAUDE.md`のPendingタスク9））<br>~~⑨ サードパーティのライブラリをCIでビルド（§3）~~（2026-10-23にCIへ入れた。`.github/third_party/`）<br>~~⑩ N947の`Wire2`のターゲットモード（§3）~~（2026-10-30に、既知の制限として確定。ユーザーが決定）<br>⑱ 上流r01libへの反映（`CLAUDE.md`のPendingタスク6）。**1.0の直前**にやる（2026-11-20、ユーザーが決定）<br>⑲ `RED`・`GREEN`・`BLUE`と`SPI_*`・`ARD_*`を`ArduinoPinNum`から外す（0.14.0で`LEDR`・`LEDG`・`LEDB`を足して`[[deprecated]]`にした（2026-11-26）。`SPI_*`・`ARD_*`も`[[deprecated]]`にし、標準の名前は内部用の`MCX_SPI_*`を指すようにした（2026-11-27）。どちらもユーザーが決定。外すときは、`arduino_pin_by_number[]`の行と、`mcxPinState`の`ALIAS_NAMES`（いまは`RED`・`SPI_CS`・`ARD_CS`など）を一緒に消し、`LEDR`・`MCX_SPI_*`の`MB_SDA + n`の値と`arduino_io.cpp`の`static_assert`も直す）<br>~~懸念2（「limits included」の書き方とN236の`A3`）~~（2026-11-26に決めた。ユーザーが決定: 制限を取り除くのは互換を壊さないと書いた。N236の`analogRead(A3)`の`-1`はそのまま固定。ピンでない番号の`analogRead()`は`panic()`にした）<br>~~⑳ ほかのピンの名前~~（2026-11-27に決めた。ユーザーが決定: `SPI_*`・`ARD_*`は⑲と一緒に外す。`PWM0`〜`PWM5`・`MB_*`・`SW2`・`SW3`は基板の表示と同じなのでそのまま固定し、API_COMPATIBILITYのKnown limitationsにぶつかることを書いた。手元のライブラリ・ほかのコア・SDKのヘッダーで調べ、クラスの中の`PWM0`（`LEDDrivers_NXP_Arduino`）とSDKの`#define PWM0`はぶつからなかった） | ・⑦は1.0で固定すると後から変えられない<br>・⑧は効果が大きいが、調べないと規模が分からないので版を決めずに並行して扱う<br>・⑩は直すか、既知の制限として確定させる |
| **1.x** | ⑪ USBデバイス（キーボード・マウス・MIDI）<br>~~⑫ `Servo`をPWMの無いピンへ広げる（タイマーの割り込み、§7-2）~~（2026-10-30に0.12.0で入れた）<br>⑬ `SoftwareSerial` | ・⑪は人気があるが規模が大きい<br>・⑫は②をPWMのピンだけで出したので（2026-10-16に決定、§7-1）<br>・⑬はGPSやBluetoothの作例でよく読み込まれ、今はビルドが止まる。ハードウェアの`Serial1`で代わりになるので効果は中くらい |
| **やらない** | A156の出荷状態を選ぶメニュー（§2-3）<br>pyOCD（§3）<br>r01libの名前空間（§10、2026-11-20）<br>`Serial1`・`SPI1`などを使ったときだけリンクする（2026-11-20。減るのは100バイト以下） | 2026-10-14にユーザーが決定 |

## 10. r01libを名前空間に入れる案（2026-10-29。2026-11-20に、やらないと決定）

### 10-1. きっかけ
0.12.0で、r01libの`class Ticker`がどのスケッチからも見え、スケッチやライブラリが自分の`Ticker`を定義すると`redefinition`になった。これは`r01lib.h`から`Ticker.h`を外して直した（`CLAUDE.md`）。同じことを個別に外すのでなく、r01libを`namespace r01lib`に入れて1回で防ぐ案。

### 10-2. いまグローバルに見えているr01libの名前（2026-10-29に`0.12.0-dev`の`4aee81f`で数えた）
- クラス: `Obj`・`DigitalInOut`・`DigitalOut`・`DigitalIn`・`InterruptIn`・`BusInOut`・`BusIn`・`BusOut`・`AnalogIn`・`PwmOut`・`I2C`・`I3C`・`SPI`・`Serial`（`Ticker`は`r01lib.h`から外した）
- `enum CCC`（列挙子の`BROADCAST_ENEC`などもグローバル）、`func_ptr`・`i3c_func_ptr`
- 関数: `init_mcu()`・`wait()`・`wait_ms()`・`wait_us()`・`panic()`・`flexcomm_keep_shared()`など
- マクロ（名前空間に入れても漏れる）: `io.h`の下の§10-3のもの、`SPI_FREQ`・`I3C_SUPPORTED`・`CUSTOM_REGISTAR_XFER`、`SDK_DEBUGCONSOLE`のときの`printf`などの置き換え
- `SPI`と`Serial`は、コアの`SPIClass SPI`・Serialのオブジェクトと同じ名前で、オブジェクトがクラスの名前を隠している（`arduino_spi.h`は`class SPI;`の前方宣言で受ける）。名前空間に入れれば`r01lib::SPI`と書けて、この無理も無くなる
- 実際にぶつかったのは`Ticker`の1件だけ。サードパーティの作例のビルド（fullの段）でもほかには出ていない

### 10-3. ピンの名前
- **スケッチから見える`D0`〜`D19`・`A0`〜`A5`・`SW2`・`MB_*`・`RED`・`SPI_*`・`ARD_CS`などと`PWM0`〜`PWM5`は、r01libの名前ではない**。`arduino_io.h`がr01libのマクロ60個を`#undef`し、コアの`enum ArduinoPinNum`（Arduinoのピン番号）として定義し直している（`I3C_SDA`・`I2C_SDA`なども作り直す）。コアの公開APIなのでグローバルのままにする
- r01libから漏れているのは2種類:
  1. 物理ピンの列挙子（`P0_0`〜、C444は`PTA1`など、と`DISABLED_PIN`）。名前の無い`enum`で、ボードごとに数百個。名前空間に入れられる
  2. `arduino_io.h`が作り直さないマクロ約30個: `ARD_D0`〜`ARD_D19`・`ARD_A0`〜`ARD_A5`・`USBTX`・`USBRX`・`PIN_INPUT`・`PIN_OUTPUT`・`PIN_LED_ON`・`PIN_LED_OFF`
- これらをコメントの外で使っているのは、作例では`test_bringup_C444`（7か所）・`r01lib_I3C_demo`（4）・`r01lib_API`（2）。ほかにmcxPinStateの`PinState.cpp`（3、上流）と、コアの`arduino_io.h`・`arduino_serial.*`・`arduino_i2c.cpp`
- 案:

| 案 | 中身 | 追加の工数 |
|---|---|---|
| A（おすすめ） | 物理ピンの`enum`とマクロはグローバルに残す（`io.h`の中で名前空間の外に置く） | ほぼ0。スケッチは変わらない。`DISABLED_PIN`や`USBTX`のような一般的な名前は漏れたまま |
| B | 物理ピンも`r01lib`に入れる。`io.h`のマクロ（`D0`→`P4_3`など）はグローバルの`arduino_io.h`の`arduino_pin_by_number[]`でも展開されるので、展開先を`r01lib::P4_3`にする（5ボードで約320行の機械的な書き換え）。残りの約30個のマクロは`constexpr`にする | 2〜3時間。`arduino_io.h`のコメントにある別名の連鎖（`I3C_SDA`が付け替えたあとの`D18`を拾った件）にもう一度注意が要る。上の作例とmcxPinStateにも修飾が要る |
| C | Bを入れ子の`r01lib::pins`にし、スケッチはピンだけ`using namespace r01lib::pins;`で取り込める | Bと同じ |

### 10-4. r01libを直接使うスケッチ
- 直す必要があるもの: `r01lib_API`・`r01lib_I3C`・`r01lib_I3C_demo`（5ファイル）・`test_r01lib_I3C_rx_leftover`、同梱の`Servo`（`PwmOut`）、mcxPinStateの`ConflictDemo`（`DigitalInOut`、上流。pushは毎回ユーザーのOKが要る）。Arduino互換の作例はほぼ影響なし
- 案a（おすすめ）: スケッチの頭に`using namespace r01lib;`を1行足してもらう。展示会・ハンズオンのr01libのスケッチにもこの1行が要る
- 案b: `Arduino.h`で`using namespace r01lib;`する。名前がまたグローバルから見え、`Ticker`のような衝突は「曖昧」というエラーに変わるだけなので、目的を果たさない
- r01libの直接の利用は1.0の公開APIの範囲外（API_COMPATIBILITYの冒頭）なので、壊しても約束は破らない

### 10-5. 作業と確かめ方の見積もり（未確認）
- r01lib本体: 29ファイル・約9,400行。ヘッダーを`namespace r01lib { }`で囲み、.cppの定義を合わせる。`extern "C"`の割り込みハンドラ（`LP_FLEXCOMM2_IRQHandler()`など）と`irq.c`はグローバルに残す（いちばん間違えやすい所）
- コアのArduino層（`arduino_api/`、約6,400行）のうちr01libを使う約15ファイル。.cppは`using namespace r01lib;`、ヘッダーは`r01lib::`で修飾する
- 文書: CHANGELOG・API_COMPATIBILITY・`docs/advanced_r01lib_i3c.md`・llms.txt・DEVELOPMENT_LOG・`CLAUDE.md`
- 確かめ方: 変わるのはシンボルの名前だけで、生成されるコードは変わらないはずなので、前後のコアで全作例×5ボードをビルドして`.bin`を比べる（`CLAUDE.md`の「パッチリリース」の比べ方）。バイト単位で同じなら実機で流し直さない。同じになるかは試していない。2回のフルビルドで数GB使うので、終わったらゴミ箱へ移す
- 工数: ピンを案Aにして半日〜1日（2〜3回に分ける規模）。案B・Cはそれに2〜3時間

### 10-6. コストと影響（おすすめ）
- **やらない**（2026-11-20、ユーザーが決定。やっても効果が薄いと判断）。上流r01libへの反映（Pendingタスク6）は1.0の直前にやる
- **0.12.0ではやらない**（2026-10-30、ユーザーが決定）。1.0の前に、上流r01libへの反映（Pendingタスク6）と一緒に決める、はおすすめのまま未決
- やる価値: 一般的な名前（`DigitalOut`・`AnalogIn`・`PwmOut`・`I2C`・`Obj`・`wait()`・`panic()`・`func_ptr`など、mbed系とも重なる）がどのスケッチにも見えている。ぶつかるたびに外すやり方は、そのたびにr01libのスケッチ側の変更が要る
- いまやらない理由:
  - 実際の衝突は`Ticker`の1件で、直してある。差し迫っていない
  - いちばん大きいコストは上流との関係。上流のr01libはMCUXpressoで使われていて名前空間を必要としない。コア側だけで入れると、いまの約1,800行の差に全ファイルの差が加わり、まとめて反映するときに、差を持ち越すか、上流にも入れて上流の利用者のコードを変えるかを選ぶことになる。タスク6の方針と一体の判断なので、先に決めると後でやり直しになりうる
  - r01libを直接使うスケッチと、展示会・ハンズオンの資料に1行の変更が要る。直す時期は利用者に合わせて選べるほうがよい
- 1.0の前に決めたい理由: 約束の上では1.0のあとでも入れられるが、1.0を「ここから変わらない」と受け取る人は多い。遅くなるほどr01libを使うスケッチや資料が増える
- それまでにすること: サードパーティの作例のビルドと、自分の`Ticker`を定義する`release_check/09`で見張りを続け、新しい衝突が出たら前倒しを考える。r01libに新しい名前を足すときは一般的な名前を避ける

## 11. ウォッチドッグのAPI（2026-11-20、0.13.0で入れると決定。作りも同日に決定）

### 11-1. 調べた事実
- **AVR（avr-libc）**: `#include <avr/wdt.h>`、`wdt_enable(WDTO_15MS〜WDTO_8S)`（値は0〜9で、15ms×2^n）、`wdt_reset()`、`wdt_disable()`。スケッチでの使われ方は、止まったときの復帰と、`wdt_enable(WDTO_15MS); while (1);`によるソフトウェアリセットの2つ
- **UNO R4（renesas_uno 1.6.0の同梱ライブラリ`WDT`）**: `#include <WDT.h>`、`WDT.begin(ms)`（1か0を返す。2回目は何もしない）、`WDT.refresh()`、`WDT.getTimeout()`。止める関数は無い（RenesasのWDTも止められない）
- **MCXの4チップ（WWDT0）**: A153はRM（`MCXA1x2-1x3-RM.pdf`、Rev.7の27章）で、A156・N947・N236はSDKの`fsl_clock`で確かめた
  - クロックは1MHz（`clk_1m`、A153/A156は`MRCC_WWDT0_CLKDIV`、N947/N236は`kCLOCK_DivWdt0Clk`で分周）。カウンタは÷4の250kHzで24ビット。**1.024ms〜約67秒を4µs刻み**で設定でき、`WDTO_15MS`〜`WDTO_8S`はどれも正確に出る
  - **`MOD[WDEN]`・`MOD[WDRESET]`は一度1にするとリセットまで0にできない**（RMの27.6.1.2）。AVRの`wdt_disable()`はハードウェアではできない
  - `MOD[DEBUG_EN]`の初期値は0で、デバッガで止めている間は数えない
  - リセット直後は止まっている。コアは触っていない
- **C444（COP）**: RM Rev.1の12.3.18・12.4.1
  - **`SIM_COPC`はリセット後に1回しか書けない**。リセット直後から動いていて、初期値はLPO（1kHz）で1024ms
  - LPOで選べる時間は32ms・256ms・1024ms（短い側）と8.192秒・65.5秒・262秒（長い側）だけ
  - `COPDBGEN`の初期値は0で、デバッガで止めている間は止まる
  - いまはコアの起動コード（`startup_mcxc444.cpp`）が、`mcx_sketch_sets_cop`（弱いシンボル）が無ければCOPを止める。`release_check/07`はこれを定義して自分でCOPを設定している
- 名前の衝突: コアにもデバイスヘッダにも`WDT`・`wdt_`・`WDTO_`は無い

### 11-2. 案（おすすめ）
- **APIは`<avr/wdt.h>`の3つと`WDTO_*`**。UNO R4の`WDT.h`も同じ仕組みの上に薄く足せる（足すかは未決）
- **MCX（WWDT）**: `wdt_enable()`でWWDTの時間をそのまま設定し、`wdt_reset()`で餌をやる（時間は正確）。`wdt_disable()`は止められないので、以後はコアのSysTickの割り込み（1ms）が餌をやり続ける。2回目の`wdt_enable()`は時間を設定し直す（`WDPROTECT`を使わなければ`TC`は書き換えられる）
- **C444（COP）**: `<avr/wdt.h>`が弱いシンボルを定義し、それを読み込んだスケッチだけ、起動コードがCOPを止めずにリセット直後の1024msのまま残す（読み込まないスケッチは今と同じ）。SysTickの割り込みが餌をやり、`wdt_enable(t)`のあとは「最後の`wdt_reset()`からtが過ぎたら餌をやめる」。リセットはtから最大1024ms遅れる。`wdt_disable()`はSysTickが餌をやり続ける形に戻す
- 決めること:
  - ~~C444の遅れ（最大1024ms）を許すか~~ → **時間が来たらSysTickの割り込みがその場でソフトウェアリセット（`NVIC_SystemReset()`）をかけ、COPは割り込みが止まったときの保険にする**（2026-11-20、ユーザーが決定）。COPは`<avr/wdt.h>`か`WDT.h`を読み込んだスケッチだけ止めずに、リセット直後の1024ms（LPO）のまま使い、`SIM_COPC`には書かない。SysTickが餌をやる。割り込みが動いていれば遅れは1ms以内、割り込みが止まった固まり方とHardFaultはCOPが1024ms以内にリセットする。短い周期（32ms・256ms）にしないのは、EEPROMライブラリがC444ではフラッシュの消去（1回に4KB＝4セクタ、データシートで標準14ms×4、最大114ms×4＝456ms）のあいだ割り込みを止めるため。ソフトウェアリセットは`RCM_SRS0`では「ソフトウェア」になるので、リセットの直前に初期化されないRAMに印を残し、リセットの原因を返す関数は「`WDOG`、またはソフトウェアリセットで印あり」をウォッチドッグとして返す。ウォッチドッグを使うスケッチが割り込みを1秒以上止めるとリセットされることはAPI_COMPATIBILITYに書く
  - ~~`panic()`や`HardFault`で止まったとき、ウォッチドッグでリセットさせるか~~ → **`wdt_enable()`して`wdt_disable()`していなければ、どちらでもリセットさせる。`wdt_disable()`のあとはどちらでもさせない**（2026-11-20、ユーザーが決定）。`panic()`のループは`wdt_disable()`済みのときだけ餌をやる（HardFaultも`panic()`を通るので同じになる。SysTickに任せると、HardFaultのときだけSysTickが動かずリセットされ、規則がそろわない）。`panic()`に入ったら最初に一度餌をやってからメッセージを出し、短い時間でもメッセージを出し終えてからリセットされるようにする
  - ~~リセットの原因（ウォッチドッグかどうか）を読む口を出すか~~ → **出す**（2026-11-20、ユーザーが決定。形は未決）。AVRの`MCUSR`の`WDRF`に当たるもの。MCXは`CMC->SRS`（`release_check/07`が`CMC_SRS_WWDT0_MASK`で読んでいる）、C444は`RCM_SRS0[WDOG]`
  - ~~UNO R4の`WDT.h`も出すか~~ → **出す**（2026-11-20、ユーザーが決定）。同梱のライブラリ（`libraries/WDT`）で、中身は`<avr/wdt.h>`と同じ仕組み。`begin(ms)`は範囲外（0や上限超）で0を返し、2回目は何もしない（R4と同じ）。`getCounter()`はR4のカウンタと同じ値にはならない（こちらはタイムアウトまでの残りのミリ秒にする案。API_COMPATIBILITYに書く）
