# mcx-arduino-core 開発引き継ぎドキュメント

## プロジェクト概要
- **リポジトリ**: https://github.com/teddokano/mcx-arduino-core
- **内容**: NXP FRDM-MCXA153 / FRDM-MCXN947 / FRDM-MCXA156（0.8.0から）/ FRDM-MCXN236（0.9.0から。ここまではCortex-M33）/ FRDM-MCXC444（0.10.0から。Cortex-M0+）向けのArduino IDEボードサポートパッケージ。
  v0.4.0以降は**ソース配布方式**（プリビルド`.a`は廃止）で、`hardware/nxp/mcx/cores/arduino/`（全ボード共有）と
  `hardware/nxp/mcx/variants/<board>/src/`（ボード固有）が唯一の実体。編集すれば次のビルドにそのまま反映される
- **同梱ライブラリ**: `mcxPinState`（ピン所有状況のデバッグ表示）、`mcxRCServo`（RCサーボ）、`EEPROM`（0.7.0で追加）、`Servo`（0.10.0で追加、公式のServoライブラリと同じAPI。0.12.0からPWMの無いピンもタイマーの割り込みで）、`MsTimer2`と`TimerOne`（0.12.0で追加、AVR向けの同じ名前のライブラリと同じAPI。TimerOneのPWMの関数は無い）、`mcxTicker`（0.12.0で追加、ESP32・ESP8266のコアの`Ticker`と同じAPI）。
  前の2つは別リポジトリが開発の本拠地で、`hardware/nxp/mcx/libraries/`配下はリリース時に同期する取り込みコピー。
  `EEPROM`はこのリポジトリが本体（フラッシュの配置がリンカスクリプトと一体なので）。`Servo`もこのリポジトリが本体で、コアの`analog_pwm_out()`（`arduino_analog.h`）でピンの`PwmOut`を受け取り、マイクロ秒でパルス幅を設定する（`analogWriteResolution()`に触らない）。PWMの無いピンはタイマーの割り込みでGPIOを上げ下げする（下の`Servo`の項目）
- **現在のリリース**: **v0.11.0**（2026-10-25。FRDM-MCXN947のオンボードのEthernetを同梱の`MCXEthernet`で、どのボードでも公式`Ethernet`とW5500で、`endTransmission()`の戻り値をAVRの0〜5に、`print()`・`String`の基数の扱いをAVR・ArduinoCore-APIに合わせ、`millis()`・`micros()`を起動から数える修正）。macOS・Windows・Linuxの3プラットフォームで、ステージング（`staging-0.11.0`）と本番の`main`のURLの両方から、インストールと動作をユーザーが確かめた。
  リリースzipはSHA-256 `61a01912d0404551bc0943c4340625e5b2954922afb9c51640712bedf12b7ee3`、12458855 bytes。タグ`0.11.0`は`2f8761d`、`main`は`29d5b37`（`cd46d55`で0.11.0のエントリ、`29d5b37`でリリースの画像）。APIのリファレンスはGitHub Pages（`https://teddokano.github.io/mcx-arduino-core/api/`）。
  その前はv0.10.0（2026-10-18。5つ目のボードFRDM-MCXC444、同梱の`Servo`、ネットワーク基底クラスと`<avr/pgmspace.h>`、`noInterrupts()`の中の`micros()`の修正。zipはSHA-256 `e2f8b9bf19cbf4ec90cceb02ba655ca5482d1c75821a9093ee4443b59a71e8e3`、タグ`0.10.0`は`9cfcf96`）と、v0.10.1（2026-10-20、Fritzingの部品だけ。パッケージのzipは無く、索引にも載っていない）
- **開発中**: **v0.12.0**（`0.12.0-dev`ブランチ、2026-10-25に`main`の`29d5b37`から切った）。**範囲**（2026-10-25に選び、2026-10-30に見直した。ユーザーが決定）: A4/A5の`setPins`（済み、下）、N947の`Wire2`のターゲットモードは既知の制限として確定（API_COMPATIBILITY・PIN_MAPPING_N947・`arduino_i2c.h`に書いてある）、LinkServer 26.9の件の調査（Pendingタスク9）、複数のインスタンスを持てる`Ticker`の同梱（ライブラリもクラスも`mcxTicker`、ヘッダーは`mcxTicker.h`。2026-10-30、済み。下の項目）、`Servo`をPWMの無いピンへ広げる（2026-10-30、済み。下の項目）。**`SoftwareSerial`・USB・r01libの名前空間は0.12.0ではやらない**。入れたものは`docs/llms.txt`（2026-10-26、ユーザーが決定）、`MsTimer2`・`TimerOne`（2026-10-27、ユーザーが決定。下の項目）、r01libの`Ticker`を`Arduino.h`から外すこと（2026-10-27、ユーザーが決定。下の項目）、r01libの`I3C`が受信FIFOに残った1バイトで止まる不具合の修正（2026-10-28、直し方はユーザーが決定。0.11.0からあった。`r01lib_I3C_demo`がN947でIBIのあと止まっていた。`i3c.cpp`の`master_irq_handler()`がSDKの割り込みの処理のあとで捨てる。経緯はDEVELOPMENT_LOG。`test_r01lib_I3C_rx_leftover`がMCXの4ボードでALL OK（2026-10-30。N236はN947のオンボードのP3T1755をD18-`MB_RX`・D19-`MB_TX`でつなぎ、**N236のI3C1のクロックを12.5MHzから25MHzに直してから**（12.5MHzではDAAが1台も見つけなかった。r01libの`I3C`はN236で初めて動いた。直し方はユーザーが決定））、A4/A5の`Wire.setPins()`と`WIRE_HAS_END`（2026-10-30、ユーザーが決定。下のPendingタスク10。`release_check/16`が実機の5ボードでALL OK）、`Servo`のPWMの無いピン（2026-10-30、ユーザーが決定。下の項目）、`mcxTicker`（2026-10-30、ユーザーが決定。下の項目）（1.0までの版の割り振りは[memo/notes_toward_1.0.0.md](memo/notes_toward_1.0.0.md)の§9）
- **`MsTimer2`・`TimerOne`（0.12.0、2026-10-27にユーザーが決定）**: UNO R3にはタイマーのAPIが無く、スケッチはこの2つのライブラリを使うので、同じAPIのものを同梱する（`Servo`と同じ扱い、自前で書く。MsTimer2はLGPL 2.1またはそれ以降、TimerOneはCC BY 3.0）。**TimerOneの`pwm()`・`setPwmDuty()`・`disablePwm()`は入れない**（`= delete`でビルドエラー。ユーザーが決定）。MsTimer2はコアのSysTickの割り込みから呼ぶ（`arduino_timer_hooks.h`の`systick_hook`。タイマーもピンも使わない）。TimerOneはMCXの4ボードが`CTIMER1`、C444がPITのチャネル1。**`CTIMER1`のクロックはコアと同じ発振器（`FRO_HF`、N947・N236ではPLL0の元）を12MHzに分周したもの**（ユーザーが決定。FRO12Mでは、A156で`micros()`に対して0.5%ずれた。いまは5ボードとも100周期が`micros()`でちょうど1000000µs）。割り込みはどちらもいちばん低い優先度（Serialの送信はバッファがいっぱいだと割り込みを待つので、同じか高い優先度だと関数の中の`Serial.print()`で止まりうる）。動きはAVRに合わせた（`setPeriod()`は止めたタイマーも動かす、`resume()`は続きから、外している間に終わった周期は付けたとたんに呼ぶ）。C444はPITの割り込みが1つなので、`PIT_IRQHandler()`（`arduino_tone.cpp`）が`tone()`のチャネル0とTimerOneのチャネル1（`pit_channel1_hook`）に分ける。**SDKの`PIT_Init()`は両方のチャネルを止める**ので、`tone()`もTimerOneもチャネルに触らない`pit_module_init()`を使う。確認は`release_check/09`（2026-10-27、ユーザーが決定）と`test_MsTimer2_TimerOne`（どちらも配線なし。`tone()`をD8で少し鳴らす）で、5ボードともALL OK（2026-10-27。C444は`delay()`を直してから。C444は`01`・`06`・`07`・`08`も流し直してALL OK）
- **`Servo`のPWMの無いピン（0.12.0、2026-10-30にユーザーが決定）**: PWMのピンは今までどおりハードウェアのPWM（ぶれない）、それ以外のピンはタイマーの割り込みでGPIO（`PSOR`/`PCOR`）を上げ下げする。AVRと同じく、インデックスの順に1本ずつパルスを出し、20msごと（合計が20msを超えたらその分延びる）。割り込みは次の一致を前の一致の時刻から決めるので、割り込みが遅れても立ち上がりと立ち下がりが同じだけずれる。
  タイマーは**A153・A156・N236が`CTIMER2`、N947が`CTIMER3`**（`CTIMER2`は`MCXEthernet`）、**C444が`TPM2`**（PWMと同じIRC48Mを32分周した1.5MHz、16ビット。比較モードの`CnV`は次のカウントで効くので、PWMの`CnV`のような周期の終わりまでの待ちは無い）。MCXはTimerOneの`CTIMER1`と同じく`FRO_HF`を12MHzに分周（`tone()`がCTIMER0、TimerOneがCTIMER1、C444のPITは2チャネルとも使用中）。
  **`MAX_SERVOS`は全ボード12**（UNOと同じ。PWMのピンとそれ以外を合わせた数。前はPWMのピンの数）。1つのパルスは最大20000µs。タイマーは最初にPWMの無いピンへ`attach()`したときに初期化し、そのサーボが全部`detach()`されると、その周期の終わりで割り込みを止める。割り込みの優先度は既定のまま。panicは存在しないピンのときだけ（前は「PWMが無い」でpanic）。
  確認は`release_check/13`に足した（2026-10-30、ユーザーが決定。既存のD2-D3・D4-D5のジャンパで、D2・D4のサーボをD3・D5のピンの割り込みと`micros()`で測る。幅・20ms・D2のパルスの直後にD4・`detach()`・もう一度`attach()`）。**5ボードでALL OK**（2026-10-30）。`13`の中の測りでは幅がC444で約5µs長く出る（サーボの割り込みが終わってからピンの割り込みが走るため）。
  **C444はロジアナ（Logic 2のMCP）でも見た**: 1000/2000/1500µsの設定に対し998.9〜999.0/1998.0/1498.3〜1498.4µs、周期19985.3〜19986.5µs（ロジアナの時計に対し0.07%短い。`micros()`では20000µsなのでIRC48Mの誤差）、前のパルスの終わりから次の始まりまで0.52〜0.56µs。PWMのピンの`Servo`は`test_Servo`のまま
- **`mcxTicker`（0.12.0、2026-10-30にユーザーが決定）**: 名前はライブラリ・クラス・ヘッダーとも`mcxTicker`（sstaubのTickerなど、`Ticker`という名前のクラスがほかのライブラリにあるため）。
  **APIはESP32・ESP8266のコアの`Ticker`の形**: `attach(秒, f)`・`attach_ms(ms, f)`・`once(秒, f)`・`once_ms(ms, f)`・`detach()`・`active()`と、それぞれ引数を1つ渡す形（ポインタか、ポインタ以下の大きさの値。ESPと同じ）。**関数は関数ポインタと引数1つの形だけ**（`std::function`は受け付けない。ヒープを使わない）。
  引数の型は関数の型だけから決める（`TArg`を引数から推論しない形にした。ESPの形のままだと`f(int)`に`RED`を渡すと推論がぶつかってビルドが止まる）。
  **時間の元はSysTick（1ms単位）**: タイマーもピンも使わない。コアに2つ目のフック`systick_ticker_hook`（`arduino_timer_hooks.h`）を足し、MsTimer2の`systick_hook`のあとで呼ぶ。最初の`attach()`で付ける。オブジェクトは連結リストでいくつでも。
  最初の呼び出しは`attach()`から少なくとも指定の時間（`loop()`からは+1ms以内。関数の中からはちょうど）、そのあとは周期どおり。割り込みが周期より長く止まっていたら、逃した回は飛ばして時刻を保つ（MsTimer2は取り戻す）。時間は四捨五入したms、最小1。関数の中から自分やほかの`mcxTicker`を`attach()`・`detach()`できる（1回のSysTickで同じオブジェクトを2回扱わないよう、通し番号で見る）。デストラクタは`detach()`。
  確認は`test_mcxTicker`と`release_check/09`（どちらも配線なし。ユーザーが決定）で、**5ボードともALL OK**（2026-10-30）。作例は`Blink`と`ArgumentAndOnce`（RGBのLEDの赤と青、ビルドだけ確認）
- **r01libの`Ticker`を`Arduino.h`から外した（0.12.0、2026-10-27にユーザーが決定）**: `r01lib.h`が`Ticker.h`を読み込んでいたので、どのスケッチからもr01libの`class Ticker`（UTICK、1つだけ）が見え、スケッチやライブラリが自分の`Ticker`を定義すると、MCXの4ボードで`redefinition of 'class Ticker'`になった（C444はr01libの`Ticker`が無いので通った）。`r01lib.h`から外し、使う`r01lib_I3C_demo`は`#include <Ticker.h>`を足した（コア同梱のr01libだけの変更。上流には触らない）。`release_check/09`が自分の`Ticker`を定義して、毎pushのビルドで見張る。検討の材料は[memo/arduino_ticker_study.md](memo/arduino_ticker_study.md)（ユーザーのチャットでの検討。複数のインスタンスを持てる`Ticker`は、名前を`mcxTicker`にして同梱した。下の項目）
- **v0.11.0の経緯**（`0.11.0-dev`ブランチ、2026-10-18に`main`の`49fccaf`から切った）。範囲はN947のEthernet（ENET・lwIP）。同梱ライブラリは`MCXEthernet.h`でクラスの名前は公式`Ethernet`と同じ、MACアドレスは`begin(mac)`と引数なしの`begin()`（チップのIDから）の両方（2026-10-23、ユーザーが決定。memoの§6-4。2026-10-24に、公式の作例が`#include`を替えないとビルドできない件で見直し、このままにした）。
  **2026-10-23の時点（ビルドだけ）**: `libraries/MCXEthernet/`（lwIP 2.2.1とSDKのENETのport・ドライバを`ref/SDK_2_16_000_FRDM-MCXN947_network.zip`から無改変で取り込み（2026-10-24に、SDKのportの4行に型変換を足した）、自前のコードはlwIPをCTIMER2の1msの割り込みで回す）ができ、公式`Ethernet` 2.0.2の作例13本が`#include`の差し替えだけでビルドできる。
  文書（LICENSE・ライブラリのREADME・PIN_MAPPING_N947・API_COMPATIBILITY・CHANGELOG・README）と`release_check/25_ethernet_pc_N947`（PC側は`pc_check.py`）も書いた。
  **実機で確かめた（2026-10-24、N947が1枚、DHCPはMacの「インターネット共有」）**: `release_check/25`がボード側・PC側ともALL OK（1msの割り込みの最長は251µs）。作例4本も動いた（`HttpGet_N947`はMacの共有を通して`example.com`まで）。
  チップのUUIDからのMACは`02:49:20:7c:08:54`。ケーブルを抜き差ししても（10秒・1分）、挿してから1秒ほどでpingが戻る。**長時間の動作も確かめた**（2026-10-24、2時間、`WebTemperature_N947`）: 2秒ごとの`GET /json`が3491回と、10分ごとに続けて200回の`GET`が11回で、失敗も再起動も無し（200回は約2.8秒のまま遅くならない）。Macの1時間のリースは30分ごとに延長された（4回）。pingは7163回のうち3回が失われた（始めの2回と、途中の1回）。**lwIPの乱数の種はELSのPRNGから、TCPの初期シーケンス番号も乱数**（2026-10-25、ユーザーが決定。前は送り元のポートが起動ごとに同じだった）。**`millis()`・`micros()`は`setup()`の前から数える**（2026-10-25に直した。前は最初の呼び出しから数え、`delay(500)`のあとでも0だった。全ボード。`release_check/09`に項目を足し、`09`・`01`が5ボードでALL OK）。**SysTickが遅れて動いても`micros()`がずれない**（2026-10-25に直した。MCXの4ボード。前は遅れて動いた時点をミリ秒の始まりにして、次の刻みまで`micros()`が遅れた分だけ小さく読めた。lwIPの負荷で1kHzの間隔が873〜1087µsに見えていた。`release_check/09`に項目を足し、`09`・`01`が5ボードでALL OK）。**公式`Ethernet`＋W5500 Lite（モジュール）も動いた**（2026-10-24。A153で公式の作例12本、A156・N236・C444・N947で4本ずつ。配線はどのボードも同じで、RSTは2026-10-25から5ボードとも`D8`（それまではN947だけ`D8`、ほかは`D9`）。memoの§6-4a）。`stop()`を`write()`の直後に呼ぶと約200msかかる（W5500の再送の時間。API_COMPATIBILITYのEthernetの行）。**`release_check/26`（W5500、オプション）**（2026-10-25、ユーザーが決定。どのボードでも、PC側は`25`の`pc_check.py`）が5ボードでALL OK。CIのpushごとのビルドには入れず、third_partyの段でビルドする（`.github/third_party/repo_sketches.txt`）。**通信速度の測定**（`test_ethernet_throughput`と`pc_throughput.py`）、**PubSubClient（MQTT）の作例4本がmosquittoで動いた**（`MCXEthernet`とW5500の両方）、**lwIPのメモリを測って決めた**（ヒープ64KB、受信の窓4×MSS、送信のバッファ12×MSS、ENETの記述子は受信8・送信32、UDPの待ち行列16。RAMは約79KBから約133KB、PC→ボード35〜47 Mbit/s、ボード→PC約70 Mbit/s）、**対応していないもの（TLS・IPv6・Wi-Fi・pingを打つこと）を文書に書いた**（`EthernetUDP::beginPacket()`が0.0.0.0・ポート0・IPv6で0を返すよう直し、`release_check/25`に項目を足した）（いずれも2026-10-25、メモリはユーザーが決定）。**R166は実装されている**（`D5`は`INPUT_PULLDOWN`でもHIGH）。**`print()`をAVRに合わせた**: 16進などの10より上の桁を大文字に（2026-10-24）、10進以外の負の数は2の補数、基数`0`は1バイトを`write()`（2026-10-25、ユーザーが決定）。`String(n, HEX)`は小文字のまま（AVRと同じ）。`String(n, base)`の10進以外の負の数も2の補数にした（ArduinoCore-APIの`ltoa()`と同じ、2026-10-25、ユーザーが決定）。`release_check/09`が5ボードでALL OK。**リリース前の確認（2026-10-25、macOS）**: チェックリストの1〜7が済んだ（1の日付だけはマージの直前）。版は0.11.0、`docs/api/`は作り直し済み、回帰コンパイルは5ボードともfull・third_partyで失敗なし、`release_check`は全部の番号が5ボードで問題なし（流し直しはジャンパ2件とUSBの挿し直し）、IDEで5枚の書き込みとデバッグも問題なし。経緯はDEVELOPMENT_LOGの「リリース前の確認」と「リリース」。
  `test_enet_bringup_N947`（lwIPなしで、MDIOでPHYのID、リンク、DHCP DISCOVERの生のフレームとOFFER。`MCXEthernet`が動かないときに、ドライバとPHYまでを切り分ける）は、DISCOVERを1回しか送らず、Macが最初のDISCOVERに答えなかったのでFAILした。1秒ごとに送り直す形に直してALL OK。
  見直しで残した点と、lwIPの乱数にELSのPRNGを使う案はmemoの§6-4。
  `--warnings all`で取り込んだSDKのportに出ていた符号の比較の警告4件は、2026-10-23には「ファイルに手を入れずに残す」としたが、2026-10-24に4行へ型変換を足して消した（ユーザーが決定。LICENSEとライブラリのREADMEに、無改変の例外として書いた）。ライブラリに自前の短い作例を入れる（フォルダー名は`_N947`で終わる形。公式の作例はコピーしない）（どちらも2026-10-23、ユーザーが決定）。作例は`DhcpAddress_N947`・`WebServerLed_N947`・`UdpEcho_N947`・`HttpGet_N947`・`WebTemperature_N947`・`WebButtonsRgb_N947`の6本（2026-10-24、どれも実機で確認。`test_enet_bringup_N947`を含む7本とも、ユーザーがIDEからも確かめた）。pingを打つ機能（公式の`Ethernet`に無い）は入れない（2026-10-24、ユーザーが決定。pingに答えるのはlwIPがしている）。1.0までの版の割り振りは[memo/notes_toward_1.0.0.md](memo/notes_toward_1.0.0.md)の§9。A4/A5の`setPins`（Pendingタスク10）は0.12.0で入れた
- **v0.10.0の経緯**（`0.10.0-dev`ブランチ、2026-10-15に`0.9.2-dev`として開始し、2026-10-16に名前を変えた）。範囲は、ネットワーク基底クラス（`Client`/`Server`/`UDP`/`IPAddress`、Pendingタスク8）、
  `Servo.h`の互換ライブラリ、`<avr/pgmspace.h>`、**FRDM-MCXC444**（2026-10-16、ユーザーが決定）。
  **A4/A5のI²C（`setPins`、Pendingタスク10）は0.10.0には入れなかった**（2026-10-18、ユーザーが決定。0.12.0で入れた）。
  機能の追加だけなのでパッチではなくマイナーの版にした。C444は「ビルドが通るだけ」のスパイク（`c444-spike`ブランチ、未push）の結果を見て入れると決めた。結果はmemoの§4-5で、スパイクのスタブは持ち込まず`0.10.0-dev`の上で作り直した。
  **`Servo`（`libraries/Servo/`）は5つのボードの実機で`test_Servo`がALL PASS（2026-10-18）。作例の`Sweep`と`Knob`（ポテンショメータは`A2`）も、5つのボードで実物のSG90を動かせた（2026-10-18、ユーザーが確認。C444は`D9`、MCXの4ボードは`PWM0`（N236では`D6`と同じピン））**。
  `test_Servo`をジャンパ1本（A153・A156・N947は`PWM0`-`D8`、N236とC444は`D6`-`D7`）で流す。
  **`micros()`が呼ばれるたびに割り込みを入れていた**（`__disable_irq()`/`__enable_irq()`で囲んでいたので、`noInterrupts()`の中でも）のを、PRIMASKを戻す形に直した（2026-10-18、全ボード）。
  `test_micros_noInterrupts`（配線なし）が直す前のC444で2項目FAIL（3msの間に`millis()`が0から4へ進んだ）、直したあと5つのボードでALL OK
  **リリース前の実機チェック（2026-10-18、macOS）**: 全サンプルの回帰コンパイルは5ボードともexit 0。`release_check`は5ボードで、`01`・`04`・`06`（2回）・`07`・`08`・`09`、`11`〜`14`（A156は`15`も）、`24`（4組、いずれもN947が相手）、`03`・`02`・`21`が問題なし。`test_Servo`・`test_avr_pgmspace`・`test_network_base_classes`・`test_micros_noInterrupts`も5ボードでALL PASS。`22`・`23`は2026-10-18の確認（5ボードとも）を使い、今回は流していない。5枚をつないだままの書き込みはarduino-cliとIDE（macOS）の両方で各ボードに入り、IDE内蔵デバッガ（macOS）も問題なし。流し直したのは2件で、A156の`15`（`Wire1`のジャンパを直して流し直し、ALL OK）とA156-N947の`24`（1回目は相手が答えずFAIL、そのままもう一度流してALL OK。原因は調べていない）。`21`のA153は、D1-D0のジャンパを直して`Serial1`のWARNINGが消えた。確認の間にN236・C444・N947が1回ずつUSBから消え、挿し直して戻った（原因は調べていない）。Windows・Linuxの書き込みとデバッグは、ステージング（`staging-0.10.0`）と本番の`main`のURLの両方で、ユーザーが確認して問題なし。
- **1.0で固定することの決定（2026-10-23、ユーザーが決定。材料はmemoの§2）**:
  インスタンス名（`Wire1`・`Serial1`・`SPI1`など）は**付け替えない**。ボードごとに意味が違うことはAPI_COMPATIBILITYの冒頭の表に書いた（別名は要望があれば1.xで足す）。
  `endTransmission()`の戻り値は**AVRの0〜5にした**（0.11.0。LPI2CはアドレスとデータのNAKを分けないので両方`2`、C444は分ける）。元のSDKのコードは標準に無い`Wire.lastStatus()`で読める。作例の`CODE_NAK`などは`2`・`5`にした。`release_check`の`01`・`14`・`15`・`24`を5ボードの実機で流してALL OK/ALL PASS（2026-10-25）。
  **公開APIの範囲**はAPI_COMPATIBILITYの冒頭の「What stays compatible」に書いた（含めるもの: 載っている関数と動き、ピン名、ボードのマクロ、インスタンス名、同梱ライブラリのAPI、載っている独自の追加。含めないもの: r01libやSDKを直接使うこと、ピン名の数値、`lastStatus()`の値、mcxPinStateの表、内部・テスト用の口）
- **ADCの無いピンへの`analogRead()`（A153の`analogRead(A5)`など）をビルド時に検出する案は入れない**（2026-10-11、ユーザー判断）。
  `__builtin_constant_p`と`__attribute__((error))`で、定数のピンならビルドエラーにできることは試作で確かめた（常に`-O2`なので確実に効く）。
  入れない理由は副作用で、実行されないコード（実行時の変数でボードを分ける分岐など）でもビルドが止まり、AVRなどでビルドできるコードがこのコアではビルドできなくなる。
  また、`-O2`のループ展開や関数の展開で定数になった呼び出しも検出されるので、エラーになるかどうかが最適化の判断に左右される。実行時の`panic()`のままにする。
  `analogWrite()`はPWMの無いピンでAVRと同じく`digitalWrite()`になるのが正しい動きで、panicしない（A153の`A5`で実機確認）
- ボード追加の順番はA156 → N236 → C444で合意済み。C444は0.10.0で入れた（A4/A5のI²C（Pendingタスク10）は2026-10-18に0.10.0から外した）。
  A156の前提と決定は下の「FRDM-MCXA156の前提と決定事項（0.8.0）」節、N236の分は「FRDM-MCXN236の前提と決定事項（0.9.0）」節
- **リリースごとの変更点**: [CHANGELOG.md](CHANGELOG.md)
- **各リリースで何をやり、どこで詰まったかの詳細**: [docs/DEVELOPMENT_LOG.md](docs/DEVELOPMENT_LOG.md)
  （v0.1.5〜v0.11.0の全作業記録と、作業中のv0.12.0。自動では読み込まれないので、経緯が必要なときだけ開く）

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
  D6とD9はsm0を共有するので周波数が連動する）。0.9.0で入れたあと、実物のSG90を`SG90_basic`で4本それぞれから動かせることをユーザーが確認した（2026-10-09、ボードから給電）。
  上流のmcxRCServoのREADMEに書き（`8dcf339`）、同梱のコピーへは`0.9.1-dev`で同期した
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

---

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
- **RMとデータシートで見つけて直した不具合（2026-10-17、ビルドだけ）**: TPMの周期が65536カウントのときMODが`0xFFFF`になり、`analogWrite(pin, 255)`の`CnV`が16ビットに切り詰められて0%になっていた（上限を65535カウントにした）。
  ADC16のクロックがSDKの既定（非同期クロックの8分周、0.38〜0.91MHz）で16ビットの下限を下回っていた（バスクロックの8分周、3MHzにした）。
  SPI1にSDKの作例（SPI0用）のバスクロックを渡していて、SCKが要求の2倍になっていた（システムクロックにした）
- **実機で確かめたこと（2026-10-18）**: `examples/Arduino_compatible_API/test_bringup_C444`（配線なし、立ち上げの順を1本で）がALL OK。
  `release_check`の`01`・`06`（2回、書き込みをまたいだ保持も）・`09`・`11`（案A: D2-D3のジャンパだけ、`Serial1`の節はSKIP。2026-10-18にユーザーが決定）・`12`（D11-D12）・`13`（D8-D9・D2-D3・D4-D5・D6-D7。データの組をD0-D1から移し、サーボはD6からD7で測る。2026-10-18にユーザーが決定）、`test_Servo`（D6-D7）、`test_avr_pgmspace`（M0+でアドレスのそろっていない読み出しも）、`test_network_base_classes`がALL OK/ALL PASS。
  `22`（外付けのLM75系）と`23`（`Waveshare_TFT_Touch`の`SDBitmapViewer`）は、ユーザーが5つのボードともIDEで流して問題なし。
  `08`（配線なし。PWMはTPM0のD10・D5とTPM1のD3、D5/D6/D9の受け渡し。プルダウンはD8（D4はSW3でプルアップ付き）、`analogRead()`のあとの割り込みはA5（A0〜A3のポートBはピン割り込みが無く、A5のR27のプルアップに対しLOWに駆動して離して立ち上がりを作る））がALL OK。
  元の`test_analogWrite_pairs_and_pinMode`・`test_attachInterrupt_keeps_pinMode`（D8）・`test_digitalRead_after_analogRead`（A5、割り込みの項目のSKIPをやめた）もALL OK。
  `04`はmcxPinStateをC444に対応させて、配線なしでCONFLICT・MISMATCHなし（2026-10-18）。ライブラリは`Wire1`=`PTE25`/`PTE24`、`SPI1`・`Serial1`・`PWM0`〜`PWM5`なし、`Serial`のピンは`USBTX`/`USBRX`がD1/D0の別名なので付け替え前の値に戻す、
  GPIOと周辺機能の区別を「ALT0」から`kPORT_MuxAsGpio`（C444はALT1）にした。コアの`pin_registry_pin_name()`はC444で`PTA1`の形で名前を作る。上流（`~/dev/Arduino/mcxPinState`）にも同じ内容をコミットしてpushした（`cc472e4`）。
  `07`はCOP（ウォッチドッグ）で1000回リセットしてALL OK（約1分半。普通の書き込みは約0.2ms、詰め直しは約37ms）。
  そのため**COPを止めるのは`SystemInit()`ではなく起動コードの`ResetISR()`にした**（C444だけ`-DDISABLE_WDOG=0`）。スケッチが`mcx_sketch_sets_cop`を定義したときだけ止めずに任せ、スケッチは`SIM_COPC`を自分で1回書くまで、リセット後の既定（LPOで1024ms）のCOPを`SIM_SRVCOP`でリセットし続ける。
  テストのための内部の口で、**利用者向けの機能としては文書に載せない**（2026-10-18、ユーザーが決定。ウォッチドッグはどのボードでも文書に無い機能で、出すなら`<avr/wdt.h>`の互換などの形をC444の書き込みが1回だけの`SIM_COPC`と合わせて設計し直すことになるので、名前を約束しない。memoの§8）。`07`のCOPはMCGIRCLK（8MHzのLIRCを4分周した2MHz、コアは使っていない）で数え、LPTMRで`micros()`と比べて速さを測る。実機でのリセットの時刻は計算と0.25%以内で合った
  `21`はN947のオンボードのP3T1755を`Wire`（C444のD18-N947の`MB_RX`、D19-`MB_TX`）で読み、`Serial1`の代わりに`Wire1`の加速度センサーのWHO_AM_Iを毎周読む（D11-D12のジャンパで`SPI`、`analogWrite`はD3、`tone`はD8）。約130周回してWARNINGなし（加速度センサーのCS/WAKEはR40でD11＝MOSIにつながるが、影響は無かった）。
  `test_fault_report`は8種類のうち7種類で`error: HardFault at PC ...`がクラッシュした行を指す（M0+にはCFSRが無いので種類は出ない。割り込みの中は`in an interrupt`）。
  スタックあふれ（kind 4）は下の「スタックあふれの扱い」の形で報告される。
  書き込みはLinkServer 26.6.137で、フラッシュを正しく判定する
- **`Wire`の`setWireTimeout()`とターゲットモード（2026-10-18）**: SDKの`I2C_MasterTransferBlocking()`は待ちに上限が無く（`I2C_RETRY_TIMES`が0）、SCLを押さえられると止まったままになるので、
  コントローラの転送はr01libの`transfer_kinetis()`（`i2c.cpp`）で自前に書いた。タイムアウトはSMBusのSCL low timeout（`SLTH`/`SLTL`、モジュールのクロックの64分の1で数える）で、**SCLだけ**を見る。上限はどの速度でも約87ms。
  コードは`uint8_t`に切り詰めて、タイムアウトが`80`、アドレスのNAKが`81`、データのNAKが`78`、ビジーが`76`。SDAを押さえられたまま始めるとバスがビジーで即座に`76`、SCLを押さえられたまま始めるとタイムアウトになる。
  SLTFはIICIFも立てるので、IICIFが立ったあとにもSLTFを見る（見ないと成功として返った）。
  ターゲットは`arduino_i2c.cpp`がI2Cの割り込みで動かす（SDKの`I2C_SlaveInit()`は同じモジュールのマスターの設定も書き換えるので使わない）。**Kinetisの1つのI2Cはマスターかスレーブのどちらかで、自分のターゲットを呼べない**
  （LPI2Cは両方が独立して動く）。そのため`release_check/01`と`14`のターゲットの節、`test_Wire_target_self`はC444ではSKIPを出し、ターゲットモードは`24`（N947との2枚）だけで確かめる。
  **コントローラの転送の間は`C1[IICIE]`と`FLT[SSIE]`の両方を止める**: SSIEが立っていると自分の転送のSTARTでもIICIFが立ち、アドレスを送り終える前に「1バイト終わった」と読んでいた
  （`24`で、応答の確認だけが通り、データを送る項目が全部FAILした）。`14`（D19-D8・D18-D7、ターゲットの節はSKIP）と`24`（C444が`0x42`・先攻）がALL OK
- **実機で見つけて直した不具合（2026-10-18）**: どれもC444の分岐か、MCXでは値の変わらない形で直した。
  - **ポートのクロック**: SDKのテンプレートの`BOARD_InitBootPins()`はPORTAのクロックしか入れず、`pinMode(RED, OUTPUT)`（PTE31）でHardFaultになった。`mcu.cpp`で5つとも入れる
  - **GPIOのALT**: KinetisはALT0が「無効（アナログ）」でGPIOはALT1（MCXはALT0がGPIO）。`DigitalInOut`がMUXを設定せず、`pinMode()`と`Serial.end()`は`pin_mux( 0 )`でGPIOに戻していたので、入出力・プル・割り込み・`tone`・`Wire`の内蔵プルアップが全部効かなかった。
    コンストラクタ（C444の分岐）でALT1にし、GPIOに戻す所は`kPORT_MuxAsGpio`（MCXでは0、C444では1）にした
  - **ADCの基準電圧**: 上のVREFHの件。`AnalogIn`はVALTHを使う（VDDAを約3.29Vと読む）
  - **`delayMicroseconds()`**: `wait_us()`の倍精度と64ビットの計算がM0+ではソフトウェアで約45µsかかり、10µsが56µsになった。M0+ではSysTickのカウンタを数える（割り込みを止めていても正しい）
  - **`delay()`**（2026-10-27に直した、ユーザーが決定）: `wait()`（`SDK_DelayAtLeastUs()`）はサイクルカウンタが無いと自分のループを数えるので、その間の割り込みの時間が上に足された。`micros()`で`delay(500)`が501.2ms、TimerOneの1kHzで502.5ms、10kHzで514.1ms（`test_MsTimer2_TimerOne`の0.5秒で504回のFAILで見つかった）。M0+では`delayMicroseconds()`と同じくSysTickのカウンタを1msずつ数える（3つとも500.005ms）。1msを超えて動く割り込みが2回の読みの間にあると、その分は数えない。MCXの4ボードはDWTを数えるので元から正しい（N947で3つとも500.005ms）
  - **I2CのSCL**: `I2C_MasterSetBaudRate()`の引数（ボーレートと元のクロック）が逆で、いつも2.4MHzだった（LPI2Cの関数は順番が逆）
  - **I2Cのリピーテッドスタート**: STOPを付けない書き込みがNAKで失敗したあとも印が残り、次の転送が失敗した（`release_check/01`のStreamの3項目）。失敗したら消す
  - **SPIのSCK**: `SPI::frequency()`が頼まれた周波数を半分にしていた（以前のバスクロックの誤りを打ち消していたもの）。2026-10-17にクロックを直したので、半分にするのをやめた
  - **TPMの`CnV`**: カウンタが動いている間、`CnV`への書き込みは周期の終わりまでバッファで待ち、その間の同じチャネルへの書き込みは捨てられる（RMのとおり。読み戻すと書いた値が返るので、読んでも分からない）。
    1周期に2回の`analogWrite()`の2回目と、D5/D6/D9の取り合いで後から来たピンのデューティが効かなかった。
    **TOF（オーバーフローのフラグ）は、`STATUS`・`SC`のどちらに書いても実機で消えないことが多く**（消えた回もある。理由は分からない）、使えない。
    そこで書いた時点の`CNT`・`MOD`・分周比から周期の終わりの時刻を出し、それより前の書き込みは覚えておいてSysTick（`pwm_tpm_service()`）が書く。同じ値は書かない。
    **その時刻の計算が、読んだ`CNT`が`MOD`より大きいと負になり、約89秒先になっていた**（2026-10-18、`release_check/08`で発見）。その間の書き込みは覚えたまま書かれず、TPM0のチャネルが止まって見えた。
    `MOD`は読むと書いた値が返るが、効くのは次の周期からなので、周期を短くした直後は古い周期の途中で`CNT`が新しい`MOD`を超える。止めて0にした直後の`CNT`も古い値を返した。
    いまは上限の見積もりにした（`MOD`を書いた直後の1周期は古い`MOD`も見る。`CNT`が`MOD`を超えていれば0xFFFFまで数えて折り返す分を足す）。
    **`MOD`も同じバッファで、1周期に2回書くと2回目が捨てられる**ので、`CnV`と同じく覚えてSysTickで書く（`set_mod()`）。`analogWriteFrequency()`を続けて2回呼んで、後の周波数になることを実機で確かめた
- **できているもの（2026-10-16、ビルドだけ。上の項目以外は実機は未確認）**: `variants/frdm_mcxc444/`（`README.md`に出どころ）、`boards.txt`（デバッグまで）、`gdb-bridge`の`c444.cfg`、
  `micros()`（SysTick。`__ARM_ARCH_6M__`で分ける）、HardFaultの報告（PCだけ）と`semihost_hardfault.c`（M0+ではスタックしたPCがフラッシュの中のときだけBKPTを見る）、
  `Serial`（`USBTX`/`USBRX`が`D1`/`D0`なので、ピン番号の付け替えの前に生の値を取る）、`Wire`/`Wire1`、`SPI`（r01libに無かった`bit_order()`を足し、CSは転送ごとに動かさずスケッチに任せる。MCXのボードと同じ）、`analogRead`（ADC16、16ビット、校正）、
  `analogWrite`/`analogWriteFrequency`（TPM、D5/D6/D9の取り合い）、`tone`（PITのチャネル0）、`EEPROM`（FTFA、16バイト単位、フラッシュの最後の8KB、命令の間は割り込みを止める）、`Servo`。
  `PIN_MAPPING_C444.md`・README・API_COMPATIBILITY・CHANGELOG・LICENSE（`startup_mcxc444.cpp`のFOPTの変更）・`docs/porting_a_new_board.md`
- **全作例のビルド（2026-10-18）**: CIと同じ`compile_examples.sh frdm_mcxc444 full`で107本のうち96本が通り、8本は他のボード専用で飛ばす。
  `#error "This sketch has no settings for this board yet"`が残っている作例は、どれもC444の分岐を持っていて、`#error`は知らないボードの`#else`にあるだけ。
  落ちたのはmcxRCServoの作例3本（`PWM0`）だけで、C444では`D3`（TPM1を独占できるので、サーボの50Hzがほかのピンの周期とぶつからない）を使う形に直し、READMEにC444のピンの表を足した。
  上流（`~/dev/Arduino/libraries/mcxRCServo`）と同梱のコピーの両方に入れ、5つのボードでビルドが通る。CIのボードの表（`regression_check.yml`）にC444を足した。
  実物のサーボで3本ともC444の`D3`から動くことをユーザーが確かめた（2026-10-18。SG90はボードの5V、FS90Rは別の電源）。READMEにも書いた
  `release_check/02`は波形もユーザーが確かめて問題なし（2026-10-18）
- **`analogRead()`を既知の電圧で確かめた（2026-10-18）**: `examples/Arduino_compatible_API/test_analogRead_precision_C444`（16ビットで64回の平均・最小・最大と、ADCの内部の入力）。
  定電圧源の1Vを`A2`へ入れ、テスターで`A2`=0.97V、3.3V（VDDA）=3.27Vのとき、読みは平均19516で、3.27V換算で0.974V（テスターの桁の範囲で一致）。
  GNDは2カウント、3.3Vは65526（フルスケールより9低い）。内部のVREFLは2.6、VALTHは65514、バンドギャップは19910（3.27V換算で0.994V、データシートは0.97〜1.03V）。
  外の入力の1回の中のばらつきは約300カウントで、バンドギャップ（約30）の10倍（定電圧源と配線のノイズと見ている。切り分けはしていない）。
  スケッチの換算の定数を`VREF`にするとビルドが止まる（`MCXC444.h`のVREFペリフェラルのマクロ）
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
| PROGMEM/pgm_read系、F()/String対応、ARDUINO/ARDUINO_ARCH_*マクロ | ✅ | v0.2.1で追加。実機確認済み。0.10.0で`<avr/pgmspace.h>`（`_near`/`_far`、`_P`/`_PF`系の関数など）を足し、PROGMEMのマクロは`Arduino.h`からそこへ移した。`test_avr_pgmspace`でA153・N947確認 |
| Wire.end、Serial.find(len)/findUntil、Serial.availableForWrite、INPUT_PULLDOWN、OUTPUT_OPENDRAIN | ✅ | v0.2.1で追加。実機確認済み（`Wire.end()`はI3C使用時のBusFaultバグを修正後に確認） |
| String operator+数値版/F()、Printable、NOT_AN_INTERRUPT、digitalPinToPort/BitMask+portOutput/Input/ModeRegister | ✅ | v0.2.1で追加。実機確認済み（fast GPIOレジスタ直接操作がD2-D3ジャンパで正しく動作、Printableカスタムクラスの出力を視覚確認） |
| Print/Stream抽象基底クラス（新設） | ✅ | v0.2.1で追加。実機確認済み——ハードウェア非依存のPrint派生クラス、Stream&への多態性、print()/println()の実バイト数返却、Printableのn+=p.print(x)イディオムすべて動作確認 |
| サードパーティライブラリ互換性（ArduinoJson/LiquidCrystal/DHT/NeoPixel/OneWire/Adafruit BusIO/Adafruit Unified Sensor） | ✅ | v0.2.1で最小スケッチのコンパイルを確認。0.7.0で各ライブラリの**同梱サンプル全47本×両ボード**に広げて再確認し、`BitOrder`の本物のenum化とavr-libcの文字列関数・`M_`定数の宣言を追加した。残る失敗8本は、BLE（4本）、ADXL343ドライバ（1本）、Ethernetが要求する`Client.h`（3本、coreにネットワーク基底クラスが無い）。0.10.0でネットワーク基底クラスと`<avr/pgmspace.h>`を足し、公式`Ethernet`の作例13本・ArduinoJsonのEthernet系3本・PubSubClientの4本が4ボードでビルドできることを確かめた（PubSubClientは`pgm_read_byte_near()`で止まっていた。`mqtt_stream`は外部の`SRAM.h`が要るので対象外）。公式のServoはライブラリ側のアーキテクチャ非対応で使えないので、0.10.0で同じAPIの`Servo`を同梱した（公式が入っていても、ビルドは同梱のほうを選ぶことを確認）。ライブラリはスクラッチパッドに`ARDUINO_DIRECTORIES_USER`で入れたので、`~/Documents/Arduino/libraries`には無い |
| Wire (I2C) | ✅ | |
| Wire1 (I3C, I2Cモード) | ✅ | オンボードP3T1755で確認、重大バグ修正済み。A156の`Wire1`はI3CではなくMikroBusの`LPI2C3`（0.8.0） |
| SPI | ✅ | |
| attachInterrupt | ✅ | |
| detachInterrupt | ✅ | v0.2.1で追加。SW2を使った実機確認済み |
| analogRead | ✅ | LPADC。A153は`A0`-`A3`、N947は`A2`-`A5`、A156は`A0`-`A5`（`A4`・`A5`はR75・R76を外したボードで）、N236は`A0`-`A2`・`A4`・`A5`（`A3`は`-1`）。AVRと同じく`0`-`5`も`A0`-`A5`（0.9.1から。`test_analogRead_channel_numbers`と`release_check/08`で4ボード確認。D0-D5はどのボードでも`A`ピンと別のピンでADCも無いので、前に動いていた呼び出しの意味は変わらない） |
| analogWrite (PWM) | ✅ | A153・A156はFlexPWM0、N947・N236はFlexPWM1。`PWM0`-`PWM5`と、A156・N236の`D3`/`D5`/`D6`/`D9`（A156はFlexPWM1、0.9.0）。組の周期は組ごとに共有（0.9.0で直した） |
| millis / micros | ✅ | SysTick(1ms) + DWT |
| delayMicroseconds | ✅ | wait_us()ベース、v0.2.1で追加 |
| tone / noTone | ✅ | CTIMER0, 任意のデジタルピン |
| Serial1（A153・A156はD0/D1、N947はMikroBusの`MB_TX`/`MB_RX`）、Serial2（A156だけ、MikroBus、0.8.0） | ✅ | 入力バッファ有効化・RX割り込み・available()の3バグ修正後、実機ループバックで確認 |
| shiftOut / shiftIn | ✅ | 割り込みベースの相互検証で確認 |
| pulseIn / pulseInLong | ✅ | |
| random / randomSeed | ✅ | |
| UNO R3/R4互換マクロ・定数一式 | ✅ | `release_check/09`の「compat macros」節で実行時に値を確認（0.8.0までは`01`） |
| String クラス | ✅ | 独自実装（WString移植ではない）。連結・数値変換・検索・置換・大小文字変換・trim等を実機確認、全項目OK |
| EEPROM（0.7.0） | ✅ | 1KB、内蔵フラッシュの末尾。`release_check/06`（API・書き込み・リセット後と書き込み後の保持）と`07`（ウォッチドッグで書き込み中に1000回リセット）で両ボード確認。CLIの`upload`とgdbの`load`、IDE（macOS）の書き込みボタンとDebugボタンで消えないことも確認 |
| Wire.setWireTimeout / getWireTimeoutFlag / clearWireTimeoutFlag（0.7.0） | ✅ | LPI2Cのピンlowタイムアウト。`Wire`・N947の`Wire2`・A156の`Wire1`（A153・N947の`Wire1`はI3Cで無効）。`release_check/14`（ジャンパ）で3ボードとも確認。C444（0.10.0）はKinetisのSMBusのSCL low timeoutで、SCLだけを見る。`14`で確認 |
| I2Cターゲット（スレーブ）モード（0.7.0） | ✅ | `Wire`とA156の`Wire1`（A153・N947の`Wire1`とN947の`Wire2`は`begin(address)`で止まる）。自分自身をターゲットにする形（`release_check/01`）と2枚接続（`release_check/24`）で確認。C444（0.10.0）は自分のターゲットを呼べないので、`24`だけで確認 |
| Wire: begin()のオーバーロード・内部プルアップ・Stream化・5引数requestFrom・バッファ上限（0.7.0） | ✅ | `test_Wire_begin_address`と`test_Wire_Stream_requestFrom5`で両ボード確認（後者は`release_check/01`に統合） |
| Serial.begin(baud, config) / end() / serialEvent（0.7.0） | ✅ | 12形式をRXピンからビット単位で読んで確認（`release_check/11`に統合） |
| AVR互換の補助関数（0.7.0） | ✅ | `itoa`/`dtostrf`/`word`/`_BV`/`analogReference`の定数/`HardwareSerial`/avr-libcの文字列関数/`M_`定数/`SDA`・`SCL`/`BitOrder`のenum化。`test_avr_compat_helpers`（`release_check/09`に統合、0.8.0までは`01`。単体でもA156を含む3ボードでALL PASS） |
| print(double)・String(double)の丸め（0.7.0） | ✅ | `test_print_float_rounding`で3ボード確認（A156は0.9.0の開発開始時。代表的な項目は`release_check/09`にも入っている） |
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
- **ローカルArduino IDE連携**: `~/Library/Arduino15/packages/nxp/hardware/mcx/<version>-dev`（今は`0.12.0-dev`。2026-10-25に`0.11.0-dev`から付け替えた。ブランチ名と同じ。パッチリリースのブランチを並行して進める間は、作業ツリーで切り替えたブランチに合わせてこの名前も付け替える）をこのリポジトリの`hardware/nxp/mcx/`へのシンボリックリンクとして設定済み（編集が即座に反映される）。ツールチェーンも`~/.xpacktools/`への symlink。`-dev`サフィックスにより、Boards Manager経由でインストールする実リリース版とはディレクトリ名が衝突せず共存できる（ただし下の項目のとおり、並んでいるとリリース版が選ばれる）
- **リリース版を入れたままだと`-dev`が使われない**（0.7.0で踏んだ）: `packages/nxp/hardware/mcx/`に
  Boards Managerで入れた`0.6.0`と`0.7.0-dev`のsymlinkが並ぶと、**IDEも既定のarduino-cliもインストール済みの`0.6.0`を選ぶ**。
  開発中の修正がIDEで一切効かず、「直したはずの不具合がIDEでは再現する」形で表に出る。
  実機確認の前に`arduino-cli compile -v`の`Using core ... from platform in folder:`が`-dev`を指しているか確かめること。
  2026-09-24から`0.6.0`は`~/Library/Arduino15/mcx-0.6.0-backup`に退避中（Boards Manager検証で使うときは戻す）。
  本番の`main`のURLから入れた`packages/nxp`（リリース版とツールチェーン）は、0.10.0の分が`~/Library/Arduino15/nxp-0.10.0-release-installed`に退避してある（macOSの確認は2026-10-18に問題なし）。
  0.11.0の確認のあいだ（2026-10-25）、開発環境は`~/Library/Arduino15/nxp-dev-env`に退避してあり（symlinkは`0.12.0-dev`に付け替え済み）、`packages/nxp`にはステージングから入れた0.11.0がある。戻すときは`script/dev_env_restore.command`
  0.10.0のステージングから入れた分と、0.9.0・0.9.1の本番の分はゴミ箱に移した（2026-10-19）。それより前のステージングの分と0.8.0の分はユーザーが片付けた（2026-10-12）
- **注意（Boards Manager経由の実インストール検証時のハマりどころ）**: 上記symlink環境を無効化する際、`~/Library/Arduino15/packages/nxp`を同じ`packages/`直下で別名（例: `nxp.dev-backup`）にリネームしただけでは不十分 — arduino-cliは`packages/*`配下の全ディレクトリ名をpackager IDとして解釈するため、リネーム後も`nxp.dev-backup:mcx`という別パッケージとして「0.1.9-dev installed」表示が残ってしまう（`arduino-cli core list --all`で再現・特定）。無効化する際は`packages/`の外（例: スクラッチパッド等）に完全に退避すること。v0.2.0リリース後、この手順でBoards Manager経由のGitHubからの実インストールを検証済み
- **開発環境の退避と復帰は`script/`の2本で行う**（2026-10-12に作成。macOSでダブルクリックで動く）:
  `dev_env_evacuate.command`が`packages/nxp`を`~/Library/Arduino15/nxp-dev-env`へ移し、`dev_env_restore.command`が戻す。
  戻すとき、退避中にBoards Managerで入れた`packages/nxp`があれば消さずに`~/Library/Arduino15/nxp-boardsmanager-<日時>`へ移してから戻す。
  退避用は`packages/nxp`に`*-dev`のsymlinkが無ければ（Boards Managerで入れたものなら）何もしない。戻す用は両方に開発版があれば何もしない。
  退避先の名前に版の番号は入れていない（0.9.1までは手で`nxp-0.9.1-dev-env`などに移していた）

## GitHub Actions
- **`regression_check.yml`**: push・PRごとに全サンプルのコンパイル（fast/fullの2段）とhygieneチェック（`.github/scripts/check_repo_hygiene.py`、タグでは`--release`）。
  matrixはボード×`suite`（`examples`と、fullのときだけ`third_party`）。ジョブの中は`SWEEP_JOBS=$(nproc)`で並列にビルドする。
  **arduino-cliのビルドキャッシュは実行をまたいで持たない**（checkoutでソースが`core.a`より新しくなり、毎回コアを作り直すので、復元したキャッシュは使われていなかった。2026-10-24に手順を消した）。
  **ccacheをコンパイラの前に置く**（2026-10-25）: インストールしたコアのコピーにだけ`platform.local.txt`を置き、`compiler.path`をgcc・g++・c++だけccacheに通すフォルダーに向ける（スクリプト・`platform.txt`・フラグは変えない）。
  ccacheは公式リリースのv4.14.1をSHA-256で確かめて入れる（**Ubuntuのaptの4.9.1はこのプラットフォームのコンパイルをほとんど断り、効かない**）。中身はactions/cacheで持ち越す。`.bin`はccacheなしとバイト単位で同じ（N947のfullの全114個で確認）なので、ccacheを使わないローカルのスイープと結果は変わらない。
  測定は、pushのfastが4分50秒→2分47秒（2026-10-24）→1分46秒（2026-10-25、ccache）、workflow_dispatchのfull＋third_partyが25分32秒→8分13秒（2026-10-24）→3分56秒（2026-10-25、ccache）で、判定はどれも前後で同じ
- **サードパーティのライブラリの作例のビルド**（2026-10-23、memoの§9の⑨）: fullの段と同じとき（`main`・タグ・手動実行）に別のジョブで、`.github/scripts/compile_third_party.sh`が`.github/third_party/libraries.txt`のライブラリ（版を固定、依存も並べて`--no-deps`で入れる）の作例を全ボードでビルドする。
  通らないと分かっている作例は`expected_failures.txt`にボードごとに書き、それ以外の失敗と、書いてあるのに通るようになった作例の両方でCIを落とす。N947では公式`Ethernet`の作例を`#include <MCXEthernet.h>`に替えて`MCXEthernet`でもビルドする。
  ローカルでは`TP_USER_DIR=<ディレクトリ>`でライブラリを入れる場所を指定して流せる（スケッチごとにキャッシュの`sketches/`を消すので、ディスクは食わない。5ボードを並列にするのは`compile_all_boards.sh third_party`）。ライブラリの版を上げるときは手で一覧を直す
- **`update_package_index.yml`**: GCCのsizeをHEADリクエストで取得、プラットフォームZIPのchecksum/sizeをダウンロードして計算・更新
- **既知の制限**: タグpush（`push: tags: '[0-9]+.[0-9]+.[0-9]+'`）で起動した場合、`actions/checkout`がdetached HEADでチェックアウトするため最後の`git push`が失敗する（過去のv0.1.6〜v0.2.0全リリースで再現）。実際のchecksum確定は、リリース後に`gh workflow run update_package_index.yml --ref main`（または Actions UI の "Run workflow"）で`main`ブランチに対し手動実行する必要がある。**リリース時は「タグpush→(失敗を確認)→mainに対してworkflow_dispatchを手動実行」の2段階が必須の手順**

### リリース前クロスプラットフォーム検証: ステージングブランチ方式（v0.3.1から採用）
これまでは「タグpush→`main`のchecksum確定→ユーザーが各OSで実機インストール検証」という順序で、`main`の`package_nxp_mcx_index.json`が確定してから初めて検証していた。ユーザーから「リリース前に各OSでのインストールを確認する方法はないか」と相談があり、以下の手順を提案・採用が決定:

1. いつも通り`gh release create`でリリースzipを添付（この時点でダウンロードURLは実在・安定する。`main`のインデックスをまだ更新していなくても関係ない）
2. `main`とは別に**ステージング用ブランチ**（例: `staging-0.3.1`）を作り、`package_nxp_mcx_index.json`だけをそこにpush——今作ったリリースのエントリ（url/checksum/sizeは実際の値）を`platforms[]`の末尾に追加したもの（既存エントリは書き換えない。v0.3.1当時は`platforms[0]`を書き換えていたが、0.4以降は過去バージョンも選べるよう追加にしている）
3. 各OS（macOS/Windows/Linux）で、Arduino IDEの**Additional Boards Manager URLsを一時的にこのステージングブランチのraw URL**に切り替えてインストール検証
4. 全OSで問題なければ、いつも通り`main`に対して`update_package_index.yml`を手動実行してchecksum確定

**`package_nxp_mcx_index.json`のエントリを入れる時期**（0.8.0・0.9.0とも）: タグとリリースzipを作ったあと、本物のchecksum・sizeで`staging-<version>`だけに入れ、3OSの確認が済んでから`main`をそのコミットまでfast-forwardする。
`main`には仮の値のエントリを入れない（Boards Managerに未確認の版が出るため）。0.10.0までは、そのためにタグのコミットを`main`にpushした回とタグの回帰チェックが、hygieneの`package-index-entry`で失敗していた。2026-10-25に、このチェックを回帰チェックの`--release`から外し、`update_package_index.yml`だけが流す`--package-index`に移した（ユーザーが決定）。`main`とタグの回帰チェックは、ほかが通れば緑になる

**リリースzipを同じ名前で作り直したら、確かめる前に各OSのダウンロードのキャッシュから消す**（2026-10-25、ユーザーが決定）: Boards Managerは`staging/packages/`（macOSは`~/Library/Arduino15/staging/packages/`、Windowsは`%LOCALAPPDATA%\Arduino15\staging\packages\`、Linuxは`~/.arduino15/staging/packages/`）に同じ名前のファイルがあると、ダウンロードし直さずにそれを使う。0.4.0では作り直す前のzipでインストールされ、直したはずの不具合が各OSで再現した。作り直したときは`mcx-arduino-core-<version>.zip`を3つのOSとも消してから確かめる（`packages/nxp`の退避や索引の`package_nxp_mcx_index.json`の削除だけでは足りない）。checksumが合わないときも、このキャッシュに壊れたファイルが残っていないかを先に見る（issue #6）

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
   - **`docs/llms.txt`も対象にする**（0.12.0から、2026-10-26にユーザーが決定）: 冒頭の「Points to keep in mind」（FQBN、インスタンス名、Ethernet、同梱ライブラリなど）がその版の内容と合っているか、新しい文書やライブラリを足したならリストに載せたかを見る。リンクが追跡下のファイルを指していることと、`PIN_MAPPING_*.md`が全部載っていることは、hygieneの`llms-links`が毎push確かめる。リンクは`main`（raw）とPagesの絶対URLなので、公開されるのは`main`にマージしてから。版の番号は本文に書かない。llms-full.txt（全文をまとめたもの）は作らない
3. **`docs/api/`（Doxygen）の再生成**: `doxygen Doxyfile`を実行し、直近のソース変更（コメントを含む）が反映された状態にする。`docs/api/index.html`のタイムスタンプより新しいソースファイルがないか確認してから判断するとよい
   **`main`の`docs/`はGitHub Pagesで公開する**（0.11.0から、2026-10-25にユーザーが決定）: 公開先は`https://teddokano.github.io/mcx-arduino-core/api/`で、READMEからリンクしている。`docs/.nojekyll`を消さないこと（無いとJekyllが`_`で始まるファイルを公開せず、Doxygenのページの多くが404になる）。Pagesの設定（Settings → Pages、`main`の`/docs`）は**0.11.0を`main`にマージしたあとで、ユーザーが一度だけ行う**（`.nojekyll`より先に有効にすると、`_`のファイルが抜けたものが公開される）
4. **ライセンスチェック**: `LICENSE`全文を読み返し、開発期間中に追加した新規ツール/ファイル（外部プロジェクトのソースを読んで挙動を参考にしたもの含む）で、帰属記載が漏れているものがないか確認する。「コードは一切コピーしていないが、他プロジェクトのソースを読んで互換動作を実装した」ケースは、コピーでなくても透明性のため記載する、というこのプロジェクトの既存の判断基準（`upload.sh`のArduinoCore-zephyr参照等）を毎回適用する
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

---

## 残りのPendingタスク
1. ~~Linux対応の実機検証~~ **解消済み（v0.2.2で確定）**: v0.2.1リリース後の実機検証で、ファイル名の大文字小文字ミスマッチ（`arduino.h`/`Arduino.h`、`spi.h`/`SPI.h`）によりLinuxでビルドが失敗することが判明・修正し、v0.2.2としてリリース。Linux実機（Ubuntu系）でBoards Manager経由インストール〜Blinkスケッチのビルド〜書き込み〜実行まで成功を確認済み。README.md/TUTORIAL.md/TUTORIAL.ja.mdの「未検証」表記もすべて「macOS, Windows 11, Linuxで検証済み」に更新済み
2. マルチボード対応（MCXN947, MCXA156, MCXN236）— **N947は`prepare0.3.0`ブランチで完了**。GPIO/Serial/Wire/Wire1/SPI/analogRead/analogWrite/tone・noTone・MikroBusの`SPI1`/`Wire2`/`Serial1`まで実機検証済み、`README.md`の対応ボード表もA153と同じ✅に変更済み（ユーザー判断、2026-08-16）。v0.3.0でリリース済み。
   **A156は0.8.0、N236は0.9.0でリリース済み**（それぞれの「前提と決定事項」節）。C444は未着手
3. ~~`examples/tests/GPIO_NXP_Arduino`の不要なgitlinkエントリの整理~~ **解消済み**: `git ls-files --stage`で`160000`（gitlink）エントリが残っているのに`.gitmodules`が存在しないと判明（外部クローンの誤`git add`の名残）。`git rm --cached`でインデックスから除去し、他4つの外部ライブラリクローンと同様`.gitignore`に追加
4. ~~v0.3.0リリース~~ **完了**: 2026-08-16リリース。詳細は[docs/DEVELOPMENT_LOG.md](docs/DEVELOPMENT_LOG.md)の「リリース前最終チェックとv0.3.0リリース完了」節
5. ~~SDライブラリビルド時の`-Waddress-of-packed-member`警告~~ **解消済み（v0.3.1で対応）**: `platform.txt`の`compiler.cpp.flags`に`-Wno-address-of-packed-member`を追加して警告クラス自体を抑制。純粋な診断抑制フラグ（`-W`系）でコード生成には一切影響しないため、プリビルド`.a`の再ビルドや実機再検証は不要と判断——両ボードで`SDBitmapViewer`（`SD`ライブラリ使用）をコンパイルし、警告が完全に消えたことを確認
6. **上流`r01lib`（`~/dev/mcuxpresso/r01lib`、github.com/teddokano/r01lib）への反映は保留**（2026-09-26、ユーザー判断）。
   コア同梱の`r01lib`は上流と21ファイル・約1,800行ずれている（コア側だけに`pin_registry.*`・`r01lib_spi.*`、上流側だけに`spi.*`・`semihost_hardfault.c`）。
   上流はA156・N236・C444も対象にしていて、それらはここでは確かめられない。そのため
   **このコアで他のボード（A156等）の対応が揃った時点でまとめて反映する**。それまで上流リポジトリには手を入れない。
   **0.10.0（C444まで5ボードが揃う版）でも反映しない**（2026-10-18、ユーザーが決定）
7. **GPIO治具は作らない**（2026-10-02、ユーザー判断）。0.7.0で保留にしていたが、0.8.0の検討で
   「リリース作業の自動化が大きく進んでいて、治具による改善は手間の削減にそれほど効かない」として見送った。
   組み合わせを設計した`.github/scripts/check_jig_pairing.py`は残っている
8. ~~ネットワーク基底クラスの実際の通信~~ **0.11.0のN947のEthernetで確かめた**（2026-10-24、`release_check/25`）。以下はそれまでの記録。
   **ネットワーク基底クラス（`Client`/`Server`/`UDP`/`IPAddress`）は0.10.0で入れる**（2026-09-26、ユーザー判断で0.7.0は見送り。2026-10-15に0.9.2と決め、2026-10-16に0.10.0へ）。
   無いため`Ethernet`ライブラリが`Client.h`で止まる（ArduinoJsonのEthernet系サンプル3本もこれで失敗）。
   **0.10.0-dev（旧0.9.2-dev）で4つのクラスを書いた**（`cores/arduino/arduino_api/`の`Client.h`・`Server.h`・`Udp.h`・`IPAddress.{h,cpp}`）。AVRと同じく`Arduino.h`からは読み込まない。
   `IPAddress`はArduinoCore-APIと同じくIPv4とIPv6を持つ。`test_network_base_classes`がA153・N947の実機でALL PASS、ライブラリの作例は4ボードでビルドが通る。残りは下の「実際に通信できるまで」
   入れるときは、ArduinoCore-APIのソースがLGPLなのでコピーせず自前で書く。
   仮想関数のシグネチャはArduinoCore-APIと完全に一致させる（派生ライブラリがそのままビルドできるように）。
   W5500系シールドなどで**実際に通信できるまでを確認**すること（コンパイルだけでは対応済みにしない。
   **0.11.0のN947のEthernetでの確認でこれを満たすとみなし、クラスそのものは0.10.0で出す**（2026-10-18、ユーザーが決定。0.10.0での確認は`test_network_base_classes`の実機と、ライブラリの作例のビルドまで）
9. **LinkServer 26.9.130はFRDM-MCXA153のフラッシュを32KBと読む**（2026-09-28に判明、N947自体は26.9で動く）。
   32KBを超えるスケッチの書き込み・デバッグが「Attempt to load into missing flash area」で失敗する。
   0.7.1で、26.9を避けて別の版を使うようにした。**N947も含めて全ボードで避ける**——LinkServerは書き込み後も`redlinkserv`を残し、
   26.9は26.6が起動したそれに再接続すると`Redlink interface error 240`で失敗するので、ボードごとに版を変えられない。避ける版の一覧は
   `upload.sh`の`flash_size_bug()`、`upload.bat`の`:consider`、`gdb-bridge`の`flashSizeBug`の**3か所**にある。
   NXPが直した版を出したら、その版で46KBの`hello_world`を書けることを確かめてから3か所を揃えて更新する。
   原因の詳細はLinkServerのライセンス（使用結果の報告の公開制限）に配慮して、公開リポジトリには症状だけを書く
10. **A4/A5でI²Cを使えるようにする**（2026-10-08、ユーザー判断）: 0.10.0から外した（2026-10-18）あと、**0.12.0で入れた**（2026-10-30、作りはユーザーが決定。**`release_check/16`とmcxPinStateの一覧が実機の5ボードでOK**（2026-10-30））。
   入口は`bool TwoWire::setPins( int sda, int scl )`（ESP32と同じ形）。引数はスケッチのピン名で、`arduino_io.cpp`の`arduino_pin_raw()`で生の値にする。
   受け付けるのは**そのバスの元のピンの組とA4/A5だけ**（A153の`Wire`をMikroBusへ、のような組は足せるが、足していない）。`begin()`のあと（`end()`まで）、A4/A5にI²Cの無いボード（A153・N947）、そのペリフェラルを別の`TwoWire`が動かしているときは`false`でピンを変えない。
   **どのインスタンスでも、ペリフェラルが空いていれば移れる**: A156は`Wire`も`Wire1`も（`LPI2C1`）、N236は`Wire`も`Wire1`も（`LPI2C2`、同時には動かせない）、**C444は`Wire`だけ**（`Wire1`の元のピン`PTE25`/`PTE24`にはArduinoのピン番号が無く、移ると戻れないため。2026-10-30の作業中に分かり、提案のときのボードごとの表と合わせた）。
   `begin()`が、別の`TwoWire`の動かしているペリフェラルを取ろうとしたらpanicで止める（`arduino_i2c.cpp`の`s_wire_pins[]`がピンの組とペリフェラルの対応、`s_wires[]`が作られた`TwoWire`）。
   移ったあと、元のピンは`begin()`が付けたALTのままならGPIOの入力に戻す（`release_pads()`。`pinMode()`で取られていたら触らない）。
   **判定用のマクロ（`WIRE_HAS_A4A5`など）は入れない**（戻り値で分かる、N236のように実行中の状態で答えが変わる、名前が後の拡張に合わない、一度出すと消せない）。**AVRと同じ`WIRE_HAS_END`（値は`1`）を足した**（`end()`があるのに定義していなかった。2026-10-30、ユーザーが決定）。
   A4/A5の基板上のプルアップ: A156は無し、N236は無し（R298・R299は未実装）、C444は4.7kΩ（R26・R27）。`begin()`はどのピンでも内蔵のプルアップを入れる。
   `TwoWire`のコンストラクタは今のまま、生のピン値を取るコア用（memoの§5-2）。確認は`release_check/16_wire_setpins_A4A5`（A156・N236はA4-D18・A5-D19、C444はA5-D8・A4-D7、いずれもジャンパ線2本でブレッドボード無し。A153・N947は配線なしで`false`の場合だけ）。
   mcxPinStateの`Wire`の行は、`TwoWire`の今のピンを見るようにした（2026-10-30、ユーザーが決定）: コアに内部用の`raw_sda()`/`raw_scl()`（APIではない）を足し、ライブラリは`TwoWire`にそれがあるかをテンプレート（SFINAE）で見て、無い古いコアでは表のピンのまま。コアの版の番号では分けない（開発中の`platform.txt`はまだ0.11.0なので）。同梱のコピーと上流（`~/dev/Arduino/mcxPinState`）の両方を直した。**上流はコミットもpushもまだ**（pushは毎回ユーザーのOKが要る）
