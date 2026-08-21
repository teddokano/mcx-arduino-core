# mcx-arduino-core

<p align="center">
  <img src="img/FRDM-MCXA153.jpg" alt="FRDM-MCXA153 running an mcx-arduino-core sketch" width="400"><br>
  <em>FRDM-MCXA153</em>
</p>

NXP FRDM MCXシリーズボード向けのArduinoボードサポートパッケージです。

English version → [README.md](README.md)

初めての方は[チュートリアル日本語版](TUTORIAL.ja.md)（[英語版もあります](TUTORIAL.md)）から始めてください。
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
**Arduino IDEのUpload/Debugボタンを使う前にインストールしておいて**ください。

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

全てソース含まれているので、Arduino IDEの「Go to Definition」も通常通り機能します——
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

Arduino公式の[ArduinoCore-zephyr](https://github.com/arduino/ArduinoCore-zephyr)は、
FRDM-MCXN947を含む一部のNXP MCXボードに、すでに公式のArduino対応を提供しています。
ただしFRDM-MCXA153だけは対象外で、これは見落としではなく意図的なものです。
mcx-arduino-coreはArduinoとは無関係の独立プロジェクトで、ArduinoCore-zephyrの
アーキテクチャではどうしても収まらないFRDM-MCXA153をカバーする目的で作られました。

ArduinoCore-zephyrは、Zephyrベースの「ローダー」をあらかじめ一度だけ書き込んでおき、
各スケッチは実行時にその上へZephyrの**LLEXT**（Loadable Extension、動的にロードできる
拡張モジュール）として読み込む方式を取っています。つまり、1つの自己完結したバイナリを
ビルドしているわけではありません。このアーキテクチャでは、Zephyrカーネル・LLEXT
ランタイム・シンボルテーブルが常にRAMに常駐し続けるうえ、アップロード中のスケッチを
受け取るためのバッファも別途必要です。ローダーのソースコードを見ると、このバッファは
`SKETCH_RAM_BUFFER_LEN 131072`（128KB）と定義されています。ところがFRDM-MCXA153の
**RAMは合計でも24KB**しかなく、このバッファひとつだけでチップの全RAMの5倍以上を
占めてしまう計算になります。RAMに余裕のあるFRDM-MCXN947であれば、このアーキテクチャでも
問題なく収まります。

mcx-arduino-coreはこれとは逆のアプローチを取っています。各スケッチをr01libコアと
一緒に1つのモノリシックなバイナリへコンパイル・静的リンクする方式で、ローダーも
動的リンクも存在せず、LLEXTに相当するものがRAMに常駐することもありません。この
仕組みのおかげで、FRDM-MCXA153の実際のリソース（RAM 24KB／フラッシュ128KB）に
無理なく収まっています（この数値は、このボード自身のビルド出力が実際に報告している
値そのものです）。

（Zephyr RTOS自体はFRDM-MCXA153上でも問題なく動作します——LinkServerはmainline
Zephyrのデフォルトのフラッシュランナーとしても使われています。収まらないのは
LLEXTベースのArduinoレイヤーに固有の制約であって、Zephyr自体の限界ではありません。）
