# Session 4 クイックスタートガイド

**前回完了**: Task 1-2 (Magic Number定数化 + ボタン処理関数分割)  
**今回取組**: Task 3-5 (UI描画分割 → EEPROM分離 → ドキュメント化)  

---

## Ⅰ 30秒でスタート

### **ビルド確認**
```bash
cd "c:\gemini\Simple Temperature Evaluation Tool"
C:\.platformio\penv\Scripts\platformio.exe run -e m5stack
# 直前: SUCCESS (56.70s | Flash 30.7% | RAM 7.0%)
```

### **現在の進捗**
```
Stage 2-B: Code Quality Refactoring
✅ Task 1: Magic Number定数化 (Global.h 40行追加)
✅ Task 2: Button処理関数分割 (handleButtonA/B/C + 4関数)
⏳ Task 3: UI描画分割 (renderXXX()関数分割) ← 次のここ
⏳ Task 4: EEPROM分離 (インターフェース整理)
⏳ Task 5: ドキュメント化 (JSDoc + 詳細コメント)
```

### **関連ドキュメント**
- 📖 **詳細実装ガイド**: STAGE_4_REFACTORING_HANDOVER.md ← 必読
- 📚 **コード参照**: CODE_REFERENCE.md
- 🎯 **全体仕様**: detailed_spec_sw.md

---

## Ⅱ Task 3 実装サマリー（分割の要点）

### **何をするのか**
Tasks.cpp の 600+ 行の UI_Task() 関数を、**renderIDLE()**, **renderRUN()**, **renderRESULT()**, **renderALARM_SETTING()** 4つの関数に分割。

### **実装の流れ**

```
Step 1: 現在の UI_Task() コード位置を確認
        ↓ Tasks.cpp line ~380-600

Step 2: renderXXX() 関数を独立に抽出
        ↓ renderIDLE()          (50行程度)
        ↓ renderRUN()           (37行程度)
        ↓ renderRESULT()        (91行程度)
        ↓ renderALARM_SETTING() (68行程度)

Step 3: UI_Task() を従来関数ディスパッチに整理
        ↓ switch文で renderXXX()を呼び出し

Step 4: ビルド・確認
        ↓ platformio run -e m5stack
```

### **期待される効果**
| 指標 | 改善前 | 改善後 |
|------|--------|--------|
| UI_Task()の行数 | 200+ | 15 |
| renderXXX()各行数 | - | 45 |
| 単一責任度 | 低 | 高 |
| テスト記述性 | 困難 | 容易 |

---

## Ⅲ コード抽出ガイド

### **確認すべきコード位置**

**Tasks.cpp からの現在の renderXXX() の確認:**

```cpp
// Line ~380
void renderIDLE() {
  // 50行ほど。IDLE状態の表示処理
}

void renderRUN() {
  // 37行ほど。RUN状態の表示処理
}

void renderRESULT() {
  // 91行ほど。RESULT状態の表示処理（グラフィングス対応）
}

void renderALARM_SETTING() {
  // 68行ほど。ALARM_SETTING状態の表示処理
}

void UI_Task() {
  // 主処理の中に含まれている
  switch (G.M_CurrentState) {
    case State::IDLE:
      // renderIDLE() の処理が直接にある
      
    case State::RUN:
      // renderRUN() の処理が直接にある
      
    case State::RESULT:
      // renderRESULT() の処理が直接にある
      
    case State::ALARM_SETTING:
      // renderALARM_SETTING() の処理が直接にある
  }
}
```

### **変更後の構造**

```cpp
// Tasks.h に宣言追加
void renderIDLE();
void renderRUN();
void renderRESULT();
void renderALARM_SETTING();

// Tasks.cpp では関数分割
void renderIDLE() { ... }
void renderRUN() { ... }
void renderRESULT() { ... }
void renderALARM_SETTING() { ... }

void UI_Task() {
  static State prevState = State::IDLE;
  
  if (G.M_CurrentState != prevState) {
    M5.Lcd.fillScreen(BLACK);
    prevState = G.M_CurrentState;
  }
  
  // クリーンになった！
  switch (G.M_CurrentState) {
    case State::IDLE:           renderIDLE(); break;
    case State::RUN:            renderRUN(); break;
    case State::RESULT:         renderRESULT(); break;
    case State::ALARM_SETTING:  renderALARM_SETTING(); break;
  }
}
```

---

## Ⅳ 実装チェックリスト

### **ステップ1: 準備**
- [ ] VS Code でワークスペース開く
- [ ] Tasks.cpp を読み込む。主要部分を確認などで・確認
- [ ] 複雑さはこのドキュメント・を参照可能に

### **ステップ2: コード分割**
- [ ] `renderIDLE()` を Tasks.cpp に抽出
- [ ] `renderRUN()` を Tasks.cpp に抽出  
- [ ] `renderRESULT()` を Tasks.cpp に抽出
- [ ] `renderALARM_SETTING()` を Tasks.cpp に抽出

### **ステップ3: UI_Task()リファクタリング**
- [ ] UI_Task() の本体を簡潔にまとめる
- [ ] 各 renderXXX() の呼び出しを switch 文に
- [ ] コメント記述

### **ステップ4: Tasks.h 更新**
- [ ] 関数宣言を追加
  ```cpp
  void renderIDLE();
  void renderRUN();
  void renderRESULT();
  void renderALARM_SETTING();
  ```

### **ステップ5: 検証**
- [ ] ビルド: `platformio run -e m5stack`
- [ ] エラーなし
- [ ] メモリ使用率チェック: flash < 40%, RAM < 10%

### **ステップ6: コミット**
```bash
git add src/Tasks.cpp src/Tasks.h
git commit -m "refactor: UI_Task decomposed into renderXXX() functions (Task 3)"
```

---

## Ⅴ よくあるトラブル

### **問題1: 「Undeclared identifier 'renderIDLE'」**
**原因**: Tasks.h に関数宣言を忘れた  
**解決**: Tasks.h に以下を追加
```cpp
void renderIDLE();
void renderRUN();
void renderRESULT();
void renderALARM_SETTING();
```

### **問題2: 「Expected ';' before '}'」**
**原因**: 関数閉じ括弧の記述が漏れている或いは異なる  
**解決**: 各 renderXXX() の末尾に `}` があるか確認

### **問題3: ビルドサイズが増加**
**対処**:
```bash
# クリーンビルド
C:\.platformio\penv\Scripts\platformio.exe run --target clean -e m5stack

# 再度ビルド
C:\.platformio\penv\Scripts\platformio.exe run -e m5stack
```

---

## Ⅵ 詳細参照

- 📖 **Task 3 詳細**: [STAGE_4_REFACTORING_HANDOVER.md](./../guides/STAGE_4_REFACTORING_HANDOVER.md)
- 📚 **コード参照**: [CODE_REFERENCE.md](./../code/CODE_REFERENCE.md)
- 🎯 **全体仕様**: [detailed_spec_sw.md](./../specs/detailed_spec_sw.md)

---

## Ⅶ 想定所要時間

- **確認・準備**: 5分
- **コード分割**: 20分  
- **ビルド・確認**: 10分
- **コミット**: 2分

**合計**: 約 40-50分

---

**次のステップ**: Task 3 実装に進んでください。頑張ってください！ 💪


