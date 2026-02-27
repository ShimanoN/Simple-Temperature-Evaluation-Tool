# UI レイアウト設計書（根本的改善）

## 1. 画面仕様

### 液晶スペック
```
- 解像度: 320 × 240 ピクセル
- フォント: M5Stack 標準（等幅フォント）
```

### テキストサイズと高さ

| Size | 設定値 | 文字高さ（px） | 用途 |
|:---|:---|:---|:---|
| Small | TEXTSIZE_GUIDE (1) | 8 | ラベル、ボタン情報 |
| Medium | TEXTSIZE_TITLE (1) | 8 | （実装上はGUIDEと同じ） |
| Large | TEXTSIZE_VALUE (2) | 16 | 値表示（温度、サンプル数） |

**重要**: 異なるサイズがある場合、行間隔を調整する必要がある


## 2. IDLE 画面レイアウト

### 設計方針
- **行ベース設計**: 各要素を「行」として明確に分離
- **テキストサイズ統一**: 同一行では同一サイズを使用
- **余白標準化**: 上下余白を 4px に統一

### レイアウト図

```
┌─────────────────────────────────────────┐
│ Y=0:   (4px margin)                     │  行1（ラベル行）
│        STATE: IDLE                      │  テキストサイズ: GUIDE (8px)
│ Y=8:   (bottom of line)                 │  
│ Y=12:  (4px margin)                     │
├─────────────────────────────────────────┤
│ Y=12:  Temp: 25.3  C                    │  行2（温度行）
│ Y=28:  (bottom of line)                 │  テキストサイズ: GUIDE + VALUE
│ Y=32:  (4px margin)                     │  
├─────────────────────────────────────────┤
│ Y=32:                                   │
│        (reserved for future)             │  行3
│ Y=48:  (4px margin)                     │
├─────────────────────────────────────────┤
│ Y=48:  Humidity: 45.2  %                │  行4（湿度行）
│ Y=64:  (bottom of line)                 │  テキストサイズ: GUIDE + VALUE
│ Y=68:  (4px margin)                     │
├─────────────────────────────────────────┤
│ Y=68:  SD Ready                         │  行5（SD状態）
│ Y=84:  (bottom of line)                 │  テキストサイズ: GUIDE
│ Y=88:  (4px margin)                     │
├─────────────────────────────────────────┤
│ Y=88:  (Alarm表示: optional)            │  行6（アラーム）
│        HI ALARM: 60.0°C / LO: 40.0°C   │  テキストサイズ: GUIDE
│ Y=104: (bottom of line)                 │
│ Y=108: (4px margin)                     │
├─────────────────────────────────────────┤
│                                         │  (space for layout flexibility)
│                                         │  行7～8: 中央領域
│                                         │
├─────────────────────────────────────────┤
│ Y=220:                                  │  行9（ボタンガイド下端固定）
│ [BtnA] Start    [BtnB] Setting          │  テキストサイズ: GUIDE (8px)
│ Y=236: (bottom of line)                 │  (240 - 4 = 236)
└─────────────────────────────────────────┘
```

### Y座標計算式

```cpp
// 基本単位
constexpr uint16_t LINE_HEIGHT_S = 12;  // Small font: 8px + 4px margin
constexpr uint16_t LINE_HEIGHT_L = 20;  // Large font: 16px + 4px margin
constexpr uint16_t TEXT_HEIGHT_S = 8;   // Small text height
constexpr uint16_t TEXT_HEIGHT_L = 16;  // Large text height
constexpr uint16_t MARGIN = 4;

// Y座標定義
constexpr uint16_t Y1_START = 0;         // STATE: IDLE
constexpr uint16_t Y1_END = Y1_START + LINE_HEIGHT_S;

constexpr uint16_t Y2_START = Y1_END;    // Temp: 25.3  C
constexpr uint16_t Y2_END = Y2_START + LINE_HEIGHT_L;

constexpr uint16_t Y3_START = Y2_END;    // (reserved)
constexpr uint16_t Y3_END = Y3_START + LINE_HEIGHT_S;

constexpr uint16_t Y4_START = Y3_END;    // Humidity: 45.2  %
constexpr uint16_t Y4_END = Y4_START + LINE_HEIGHT_L;

constexpr uint16_t Y5_START = Y4_END;    // SD Ready
constexpr uint16_t Y5_END = Y5_START + LINE_HEIGHT_S;

constexpr uint16_t Y6_START = Y5_END;    // Alarm display (optional)
constexpr uint16_t Y6_END = Y6_START + LINE_HEIGHT_S;

// ボタンガイドは下端固定
constexpr uint16_t Y_BUTTON = 220;       // LCD_HEIGHT - margin - height
```

### X座標定義（位置揃え）

```cpp
// 左寄せ
constexpr uint16_t X_LEFT = 0;

// 値の開始位置（ラベル部: 60px の後）
// 小文字: 1文字 = 6px
// "Temp: " = 6文字 = 36px → 40px（padding含む）
constexpr uint16_t X_VALUE_START = 70;

// ラベルと値を分割する場合
// "Temp:" ラベル部分のみを70px以内に納める
constexpr uint16_t X_LABEL_MAX = 60;
constexpr uint16_t X_VALUE = 70;

// 中央揃え（SD状態など）
constexpr uint16_t X_CENTER = 160;

// 右寄せ
constexpr uint16_t X_RIGHT = 310;
```

### テンプレート実装方針

#### パターン1：「ラベル + 値」行（温度、湿度）

```cpp
// テンプレート: Label + Value (mixed font size)
// 該当: Temp, Humidity
void renderLabelValueLine(uint16_t y,
                          const char *label,
                          float value,
                          const char *unit,
                          uint16_t textColor) {
  // ラベル（小文字）
  M5.Lcd.setTextSize(UI::TEXTSIZE_GUIDE);
  M5.Lcd.setCursor(0, y);
  M5.Lcd.setTextColor(textColor, BLACK);
  M5.Lcd.printf("%s", label);
  
  // 値（大文字、右配置）
  M5.Lcd.setTextSize(UI::TEXTSIZE_VALUE);
  M5.Lcd.setCursor(X_VALUE, y - 4);  // 大文字は高さが異なるため Y を調整
  M5.Lcd.printf("%6.1f %s", value, unit);
}
```

#### パターン2：単純テキスト行（STATE, SD状態）

```cpp
// テンプレート: Simple text (single font size)
// 該当: STATE, SD status, Alarm info
void renderSimpleLine(uint16_t y,
                      const char *text,
                      uint16_t textColor) {
  M5.Lcd.setTextSize(UI::TEXTSIZE_GUIDE);
  M5.Lcd.setCursor(0, y);
  M5.Lcd.setTextColor(textColor, BLACK);
  M5.Lcd.printf("%s\n", text);
}
```

#### パターン3：中央揃えテキスト

```cpp
// テンプレート: Center-aligned text
void renderCenterLine(uint16_t y,
                      const char *text) {
  M5.Lcd.setTextSize(UI::TEXTSIZE_GUIDE);
  // 簡易版：固定幅フォント 6px × 文字数で計算
  uint16_t textWidth = strlen(text) * 6;
  uint16_t x = (320 - textWidth) / 2;
  M5.Lcd.setCursor(x, y);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.printf("%s\n", text);
}
```

## 3. RUN 画面レイアウト

```
┌─────────────────────────────────────────┐
│ Y=0:   STATE: RUN             ##         │  行1（ステータス行 + 実行時間）
├─────────────────────────────────────────┤
│ Y=12:  Temp: 25.3  C                    │  行2（リアルタイム温度）
├─────────────────────────────────────────┤
│ Y=32:  Humidity: 45.2  %                │  行3（リアルタイム湿度）
├─────────────────────────────────────────┤
│ Y=52:  Samples: 143                     │  行4（サンプル数）
├─────────────────────────────────────────┤
│ Y=72:  Avg: 24.8°C  SD: 0.3°C           │  行5（統計値）
├─────────────────────────────────────────┤
│                                         │  (space)
│                                         │
├─────────────────────────────────────────┤
│ Y=200: [BtnA] Stop                      │  行9（操作ガイド）
└─────────────────────────────────────────┘
```

## 4. RESULT 画面レイアウト

```
┌─────────────────────────────────────────┐
│ Y=0:   STATE: RESULT          ##         │  行1
├─────────────────────────────────────────┤
│ Y=12:  Average: 24.8°C                  │  行2
│ Y=32:  StdDev:  0.30°C                  │  行3
│ Y=52:  Min:    23.5°C  Max: 26.1°C      │  行4
├─────────────────────────────────────────┤
│ Y=72:  Samples: 143                     │  行5
│ Y=92:  Duration: 71.5 sec               │  行6
├─────────────────────────────────────────┤
│ Y=112: SD: Saved (filename)             │  行7
│                                         │
├─────────────────────────────────────────┤
│ Y=200: [BtnA] Save  [BtnB] IDLE         │  行8
└─────────────────────────────────────────┘
```

## 5. 実装チェックリスト

- [ ] Global.h に新しい定数系を追加
- [ ] UI テンプレート関数を実装
- [ ] renderIDLE() を新タンプレートで再実装
- [ ] renderRUN() を新テンプレートで実装
- [ ] renderRESULT() を新テンプレートで実装
- [ ] すべての画面でテスト検証

## 6. ちらつき削減戦略

### 現在の発生原因
1. 毎フレーム fillScreen(BLACK) で全消去
2. その後に printf で描画
3. 200ms の再描画周期中にテキストが瞬間的に見えなくなる

### 改善方法（短期）
- 画面モード（IDLE ↔ RUN ↔ RESULT）遷移時のみ fillScreen
- 同一画面内在宅時は **部分的な消去 + 再描画** のみ

### 改善方法（中期）
- ダブルバッファリングの導入（RAM許容範囲内）
- または dirty rectangle の概念で必要な部分だけ更新

### 具体的最適化
```cpp
// 【改善前】毎フレーム全消去
void UI_Task() {
  M5.Lcd.fillScreen(BLACK);  // ← 全画面消去
  renderIDLE();               // ← 再描画
}

// 【改善後】初回変更時のみ全消去、その後は部分更新
static bool needsClear = true;
void UI_Task() {
  if (needsClear) {
    M5.Lcd.fillScreen(BLACK);
    needsClear = false;
  }
  
  // 変更があった部分だけを消去・再描画
  if (tempChanged) {
    clearLine(Y2_START, Y2_END);
    renderTempLine();
  }
  
  if (sampleCountChanged) {
    clearLine(Y4_START, Y4_END);
    renderSampleLine();
  }
  
  // ステート遷移時は全消去フラグを立てる
}

// 補助関数: 指定行を消去
void clearLine(uint16_t y_start, uint16_t y_end) {
  M5.Lcd.fillRect(0, y_start, 320, y_end - y_start, BLACK);
}
```

---

**版**: v1.0（根本的改善版）
**最終更新**: 2026年2月27日
