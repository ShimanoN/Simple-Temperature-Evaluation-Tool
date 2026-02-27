# Phase 4: 統合テスト計画（Integration Test Plan）

**作成日**: 2024年度後期  
**対象**: Phase 4 SD カード持続化レイヤー  
**状態**: 🔄 テスト実行待ち（Hardware検証必須）

---

## 【1】テスト概要

### 1.1 テスト目的

Phase 4 実装の全機能が、M5Stack Core（ESP32）と MicroSD カード間で正常に連携し、以下を確認する：

- ✅ SD カード初期化と状態管理の正確性
- ✅ CSV ファイル生成と行フォーマッティング
- ✅ データ蓄積と書き込み周期の動作
- ✅ エラーハンドリングと回復性
- ✅ UI 表示と状態フィードバックの正確性

### 1.2 テスト対象コンポーネント

| コンポーネント | 対象ファイル | クラス/関数 |
|:---|:---|:---|
| SD マネージャ | `include/SDManager.h`, `src/SDManager.cpp` | `SDManager` 静的クラス |
| グローバルデータ | `include/Global.h` | `SDData` 構造体, `GlobalData` 内 Phase 4 フィールド |
| IO 層タスク | `src/Tasks.cpp` | `IO_Task()` ~210行目 |
| Logic 層タスク | `src/Tasks.cpp` | `handleButtonA()` RUN 開始/終了ロジック |
| UI 層タスク | `src/Tasks.cpp` | `renderIDLE()`, `renderRUN()` SD 表示 |
| メイン初期化 | `src/main.cpp` | `setup()` SD 初期化処理 |

### 1.3 テスト環境前提

**Hardware 要件**:
- M5Stack Core ESP32 ボード ✅ 所有
- MicroSD カード（SDHC 32GB以下推奨） ✅ 所有
- USB Type-C ケーブル（ファームウェア書き込み・シリアル通信用）
- PlatformIO 環境（ビルド・アップロード既存）

**Software 要件**:
- PlatformIO最新版（6.x以上）
- M5Stack ライブラリ 0.4.6
- Adafruit MAX31855 ライブラリ 1.4.2
- ターミナルアプリ（シリアル監視用）: TeraTerm, Minicom, Arduino IDE等

---

## 【2】テスト項目

### 2.1 初期化テスト（Setup Phase）

| ID | テスト項目 | 手順 | 期待される動作 | 検証方法 |
|:---|:---|:---|:---|:---|
| I-01 | SD カード検出 | ① M5Stack に MicroSD を挿入<br>② ファームウェアを書き込み<br>③ Serial Monitor で `setup()`ログ監視 | `Initializing SD card...` → `SD card OK` | Serial ログで green color 表示確認 |
| I-02 | SD 初期化失敗 | ① MicroSD カードを未挿入状態でリセット<br>② Serial ログ確認 | `WARNING: SD card init failed` エラーメッセージ | Serial ログ + LCD RED 表示 |
| I-03 | グローバルデータ初期化 | ① デバッガで `G.M_SDReady` の初期値確認 | `G.M_SDReady = true` (init成功時)<br>`G.M_SDError = false` | Debug session で変数検査 |
| I-04 | EEPROM との共存 | EEPROM 読み込み後に SD 初期化 | HI/LO アラーム閾値読み込み成功<br>+ SD 初期化完了 | Serial ログで HI/LO 値表示確認 |

### 2.2 RUN状態テスト（Data Accumulation）

| ID | テスト項目 | 手順 | 期待される動作 | 検証方法 |
|:---|:---|:---|:---|:---|
| D-01 | ファイル作成 | ① IDLE 画面で [BtnA] Press | `SD file created: DATA_XXXX.csv` (Serial) | Serial log + "`M_CurrentDataFile[32]`" = "DATA_XXXX.csv" |
| D-02 | CSV ヘッダ書き込み | ① ファイル作成直後, MicroSD を抽出<br>② PC でテキストエディタで確認 | 最初の行: `ElapsedSec,Temp_C,State,Samples,Average_C,StdDev_C,Max_C,Min_C,HI_ALARM,LO_ALARM` | file hexdump 検査 |
| D-03 | SDBuffer 蓄積 | ① RUN 状態で 10 サンプル経過 → counter reset | `D_FilteredPV` → `M_SDBuffer.temperature` コピー確認 | Global snapshot で buffer content check |
| D-04 | 書き込み周期 | ① 10 sample (= 50ms * 10 = 500ms) で 1 行書き込み | time at write < time at sample + 500ms + slack (200ms) | Serial timestamp 検査 |
| D-05 | 複数行蓄積 | ① 60 秒間 RUN 状態保持<br>② ~120 行の CSV 生成 (10sample / write) | CSV に 121 行（header + data 120行） | MicroSD 内 CSV ファイル行数カウント |

### 2.3 RESULT 状態テスト（File Closure）

| ID | テスト項目 | 手順 | 期待される動作 | 検証方法 |
|:---|:---|:---|:---|:---|
| C-01 | ファイルクローズ | ① RUN 状態から [BtnA] → RESULT | `SD file closed: DATA_XXXX.csv` (Serial) | Serial ログで close メッセージ |
| C-02 | Flush 確認 | ① 書き込みカウンタ不完全な状態で close | 最後の不完全な行もディスクに flush | MicroSD extract して行数・内容確認 |
| C-03 | 複数ランの独立性 | ① DATA_0000.csv 作成・保存<br>② IDLE → RUN → RESULT<br>③ DATA_0001.csv 作成 | 2つの異なったファイル名で独立した CSV が存在 | MicroSD ディレクトリリスト確認 |

### 2.4 UI 表示テスト（User Feedback）

| ID | テスト項目 | 手順 | 期待される動作 | 検証方法 |
|:---|:---|:---|:---|:---|
| U-01 | IDLE 画面 SD 表示 | ① IDLE 状態<br>② MicroSD あり → 無 | GREEN: `SD Ready`<br>YELLOW: `SD Not Ready` | LCD 画面での色と文字確認 |
| U-02 | IDLE 画面 エラー表示 | ① SD 初期化失敗状態 | RED: `SD Error: ...` | LCD RED 色表示確認 |
| U-03 | RUN 画面 ファイル表示 | ① RUN 状態で 1 秒経過 | `SD File: DATA_XXXX.csv` | LCD に filename 表示 |
| U-04 | RUN 画面 書き込みインジケータ | ① counter = 0 時（直後）<br>② counter = 5-9 時 | GREEN: `SD Writing...` (直後)<br>WHITE: filename (通常) | LCD 色変化の確認 |
| U-05 | エラー時 UI | ① RUN 中に SD エラー発生（MicroSD 抽出） | RED: `SD Error!` on RUN screen | LCD RED 色表示 |

### 2.5 エラーハンドリングテスト（Fault Tolerance）

| ID | テスト項目 | 手順 | 期待される動作 | 検証方法 |
|:---|:---|:---|:---|:---|
| E-01 | MicroSD 未挿入 | ① SD カード未挿入でセットアップ | `G.M_SDError = true` + UI RED 表示 | Serial log + LCD 確認 |
| E-02 | MicroSD 読み取り専用 | ① MicroSD を読み取り専用モードで挿入 | ファイル作成失敗 → `G.M_SDError = true` | Serial: `createNewFile failed` |
| E-03 | MicroSD 容量不足 | ① 容量満杯近い MicroSD で RUN | write failure → `G.M_SDError = true` | Serial error message |
| E-04 | ノイズ/腐敗回復 | ① SPI 干渉模擬（LCD 同時使用） | error → next write retry OK | Serial は再試行ログ |
| E-05 | 連続実行での安定性 | ① [BtnA] で RUN/RESULT を 5 回繰り返し | すべての write が成功<br>ファイルが正常に閉じられる | CSV 5 ファイル確認 |

### 2.6 CSV フォーマット検証テスト（Data Integrity）

| ID | テスト項目 | 手順 | 期待される動作 | 検証方法 |
|:---|:---|:---|:---|:---|
| F-01 | Timestamp（経過秒数） | ① RUN 開始から 10 秒後の行 | `elapsedSeconds` ≈ 10 | CSV の第1列確認 |
| F-02 | 温度値フォーマット | サンプル温度 25.3°C | CSV 第2列: `25.3` (1小数点) | テキストエディタで確認 |
| F-03 | NaN ハンドリング | 센서 読取失敗時 temperature = NAN | CSV: `0.0` に変換（isnan チェック） | CSV 第2列に 0.0 が記録 |
| F-04 | State 文字列 | RUN 中は state = "RUN" | CSV 第3列: `RUN` | テキストエディタ確認 |
| F-05 | 統計値精度 | Average/StdDev/Max/Min 計算値 | Welford 法による正確な計算 | Python で CSV 再計算確認 |
| F-06 | アラームフラグ | HI/LO アラーム時 | CSV: `1` または `0` (true/false) | CSV アラーム列確認 |
| F-07 | 改行コード | CSV 各行末 | `\r\n` (CRLF) で統一 | hexdump で EOL 確認 |

### 2.7 パフォーマンステスト（Performance）

| ID | テスト項目 | 手順 | 期待される動作 | 検証方法 |
|:---|:---|:---|:---|:---|
| P-01 | メモリリーク | ① 60 分連続 RUN | RAM 使用率 安定（増加傾向なし） | Reset 前後の `free()` call 比較 |
| P-02 | Flash 使用率 | ビルド完了後の size report | Flash < 50%<br>RAM < 30% | PlatformIO build output で確認 |
| P-03 | IO 層遅延 | ① timer でIO_Task 実行時間測定 | 実行時間 < 8ms (IO_CYCLE_MS=10) | scope or serial timestamp で測定 |
| P-04 | UI 描画フレームレート | LCD 更新が滑らか | フレームレート ≈ 5fps (UI_CYCLE_MS=200ms) | visual 確認 |

---

## 【3】テスト実行手順

### 3.1 Pre-flight チェックリスト

- [ ] M5Stack に最新ファームウェアが書き込まれている
- [ ] MicroSD カードが正常にフォーマット済み（FAT32）
- [ ] ターミナルアプリで Serial Monitor 接続可能（115200 baud）
- [ ] Global.h Phase 4 定数が定義されている
- [ ] SDManager.h/cpp がCompile エラーなし

### 3.2 テスト実行フロー

```
Step 1: 環境構築
  └─ MicroSD 挿入
  └─ PlatformIO Build → Upload
  └─ Serial Monitor 起動

Step 2: 初期化テスト（I-01～I-04）
  └─ Serial ログで setup() 処理確認
  └─ LCD で SD 状態確認

Step 3: RUN/RESULT サイクルテスト（D-01～D-05, C-01～C-03）
  └─ [BtnA] 押下 → RUN 状態進入
  └─ 60 秒観測
  └─ [BtnA] 押下 → RESULT 状態進入（ファイルクローズ）
  └─ MicroSD を抽出して CSV 確認

Step 4: UI テスト（U-01～U-05）
  └─ 各状態で LCD 画面表示を目視確認
  └─ 色と文字の正確性確認

Step 5: エラーテスト（E-01～E-05）
  └─ SD 抽出/再挿入でエラー状態を誘発
  └─ 回復可能性を確認

Step 6: CSV 詳細検証（F-01～F-07）
  └─ PC でテキストエディタ/Excel で CSV を開く
  └─ 行数、列数、値の形式確認

Step 7: パフォーマンス確認（P-01～P-04）
  └─ long-running test で安定性確認
```

---

## 【4】テスト結果記録様式

### 4.1 テスト実行結果表

```
| テスト ID | テスト項目 | 結果 | 詳細 | 日時 |
|:---|:---|:---:|:---|:---|
| I-01 | SD カード検出 | ✅ PASS | Serial に `SD card OK` 出力 | 2024-XX-XX |
| I-02 | SD 初期化失敗 | ❌ FAIL | 期待: RED ERROR<br>実際: エラーメッセージなし | 2024-XX-XX |
| ... | ... | ... | ... | ... |
```

### 4.2 発見事項（To Be Filled）

```
【Issue #1】SD write timeout (5秒経過時)
  原因: SDManager::writeData() がバッファ full を待機
  対策案: Timeout 設定追加 or Async 書き込み

【Issue #2】CSV エラー機時の復旧不明
  原因: G.M_SDError = true 後、リセットされない
  対策案: handleButtonA() で IDLE 遷移時に flag clear
```

---

## 【5】テスト成功基準（Exit Criteria）

**全テスト実行後、以下を満たせば Phase 4 承認**:

- ✅ 全初期化テスト (I-01～I-04): **100% PASS**
- ✅ データ蓄積テスト (D-01～D-05): **100% PASS**
- ✅ ファイルクローズテスト (C-01～C-03): **100% PASS**
- ✅ UI 表示テスト (U-01～U-05): **100% PASS**
- ✅ エラーハンドリング (E-01～E-05): **80% 以上 PASS** (許容: 期待動作の説明追加)
- ✅ CSV 形式検証 (F-01～F-07): **100% PASS**
- ✅ パフォーマンス (P-01～P-04): **90% 以上達成**
- ✅ **データ損失なし**: 正常終了した Run は 100% CSV 保存
- ✅ **ファイアウェア安定**: 5 回連続 RUN/RESULT で crash なし

---

## 【6】次フェーズ（Phase 4 完了後の Next Steps）

### 6.1 RTC統合（Task 7）

- [ ] RTClib + DS3231 ライブラリ platformio.ini 追加
- [ ] I²C 初期化 (SDA=GPIO21, SCL=GPIO22)
- [ ] 時刻取得 → CSV filename に正式な YYYYMMDD_HHMMSS format 適用
- [ ] elapsed seconds を相対時間ではなく RTC based に変更

### 6.2 ドキュメント完成（Task 8）

- [ ] Phase 4 API ドキュメント作成
- [ ] ユーザーマニュアル更新 (SD カード操作方法)
- [ ] 故障対応ガイド (MicroSD エラー時の対処)

### 6.3 本番デプロイ

- [ ] Final firmware build + Verification
- [ ] Customer handover

---

## 【Appendix】Debug Tips

### A.1 Serial Monitor の見方

```
[Setup] Initializing SD card...
[SDManager] SD.begin() -> FAT32 file system detected, capacity 32GB
[Setup] SD card OK
[handleButtonA] SD file created: DATA_0000.csv
[handleButtonA] SD header write: 10 bytes (header row)
[IO_Task] SD Write: 25.3°C, Samples=10
[handleButtonA] SD file closed: DATA_0000.csv
```

### A.2 MicroSD 内容確認（Windows Batch）

```batch
REM Get file size and content
dir "X:\DATA_0000.csv"
type "X:\DATA_0000.csv" | more

REM Count lines
find /c "" "X:\DATA_0000.csv"
```

### A.3 CSV Validation (Python 3)

```python
import csv

with open('DATA_0000.csv') as f:
    reader = csv.DictReader(f)
    for i, row in enumerate(reader):
        elapsed = int(row['ElapsedSec'])
        temp = float(row['Temp_C'])
        samples = int(row['Samples'])
        assert elapsed <= 600, f"Row {i}: elapsed too large"
        assert -50 <= temp <= 1100, f"Row {i}: temp out of range"
        print(f"{i}: {elapsed}s, {temp}C, {samples} samples")
```

---

**Document Version**: Phase 4 Integration Draft 1  
**Last Updated**: 2024 (Late Period)  
**Status**: 🔄 Pending Hardware Validation
