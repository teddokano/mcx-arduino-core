# mcx-arduino-core 開発引き継ぎドキュメント

## プロジェクト概要
- **リポジトリ**: https://github.com/teddokano/mcx-arduino-core
- **現在のバージョン**: v0.1.5
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

## 動作確認済み（v0.1.5）

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

## 残りのPendingタスク（v0.1.5以降）
1. マルチボード対応（MCXN947, MCXA156, MCXN236）
2. `analogWrite` / PWM実装
3. `millis` / `micros` 実装
4. IchigoJam-firm GPIO/PWMサポート（FRDM-MCXA153）
