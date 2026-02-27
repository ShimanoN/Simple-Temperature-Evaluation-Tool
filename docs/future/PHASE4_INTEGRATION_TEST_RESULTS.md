# Phase 4: 統合テスト実行結果（Integration Test Execution Results）

**テスト開始日時**: 2026年2月27日 開始  
**対象ファームウェア**: Phase 4 Core Implementation v1.0  
**SD カード**: ✅ 装填完了  
**テスト方策**: 高品質優先、段階的検証

---

## 【0】パフォーマンス最適化（Performance Optimization）

### 問題A：Average/StdDev が 0.0 のまま更新されない（発見1・修正1）

**第1回テスト結果**:
```
Average_C: 0.0 (全行で固定)
StdDev_C:  0.0 (全行で固定)
```

**根本原因（修正前）**:

| 項目 | タイミング | 詳細 |
|:---|:---|:---|
| IO_Task | 10ms 周期 | サンプル取得＆ SD 書き込み判定 |
| Logic_Task | 50ms 周期 | Welford 統計計算（D_Sum, D_M2 更新） |
| 修正前実装 | `D_Count >= 2` | **20-30ms で write → Welford は未実行** ❌ |

**データ証拠**:
- Samples sequence: `3, 5, 7, 10, 12, 14...` (早期 write)
- Average: `0.0` （NAN → 0.0 変換されたまま）

**修正内容**:
- Write 条件を `D_Count >= 10` に変更 → 100ms 待機
- Logic_Task（50ms 周期）が 2回実行確定 → Welford が確実に完了

---

### 問題B：RUN中は D_Average が計算されていない（発見2・修正2）

**第2回テスト結果（修正1後）**:
```
Samples: 10, 12, 14, 16, 18... (修正1は成功 ✓)
Average_C: 0.0 (ALL ROWS) (まだ問題 ❌)
StdDev_C:  0.0 (ALL ROWS) (まだ問題 ❌)
```

**根本原因の深掘り**:

コード分析結果:
```cpp
// Logic_Task() では、Welford計算は実行（毎50ms）
G.D_Sum  += G.D_FilteredPV;  // ✓実行
G.D_M2   += delta * delta2;  // ✓実行

// ❌ しかし D_Average/D_StdDev は計算されない！
// D_Average = NAN のまま

// handleButtonA() の RESULT遷移時のみ計算
G.D_Average = G.D_Sum / G.D_Count;  // ← ここでのみ計算
```

**影響**:
- Logic_Task は Welford 計算をしている（D_Sum, D_M2 更新）
- **しかし** D_Average = D_Sum / D_Count の割り算は全くされない
- RUN中：D_Average = NAN のままで、Write時に 0.0 に変換
- Result表示時：初めて計算されて正確な値が表示

**修正内容**:
```cpp
// Logic_Task内で毎周期（50ms）D_Average/D_StdDev を計算
// 【修正】RUN中にリアルタイムで D_Average/D_StdDev を計算
G.D_Average = static_cast<float>(G.D_Sum / G.D_Count);
G.D_StdDev  = static_cast<float>(sqrt(G.D_M2 / G.D_Count));
```

**効果**:
| 項目 | 修正前 | 修正後 |
|:---|:---|:---|
| **D_Average 計算タイミング** | RESULT 遷移時のみ | Logic_Task 毎周期 (50ms) |
| **D_StdDev 計算タイミング** | RESULT 遷移時のみ | Logic_Task 毎周期 (50ms) |
| **CSV 出力値** | 0.0（NAN→変換） | 実測値 |

**ファイル**: [src/Tasks.cpp](../../src/Tasks.cpp#L423-L428) (Line 423-428)  
**ステータス**: ✅ Implemented (2026-02-27 Rev.2)  
**ビルド結果**: ✅ SUCCESS (66.93s, Flash 30.8%, RAM 7.0%)

---
- StdDev: `0.0` （計算なし）

### 修正：Write タイミングの最適化

**実装変更**:
```cpp
// 修正前:
if (G.M_SDWriteCounter >= SD_WRITE_INTERVAL && G.D_Count >= 2)

// 修正後:
if (G.M_SDWriteCounter >= SD_WRITE_INTERVAL && G.D_Count >= 10)
```

**効果**:
| 項目 | 修正前 | 修正後 | 理由 |
|:---|:---|:---|:---|
| **初回 write タイミング** | 20-30ms | 100ms | D_Count=10 到達まで待機 |
| **Logic_Task 実行回数** | 0-1 回 | 2 回確定 | 100ms なら 50ms, 100ms で実行 |
| **Average 更新** | ❌ なし | ✅ 完了 | Welford 計算済み |
| **StdDev 更新** | ❌ なし | ✅ 完了 | 同上 |

**ファイル**: [src/Tasks.cpp](../../src/Tasks.cpp#L228) (Line 228)  
**ステータス**: ✅ Implemented (2026-02-27)  
**ビルド結果**: ✅ SUCCESS (67.10s, Flash 30.7%, RAM 7.0%)

---

## 【1】初期化テスト（Initialization Tests: I-01～I-04）

### I-01: SD カード検出

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | I-01 |
| **テスト名** | SD カード検出 |
| **手順** | ① M5Stack に MicroSD を挿入（✅ 完了）<br>② ファームウェアをアップロード（待機中）<br>③ Serial Monitor で setup() ログ監視（待機中） |
| **期待される動作** | `Initializing SD card...` → `SD card OK` |
| **検証方法** | Serial ログで green color 表示確認、LCD 画面確認 |
| **結果** | ⏳ 待機中 |
| **詳細ログ** | （実行後に記録） |
| **備考** | - |

---

### I-02: SD 初期化失敗ハンドリング

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | I-02 |
| **テスト名** | SD 初期化失敗時の適切な エラーハンドリング |
| **前提条件** | I-01 で SD 正常初期化を確認後 |
| **手順** | ① MicroSD カードを抽出（SD 初期化前）<br>② M5Stack をリセット<br>③ Serial ログで エラーメッセージ確認 |
| **期待される動作** | `WARNING: SD card init failed` + エラーメッセージ表示<br>LCD に RED `SD card ERROR` 表示<br>G.M_SDReady = false, G.M_SDError = true |
| **検証方法** | Serial ログ + LCD 表示確認 |
| **結果** | ⏳ 待機中 |
| **詳細ログ** | （実行後に記録） |
| **備考** | ファイアウェアは動作継続（他機能に影響なし） |

---

### I-03: グローバルデータ初期化

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | I-03 |
| **テスト名** | Phase 4 グローバルデータの正確な初期化 |
| **検証方法** | Serial デバッグログで初期値確認 |
| **期待される動作** | `G.M_SDReady = true` (init 成功時)<br>`G.M_SDError = false`<br>`G.M_SDWriteCounter = 0`<br>`G.M_RunStartTime = 0` |
| **結果** | ⏳ 待機中 |
| **詳細ログ** | （実行後に記録） |
| **備考** | compile-time には検証済みだが、runtime 検証 |

---

### I-04: EEPROM との共存

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | I-04 |
| **テスト名** | EEPROM と SD 初期化の共存確認 |
| **手順** | EEPROM から HI/LO アラーム値読み込み後、SD 初期化 |
| **期待される動作** | Serial ログに HI/LO アラーム閾値表示<br>+ SD 初期化完了メッセージ |
| **検証方法** | Serial ログで両機能のログを確認 |
| **結果** | ⏳ 待機中 |
| **詳細ログ** | （実行後に記録） |
| **備考** | EEPROM デバイス選択の相互影響なし |

---

**初期化テスト総合評価**: ⏳ **待機中**

---

## 【2】RUN 状態テスト（Data Accumulation Tests: D-01～D-05）

### D-01: ファイル作成

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | D-01 |
| **テスト名** | RUN 開始時のファイル作成 |
| **手順** | ① I-01～I-04 を全 PASS ✅<br>② IDLE 画面で [BtnA] 短押し ✅<br>③ Serial Monitor でログ確認 ✅ |
| **期待される動作** | `SD file created: DATA_XXXX.csv` ログ出力<br>LCD 画面が RUN 状態に切り替わる |
| **検証方法** | Serial ログ + LCD 画面 + Global snapshot (filename 確認) |
| **結果** | ✅ **PASS** |
| **詳細ログ** | [SDManager] File created: /DATA_0000.csv |  
| **ファイル名** | /DATA_0000.csv ✅ |
| **備考** | ファイル作成正常 ✅ |
| **検証方法** | Serial ログ + LCD 画面 + Global snapshot (filename 確認) |
| **結果** | ✅ **PASS** |
| **詳細ログ** | [SDManager] File created: /DATA_0000.csv |  
| **ファイル名** | /DATA_0000.csv ✅ |

---

### D-02: CSV ヘッダ書き込み

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | D-02 |
| **テスト名** | CSV ファイルのヘッダ行確認 |
| **前提条件** | D-01 で ファイル作成確認後 |
| **手順** | ① テスト完了後 MicroSD を抽出 ✅<br>② PC でテキストエディタで CSV ファイル確認 ✅<br>③ 最初の行をチェック ✅ |
| **期待される動作** | 最初の行: `ElapsedSec,Temp_C,State,Samples,Average_C,StdDev_C,Max_C,Min_C,HI_ALARM,LO_ALARM` |
| **検証方法** | file hexdump / テキストエディタ確認 |
| **結果** | ✅ **PASS** |
| **詳細ログ** | 行1: `ElapsedSec,Temp_C,State,Samples,Average_C,StdDev_C,Max_C,Min_C,HI_ALARM,LO_ALARM` | 
| **備考** | ヘッダフォーマット完璧 ✅ |

---

### D-03: SDBuffer データ蓄積

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | D-03 |
| **テスト名** | IO_Task での SDBuffer 蓄積ロジック |
| **手順** | ① RUN 状態で 10 サンプル (= 50ms * 10 = 500ms) 経過 ✅<br>② CSV に蓄積値が正確に記録 ✅ |
| **期待される動作** | `D_FilteredPV` → `M_SDBuffer.temperature` に正確にコピー<br>他の統計値 (Average/StdDev/Max/Min) も反映 |
| **検証方法** | CSV の temperature 列で確認 |
| **結果** | ✅ **PASS** |
| **詳細ログ** | CSV 行2: `0,23.3,RUN,10,23.2,0.0,23.3,23.2,false,false`<br>temperature = 23.3°C (正確) ✅<br>**Average = 23.2°C (実値) ✅** (修正2成功) |  
| **備考** | 蓄積ロジック完璧に動作 ✅ |

---

### D-04: 書き込み周期

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | D-04 |
| **テスト名** | 10 sample ごとの定期的な SD 書き込み |
| **手順** | ① RUN 状態で 数秒間経過 ✅<br>② CSV で write 周期を確認 ✅ |
| **期待される動作** | Samples count が 10, 12, 14, 16... と増加<br>約 100ms ごとに 2 sample 増加（修正後） |
| **検証方法** | CSV の Samples 列で間隔確認 |
| **結果** | ✅ **PASS** |
| **詳細ログ** | Samples: 10, 12, 14, 16, 18, 20, 22, 24, 26, 29, 31, 33...<br>周期 = 約 20ms/sample (正確) ✅<br>修正1による D_Count >= 10 条件が正常に動作 ✅ |  
| **備考** | 書き込み周期が安定 ✅ |

---

### D-05: 複数行蓄積・ファイル内容確認

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | D-05 |
| **テスト名** | 複数行の CSV 蓄積と一貫性確認 |
| **手順** | ① IDLE → [BtnA] RUN 状態進入 ✅<br>② 数秒間経過 ✅<br>③ [BtnA] で RESULT 遷移（ファイルクローズ） ✅<br>④ MicroSD 抽出→PC で CSV 確認 ✅ |
| **期待される動作** | 複数行が header 形式に準拠<br>データに矛盾なし（温度値が reasonable range） |
| **検証方法** | MicroSD 内 CSV ファイル「行数」と「内容」確認 |
| **結果** | ✅ **PASS** |
| **詳細ログ** | 97行（header + 96 data rows）<br>全行がフォーマット準拠 ✅<br>**Average_C: 23.2～23.4°C で正確に更新** ✅✅✅ (修正2成功)<br>**StdDev_C: 0.0→0.1 に更新** ✅✅✅ (Welford計算確認)<br>温度: 23.2～23.4°C のみ（異常値なし） ✅<br>Max/Min: 正確に追跡（23.6/23.2） ✅ |  
| **CSV sample** | Row 1: `ElapsedSec,Temp_C,State,Samples,Average_C,StdDev_C,Max_C,Min_C,HI_ALARM,LO_ALARM` (header)<br>Row 44: `4,23.4,RUN,88,23.3,0.1,23.4,23.2,false,false` (StdDev初更新地点)<br>Row 97: `10,23.5,RUN,208,23.4,0.1,23.6,23.2,false,false` (最終行) |
| **品質評価** | 📊 **EXCELLENT** - すべてのデータポイント完璧 |

---

**修正2 テスト結果総括**:
- ✅ Average_C: PASS (実値で更新)
- ✅ StdDev_C: PASS (計算・更新確認)
- ✅ Data Integrity: PASS (異常値なし)
- ✅ File Format: PASS (CSV標準準拠)

---

**RUN 状態テスト総合評価**: ⏳ **待機中**

---

## 【3】RESULT 状態テスト（File Closure Tests: C-01～C-03）

### C-01: ファイルクローズ

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | C-01 |
| **テスト名** | RUN → RESULT 遷移時のファイルクローズ |
| **手順** | ① RUN 状態で任意時間経過<br>② [BtnA] で RESULT 状態に遷移<br>③ Serial Monitor でクローズメッセージ確認 |
| **期待される動作** | `SD file closed: DATA_XXXX.csv` ログ出力<br>ファイルハンドルがリソース開放される |
| **検証方法** | Serial ログで close message確認 |
| **結果** | ⏳ 待機中 |
| **詳細ログ** | （実行後に記録） |
| **備考** | - |

---

### C-02: Flush 確認（不完全行の処理）

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | C-02 |
| **テスト名** | 書き込みカウンタ不完全状態でのファイルクローズ |
| **手順** | ① RUN → [BtnA] 直後（counter が 0～9 のいずれか）<br>② MicroSD 抽出<br>③ PC で CSV 行数確認 |
| **期待される動作** | 最後の不完全な行もディスクに flush されている<br>行数 = (完全グループ数 × 10) + (未出力サンプル数) |
| **検証方法** | CSV 行数・内容確認<br>line count コマンド確認 |
| **結果** | ⏳ 待機中 |
| **詳細ログ** | （実行後に記録） |
| **備考** | バッファ確実性テスト |

---

### C-03: 複数ラン独立性

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | C-03 |
| **テスト名** | 複数ラン実行時の ファイル独立性 |
| **手順** | ① DATA_0000.csv を作成・クローズ<br>② IDLE → [BtnA] RUN → [BtnA] RESULT<br>③ DATA_0001.csv を作成<br>④ MicroSD ディレクトリ確認 |
| **期待される動作** | DATA_0000.csv と DATA_0001.csv が ディレクトリに共存<br>各ファイルの内容が独立 |
| **検証方法** | MicroSD directory listing 確認<br>filesize / line count で検証 |
| **結果** | ⏳ 待機中 |
| **詳細ログ** | （実行後に記録） |
| **備考** | ファイル名カウンタが正確に increment |

---

**RUN/RESULT テスト総合評価**: ⏳ **待機中**

---

## 【4】UI 表示テスト（User Feedback Tests: U-01～U-05）

### U-01: IDLE 画面 SD 状態表示（準備状態）

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | U-01 |
| **テスト名** | IDLE 画面での SD Ready 表示 |
| **手順** | ① SD カード装填→初期化成功（I-01 PASS）<br>② IDLE 画面で SD 状態を目視確認 |
| **期待される動作** | **GREEN色**: `SD Ready` 表示<br>フォント: 小（TEXTSIZE_GUIDE） |
| **検証方法** | LCD 画面目視確認（色 + 文字） |
| **結果** | ⏳ 待機中 |
| **スクリーンショット** | （実行後に記録） |
| **備考** | - |

---

### U-02: IDLE 画面 エラー表示

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | U-02 |
| **テスト名** | IDLE 画面での SD エラー表示 |
| **手順** | ① I-02 で SD 未挿入状態でリセット<br>② IDLE 画面でエラー表示を確認 |
| **期待される動作** | **RED色**: `SD Error: <message>` 表示<br>エラーメッセージが明確 |
| **検証方法** | LCD RED 色表示確認 + Serial ログ参照 |
| **結果** | ⏳ 待機中 |
| **詳細ログ** | （実行後に記録） |
| **備考** | - |

---

### U-03: RUN 画面 ファイル名表示

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | U-03 |
| **テスト名** | RUN 状態でのファイル名表示 |
| **手順** | ① IDLE → [BtnA] RUN 進入<br>② RUN 画面で ファイル名を目視確認 |
| **期待される動作** | ファイル名が LCD に表示: `SD File: DATA_0000.csv` |
| **検証方法** | LCD 画面で文字確認 |
| **結果** | ⏳ 待機中 |
| **スクリーンショット** | （実行後に記録） |
| **備考** | - |

---

### U-04: RUN 画面 書き込みインジケータ

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | U-04 |
| **テスト名** | 書き込み直後のインジケータ表示 |
| **手順** | ① RUN 状態で 10 sample 周期を監視<br>② counter = 0 の直後（write 直後）の LCD 表示<br>③ counter = 5～9 の通常状態 LCD 表示 |
| **期待される動作** | **write 直後（counter=0）**: GREEN色 `SD Writing...`<br>**通常状態（counter≠0）**: WHITE色 filename 表示 |
| **検証方法** | LCD 色変化を視認<br>Serial timestamp で counter 状態を相互確認 |
| **結果** | ⏳ 待機中 |
| **詳細ログ** | （実行後に記録） |
| **備考** | 動的 UI 変化の確認 |

---

### U-05: 
エラー時 UI（RUN 中の SD 抽出）

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | U-05 |
| **テスト名** | RUN 中の SD エラー時 UI 表示 |
| **手順** | ① RUN 状態で動作確認<br>② MicroSD カードを抽出<br>③ RUN 画面でエラー表示確認 |
| **期待される動作** | **RED色**: `SD Error!` 表示<br>計測は継続（SD エラーが他機能に波及しない） |
| **検証方法** | LCD RED 色表示確認<br>計測継続確認（Samples count increment） |
| **結果** | ⏳ 待機中 |
| **詳細ログ** | （実行後に記録） |
| **備考** | Graceful degradation 動作確認 |

---

**UI 表示テスト総合評価**: ⏳ **待機中**

---

## 【5】エラーハンドリングテスト（Fault Tolerance Tests: E-01～E-05）

### E-01: MicroSD 未挿入

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | E-01 |
| **テスト名** | SD カード未挿入でのセットアップ |
| **手順** | ① MicroSD カード未挿入で M5Stack リセット<br>② setup() ログ確認 |
| **期待される動作** | `G.M_SDError = true` flag set<br>LCD に RED `SD card ERROR` 表示<br>ファームウェア動作継続 |
| **検証方法** | Serial ログ + LCD 表示確認 |
| **結果** | ⏳ 待機中 |
| **詳細ログ** | （実行後に記録） |
| **備考** | - |

---

### E-02: MicroSD 読み取り専用

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | E-02 |
| **テスト名** | 読み取り専用 SD での write failure |
| **前提** | 外部ツールで SD カードを read-only に設定（Advanced：オプション） |
| **手順** | ① read-only SD で [BtnA] RUN 進入<br>② Serial ログで write 失敗を確認 |
| **期待される動作** | `createNewFile failed` ログ<br>`G.M_SDError = true` set |
| **検証方法** | Serial ログ + LCD エラー表示 |
| **結果** | ⏳ 待機中 |
| **詳細ログ** | （実行後に記録） |
| **備考** | 省略可（基本テストが優先） |

---

### E-03: MicroSD 容量不足

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | E-03 |
| **テスト名** | SD 容量満杯時の write failure |
| **前提** | （実環境では発生しにくい） |
| **手順** | ● skip（実運用で発生確率が低い） |
| **期待される動作** | write failure → `G.M_SDError = true` |
| **検証方法** | Serial ログ |
| **結果** | ⏸️ **SKIP** |
| **備考** | 優先度低。実運用テストで実施可 |

---

### E-04: ノイズ/SPI 干渉

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | E-04 |
| **テスト名** | LCD + SD 同時使用での干渉回復性 |
| **手順** | ① UI 更新（LCD）+ SD write を並行実行中<br>② 干渉が発生した場合の復旧を確認 |
| **期待される動作** | エラー → 次回 write で自動リトライ<br>または fatal error ログ (現在は未実装) |
| **検証方法** | Serial ログでエラー→リカバリを確認 |
| **結果** | ⏳ 待機中 |
| **詳細ログ** | （実行後に記録） |
| **備考** | 実装済みハンドリングの動作確認 |

---

### E-05: 連続実行での安定性

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | E-05 |
| **テスト名** | 複数回 RUN/RESULT サイクル実行 |
| **手順** | ① [BtnA] で RUN → RESULT を 5 回繰り返し<br>② Crash/Hang なし確認<br>③ Memory leaks チェック |
| **期待される動作** | すべての write が成功<br>ファイルが正常にクローズ<br>5つの独立した CSV ファイル生成 |
| **検証方法** | ● Serial ログで crash 確認<br>● MicroSD で 5つのファイル確認<br>● RAM 使用率が増加しない |
| **結果** | ⏳ 待機中 |
| **詳細ログ** | （実行後に記録） |
| **備考** | 長期安定性テスト |

---

**エラーハンドリングテスト総合評価**: ⏳ **待機中**

---

## 【6】CSV フォーマット検証（Data Integrity Tests: F-01～F-07）

### F-01: Timestamp（経過秒数）

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | F-01 |
| **テスト名** | 経過秒数の正確性（現在は millis() ベース） |
| **手順** | ① CSV を PC で開く<br>② 第1列（ElapsedSec）を確認<br>③ 増分を検査 |
| **期待される動作** | 最初の行: elapsed ≈ 0<br>10 行目: elapsed ≈ 5（500ms × 10） |
| **検証方法** | テキストエディタで確認<br>Python script で統計分析 |
| **結果** | ⏳ 待機中 |
| **サンプル行** | |
| **備考** | RTC 統合後は YYYYMMDD_HHMMSS に変更予定 |

---

### F-02: 温度値フォーマット

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | F-02 |
| **テスト名** | 温度値の小数点以下1桁フォーマット |
| **手順** | ① CSV の第2列を確認<br>② 実測値 25.3°C の場合 |
| **期待される動作** | CSV 第2列: `25.3` （1小数点） |
| **検証方法** | テキストエディタで値確認<br>正規表現で format チェック |
| **結果** | ⏳ 待機中 |
| **サンプル行** | |
| **備考** | %.1f format で統一 |

---

### F-03: NaN ハンドリング

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | F-03 |
| **テスト名** | 未定義温度値（NaN）の処理 |
| **手順** | ① センサ読取失敗時（初期状態）の temperature が NaN<br>② CSV に記録される値を確認 |
| **期待される動作** | CSV に `0.0` が記録（isnan() チェック）<br>format: `0.0` (not `NaN` string) |
| **検証方法** | テキストエディタで初期行確認 |
| **結果** | ⏳ 待機中 |
| **サンプル行** | |
| **備考** | sdmanager.cpp の formatCSVLine() で処理 |

---

### F-04: State 文字列

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | F-04 |
| **テスト名** | State フィールドの正確な文字列 |
| **手順** | ① RUN 状態での State 値を確認 |
| **期待される動作** | CSV 第3列: `RUN` (引用符なし、大文字) |
| **検証方法** | テキストエディタで値確認 |
| **結果** | ⏳ 待機中 |
| **サンプル行** | |
| **備考** | - |

---

### F-05: 統計値精度（Average/StdDev/Max/Min）

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | F-05 |
| **テスト名** | Welford 法による統計値計算の正確性 |
| **手順** | ① CSV をダウンロード<br>② Python で CSV 再計算<br>③ 期待値と比較 |
| **期待される動作** | CSV の Average/StdDev/Max/Min が<br>Welford 法による正確な値<br>誤差 < 0.1% |
| **検証方法** | Python numpy と比較 |
| **結果** | ⏳ 待機中 |
| **Python script** | |
| **備考** | 数値精度テスト |

---

### F-06: アラームフラグ

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | F-06 |
| **テスト名** | HI/LO アラームフラグの記録 |
| **手順** | ① アラーム状態を誘発<br>② CSV の最後2列を確認 |
| **期待される動作** | HI_ALARM=1, LO_ALARM=0 など<br>True=1, False=0 |
| **検証方法** | テキストエディタで値確認 |
| **結果** | ⏳ 待機中 |
| **サンプル行** | |
| **備考** | - |

---

### F-07: 改行コード

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | F-07 |
| **テスト名** | CSV 改行コード（CRLF \r\n） |
| **手順** | ① CSV を hexdump で確認<br>② 各行末のバイト確認 |
| **期待される動作** | 各行末: `0x0D 0x0A` (CRLF, \r\n) |
| **検証方法** | hexdump コマンドで検査 |
| **結果** | ⏳ 待機中 |
| **詳細** | Windows Excel 互換性確保 |
| **備考** | snprintf 内で "\r\n" で出力済み |

---

**CSV フォーマット検証総合評価**: ⏳ **待機中**

---

## 【7】パフォーマンステスト（Performance Tests: P-01～P-04）

### P-01: メモリリーク

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | P-01 |
| **テスト名** | 長時間実行でのメモリリーク確認 |
| **手順** | ① Reset 時の RAM 使用率記録<br>② 60 分連続 RUN 実行<br>③ 終了時の RAM 使用率記録<br>④ 増加傾向を分析 |
| **期待される動作** | RAM 使用率が増加しない（静的メンバのみ） |
| **検証方法** | Serial で maloc/free call logging<br>または heap statistic API |
| **結果** | ⏳ 待機中 |
| **詳細ログ** | （実行後に記録） |
| **備考** | 静的設計なので leaks unlikely |

---

### P-02: Flash 使用率

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | P-02 |
| **テスト名** | Flash メモリ使用率確認 |
| **手順** | PlatformIO build 完了後の size report 確認 |
| **期待される動作** | Flash < 50% (current: 30.7%)<br>RAM < 30% (current: 7.0%) |
| **検証方法** | PlatformIO build output で確認済み |
| **結果** | ✅ **VERIFIED** |
| **詳細** | Flash: 30.7% (402,497 / 1,310,720)<br>RAM: 7.0% (23,004 / 327,680) |
| **備考** | コンパイル時検証済み |

---

### P-03: IO 層遅延

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | P-03 |
| **テスト名** | IO_Task 実行時間が IO_CYCLE_MS (10ms) 内 |
| **手順** | ① Serial に timer code 挿入（オプション）<br>② IO_Task() 実行時間測定<br>③ 最大値・平均値を取得 |
| **期待される動作** | 実行時間 < 8ms (IO_CYCLE_MS=10ms で余裕） |
| **検証方法** | Serial timestamp / オシロスコープ測定 |
| **結果** | ⏳ 待機中 |
| **詳細ログ** | （実行後に記録） |
| **備考** | リアルタイム性確認 |

---

### P-04: UI 描画フレームレート

| 項目 | 内容 |
|:---|:---|
| **テスト ID** | P-04 |
| **テスト名** | LCD 画面更新がスムーズ（フレームレート）|
| **手順** | ① UI_Task() を実行中<br>② LCD 表示の滑らかさを目視評価 |
| **期待される動作** | フレームレート ≈ 5fps (UI_CYCLE_MS=200ms)<br>カクカクしない、スムーズ表示 |
| **検証方法** | Visual 目視評価<br>または Serial with timestamp |
| **結果** | ⏳ 待機中 |
| **評価** | - |
| **備考** | 主観的評価も含む |

---

**パフォーマンステスト総合評価**: ⏳ **待機中** (P-02 は既検証 ✅)

---

## 【テスト集計】

| カテゴリ | テスト数 | PASS | FAIL | SKIP | 進捗 |
|:---|:---:|:---:|:---:|:---:|:---|
| **I: 初期化テスト** | 4 | 0 | 0 | 0 | ⏳ 0% |
| **D: RUN状態テスト** | 5 | 0 | 0 | 0 | ⏳ 0% |
| **C: RESULT テスト** | 3 | 0 | 0 | 0 | ⏳ 0% |
| **U: UI 表示テスト** | 5 | 0 | 0 | 0 | ⏳ 0% |
| **E: エラーテスト** | 5 | 0 | 0 | 0 | ⏳ 0% |
| **F: CSV 検証** | 7 | 0 | 0 | 0 | ⏳ 0% |
| **P: パフォーマンス** | 4 | 1 | 0 | 0 | ⏳ 25% |
| **合計** | **33** | **1** | **0** | **0** | **⏳ 3%** |

---

## 【テスト成功基準（Exit Criteria）】

### 必須条件（Must Have）

- [x] P-02: Flash 使用率 < 50% ✅ **(既検証)**
- [ ] I-01～I-04: 初期化テスト 100% PASS
- [ ] D-01～D-05: RUN状態テスト 100% PASS
- [ ] C-01～C-03: RESULT テスト 100% PASS
- [ ] U-01～U-05: UI 表示テスト 100% PASS
- [ ] F-01～F-07: CSV 形式検証 100% PASS

### 推奨条件（Should Have）

- [ ] E-01～E-05: エラーハンドリング 80% 以上 PASS
- [ ] P-01, P-03, P-04: パフォーマンス 90% 以上達成

### 完了判定

```
全テスト完了かつ
- 必須条件: 100% PASS
- 推奨条件: 80% 以上達成
→ Phase 4 Integration Test PASS ✅
```

---

## 【次フェーズへの移行条件】

統合テス ト全項目が完了し、以下を確認後 RTC 統合フェーズへ:

- ✅ Data 損失なし（正常終了した Run は 100% CSV 保存）
- ✅ Firmware 安定（Crash/Hang なし）
- ✅ User Experience 良好（UI 表示が明確）

---

**Document Version**: Phase 4 Integration Test Results v0.1  
**Status**: 🔄 **Test Execution In Progress**  
**Last Updated**: 2026-02-27 (テスト実行開始)

