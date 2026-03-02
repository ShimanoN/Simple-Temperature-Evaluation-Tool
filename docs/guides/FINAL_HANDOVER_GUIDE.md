# 最終ハンドオーバーガイド（高卒1年目向け）

**作成日**: 2026年3月2日  
**対象**: 新人開発者（保守・運用・拡張担当）  
**前任者**: Shimano  
**引き継ぎ期間**: 2026年3月～4月初旬（1ヶ月）

---

## 📋 このガイドの使い方

このドキュメントは、前任者（Shimano）が転籍する前に、**このプロジェクトのすべてを理解し、独立運用・保守できるようにするため**に作成されています。

**1ヶ月の学習ロードマップ：**
1. **Week 1**: プロジェクト全体理解 + 環境構築
2. **Week 2**: コード・アーキテクチャ理解
3. **Week 3**: 簡単な修正・テスト実施
4. **Week 4**: 独立運用・引き継ぎ完了確認

---

## 🎯 Part 1: プロジェクト全体理解（30分）

### このプロジェクトは何か？

**製品**: K型熱電対で温度を計測し、CSV形式でデータを記録するツール

**使用場面**: 
- ヒートガンから流れるエアの温度測定
- チャンバー内のエア温度監視
- 金型・チャンバーの加熱/冷却過程の記録

**例:**
```
ヒートガン → チャンバー内 → ヒートエア
  ↓
M5StackとK型熱電対で温度を読取
  ↓
画面に表示 + CSV保存（MicroSD）
```

### 構成（誰が何を担当？）

| 部品・役割 | 詳細 |
|:---|:---|
| **M5Stack Core2** | 脳（計算・表示・制御）。液晶画面に温度表示。USB給電。 |
| **MAX31855** | 温度センサ（K型熱電対の電圧をデジタル値に変換）。I2Cで接続。 |
| **K型熱電対** | 実物の温度に触れて電圧発生。MAX31855に入力。 |
| **MicroSD** | データ保存（CSV形式）。計測結果をここに記録。 |
| **EEPROM** | アラーム値（高温/低温）を記憶。電源オフ後も保持。 |

### 状態図（最重要！）

```
┌─────────────┐
│    IDLE     │  待機状態
│ 現在温度表示 │
└────┬────────┘
     │ [BtnA]短押し
     ↓
┌─────────────┐
│    RUN      │  計測中
│ リアルタイム│─→ 平均値・標準偏差を計算
│  更新中      │
└────┬────────┘
     │ [BtnA]短押し
     ↓
┌──────────────────────────────┐
│   RESULT                     │  計測終了
│ 結果表示＋CSV自動保存（SD）    │
└────┬─────────────────────────┘
     │ [BtnA]短押し
     └─→ IDLE に戻る
```

---

## 🔧 Part 2: 環境構築（1時間）

### 必要なもの
- [ ] Windows 10/11 PC
- [ ] VS Code（既インストール）
- [ ] PlatformIO IDE for VSCode（既インストール）
- [ ] M5Stack Core2 + USB-C ケーブル
- [ ] MicroSD カード（初期化済み）
- [ ] MAX31855 変換ボード
- [ ] K型熱電対

### セットアップ手順

#### Step 1: リポジトリをクローン
```bash
cd c:\
git clone https://github.com/your-org/Simple-Temperature-Evaluation-Tool.git
cd Simple\ Temperature\ Evaluation\ Tool
```

#### Step 2: ビルド確認
```bash
platformio run -e m5stack
```
**期待する結果**: `SUCCESS` と表示される（13秒程度）

#### Step 3: M5Stackをアップロード
```bash
# M5StackをUSB接続
platformio run --target upload --environment m5stack
```
**期待する結果**: `SUCCESS` + LCD画面に「SD Ready」(GREEN) が表示される

#### Step 4: 動作確認
- M5Stackの液晶に IDLE画面が見える
- [BtnA]（本体上部）を押して、状態遷移が起こる
- 5秒待機してから、RESULT に遷移してCSV保存を確認

**うまくいかない場合**: [トラブルシューティング](../troubleshooting/TROUBLESHOOTING.md) 参照

---

## 📂 Part 3: コード構造（1時間）

### ディレクトリ構成
```
src/
├── main.cpp              ← エントリーポイント・初期化
├── Tasks.cpp/h           ← 定期タスク（10ms毎のIO, 50ms毎のLogic, 200ms毎のUI）
├── SDManager.cpp/h       ← CSV書き込み・ファイル操作
├── DisplayManager.cpp/h  ← LCD表示更新
├── EEPROMManager.cpp/h   ← アラーム値の読み書き
├── IOController.cpp/h    ← ボタン・GPIO制御
└── MeasurementCore.cpp/h ← 温度計測・統計計算（Welford法）
```

### 重要な変数・ファイル

#### グローバル構造体 `G` (Global.h)
```cpp
struct {
  // 計測データ
  float D_FilteredPV;      // 現在の温度（℃）
  float D_Average;         // 平均値
  float D_StdDev;          // 標準偏差
  int D_Count;             // サンプル数
  float D_Max, D_Min;      // 最大/最小値
  
  // SD操作
  char M_FileName[32];     // "DATA_0000.csv"等
  bool M_SDReady;          // SD準備OK？
  bool M_SDError;          // エラー発生？
  
  // アラーム
  float HI_ALARM;          // 高温閾値（EEPROM保持）
  float LO_ALARM;          // 低温閾値（EEPROM保持）
  bool HI_AlarmTriggered;  // 超過フラグ
  bool LO_AlarmTriggered;  // 低下フラグ
} G;
```

#### 状態遷移の実装
```cpp
// main.cpp より
enum State { IDLE, RUN, RESULT };
State currentState = IDLE;  // ← この値で画面表示が変わる

// Tasks.cpp: handleButtonA() で状態遷移
if (G.M_CurrentState == IDLE) {
  // → RUN に遷移、CSV新規作成
  G.M_CurrentState = RUN;
  SDManager::createNewFile();
}
```

### フロー図（計測中のデータの流れ）

```
┌─────────────────────────────────┐
│  IO_Task (10ms周期)             │
│ - MAX31855から温度読取           │
│ - SDBuffer に蓄積（10sample毎）  │
│ - SD カード書き込み判定          │
└──────────┬──────────────────────┘
           ↓
┌──────────────────────────────────┐
│  Logic_Task (50ms周期)           │
│ - Welford法で統計計算            │
│ - Average, StdDev 更新          │
│ - アラーム判定                    │
└──────────┬───────────────────────┘
           ↓
┌──────────────────────────────────┐
│  UI_Task (200ms周期)             │
│ - LCD画面更新                    │
│ - 現在温度・平均値表示           │
└──────────────────────────────────┘
```

---

## 🧠 Part 4: キーポイント（1時間）

### ① CSV写込みの仕組み

```cpp
// SD書き込みは効率化のため、10個サンプルごとに行う
// (毎回書き込むと遅い)

// IO_Task内
if (G.D_Count % 10 == 0) {  // 10の倍数
  SDManager::writeData();   // CSV 1行書き込み
}
```

### ② Welford法（統計計算）

```cpp
// 平均と標準偏差を「リアルタイム」で計算
// （すべてのデータを保持しない = メモリ効率的）

// Logic_Task内で毎周期実行
float delta = sample - G.D_Average;
float delta2 = sample - (G.D_Average + delta / G.D_Count);

G.D_Sum += delta / G.D_Count;           // 平均更新
G.D_M2 += delta * delta2;                // 分散更新
G.D_StdDev = sqrt(G.D_M2 / G.D_Count); // 標準偏差
```

### ③ アラーム機能の流れ

```
新人重要メモ: アラーム値はEEPROMに保存されます
  HI_ALARM = 30.0°C (例)
  LO_ALARM = 15.0°C (例)

RUN中に、計測値がこれを超えると：
  → HI_AlarmTriggered = true
  → LCD に RED で警告表示
  → CSV に 1 が記録
```

### ④ よく修正するポイント

| 項目 | ファイル | 行番号（参考） | 修正例 |
|:---|:---|:---|:---|
| 計測周期を変更 | `src/Tasks.cpp` | L50 `IO_CYCLE_MS=10` | 20msに経ウ → `=20` |
| CSV書き込み頻度変更 | `src/Tasks.cpp` | L228 `D_Count >= 10` | 20個毎 → `>= 20` |
| アラーム閾値デフォルト | `src/EEPROMManager.cpp` | L80 | `HI = 35.0` に変更 |
| UI更新周期 | `src/Tasks.cpp` | L100 `UI_CYCLE_MS=200` | 500msに遅く → `=500` |
| LCD 表示フォント | `src/DisplayManager.cpp` | L180 | サイズ変更 |

---

## 📝 Part 5: よくある質問・トラブル（30分）

### Q1: ビルドに失敗する
**症状**: `Compiling failed` / `undefined reference`  
**対応**:
```bash
# 1. クリーンビルド
platformio run --target clean -e m5stack

# 2. ライブラリ更新
platformio lib update

# 3. 再ビルド
platformio run -e m5stack
```

### Q2: M5Stackに書き込めない / 認識されない
**症状**: `Com port not found`  
**対応**:
```bash
# デバイス一覧確認
platformio device list

# ドライバ再インストール（Windows）
# → M5Stack公式サイトからUSBドライバをダウンロード
```

### Q3: SD カードにデータが保存されない
**症状**: CSV ファイルが作成されない  
**対応**:
1. MicroSD を FAT32 でフォーマット
2. M5Stack で IDLE 状態で「SD Ready」(GREEN) が表示されることを確認
3. SD 初期化失敗なら [トラブルシューティング](../troubleshooting/TROUBLESHOOTING.md) 参照

### Q4: 温度値がおかしい（999℃ 等が出る）
**症状**: MAX31855が応答していない / 接続不良  
**対応**:
1. I2C接続確認（配線を再度確認）
2. MAX31855ボード上のLED点灯確認
3. Serial Monitor で `[IO_Task] readCelsius returned: -999` のログを確認

### Q5: CSV に「StdDev_C: 0.0」のまま変わらない
**症状**: 標準偏差が常に0  
**対応**: 温度値がほぼ一定 = 正常動作です。温度変化がない場合は 0 になります。

---

## 🔄 Part 6: 修正・テスト方法（1時間）

### 修正フロー

**例: IO周期を 10ms → 20ms に変更したい場合**

```
1. src/Tasks.cpp を開く
   L50 付近: const int IO_CYCLE_MS = 10;
   
2. 値を変更
   const int IO_CYCLE_MS = 20;
   
3. ビルド
   platformio run -e m5stack
   
4. アップロード
   platformio run --target upload -e m5stack
   
5. テスト
   M5Stack起動 → Serial Monitor で周期確認
   (ログに [IO_Task] entry (millis=...) が出る)
```

### テスト手順

#### テストケース 1: ビルド成功確認
```bash
platformio run -e m5stack
# SUCCESS が出る → ✅
```

#### テストケース 2: 初期化確認
```
M5Stack起動直後
LCD に「SD Ready」(GREEN) が見える
  + Serial に「SD card OK」ログ
  → ✅
```

#### テストケース 3: 計測ラン確認
```
1. IDLE 画面を確認
2. [BtnA]短押し → RUN （LCD が RUN 表示に変わる）
3. 5秒待つ
4. [BtnA]短押し → RESULT （LCD が RESULT 表示 + "Closing file" ログ）
5. MicroSD を PC に接続 → CSV が増えている
   → ✅
```

---

## 📚 Part 7: 次のステップ（今後の学習）

### Week 1 チェックリスト（環境構築）
- [ ] VS Code + PlatformIO が起動できる
- [ ] ビルド成功（13秒程度）
- [ ] M5Stack へアップロード成功
- [ ] LCD に「SD Ready」が表示される
- [ ] [BtnA] で IDLE → RUN → RESULT → IDLE 遷移ができる

### Week 2 チェックリスト（コード理解）
- [ ] ファイル構成を把握（src/ に 6つのモジュール）
- [ ] グローバル構造体 `G` の主要メンバを理解
- [ ] Tasks.cpp の3つのタスク（IO, Logic, UI）の役割を理解
- [ ] 状態遷移（IDLE → RUN → RESULT）を理解
- [ ] Serial Monitor でログを見て、流れを追える

### Week 3 チェックリスト（簡単な修正）
- [ ] IO周期を変更して、ビルド・アップロード・テストできる
- [ ] CSV書き込み頻度を変更できる
- [ ] アラーム値をコード内で変更できる
- [ ] デバッグログを追加・削除できる

### Week 4 チェックリスト（独立運用）
- [ ] 前任者なしで、新しい計測ラン実行できる
- [ ] CSV を分析・データ確認できる
- [ ] 簡単なバグ修正ができる
- [ ] このガイドをもとに他者に説明できる

---

## 📞 Part 8: 前任者への連絡先・質問方法

### 転籍前の連絡期間

**引き継ぎ期間**: 2026年3月1日 ～ 3月末  
**質問方法**: 
- 緊急: Slack/メール
- 通常: GitHub Issues に記録 (後日回答)

### よくある質問フォーマット
```
【質問】コード修正時の確認

修正内容: IO周期を20msに変更
ファイル: src/Tasks.cpp L50
変更前: const int IO_CYCLE_MS = 10;
変更後: const int IO_CYCLE_MS = 20;

ビルド: SUCCESS ✅
テスト結果: RUN中にログが2倍遅くなった ✅

→ 修正成功かどうか確認願います
```

---

## 📋 引き継ぎ最終チェック（月末）

以下が全て「✅」になれば、引き継ぎ完了です：

- [ ] 環境構築できる（VS Code + PlatformIO なし状態から復帰可能）
- [ ] ビルド・アップロードができる
- [ ] 計測ラン（IDLE → RUN → RESULT）ができる
- [ ] CSV を読んで、データを分析できる
- [ ] リポジトリを git clone → 修正 → commit → push できる
- [ ] このガイド内容を他者に説明できる
- [ ] 不具合があれば、[トラブルシューティング](../troubleshooting/TROUBLESHOOTING.md) で自力解決できる
- [ ] 簡単な機能追加（アラーム値変更など）ができる

---

## 📖 関連ドキュメント

| ドキュメント | 用途 |
|:---|:---|
| [セットアップガイド](./QUICKSTART_SESSION4.md) | 初回環境構築 |
| [実装仕様書](../code/IMPLEMENTATION_GUIDE.md) | コード詳細 |
| [ハードウェア仕様](../specs/detailed_spec_hw.md) | 配線・GPIO |
| [トラブルシューティング](../troubleshooting/TROUBLESHOOTING.md) | 問題対応 |
| [テスト結果](../future/PHASE4_INTEGRATION_TEST_RESULTS.md) | 品質保証情報 |

---

## 📝 このガイド内での質問・フィードバック

このガイドをもとに学習を進めるなかで：
- 「ここがわかりにくい」
- 「このステップが難しい」
- 「追加で説明が欲しい」

という点があれば、GitHub Issues で報告してください。**次の引き継ぎの際に改善します。**

---

**作成者**: Shimano  
**最終更新**: 2026年3月2日  
**バージョン**: v1.0.0
