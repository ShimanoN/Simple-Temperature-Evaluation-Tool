# Phase 3 拡張機能：ボタン設定モード - 実装計画書

**実装対象**: アラーム閾値のボタン設定化  
**作成日**: 2026年2月25日  
**優先度**: ★★★  
**開発難易度**: 中（EEPROM処理含む）  

---

## 📋 概要

Phase 3 で実装した閾値アラーム（HI_ALARM_TEMP/LO_ALARM_TEMP）の定数値を、プログラム再コンパイル不要で、M5Stack のボタン操作から現場で変更できるようにする。

---

## 🎯 実装目標

| 項目 | 要件 |
|:---|:---|
| **操作性** | M5Stack のボタンのみで設定完了。PC・シリアル通信不要 |
| **保持性** | 電源OFF後も設定値が保存される（EEPROM） |
| **可視性** | 設定画面で現在値と調整中の値が明確に表示 |
| **安全性** | 設定値保存時の確認画面。誤操作防止 |
| **前方互換性** | 既存の IDLE/RUN/RESULT 状態の動作に干渉しない |

---

## 🔧 技術仕様

### 新規状態：ALARM_SETTING

```
IDLE ─(BtnA長押し 2秒)─→ ALARM_SETTING(HI) ─(BtnA)─→ ALARM_SETTING(LO) ─(BtnA)─→ IDLE
                                 ↓                           ↓
                        画面: HI_ALARM設定       画面: LO_ALARM設定
```

### ボタン動作仕様

| 状態 | ボタン | 動作 | 効果 |
|:---|:---|:---|:---|
| ALARM_SETTING(HI) | BtnB | 1回押下 | HI_ALARM +5℃ |
| ALARM_SETTING(HI) | BtnC | 1回押下 | HI_ALARM -5℃ |
| ALARM_SETTING(HI) | BtnA | 1回押下 | LO_ALARM設定へ進行 |
| ALARM_SETTING(LO) | BtnB | 1回押下 | LO_ALARM +5℃ |
| ALARM_SETTING(LO) | BtnC | 1回押下 | LO_ALARM -5℃ |
| ALARM_SETTING(LO) | BtnA | 1回押下 | 設定値保存 & IDLE に戻る |

### 値の制約

```cpp
// HI_ALARM: 300～800℃（現実的な上限設定域）
HI_ALARM_MIN = 300.0f
HI_ALARM_MAX = 800.0f

// LO_ALARM: 100～300℃（現実的な下限設定域）
LO_ALARM_MIN = 100.0f
LO_ALARM_MAX = 300.0f

// HI_ALARM > LO_ALARM 制約は保証（UI側でチェック）
```

### EEPROM 設計

```cpp
// ESP32 EEPROM レイアウト
// アドレス 0-3: HI_ALARM_TEMP (float)
// アドレス 4-7: LO_ALARM_TEMP (float)
// アドレス 8: チェックサム（初期化判定用）

struct AlarmSettings {
  float HI_ALARM;   // offset: 0
  float LO_ALARM;   // offset: 4
  uint8_t checksum; // offset: 8 (初期化フラグ)
};

// EEPROM容量: 4KB（ESP32標準）
// 使用量: 9バイト（余裕十分）
```

---

## 📐 実装手順（10段階）

### フェーズ 1: 設計・準備（タスク 1-2）
- [ ] ボタン設定UX の詳細設計
- [ ] ESP32 EEPROM 動作仕様の確認

### フェーズ 2: コア実装（タスク 3-7）
- [ ] Global.h に設定構造体と EEPROM インターフェース追加
- [ ] State enum に ALARM_SETTING 追加
- [ ] IO_Task にBtnA長押し検出ロジック実装
- [ ] Logic_Task に状態遷移ロジック実装
- [ ] UI_Task に設定画面実装（見やすさ重視）

### フェーズ 3: テスト・デプロイ（タスク 8-10）
- [ ] EEPROM 読書き動作テスト
- [ ] 各状態間の遷移テスト
- [ ] コンパイル・ビルド・デプロイ
- [ ] ドキュメント更新

---

## 🎨 UI設計（詳細）

### 設定画面 (ALARM_SETTING(HI))

```
HI_ALARM SETTING

Current: 600℃
         ↑ BtnB: +5℃

[BtnA] Next
```

**特徴:**
- textSize(2) で大きく表示
- 現在値明示で直感的に理解可能
- ↑↓で上下方向を視覚化

### 設定画面 (ALARM_SETTING(LO))

```
LO_ALARM SETTING

Current: 400℃
         ↓ BtnC: -5℃

[BtnA] Save & Exit
```

### 遷移アニメーション

状態遷移時に画面クリアして混乱を防止（Phase 2 のページング と同じ設計）

---

## 💾 EEPROM コード設計

### 初期化時のフロー

```cpp
// setup() 内
AlarmSettings settings;
if (!EEPROM_ReadSettings(settings)) {
  // 初回起動 or チェックサム不一致
  // → デフォルト値で初期化して保存
  settings.HI_ALARM = HI_ALARM_TEMP;  // 600.0f
  settings.LO_ALARM = LO_ALARM_TEMP;  // 400.0f
  EEPROM_WriteSettings(settings);
}
// グローバル変数に設定値を反映
G.D_HI_ALARM = settings.HI_ALARM;
G.D_LO_ALARM = settings.LO_ALARM;
```

### 動的読み書きの安全性

- 読み込み時: チェックサム検証してから採用
- 書き込み時: 書き込み前後に値を検証

---

## 🔍 既存機能への影響

### 変更なし

- ✅ IDLE/RUN/RESULT状態の動作
- ✅ Phase 2 のページングロジック
- ✅ IO_Task の MAX31855読取
- ✅ Logic_Task の統計計算

### 追加変更

- IO_Task: BtnA長押し検出（+20行）
- Logic_Task: ALARM_SETTING状態遷移（+40行）
- UI_Task: 設定画面表示（+60行）
- EEPROM関数（+50行）

**合計追加: 約170行**

---

## ⚠️ 実装時の注意点

### 1. 長押し検出の堅牢性

```cpp
// 単純な長押しだと、即座に IDLE→ALARM_SETTING に遷移してしまう
// ↓↓↓ 誤動作防止のため、時間計測が必須

static unsigned long btnAPressDuration = 0;
static bool btnAWasPressed = false;

// 長押し開始を検出
if (M5.BtnA.isPressed() && !btnAWasPressed) {
  btnAPressDuration = millis();
  btnAWasPressed = true;
}

// 長押し確定（2秒以上）を検出
if (M5.BtnA.isPressed() && btnAWasPressed) {
  if (millis() - btnAPressDuration >= 2000) {
    // 長押し確定 → ALARM_SETTING へ遷移
  }
}

// 長押し終了
if (!M5.BtnA.isPressed() && btnAWasPressed) {
  btnAWasPressed = false;
}
```

### 2. EEPROM 動作の信頼性

- EEPROM書き込み後に即座に読み込み検証
- チェックサム（簡易的: sum of bytes）で初期化判定

### 3. UI 設計の配慮

- 設定値の上限・下限チェック（300～800℃, 100～300℃）
- HI_ALARM > LO_ALARM を保証（UI側で調整を制限）
- フォント大きさを textSize(2) に統一

---

## 📚 関連ドキュメント

- [Phase 1基本仕様](basic_spec.md)
- [Phase 3実装済仕様](future_plan.md)

---

**実装開始日**: 2026年2月25日  
**期待完了日**: 2026年2月25日中（高品質優先）  
**優先度**: Phase 3の拡張機能として中核

