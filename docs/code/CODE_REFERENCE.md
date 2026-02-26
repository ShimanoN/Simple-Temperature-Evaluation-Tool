# コード構造・インターフェース リファレンス

**作成日**: 2026年2月26日  
**対象**: 段階2-3 実施時の参照用（クイックリファレンス）

---

## 📋 Global.h - 主要定義

### UI定数 （line 26-45）
```cpp
namespace UI {
  // テキストサイズ
  constexpr uint8_t TEXTSIZE_TITLE = 1;
  constexpr uint8_t TEXTSIZE_VALUE = 2;
  constexpr uint8_t TEXTSIZE_GUIDE = 1;

  // LCD Y座標（320x240解像度）
  namespace PosY {
    constexpr uint16_t TITLE = 0;
    constexpr uint16_t STATE_LABEL = 10;
    constexpr uint16_t TEMP_LABEL = 50;
    constexpr uint16_t TEMP_VALUE = 90;
    constexpr uint16_t BUTTON_INFO = 220;
  }

  // LCD寸法
  constexpr uint16_t LCD_WIDTH = 320;
  constexpr uint16_t LCD_HEIGHT = 240;

  // デバッグ表示フラグ
  constexpr bool SHOW_DEBUG_LOGS = true;
  constexpr bool SHOW_ALARM_DEBUG = true;
  constexpr bool SHOW_EEPROM_DEBUG = true;
}
```

### グローバル変数（主要）
```cpp
// センサー読み込み結果
extern volatile float G_CurrentTemp;
extern volatile float G_FilteredTemp;

// アラーム設定値（℃単位）
extern volatile float D_HI_ALARM_CURRENT;  // デフォルト: 600.0f (60℃)
extern volatile float D_LO_ALARM_CURRENT;  // デフォルト: 400.0f (40℃)

// アラーム状態フラグ
extern volatile bool G_HiAlarm;
extern volatile bool G_LoAlarm;

// 統計値
extern volatile uint32_t G_SampleCount;
extern volatile float G_AvgTemp;
extern volatile float G_MaxTemp;
extern volatile float G_MinTemp;
extern volatile float G_StdDev;
```

### 関数シグネチャ

```cpp
// アラーム判定（Task 2で新規関数化）
void updateAlarmFlags(float currentTemp, float hiThreshold, float loThreshold,
                      float hysteresis, bool& hiFlag, bool& loFlag);

// EEPROM統合初期化
void EEPROM_LoadToGlobal();

// 統計リセット（Phase 2）
void StatisticsReset();

// 統計更新（Phase 2）
void UpdateStatistics(float newValue);
```

---

## 🔌 EEPROMManager クラス詳細

### ヘッダー (include/EEPROMManager.h - 55行)

```cpp
#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include <EEPROM.h>
#include <cmath>

// EEPROM アドレス定義
constexpr uint16_t EEPROM_ADDR_HI_ALARM = 0;     // float (4byte)
constexpr uint16_t EEPROM_ADDR_LO_ALARM = 4;     // float (4byte)
constexpr uint16_t EEPROM_ADDR_CHECKSUM = 8;     // uint8_t (1byte)
constexpr uint16_t EEPROM_SIZE = 9;

// アラーム設定構造体
struct AlarmSettings {
  float hiAlarm;
  float loAlarm;
  // 初期値
  AlarmSettings() : hiAlarm(60.0f), loAlarm(40.0f) {}
};

// EEPROM操作クラス（staticメソッドのみ）
class EEPROMManager {
public:
  // EEPROM初期化
  static void init(size_t size = EEPROM_SIZE);
  
  // 設定値を読み込む
  // @return true: 読み込み成功, false: 検証エラー
  static bool readSettings(AlarmSettings& settings);
  
  // 設定値を書き込む
  // @return true: 書き込み+検証成功, false: 検証エラー
  static bool writeSettings(const AlarmSettings& settings);
  
  // デバッグ情報出力（Serial）
  static void printDebugInfo();

private:
  // 値の有効性チェック（NaN/Inf検出）
  static bool isValueValid(float value);
  
  // 値の範囲チェック（-50℃～1100℃）
  static bool isValueInRange(float value, float minVal = -50.0f, float maxVal = 1100.0f);
};

#endif
```

### 実装詳細 (src/EEPROMManager.cpp - 112行)

**readSettings()アルゴリズム**:
```
1. EEPROM[0-3]から hiAlarm (float) 読み込み
2. EEPROM[4-7]から loAlarm (float) 読み込み
3. EEPROM[8]のチェックサム (0xA5) を確認
4. isValueValid() で NaN/Inf チェック
5. isValueInRange() で -50℃～1100℃ 確認
6. すべてOKで true, 1つでも失敗で false
```

**writeSettings()アルゴリズム**:
```
1. hiAlarm を EEPROM[0-3] に書き込み
2. loAlarm を EEPROM[4-7] に書き込み
3. チェックサム 0xA5 を EEPROM[8] に書き込み
4. EEPROM.commit() で永続化
5. 50ms 待機（EEPROM安定化）
6. 書き込み値を読み戻して検証（±0.01℃ 許容）
7. 検証OK で true, NG で false
```

---

## 🎮 Tasks.cpp - 関数一覧

### updateAlarmFlags() （line 33-77, 45行）

**責務**: アラーム判定（ヒステリシス付き）

**呼び出し**: IO_Task() 内（10ms周期）

**ロジック**:
```
HI_ALARM判定:
  現在値 >= HI_THRESHOLD に達したら → hiFlag = true
  現在値 < (HI_THRESHOLD - HYSTERESIS) されたら → hiFlag = false

LO_ALARM判定:
  現在値 <= LO_THRESHOLD に達したら → loFlag = true
  現在値 > (LO_THRESHOLD + HYSTERESIS) されたら → loFlag = false

NaN処理:   
  isnan(currentTemp) なら処理スキップ

音声フィードバック:
  HI判定時: 2kHz ビープ音 (100ms)
  LO判定時: 1kHz ビープ音 (100ms)
```

**テスト対象**: ✅ ユニットテストで 100% カバー予定（Task 8）

---

### renderIDLE() （line 300-325, 26行）

**責務**: IDLE状態の液晶表示

**表示内容**:
```
STATE: IDLE
Temp: XX.X°C
Alarm: HI=XX.X  LO=YY.Y

[BtnA] Start  [BtnB] Setting
```

**色分け**:
- 通常: 白文字 (WHITE: 0xFFFF)
- アラーム時: 赤文字 (RED: 0xF800)

---

### renderRUN() （line 327-350, 24行）

**責務**: RUN状態の液晶表示

**表示内容**:
```
STATE: RUN
Temp: XX.X°C
Samples: NNN

[BtnA] Stop/Reset
```

**役割**: サンプル数カウント表示、温度のリアルタイム監視

---

### renderALARM_SETTING() （line 352-390, 39行）

**責務**: アラーム設定画面の表示と調整

**HI_ALARM設定ページ (M_SettingIndex == 0)**:
```
SETTING HI_ALARM
Current: XX.X°C

[BtnB]+5°C [BtnC]-5°C
[BtnA] Next → LO
```

**LO_ALARM設定ページ (M_SettingIndex == 1)**:
```
SETTING LO_ALARM
Current: YY.Y°C

[BtnB]+5°C [BtnC]-5°C
[BtnA] Save & Exit
```

**調整範囲**: -50℃ ～ 1100℃（EEPROMManager範囲と同じ）

**単位**: 5℃刻み（SETTING_STEP = 5.0f）

---

### renderRESULT() （line 392-420, 29行）

**責務**: 統計結果の2ページ表示

**ページ1 (pageIdx == 0)**:
```
Current: XX.X°C
Average: ZZ.Z°C
```

**ページ2 (pageIdx == 1)**:
```
StdDev: S.SSS°C  Range: R.R°C
Max: MM.M°C      Min: mm.m°C
```

**ページネーション**:
- BtnB / BtnC で page 切替
- BtnA でリセット（IDLE へ戻る、統計クリア）

---

## 🔄 状態機械（State.h に定義）

```cpp
enum class State {
  IDLE,           // 待機状態
  RUN,            // 計測実行中
  RESULT,         // 結果表示
  ALARM_SETTING   // アラーム設定
};
```

**遷移フロー**:
```
IDLE
  ↓ BtnA (START)
  RUN
  ↓ BtnA (STOP/RESET) 
  RESULT
  ↓ BtnA (BACK)
  IDLE

IDLE
  ↓ BtnB (SETTING)
  ALARM_SETTING
  ↓ BtnA (EXIT)
  IDLE
```

---

## 📊 Task周期・責務

| Task | 周期 | 責務 | 関数 |
|:---:|:---:|:---|:---|
| **IO_Task** | 10ms | センサ読み込み<br/>ボタン入力<br/>アラーム判定 | readSensor()<br/>updateButtons()<br/>updateAlarmFlags() |
| **Logic_Task** | 50ms | 状態遷移<br/>統計計算 | idle/run/result/setting処理<br/>UpdateStatistics() |
| **UI_Task** | 200ms | 液晶表示 | renderIDLE/RUN/ALARM_SETTING/RESULT |

---

## 🐛 デバッグ出力フラグ

Global.h に定義（line 44, 45, 46）:
```cpp
const bool SHOW_DEBUG_LOGS = true;         // 一般ログ
const bool SHOW_ALARM_DEBUG = true;        // アラーム専用
const bool SHOW_EEPROM_DEBUG = true;       // EEPROM専用
```

**Serial 出力例** (115200 baud):
```
[Setup] Initializing...
[Setup] MAX31855: OK
[Setup] EEPROM init ...
[Setup] HI_ALARM: 60.0 C, LO_ALARM: 40.0 C
[Setup] Alarm flags reset before main loop
[IO] Temp=25.5C, HI=false, LO=false
[UI] STATE: IDLE, Temp=25.5C
[ALARM_DEBUG] HI: ACTIVE (threshold=60.0, hysteresis=5.0)
[EEPROM] Written: HI=65.0 LO=45.0
```

---

## 🛠️ ビルド・デプロイコマンド

### ビルド
```bash
platformio run -e m5stack
# 出力: Build [m5stack] - SUCCESS (61.68s)
```

### アップロード
```bash
platformio run --target upload --environment m5stack
# 出力: Upload [m5stack] - SUCCESS (44.14s)
```

### ユニットテスト（段階3で追加）
```bash
platformio test -e native
# 出力: test_measurement_core.cpp: PASS all tests
```

### シリアルモニタ表示
```bash
platformio device monitor -e m5stack
# Baud: 115200, DataBits: 8, StopBits: 1, Parity: None
```

---

## ✅ 検証チェックリスト

### 実装検証（段階2-3実施時）

- [ ] **Task 5 完了**: CODE_EXPLANATION.md にアーキテクチャ図・状態遷移図追加
- [ ] **Task 6 完了**: TROUBLESHOOTING.md 新規作成（5症状以上記載）
- [ ] **Task 7 完了**: コメント充実（Welford法・ヒステリシス説明）
- [ ] **Task 8 完了**: ユニットテスト 5個以上実装、`platformio test -e native` 全PASS
- [ ] **Task 9 完了**: 動作検証チェックリスト全項目確認
- [ ] **Task 10 完了**: パフォーマンス測定結果を REFACTORING_PROGRESS.md に記録

### ビルド検証

- [ ] `platformio run -e m5stack` → SUCCESS
- [ ] `platformio run --target upload --environment m5stack` → SUCCESS
- [ ] M5Stack 起動ログ: "[Setup] Alarm flags reset..." が表示される

### コード品質検証

- [ ] Magic Number: UI:: namespace で全削減
- [ ] updateAlarmFlags(): スタンドアロン関数、テスト可能
- [ ] EEPROMManager: 複雑度削減、再利用可能
- [ ] 関数平均行数: 150行 → 50行以下に削減

---

## 🔗 参考リンク（このプロジェクト内）

- **REFACTORING_PLAN.md**: 全体計画（10タスク）
- **REFACTORING_PROGRESS.md**: 進捗報告（Task 1-4完了）
- **CODE_EXPLANATION.md**: コード解説（拡充予定）
- **TROUBLESHOOTING.md**: トラブル対応（新規予定）

---

**リファレンス更新日**: 2026年2月26日  
**適用対象**: 段階2-3（Task 5-10）

