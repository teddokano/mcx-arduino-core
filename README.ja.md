# mcx-arduino-core

<p align="center">
  <img src="img/FRDM-MCXA153.jpg" alt="FRDM-MCXA153 running an mcx-arduino-core sketch" width="400"><br>
  <em>FRDM-MCXA153</em>
</p>

NXP FRDM MCXシリーズボード向けのArduinoボードサポートパッケージです。

English version → [README.md](README.md)

初めての方は[チュートリアル](TUTORIAL.ja.md)（[English](TUTORIAL.md)）から始めてください。
Arduino API対応状況の一覧は[API_COMPATIBILITY.md](API_COMPATIBILITY.md)、
各ボードのピン配置は[PIN_MAPPING_A153.md](PIN_MAPPING_A153.md) / [PIN_MAPPING_N947.md](PIN_MAPPING_N947.md)、
バージョン間の変更点は[CHANGELOG.md](CHANGELOG.md)、
生成済みDoxygenクラスリファレンス（r01libドライバコアとArduino互換APIレイヤー）は
[docs/api/](docs/api/index.html)を参照してください（これらは英語のみです）。

標準Arduino APIの先にある上級者向けガイドもあります（いずれも英語のみ）:
[MCUXpresso SDKを直接呼び出す](docs/advanced_sdk_tuning.md)（GPIO速度チューニング）、
[r01libによるネイティブI3C](docs/advanced_r01lib_i3c.md)（`Wire`形式のAPIでは扱えない機能向け）、
[mcxPinStateによるピン所有状況のデバッグ](docs/mcxpinstate_guide.md)（そのための専用ライブラリ）。

[![youtube](img/youtube.png) セットアップガイド動画](https://youtu.be/g_rDAxnVnro)もあります。

## 対応ボード

| ボード | MCU | コア |
|-------|-----|------|
| FRDM-MCXA153 | MCXA153 (Cortex-M33) | ✅ |
| FRDM-MCXA156 | MCXA156 (Cortex-M33) | 🔜 |
| FRDM-MCXN947 | MCXN947 (Cortex-M33) | ✅ |
| FRDM-MCXN236 | MCXN236 (Cortex-M33) | 🔜 |

> **注**: mcx-arduino-coreは独立したコミュニティプロジェクトであり、Arduino公式の
> [ArduinoCore-zephyr](https://github.com/arduino/ArduinoCore-zephyr)の一部でも
> 関連プロジェクトでもありません。このプロジェクトが（ArduinoCore-zephyrと並行して）
> 存在する理由は、本ドキュメント末尾の
> [ArduinoCore-zephyrとの関係](#arduinocore-zephyrとの関係)を参照してください。

## 必要なもの

### NXP LinkServer（アップロード・デバッグに必須）

このパッケージはスケッチをボードへアップロードする際に**NXP LinkServer**を使用します。
また、Arduino IDE 2の内蔵デバッガ（ブレークポイント、ステップ実行、変数参照）のバックエンドとしても
LinkServer自身のgdbserverを使用します——本家OpenOCDはMCXチップファミリにまだ対応していないためです。
Arduino IDEのUpload/Debugボタンを使う前にインストールしておいてください。

👉 ダウンロード: https://www.nxp.com/linkserver

| OS | インストーラー |
|----|-----------|
| macOS | `.pkg`ファイル、ダブルクリックでインストール |
| Windows | `.exe`インストーラー |
| Linux | `.deb.bin`ファイル |

インストール後、アップロードスクリプトが自動的にLinkServerを検出します——パス設定は不要です。

インストール→ビルド→アップロードの一連の流れは**macOS・Windows 11・Linux**で検証済みです。

## インストール

1. [Arduino IDE](https://docs.arduino.cc/software/ide/) 2.xを開く
2. **File → Preferences**を開く
3. **Additional boards manager URLs**に以下のURLを追加:
```
https://raw.githubusercontent.com/teddokano/mcx-arduino-core/main/package_nxp_mcx_index.json
```

4. **Tools → Board → Boards Manager**を開く
5. `NXP MCX`を検索して**Install**をクリック

## アーキテクチャ

このパッケージはプリビルドライブラリを使わず、他のArduinoコア（AVR、SAMD、renesas_uno等）と
同じ方式で完全なソースを配布しています:
```
mcx-arduino-core/
├── hardware/nxp/mcx/
│   ├── platform.txt          # コンパイラ/リンカ/デバッガの設定
│   ├── boards.txt            # ボード定義
│   ├── cores/arduino/        # 全ボード共有のソース（core.aにビルドされる）、出自ごとに分割:
│   │   ├── arduino_api/      #   Arduino互換APIレイヤー（digitalWrite、Wire、SPI、
│   │   │                     #   Serial.print、String、Print/Stream等）
│   │   ├── r01lib/           #   r01libハードウェアドライバコア（Serial、I2C/I3C、SPI、GPIO、
│   │   │                     #   AnalogIn、PwmOut、InterruptIn、Ticker等）
│   │   └── sdk/               #   対応する全チップ共通のNXP MCX SDKドライバファイル
│   ├── tools/
│   │   ├── upload.sh         # アップロードスクリプト（LinkServer自動検出）、Windows用はupload.bat
│   │   └── gdb-bridge/       # Arduino IDE 2のcortex-debug（OpenOCDを想定）をLinkServer自身の
│   │                         #   gdbserverへ橋渡しし、IDE内デバッグを実現
│   └── variants/
│       └── frdm_mcxa153/     # 対応ボードごとにこのようなディレクトリが1つずつ
│           ├── include/      # ボード固有のヘッダ
│           ├── linker/       # リンカスクリプト
│           ├── svd/          # CMSIS-SVDペリフェラルレジスタ記述子（IDEデバッガの
│           │                 #   Cortex Peripheralsレジスタビュー用）
│           └── src/          # ボード固有のソース: pin_mux、clock_config、board、
│                              #   デバイス起動処理、チップごとに異なるSDKドライバ
└── package_nxp_mcx_index.json
```

全てソースなので、Arduino IDEの「Go to Definition」も通常通り機能します——
`pinMode()`や`Serial`など任意の関数にジャンプすると、ヘッダの宣言だけでなく
実装している実際の`.cpp`に飛びます。

## サンプルスケッチ
```cpp
#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    Serial.println("Hello from FRDM-MCXA153!");
    pinMode(RED, OUTPUT);
}

void loop() {
    digitalWrite(RED, LOW);
    delay(500);
    digitalWrite(RED, HIGH);
    delay(500);
}
```

## ライセンス

MIT License — [LICENSE](LICENSE)を参照

## ピン配置

対応ボードごとのArduinoピン↔MCUピンの完全な対応表（オンボードLED/ボタン・ペリフェラルピン
（`Wire1`、`SPI`、`PWM`等）込み）は
[PIN_MAPPING_A153.md](PIN_MAPPING_A153.md) / [PIN_MAPPING_N947.md](PIN_MAPPING_N947.md)
を参照してください。

## 対応Arduino API

GPIO、割り込み、Serial（USB＋ハードウェアUART）、Wire（I2CおよびI3CのI2Cモード）、SPI、
analogRead/analogWrite、millis/micros、tone/noTone、delay系、String、本物の
`Print`/`Stream`/`Printable`基底クラス、F()/PROGMEM、UNO R3/R4互換マクロ、いずれも対応済みです。
I2Cスレーブモードと`Wire.setWireTimeout`が既知の未対応項目です。`Print`を直接継承する、
または`Stream&`を受け取るサードパーティライブラリ（ArduinoJson、LiquidCrystal、Adafruit系
センサーライブラリ等）もこのコア上でコンパイルできます。

API単位の対応状況・注意点の全表は[API_COMPATIBILITY.md](API_COMPATIBILITY.md)を参照してください。

## ArduinoCore-zephyrとの関係

Arduino公式の[ArduinoCore-zephyr](https://github.com/arduino/ArduinoCore-zephyr)プロジェクトは、
FRDM-MCXN947を含む一部のNXP MCXボードに既に公式Arduinoサポートをもたらしています——
しかしFRDM-MCXA153には対応しておらず、これは見落としではありません。本プロジェクト
（mcx-arduino-core）はArduinoとは独立・無関係のプロジェクトで、ArduinoCore-zephyrの
アーキテクチャでは収まらないFRDM-MCXA153をカバーするために存在しています。

ArduinoCore-zephyrはZephyrベースの「ローダー」を一度書き込み、各スケッチはその上に
Zephyrの**LLEXT**（Loadable Extension）として実行時にロードする方式です——1つの自己完結した
バイナリをビルドするわけではありません。このアーキテクチャでは、Zephyrカーネル・LLEXT
ランタイム・シンボルテーブルが常時RAMに常駐し、さらにアップロード中のスケッチを受け取る
バッファも必要です——ローダーのソースはこのバッファを`SKETCH_RAM_BUFFER_LEN 131072`
（128KB）と定義しています。FRDM-MCXA153は**RAMが合計24KB**しかないため、このバッファ
1つだけでチップの全RAMの5倍以上になってしまいます。RAMに余裕のあるFRDM-MCXN947では、
このアーキテクチャは問題なく収まります。

mcx-arduino-coreは逆のアプローチを取ります: 各スケッチはr01libコアと一緒に1つの
モノリシックなバイナリへコンパイル・静的リンクされます——ローダーなし、動的リンクなし、
LLEXT的なものがRAMに常駐することもありません。これによってFRDM-MCXA153の実際の
RAM 24KB / フラッシュ128KBという予算内に収まっています（この数値はこのボード自身の
ビルド出力が報告する値と同じです）。

（Zephyr RTOS自体はFRDM-MCXA153上で問題なく動作します——LinkServerはmainline Zephyrでの
デフォルトのフラッシュランナーでもあります。収まらないのはLLEXTベースのArduinoレイヤー
特有の話であって、Zephyr全体の話ではありません。）
