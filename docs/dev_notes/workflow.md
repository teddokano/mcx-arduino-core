# ローカル開発環境・GitHub Actions・ステージング・ブランチ運用

CLAUDE.mdが大きくなったので、読むときだけ開くようにここへ移した（2026-11-09、ユーザーが決定）。中身は移した時点のまま（「下の項目」「上の項目」などの参照は元のCLAUDE.mdでの位置のまま）。

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
  いまの`~/Library/Arduino15/packages/nxp/hardware/mcx/`には`0.12.0-dev`のsymlinkだけがある（開発環境は戻してある。2026-11-09）。
  Boards Manager検証のために退避した分は、すべてゴミ箱に移した: 0.6.0（`mcx-0.6.0-backup`）、本番の`main`のURLから入れた0.10.0（`nxp-0.10.0-release-installed`）、`dev_env_restore.command`が避けたBoards Manager版の0.10.0・0.11.0・0.12.0（`nxp-boardsmanager-<日時>`）、IDEのダウンロードキャッシュ`Arduino15/staging/`、ビルドのキャッシュ`~/Library/Caches/arduino`（2026-11-09、ユーザーが決定。どの版の確認も済んでいる）。
  それより前の分は、0.10.0のステージングから入れた分と0.9.0・0.9.1の本番の分をゴミ箱に移し（2026-10-19）、0.8.0までのステージングの分と0.8.0の分はユーザーが片付けた（2026-10-12）
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

## 開発ブランチ運用方針（v0.3.2から採用）
ユーザー指示: 「0.3.2の開発版をスタート。このバージョンから開発用ブランチを切って進める。ドキュメント関連の更新はmainブランチで。開発用ブランチは0.3.2-devにする」

- 各バージョンの開発作業（ソースコード変更、r01lib/arduino_layerの修正、サンプル追加等）は**`<version>-dev`という名前の専用ブランチ**（例: `0.3.2-dev`）上で行う。これは`prepare0.3.0`ブランチでのN947対応と同じ「開発用ブランチを切ってから最後に`main`へマージ」というパターンだが、ブランチ命名規則を`prepare<version>`から`<version>-dev`に変更し、以後この命名で統一する
- ローカル開発用symlink（`~/Library/Arduino15/packages/nxp/hardware/mcx/<version>-dev`）も、ブランチ名と同じ`<version>-dev`という命名になるため、今後はgitブランチ名とsymlink名が常に一致する（偶然ではなく意図した対応）

**訂正（同セッション内）**: 当初「ドキュメントのみの更新は`main`に直接コミットする」という方針で着手し、実際に`analogWriteFrequency()`関連のドキュメント更新（`PIN_MAPPING_*.md`/`API_COMPATIBILITY.md`/`CHANGELOG.md`）を`main`にコミット・pushしたが、ユーザーから指摘で撤回: 「今後は作業中のブランチでやる。そうでないとリリース版との整合が取れないから」。`main`はいつでもBoards Manager経由でユーザーが参照しうる「現在のリリース版」の実体であり、未リリースの`0.3.2-dev`の機能を説明するドキュメントを`main`に置くと、その機能がまだ存在しない状態のユーザーに向けて存在するかのような記述を見せてしまう——上記の「利点」は誤りだった。
- **現在の方針: ドキュメント（CLAUDE.mdも含む）も含めて、そのバージョンの開発作業はすべて`<version>-dev`ブランチ上で行い、リリース時に`main`へまとめてマージする**
- `main`にコミット済みだった該当ドキュメント更新（`833f949`）は`git revert`で取り消し（`9c0f9ad`）、同じ内容を`0.3.2-dev`へ`git cherry-pick`で移設（`7e20c89`）
