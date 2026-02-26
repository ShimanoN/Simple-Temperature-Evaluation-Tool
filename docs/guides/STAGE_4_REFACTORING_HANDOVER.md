# Stage 4 リファクタリング 引き継ぎドキュメント

**作成日**: 2026-02-26（Session 3）  
**対象**: Session 4 以降のため  
**ステータス**: ハンドオーバー準備完了  

---

## 📋 Session 3 完了内容

### **完了したタスク（2/5）**

| # | タスク | 内容 | 状態 | ビルド |
|---|--------|------|------|--------|
| **1** | Magic Number定数化 | Global.h に10個の定数追加 | ✅ 完了 | SUCCESS (55.68s) |
| **2** | ボタン処理関数化 | handleButtonA/B/C() 実装 | ✅ 完了 | SUCCESS (62.06s) |
| **3** | UI描画分割 | renderXXX() 関数整理 | ⏳ 準備完了 | - |
| **4** | EEPROM独立化 | EEPROMManager 強化 | ⏳ 設計済 | - |
| **5** | ドキュメント充実 | 詳細コメント追記 | ⏳ 設計済 | - |

### **ビルド状態**
```
Environment    Status     Duration
m5stack        SUCCESS    00:01:02.064
Flash: 30.7% used (402237 / 1310720 bytes)
RAM:    7.0% used (23004 / 327680 bytes)
```

---

## 🎯 Task 3: UI描画分割（詳細実装ガイド）

### **目的**
UI_Task() の 600+ 行の描画ロジックを、**状態別の関数に分割**して保守性向上

### **現在の構造**（Tasks.cpp 行 380-600）

```cpp
void UI_Task() {
  static State prevState = State::IDLE;
  
  if (G.M_CurrentState != prevState) {
    M5.Lcd.fillScreen(BLACK);  // 状態変化時にクリア
    prevState = G.M_CurrentState;
  }
  
  // 状態ごとの描画処理が巨大な switch/if で記述
  switch (G.M_CurrentState) {
    case State::IDLE:
      // ~60 行の描画処理
    case State::RUN:
      // ~40 行の描画処理
    case State::RESULT:
      // ~80 行の描画処理（ページング対応）
    case State::ALARM_SETTING:
      // ~60 行の描画処理
  }
}
```

### **対象となるコード位置**

| 関数 | 行番号 | 行数 |
|------|--------|------|
| renderIDLE() | 380-430 | 50 |
| renderRUN() | 431-468 | 37 |
| renderRESULT() | 469-560 | 91 |
| renderALARM_SETTING() | 361-530 | 68 |

### **分割後の構造（実装予定）**

```cpp
// ========== UI Layer (200ms周期) =================================================

/**
 * @brief IDLE状態の描画
 * 
 * @details
 * 温度リアルタイム表示 + 設定値（デバッグ用）
 * アラーム中は文字色を RED に変更
 */
void renderIDLE() {
  // IDLE専用の描画ロジック（現在 ~50行）
}

/**
 * @brief RUN状態の描画（計測中）
 * 
 * @details
 * リアルタイム温度 + サンプル数を表示
 */
void renderRUN() {
  // RUN専用の描画ロジック（現在 ~37行）
}

/**
 * @brief RESULT状態の描画（ページング対応）
 * 
 * @details
 * Page 0: 最新値 + 平均値
 * Page 1: 標準偏差 + 範囲 + Max/Min
 * ボタンBでページング
 */
void renderRESULT() {
  // RESULT専用の描画ロジック（現在 ~91行）
}

/**
 * @brief ALARM_SETTING状態の描画
 * 
 * @details
 * HI_ALARM 設定画面 or LO_ALARM 設定画面
 * BtnB: +5°C, BtnC: -5°C
 */
void renderALARM_SETTING() {
  // ALARM_SETTING専用の描画ロジック（現在 ~68行）
}

// 統合メイン関数
void UI_Task() {
  static State prevState = State::IDLE;
  
  // 状態遷移時の画面クリア
  if (G.M_CurrentState != prevState) {
    M5.Lcd.fillScreen(BLACK);
    prevState = G.M_CurrentState;
  }
  
  // 状態ごとの描画関数を呼び出し（シンプルなディスパッチ）
  switch (G.M_CurrentState) {
    case State::IDLE:           renderIDLE(); break;
    case State::RUN:            renderRUN(); break;
    case State::RESULT:         renderRESULT(); break;
    case State::ALARM_SETTING:  renderALARM_SETTING(); break;
  }
}
```

### **実装ステップ**

#### **ステップ 1: 関数宣言を Tasks.h に追加**

```cpp
// Tasks.h に追加
void renderIDLE();
void renderRUN();
void renderRESULT();
void renderALARM_SETTING();
void UI_Task();
```

#### **ステップ 2: renderXXX() 各関数を実装**

**2-1: renderIDLE()** （現在 Tasks.cpp 行 380-430 のコード）
- M5.Lcd.setTextSize(UI::TEXTSIZE_VALUE);
- 状態表示: "STATE: IDLE"
- 温度表示（アラーム中は RED）
- 設定値表示（デバッグ用）
- ボタンガイド

**2-2: renderRUN()** （現在 Tasks.cpp 行 431-468 のコード）
- 状態表示: "STATE: RUN"
- リアルタイム温度
- サンプル数
- ボタンガイド: [BtnA] Stop / Reset

**2-3: renderRESULT()** （現在 Tasks.cpp 行 469-560 のコード）
- **Page 0**: Temp + Average
- **Page 1**: StdDev + Range / Max + Min
- ボタンガイド: [BtnA] Reset [BtnB] Next/Prev

**2-4: renderALARM_SETTING()** （現在 Tasks.cpp 行 361-530 のコード）
- HI_ALARM 設定 or LO_ALARM 設定
- 現在値表示
- 調整ガイド: [BtnB] +5C [BtnC] -5C
- [BtnA] Next or Save & Exit

#### **ステップ 3: UI_Task() を簡潔にリファクタリング**

```cpp
void UI_Task() {
  static State prevState = State::IDLE;
  
  if (G.M_CurrentState != prevState) {
    M5.Lcd.fillScreen(BLACK);
    prevState = G.M_CurrentState;
  }
  
  // 単純なディスパッチパターン
  switch (G.M_CurrentState) {
    case State::IDLE:           renderIDLE(); break;
    case State::RUN:            renderRUN(); break;
    case State::RESULT:         renderRESULT(); break;
    case State::ALARM_SETTING:  renderALARM_SETTING(); break;
  }
}
```

#### **ステップ 4: ビルド & 検証**

```bash
platformio run -e m5stack
# 期待: SUCCESS （62秒前後）
```

### **メリット**
- ✅ 各関数が単一責任（一状態のみ担当）
- ✅ UI_Task() が直感的（ディスパッチパターン明確）
- ✅ テスト容易（各renderXXX()を個別テスト可能）
- ✅ 変更時の影響範囲を限定

---

## 🔧 Task 4: EEPROM独立化（設計概要）

### **現在の問題**
- EEPROMManager は既に独立化されているが、**Tasks.cpp で直接呼び出してる**
- EEPROMManager の公開インターフェースが広すぎる

### **改善案**

```cpp
// Tasks.cpp から呼び出すのは、最小限のAPI のみに限定

// ✅ 推奨インターフェース
void EEPROM_LoadToGlobal();          // 系統化・初期化
void EEPROM_SaveFromGlobal();        // EEPROM保存
bool EEPROM_ValidateSettings();      // 検証

// ❌ 避けるべき直接呼び出し
EEPROMManager::readSettings();       // privateに
EEPROMManager::writeSettings();      // privateに
```

### **実装ステップ**
1. EEPROMManager.h で、public / private インターフェース整理
2. Tasks.cpp で呼び出す部分を最小化
3. wrapperfunctions (EEPROM_LoadToGlobal など) をシングルトンパターンで管理

---

## 📝 Task 5: ドキュメント充実（計画）

### **対象ファイル**

| ファイル | 追記内容 | 行数 |
|---------|---------|------|
| Tasks.cpp | renderXXX() の JSDoc | +150 |
| Tasks.h | 関数宣言と説明 | +30 |
| Global.h | UI定数説明コメント | +20 |
| EEPROMManager.cpp | インターフェース整理コメント | +40 |

### **優先度**
1. ⭐⭐⭐ renderXXX() 関数の詳細JSDoc（Task 3 完了直後）
2. ⭐⭐⭐ EEPROM_*() のwrapper関数説明（Task 4 直後）
3. ⭐⭐ UI/ namespace の座標定数説明
4. ⭐ 全体の統合説明文書

---

## 🚀 次セッション実装チェックリスト

### **Session 4 開始時の確認事項**

- [ ] コンテキストが適切に引き継がれているか確認
- [ ] 最新ビルド状態: `platformio run -e m5stack` で SUCCESS
- [ ] Git status の確認: コミットできるか？

### **Task 3 実装手順（コピペ用）**

```bash
# 1. 現在のビルド状態確認
cd "c:\gemini\Simple Temperature Evaluation Tool"
C:\.platformio\penv\Scripts\platformio.exe run -e m5stack

# 2. Tasks.cpp の行 380-560 から renderXXX() 関数を抽出
# （このドキュメントの「対象となるコード位置」参照）

# 3. 各関数を実装（実装ガイド参照）

# 4. ビルド確認
C:\.platformio\penv\Scripts\platformio.exe run -e m5stack

# 5. Git コミット
git add src/Tasks.cpp src/Tasks.h
git commit -m "Refactor: UI_Task decomposed into renderXXX() functions (Task 3)"
```

### **予想所要時間**
- Task 3: 45 分（コード抽出 + 関数化 + テスト）
- Task 4: 30 分（インターフェース整理）
- Task 5: 60 分（全体ドキュメント・コメント追記）

**合計**: 約 **2.5 時間**（高品質優先で段階的実行推奨）

### **品質チェックポイント**
- [ ] 各 renderXXX() 関数の責任範囲が明確 (単一責任原則)
- [ ] JSDoc コメントが完全（@brief, @details, @see）
- [ ] ビルド成功 かつ メモリ使用率が許容範囲（Flash < 40%, RAM < 10%）
- [ ] 既存機能の動作確認（ボタン反応、画面遷移、表示）

---

## 📁 参考ドキュメント

- **Stage 2-3 完了内容**: STAGE2_3_COMPLETION_SUMMARY.md
- **コード参考**: CODE_REFERENCE.md
- **ハードウェア検証**: HARDWARE_VALIDATION.md
- **パフォーマンス測定**: PERFORMANCE.md
- **トラブルシューティング**: TROUBLESHOOTING.md

---

## 💡 継続のコツ

**高品質で段階的に進めるために**:
1. 一度に全タスクをしない → 1タスク = 1ビルド検証
2. 各関数の役割を明確に説明するコメント記述
3. 変更前にビルド成功を確認、変更後も常にビルド検証
4. 新しい機能追加ではなく **整理と削除** の思想を優先

**品質を保つ約束**:
> 「時間をかけて高品質優先で考えろ。急ぐ必要なし」
> 
> → 各ステップで丁寧にテスト → 複数回ビルド → 不具合は早期発見

---

**次セッション予告**: 
Session 4 で Task 3-5 完了後、**Phase 4（ハードウェア拡張・Web UI）の基盤設計** に進みます。

