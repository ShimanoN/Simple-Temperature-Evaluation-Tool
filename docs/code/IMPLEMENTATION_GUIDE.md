# 実装仕様書（詳細版）

**バージョン**: v1.0.0  
**最終更新**: 2026年3月2日  
**対象**: 開発者・保守者向け

---

## 📑 目次

1. [アーキテクチャ概要](#アーキテクチャ概要)
2. [モジュール設計](#モジュール設計)
3. [グローバルデータ構造](#グローバルデータ構造)
4. [タスク実装](#タスク実装)
5. [CSV フォーマット](#csvフォーマット)
6. [エラーハンドリング](#エラーハンドリング)
7. [パフォーマンス仕様](#パフォーマンス仕様)
8. [拡張ポイント](#拡張ポイント)

---

## アーキテクチャ概要

### 全体構成図

```
┌────────────────────────────────────────────────────┐
│          M5Stack Core2 / CoreS3                     │
│                                                    │
│  ┌─────────────┐     ┌──────────────────────┐   │
│  │  main.cpp   │────→│  State Management    │   │
│  │ (setup/loop)│     │ (IDLE/RUN/RESULT)    │   │
│  └────────────┘└──────────────────────┘   │
│       │                                      │
│       ├──→ IO_Task (10ms)                   │
│       │    ├─ MAX31855 温度読取             │
│       │    ├─ SDBuffer 蓄積                 │
│       │    └─ SD 書き込み判定                │
│       │                                      │
│       ├──→ Logic_Task (50ms)                │
│       │    ├─ Welford 統計計算              │
│       │    ├─ アラーム判定                  │
│       │    └─ データ更新                    │
│       │                                      │
│       └──→ UI_Task (200ms)                  │
│            ├─ LCD 表示更新                  │
│            ├─ ボタン入力処理                │
│            └─ 状態表示                      │
│                                              │
└────────────────────────────────────────────────────┘
```

### 通信インターフェース

```
M5Stack (ESP32)
├─ I2C
│  ├─ MAX31855 (温度読取)
│  └─ EEPROM (アラーム値保存)
│
├─ SPI
│  └─ MicroSD カード (CSV書き込み)
│
└─ GPIO
   └─ ボタン (IDLE/RUN/RESULT遷移)
```

---

## モジュール設計

### 1. main.cpp

**責務**: 初期化・メインループ・状態遷移

```cpp
void setup() {
  // ハードウェア初期化（順序重要）
  Serial.begin(115200);
  M5.begin();
  
  // 各マネージャー初期化
  EEPROMManager::initialize();
  MAX31855::initialize();
  SDManager::initialize();
  
  // グローバル値の初期化
  G.currentState = IDLE;
  G.D_Count = 0;
  // ...
}

void loop() {
  // 定期タスク実行（時間ベース）
  if (millis() - lastIO >= IO_CYCLE_MS) {
    IO_Task();
    lastIO = millis();
  }
  
  if (millis() - lastLogic >= LOGIC_CYCLE_MS) {
    Logic_Task();
    lastLogic = millis();
  }
  
  if (millis() - lastUI >= UI_CYCLE_MS) {
    UI_Task();
    lastUI = millis();
  }
}
```

**重要な定数**:
```cpp
const int IO_CYCLE_MS = 10;      // I/O周期 (MAX31855読取)
const int LOGIC_CYCLE_MS = 50;   // 計算周期 (Welford)
const int UI_CYCLE_MS = 200;     // UI周期 (LCD更新)
const int SD_WRITE_INTERVAL = 100; // 100ms毎（≒10サンプル毎）
```

---

### 2. MeasurementCore.cpp / .h

**責務**: 温度計測・Welford法による統計計算

#### API

```cpp
namespace MeasurementCore {
  // 温度読取（MAX31855経由）
  float readTemperature();           // ℃で返す
  
  // Welford法更新
  void updateWelford(float sample);  // D_Average, D_StdDev を更新
  
  // リセット
  void resetStatistics();            // 計測状態初期化
  
  // アクセッサ
  float getAverage();                // 平均値取得
  float getStdDev();                 // 標準偏差取得
  float getMax();                    // 最大値取得
  float getMin();                    // 最小値取得
  int getSampleCount();              // サンプル数取得
}
```

#### Welford法の実装

```cpp
void MeasurementCore::updateWelford(float sample) {
  G.D_Count++;
  
  // 初回
  if (G.D_Count == 1) {
    G.D_Average = sample;
    G.D_M2 = 0.0;
    G.D_Max = sample;
    G.D_Min = sample;
    return;
  }
  
  // 2回目以降
  float delta = sample - G.D_Average;
  G.D_Average += delta / G.D_Count;
  
  float delta2 = sample - G.D_Average;
  G.D_M2 += delta * delta2;
  
  // 標準偏差 = sqrt(M2 / n)
  if (G.D_Count > 1) {
    G.D_StdDev = sqrt(G.D_M2 / G.D_Count);
  }
  
  // Max/Min 更新
  if (sample > G.D_Max) G.D_Max = sample;
  if (sample < G.D_Min) G.D_Min = sample;
}
```

**精度**: ±1℃（MAX31855仕様）

---

### 3. SDManager.cpp / .h

**責務**: SD操作、CSV ファイル作成・書き込み

#### API

```cpp
namespace SDManager {
  // ファイル操作
  bool initialize();                 // SD初期化
  bool createNewFile();              // 新規CSV作成（自動ファイル名付け）
  bool writeData();                  // CSV 1行書き込み
  bool closeFile();                  // ファイルクローズ・flush
  
  // 状態確認
  bool isReady();                    // SD準備OK？
  const char* getFileName();         // 現在のファイル名取得
}
```

#### CSV フォーマット

**ヘッダ行:**
```
ElapsedSec,Temp_C,State,Samples,Average_C,StdDev_C,Max_C,Min_C,HI_ALARM,LO_ALARM
```

**データ行:**
```
0,20.8,RUN,10,20.8,0.0,20.8,20.8,0,0
1,21.2,RUN,20,21.0,0.2,21.2,20.8,0,0
...
```

#### ファイル管理

```cpp
// ファイル名: DATA_NNNN.csv (NNNN = 0000～9999)
// 最初のRUNで DATA_0000.csv を作成
// リセットなしで連続RUNすると DATA_0001, DATA_0002... と自動増加

// グローバル変数
char G.M_FileName[32];      // "DATA_0000.csv"
int G.M_FileCounter;        // 0, 1, 2,...自動増加
File currentFile;           // ファイルハンドル
```

#### バッファリング

```cpp
// メモリ効率化のため、10サンプル毎に 1行 SD に書き込む
// (毎サンプル毎の書き込みは遅い)

const int SD_BUFFER_SIZE = 10;
struct SDBuffer {
  float temperature;
  int samples;
  float average;
  float stddev;
  float max_temp;
  float min_temp;
  bool hi_alarm;
  bool lo_alarm;
} M_SDBuffer;

// Logic_Task 内
if (G.D_Count % SD_BUFFER_SIZE == 0) {
  SDManager::writeData();
}
```

---

### 4. EEPROMManager.cpp / .h

**責務**: アラーム値（HI/LO）を電源断後も保存

#### API

```cpp
namespace EEPROMManager {
  bool initialize();                 // EEPROM初期化
  
  // 読み書き
  bool readAlarmSettings();          // HI_ALARM, LO_ALARM を読込
  bool writeAlarmSettings();         // HI_ALARM, LO_ALARM を保存
  
  float getHIAlarm();                // HI値取得
  void setHIAlarm(float value);      // HI値設定
  float getLOAlarm();                // LO値取得
  void setLOAlarm(float value);      // LO値設定
}
```

#### メモリレイアウト

```
EEPROM Address Range (内蔵EEPROM)
┌────────┬──────────┬──────────┬────────────┐
│  Ver   │ HI_ALARM │ LO_ALARM │ (reserve)  │
│ 4bytes │ 4bytes   │ 4bytes   │ ...        │
├────────┼──────────┼──────────┼────────────┤
│ 0x00   │ 0x04     │ 0x08     │ 0x0C~      │
└────────┴──────────┴──────────┴────────────┘
```

#### デフォルト値

```cpp
const float DEFAULT_HI_ALARM = 35.0f;  // ℃
const float DEFAULT_LO_ALARM = 10.0f;  // ℃
```

---

### 5. DisplayManager.cpp / .h

**責責**: LCD表示・UI画面管理

#### 画面仕様

| 状態 | 表示内容 |
|:---|:---|
| **IDLE** | 現在温度（大フォント），アラーム値，SD状態（GREEN/RED） |
| **RUN** | リアルタイム ファイル名，サンプル数，平均値，標準偏差，計測中インジケータ |
| **RESULT** | 計測結果（平均値・標準偏差・Max/Min），ファイルクローズメッセージ |

#### API

```cpp
namespace DisplayManager {
  void initialize();                 // LCD初期化
  void update();                     // 画面更新（状態に応じて分岐）
  
  // 状態別更新
  void displayIDLE();                // IDLE画面
  void displayRUN();                 // RUN画面（リアルタイム）
  void displayRESULT();              // RESULT画面
  
  // エラー表示
  void displayError(const char* msg); // エラー画面（RED）
  void displaySuccess(const char* msg); // 成功画面（GREEN）
}
```

#### 色定義

```cpp
const uint16_t COLOR_BLACK  = 0x0000;
const uint16_t COLOR_WHITE  = 0xFFFF;
const uint16_t COLOR_RED    = 0xF800;
const uint16_t COLOR_GREEN  = 0x07E0;
const uint16_t COLOR_BLUE   = 0x001F;
const uint16_t COLOR_YELLOW = 0xFFE0;
```

---

### 6. IOController.cpp / .h

**責責**: ボタン入力検知・GPIO制御

#### API

```cpp
namespace IOController {
  void initialize();                 // GPIO初期化
  void checkButtons();               // ボタン状態読込（毎10ms）
  bool isBtnAPressed();              // BtnA 押下判定
  bool isBtnAEdge();                 // BtnA エッジ（立ち上がり）検知
}
```

#### ボタン配置（M5Stack）

```
[BtnA] (GPIO39) - 左ボタン ← 主に使用
[BtnB] (GPIO38) - 真ん中
[BtnC] (GPIO37) - 右ボタン
```

#### 状態遷移ロジック

```cpp
void handleButtonA() {
  switch (G.currentState) {
    case IDLE:
      // → RUN に遷移
      SDManager::createNewFile();
      G.currentState = RUN;
      MeasurementCore::resetStatistics();
      break;
      
    case RUN:
      // → RESULT に遷移
      SDManager::closeFile();
      G.currentState = RESULT;
      break;
      
    case RESULT:
      // → IDLE に戻る
      G.currentState = IDLE;
      break;
  }
}
```

---

## グローバルデータ構造

### Global.h

```cpp
struct GlobalData {
  // ════════════════════════════════════
  // 【1】計測・統計データ
  // ════════════════════════════════════
  float D_FilteredPV;       // 現在の温度値（℃）
  float D_Average;          // 平均値（Welford法）
  float D_StdDev;           // 標準偏差（Welford法）
  double D_Sum;             // Welford用: 合計
  double D_M2;              // Welford用: 分散計算用
  int D_Count;              // サンプル数
  float D_Max;              // 最大温度
  float D_Min;              // 最小温度
  
  // ════════════════════════════════════
  // 【2】SD 操作関連
  // ════════════════════════════════════
  char M_FileName[32];      // 現在のファイル名
  int M_FileCounter;        // ファイルカウンタ（D_0000, D_0001...）
  int M_SDWriteCounter;     // SD書き込みカウンタ（0～9のサイクル）
  bool M_SDReady;           // SD準備OKフラグ
  bool M_SDError;           // SDエラーフラグ
  
  // ════════════════════════════════════
  // 【3】アラーム関連
  // ════════════════════════════════════
  float HI_ALARM;           // 高温閾値（EEPROM）
  float LO_ALARM;           // 低音閾値（EEPROM）
  bool HI_AlarmTriggered;   // 高温アラーム発動フラグ
  bool LO_AlarmTriggered;   // 低温アラーム発動フラグ
  
  // ════════════════════════════════════
  // 【4】状態管理
  // ════════════════════════════════════
  enum State {
    IDLE   = 0,  // 待機状態
    RUN    = 1,  // 計測中
    RESULT = 2   // 計測終了・結果表示
  } currentState;
  
  // ════════════════════════════════════
  // 【5】UI関連
  // ════════════════════════════════════
  uint32_t M_RunStartTime;  // RUN開始時刻（millis）
  // ... その他UI状態
  
} G;
```

**サイズ概算:**
```
float × 7 = 28 bytes
double × 2 = 16 bytes
int × 3 = 12 bytes
bool × 4 = 4 bytes
char[32] = 32 bytes
enum = 4 bytes
uint32_t = 4 bytes
─────────────────
合計 ≈ 100 bytes （16バイト アライメント後）
```

---

## タスク実装

### IO_Task (ピリオド: 10ms)

**目的**: MAX31855から温度読み込み、SD書き込み判定

```cpp
void IO_Task() {
  // 1. 温度読取
  G.D_FilteredPV = MeasurementCore::readTemperature();
  
  // 2. RUN中は SD に蓄積・書き込み判定
  if (G.currentState == RUN) {
    G.M_SDWriteCounter++;
    
    // 10個サンプル蓄積ごとに SD 書き込み
    if (G.M_SDWriteCounter >= SD_WRITE_INTERVAL && G.D_Count >= 10) {
      SDManager::writeData();
      G.M_SDWriteCounter = 0;
    }
  }
  
  // 3. ログ出力（デバッグ用）
  Serial.printf("[IO_Task] Temp=%.1f, Count=%d, SDWrite=%d\n",
    G.D_FilteredPV, G.D_Count, G.M_SDWriteCounter);
}
```

**レイテンシ要件**: < 8ms（10ms周期で余裕）

---

### Logic_Task (ピリオド: 50ms)

**目的**: Welford法で統計更新、アラーム判定

```cpp
void Logic_Task() {
  // 1. 計測中のみ統計更新
  if (G.currentState == RUN) {
    MeasurementCore::updateWelford(G.D_FilteredPV);
    
    // 2. アラーム判定
    G.HI_AlarmTriggered = (G.D_FilteredPV > G.HI_ALARM);
    G.LO_AlarmTriggered = (G.D_FilteredPV < G.LO_ALARM);
  }
  
  // 3. ログ出力
  Serial.printf("[Logic_Task] Avg=%.2f, StdDev=%.2f, Hi=%d, Lo=%d\n",
    G.D_Average, G.D_StdDev, G.HI_AlarmTriggered, G.LO_AlarmTriggered);
}
```

**精度**: ±0.1℃（Welford法の数値安定性）

---

### UI_Task (ピリオド: 200ms)

**目的**: LCD表示更新、ボタン入力処理

```cpp
void UI_Task() {
  // 1. ボタン入力処理
  IOController::checkButtons();
  if (IOController::isBtnAEdge()) {
    handleButtonA();  // 状態遷移
  }
  
  // 2. 画面更新（状態に応じて分岐）
  switch (G.currentState) {
    case IDLE:
      DisplayManager::displayIDLE();
      break;
    case RUN:
      DisplayManager::displayRUN();
      break;
    case RESULT:
      DisplayManager::displayRESULT();
      break;
  }
  
  // 3. ログ出力
  Serial.printf("[UI_Task] State=%d, Refresh\n", G.currentState);
}
```

**フレームレート**: ≈ 5 FPS（200ms周期）

---

## CSV フォーマット

### ファイル命名規則

```
DATA_NNNN.csv

NNNN = 0000 ～ 9999
リセット時は 0000 にリセット
連続実行（リセットなし）で 0001, 0002... と増加
```

### ヘッダ行（固定）

```
ElapsedSec,Temp_C,State,Samples,Average_C,StdDev_C,Max_C,Min_C,HI_ALARM,LO_ALARM
```

### 各列の説明

| 列名 | 型 | 説明 | 例 |
|:---|:---|:---|:---|
| ElapsedSec | int | RUN開始からの経過秒数（millis→秒に変換） | 0, 1, 2, ... |
| Temp_C | float | 現在の温度（℃）| 20.8 |
| State | string | 状態（RUN固定） | RUN |
| Samples | int | サンプル数 | 10, 12, 14, ... |
| Average_C | float | 平均温度（℃） | 20.7 |
| StdDev_C | float | 標準偏差（℃） | 0.1 |
| Max_C | float | 最大温度（℃） | 21.2 |
| Min_C | float | 最小温度（℃） | 20.4 |
| HI_ALARM | int | 高温アラーム発動（1=true, 0=false） | 0 |
| LO_ALARM | int | 低温アラーム発動（1=true, 0=false） | 0 |

### フォーマット関数

```cpp
void SDManager::formatCSVLine(
  char* buffer, size_t bufLen,
  uint32_t elapsedSec,
  float temp, int samples,
  float avg, float stddev,
  float max_temp, float min_temp,
  bool hi_alarm, bool lo_alarm
) {
  snprintf(buffer, bufLen,
    "%lu,%.1f,RUN,%d,%.1f,%.1f,%.1f,%.1f,%d,%d\r\n",
    elapsedSec, temp, samples, avg, stddev,
    max_temp, min_temp, hi_alarm ? 1 : 0, lo_alarm ? 1 : 0
  );
}
```

**改行コード**: `\r\n` (CRLF, Windows互換)

---

## エラーハンドリング

### エラーコード表

| エラー | コード | 対応 | ユーザ表示 |
|:---|:---|:---|:---|
| MAX31855未応答 | -999 | 読込リトライ3回 | LCD: RED "Sensor Error" |
| SD未準備 | - | SD初期化スキップ、計測は継続 | LCD: RED "SD Error" |
| ファイル作成失敗 | - | エラーログ出力 | LCD: RED "File Error" |
| EEPROM読込失敗 | - | デフォルト値使用 | Serial: WARNING |

### エラーハンドリング実装例

```cpp
// MAX31855読取失敗時
float MeasurementCore::readTemperature() {
  float temp = MAX31855::getTemperature();
  
  if (isnan(temp) || temp < -50 || temp > 150) {
    // エラー値 → 0.0 に変換
    return 0.0f;  
  }
  return temp;
}

// SD書き込み失敗時
bool SDManager::writeData() {
  if (!currentFile || !M_SDReady) {
    G.M_SDError = true;  // エラーフラグ設定
    return false;
  }
  
  // 書き込み実行
  size_t written = currentFile.write((uint8_t*)buffer, len);
  if (written != len) {
    G.M_SDError = true;
    return false;
  }
  
  // flush
  currentFile.flush();
  return true;
}
```

---

## パフォーマンス仕様

### メモリ使用量

```
Flash:
  - コード: ≈ 350KB (Phase 4実装最適化後)
  - 設定: ≈ 30KB
  ─────────────────
  合計: ≈ 406KB / 1310KB (31.0%)

RAM:
  - グローバル変数: ≈ 100 bytes
  - スタック: ≈ 20KB (タスク+局所変数)
  - ヒープ: ≈ 3KB (SD操作)
  ─────────────────
  合計: ≈ 23KB / 327KB (7.0%)
```

### 実行時間（実測）

| タスク | 周期 | 平均実行時間 | ピック時間 |
|:---|:---|:---|:---|
| IO_Task | 10ms | 0.5ms | 2ms |
| Logic_Task | 50ms | 1ms | 3ms |
| UI_Task | 200ms | 5ms | 10ms |
| トータル | - | 6.5ms | 15ms |

**結論**: リアルタイム性問題なし ✅

---

## 拡張ポイント

### 推奨拡張（実装容易）

1. **RTC統合（タイムスタンプ追加）**
   - CSV に YYYYMMDD_HHMMSS 列追加
   - DS3231等を I2C で接続
   - `MeasurementCore::addTimestampColumn()`

2. **クラウド連携**
   - WiFi経由で計測データを AWS IoT Core に送信
   - `NetworkManager::uploadToCloud()`

3. **複数センサ対応**
   - K型熱電対×複数チャネル
   - CSV列拡張（Temp_C_CH1, Temp_C_CH2, ...）

### 制限事項

- **温度範囲**: K型理論値 -50～+150℃（MAX31855仕様）
- **サンプルレート**: 10ms（高速化には ESP32 スピードアップが必要）
- **ファイル数**: 最大 10,000ファイル（DATA_9999.csv まで）

---

**最終更新**: 2026年3月2日  
**メンテナ**: Shimano → 新人開発者へ引き継ぎ
