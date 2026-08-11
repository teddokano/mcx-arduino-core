# mcx-arduino-core 開発引き継ぎドキュメント

## プロジェクト概要
- **リポジトリ**: https://github.com/teddokano/mcx-arduino-core
- **現在のバージョン**: v0.1.8（`package_nxp_mcx_index.json` 上の最新リリース）
- **作業中バージョン**: v0.1.9（`prepare0.1.9` ブランチ、`main` 未マージ・未リリース）
- **内容**: NXP FRDM-MCXA153 (Cortex-M33) 向けArduino IDEボードサポートパッケージ

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

## v0.1.9 で作業中の内容（`prepare0.1.9` ブランチ・未リリース）

オンボードのP3T1755温度センサーをI3Cバス経由でI2Cモードとして使えるようにする対応。

### arduino_i2c.cpp / arduino_i2c.h（`MCUXpresso_project/_r01lib_frdm_mcxa153/arduino_layer/` および `hardware/nxp/mcx/variants/frdm_mcxa153/include/` の両方に同様の変更）
- `TwoWire Wire1( I3C_SDA, I3C_SCL );` を新規追加（`extern TwoWire Wire1;` をヘッダへ宣言）
- `TwoWire::begin()` で、SDA/SCLピンが `I3C_SDA`/`I3C_SCL` と一致する場合は `I3C` インスタンスを生成し `mode( I3C::MODE::I2C_MODE )` に設定してから `i2c` ポインタへ代入するよう分岐を追加（一致しない場合は従来通り `I2C` を生成）
- 使用例: `examples/Arduino_compatible_API/test_Wire_P3T1755/test_Wire_P3T1755.ino` — `P3T1755 sensor(Wire1, 0x48);`

### analogRead / analogWrite 実装（ADC・PWM対応、完了・未コミット）
`/Users/tedd/dev/mcuxpresso/r01lib_prj_generator/` で生成されたFRDM-MCXA153向け `AnalogIn`（LPADC）/ `PwmOut`（FlexPWM0）クラスを移植し、Arduino API化。

- **r01lib本体**（`MCUXpresso_project/_r01lib_frdm_mcxa153/source/r01lib/`）
  - `AnalogIn.h/.cpp`（12bit LPADC、A0-A3対応）、`PwmOut.h/.cpp`（FlexPWM0 sm0-2、PWM0-PWM5対応）を新規追加
  - `io.h` に `PWM0`〜`PWM5`（P3_6〜P3_11。既存のD0-D19とは物理的に重複しない新規ピン、どのコネクタに出ているか要確認）を追加
  - `r01lib.h` に両クラスのincludeを追加
  - 依存する `fsl_lpadc.c/h` SDKドライバを `drivers/` に追加（`fsl_pwm.c/h`は既存を流用）
- **Arduinoレイヤー**（`arduino_layer/arduino_analog.cpp/.h` 新規）
  - `analogRead(pin)`：16bit値を10bit（0-1023、classic Arduino準拠）に変換。ピンごとに `AnalogIn` を遅延生成（Wire/SPIと同じ遅延初期化パターン）
  - `analogWrite(pin, value)`：0-255のduty値を `PwmOut` に反映。初回生成時に周期1kHzを設定
  - `arduino.h`（variant側・core側の両方、`hardware/nxp/mcx/cores/arduino/arduino.h` にも同じincludeが必要な点に注意 — コンパイラの `-I` 順序でcore側が優先解決されるため）、`arduino_io.h` のピン再番号付けテーブルに反映
- **ビルド**：`Debug/`配下の`subdir.mk`を手動更新し、xPack arm-none-eabiツールチェーンで`lib_r01lib_frdm_mcxa153.a`を再ビルド → `hardware/nxp/mcx/variants/frdm_mcxa153/`（include/lib）に同期
- **動作確認**：テストスケッチ `examples/Arduino_compatible_API/test_Analog_read_write/test_Analog_read_write.ino` を作成し、platform.txt準拠のビルドレシピを手動再現してコンパイル・リンク・シンボル解決を確認。既存`hello_world.ino`の回帰も確認済み
- 未コミット（作業ツリーに変更あり）

### 未追跡（未コミット）の作業ツリー内容
- `examples/tests/` 配下に以下4つの外部ライブラリが独立git repoとして手元clone状態で存在（サブモジュール化はされていない、`.gitmodules`なし）。動作テスト目的とみられ、コミットするかどうか要判断：
  - `I2C_device_Arduino`, `LCDDriver_NXP_Arduino`, `LEDDriver_NXP_Arduino`, `TempSensor_NXP_Arduino`（いずれも `github.com/teddokano/...`）

### 未対応
- `package_nxp_mcx_index.json` のバージョンはまだ `0.1.8` のまま。v0.1.9として正式リリースするにはバージョン番号更新・checksum更新・`main`へのマージが必要

---

## 動作確認済み（v0.1.5時点、以降未更新）

| API | macOS | Windows |
|---|---|---|
| GPIO / Lチカ | ✅ | ✅ |
| Serial | ✅ | ✅ |
| Wire (I2C) | ✅ | ✅ |
| SPI | ✅ | ✅ |
| attachInterrupt | ✅ | ✅ |
| ボードマネージャーインストール | ✅ | ✅ |

---

## ローカル開発環境
- **OS**: macOS（Saitama, Japan）
- **リポジトリパス**: `~/dev/mcx-arduino-core`
- **MCUXpressoプロジェクト**: `~/dev/mcx-arduino-core/MCUXpresso_project/_r01lib_frdm_mcxa153/`
- **ビルド済み.a**: `~/dev/mcx-arduino-core/MCUXpresso_project/_r01lib_frdm_mcxa153/Debug/lib_r01lib_frdm_mcxa153.a`
- **Arduino15インストール済みパス**: `~/Library/Arduino15/packages/nxp/hardware/mcx/0.1.5/`

## GitHub Actions
- **Workflow**: `.github/workflows/update_package_index.yml`
- **役割**: GCCのsizeをHEADリクエストで取得、プラットフォームZIPのchecksum/sizeをダウンロードして計算・更新

---

## 残りのPendingタスク
1. analogRead/analogWrite実装のコミット、PWM0-PWM5が実際にどのコネクタ/ピンに出ているか確認
2. v0.1.9リリース作業：`package_nxp_mcx_index.json` のバージョン/checksum更新、`prepare0.1.9` → `main` マージ
3. `examples/tests/` 配下の未追跡外部ライブラリ4件の扱い決定（コミット対象外にする/サブモジュール化する等）
4. マルチボード対応（MCXN947, MCXA156, MCXN236）
5. `millis` / `micros` 実装
6. IchigoJam-firm GPIO/PWMサポート（FRDM-MCXA153）
