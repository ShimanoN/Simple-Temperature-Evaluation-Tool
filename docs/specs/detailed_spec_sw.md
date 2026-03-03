# 簡易温度評価ツール - 詳細仕様書（ソフトウェア編）

> **更新ノート**: 本ドキュメントは2026年2月27日に改訂され、
> - 単一チャネル設計に特化し無駄な多チャネル記述を削除
> - MAX31855読み取りのリトライ処理を導入
> - SDカード初期化時のCSピン衝突問題と解決策（GPIO4固定参照）を追記
> - Logic_Task でも D_Average / D_StdDev をリアルタイム更新するよう修正
>
> 原稿は最終バージョンです。この仕様とコードは完全に同期しています。


**参照元**: basic_spec.md  
**関連**: detailed_spec_hw.md（組み立て・精度確認・現場運用）  
**開発環境**: PlatformIO (VSCode拡張)

---

## 開発環境セットアップ

### 前提条件

- **VSCode** がインストール済み
- インターネット接続環境

### セットアップ手順

#### 1. PlatformIO IDE のインストール

1. VSCode を起動
2. 左サイドバーの**拡張機能**アイコン（四角いアイコン）をクリック
3. 検索欄に「**PlatformIO IDE**」と入力
4. 「**PlatformIO IDE**」を選択し「**インストール**」をクリック
5. インストール完了後、VSCode を**再起動**
6. 左サイドバーに**PlatformIO のアイコン**（アンテナ形状）が表示されることを確認

#### 2. プロジェクトの作成

1. PlatformIO アイコン → 「**New Project**」をクリック
2. 以下の設定で「**Finish**」をクリック

| 項目               | 設定値                |
| ---------------- | ------------------ |
| **Project Name** | temp_eval_tool     |
| **Board**        | M5Stack Core ESP32 |
| **Framework**    | Arduino            |
| **Location**     | デフォルトのまま           |

3. プロジェクト生成完了まで待つ（約1〜2分）

#### 3. platformio.ini の編集

プロジェクトルートにある `platformio.ini` を開き、内容を以下に**全て置き換えて保存**:

```ini
[env:m5stack]
platform = espressif32
board = m5stack-core-esp32
framework = arduino
upload_speed = 115200
monitor_speed = 115200
build_flags =
    -DCORE_DEBUG_LEVEL=0
    -Os
lib_deps =
    m5stack/M5Stack @ ^0.4.6
    adafruit/Adafruit MAX31855 library @ ^1.1.2
```

4. 保存後、画面下部に「**PlatformIO: Installing dependencies...**」と表示される
5. インストール完了まで待つ（約2〜3分、初回のみ）

**💡 ヒント**: ライブラリが自動インストールされない場合は、画面下部の「**ホームアイコン**」→「**Libraries**」から手動でインストールできます。

---

## プロジェクト構成

セットアップ完了後、以下のファイル構成を作成します。

```
temp_eval_tool/
├── platformio.ini              ← PlatformIO 設定
├── include/
│   ├── Global.h                ← 共通型・定数・GlobalData 構造体・ピン定義
│   ├── Tasks.h                 ← タスク関数宣言
│   ├── EEPROMManager.h         ← EEPROM 操作クラス（アラーム閾値永続保存）
│   └── SDManager.h             ← SD カード操作クラス宣言
├── src/
│   ├── main.cpp                ← setup / loop
│   ├── Tasks.cpp               ← IO / Logic / UI タスク実装（アラーム・SD記録含む）
│   ├── DisplayManager.h/.cpp   ← UI 表示管理クラス
│   ├── IOController.h/.cpp     ← IO 制御クラス（ユニットテスト対応）
│   ├── MeasurementCore.h/.cpp  ← 計測ロジッククラス（ユニットテスト対応）
│   ├── EEPROMManager.cpp       ← EEPROM 実装
│   └── SDManager.cpp           ← SD カード CSV 記録実装
└── test/
    └── test_measurement_core.cpp  ← ネイティブユニットテスト
```

> **層構造の考え方**: `Global.h` → 共通型・定数 / `Tasks.cpp` → 3層タスクのエントリーポイント /
> `DisplayManager` → UI層 / `IOController` → IO層 / `MeasurementCore` → Logic層 /
> `EEPROMManager` → アラーム設定永続化 / `SDManager` → CSV 記録

---

## ソースコードの作成

以下のファイルを作成・編集します。

### 1. include/Global.h（新規作成）

`include` フォルダを右クリック → 「**新しいファイル**」→ ファイル名「**Global.h**」

> **注**: 以下は設計の要点を抜粋した概要です。完全なコードは `include/Global.h` を参照してください。

```cpp
#pragma once
#include <M5Stack.h>
#include <Adafruit_MAX31855.h>
#include <EEPROM.h>
#include <FS.h>
#include <SD.h>
#include "EEPROMManager.h"

// ── ピン定義 ────────────────────────────────────────────────────────────
constexpr uint8_t MAX31855_CS = 5;   // センサ CS (GPIO5)
// SD カード CS は TFCARD_CS_PIN (GPIO4) — M5Stack 内部ハードウェア配線

// ── タイマー周期 [ms] ───────────────────────────────────────────────────
constexpr unsigned long IO_CYCLE_MS         =  10UL;  // IO 層
constexpr unsigned long LOGIC_CYCLE_MS      =  50UL;  // Logic 層
constexpr unsigned long UI_CYCLE_MS         = 200UL;  // UI 層
constexpr unsigned long TC_READ_INTERVAL_MS = 500UL;  // MAX31855 サンプリング

// ── フィルタ / アラーム定数 ──────────────────────────────────────────────
constexpr float FILTER_ALPHA     = 0.1f;
constexpr float HI_ALARM_TEMP    = 600.0f;   // 上限デフォルト [°C]
constexpr float LO_ALARM_TEMP    = 400.0f;   // 下限デフォルト [°C]
constexpr float ALARM_HYSTERESIS =   5.0f;   // ヒステリシス幅 [°C]

// ── 状態定義 ────────────────────────────────────────────────────────────
enum class State : uint8_t {
  IDLE,           // 待機中
  RUN,            // 計測中
  RESULT,         // 結果表示中
  ALARM_SETTING   // アラーム閾値設定中
};

// ── グローバルデータ構造体 ────────────────────────────────────────────
struct GlobalData {
  // 計測データ (D_ = データレジスタ相当)
  float  D_RawPV;            // 生の温度測定値 [°C]
  float  D_FilteredPV;       // フィルタ後の温度値 [°C]
  double D_Sum;              // 積算値 (平均計算用)
  long   D_Count;            // サンプル数
  float  D_Average;          // 平均温度 [°C]

  // Phase 2: Welford 統計
  float  D_Max;              // 最高温度 [°C]
  float  D_Min;              // 最低温度 [°C]
  float  D_Range;            // Max - Min [°C]
  double D_M2;               // Welford M2 累積 (分散計算用)
  float  D_StdDev;           // 標準偏差 σ [°C]

  // 内部リレー (M_ = 内部リレー相当)
  State  M_CurrentState;     // 現在の状態
  bool   M_BtnA_Pressed;     // BtnA 立ち上がりエッジ
  bool   M_BtnB_Pressed;     // BtnB 立ち上がりエッジ (ページング / 設定進入)
  bool   M_BtnC_Pressed;     // BtnC 立ち上がりエッジ (設定値変更)
  int    M_ResultPage;       // RESULT 画面ページ (0=平均, 1=統計詳細)

  // Phase 3: アラーム
  bool   M_HiAlarm;          // 上限アラーム中フラグ
  bool   M_LoAlarm;          // 下限アラーム中フラグ
  float  D_HI_ALARM_CURRENT; // 現在の上限閾値 [°C] (EEPROM 保存)
  float  D_LO_ALARM_CURRENT; // 現在の下限閾値 [°C] (EEPROM 保存)
  int    M_SettingIndex;     // 設定モード: 0=HI 側, 1=LO 側

  // Phase 4: SD カード
  bool     M_SDReady;        // SD 検出フラグ
  bool     M_SDError;        // SD エラーフラグ
  char     M_CurrentDataFile[32]; // 現在のファイル名 (DATA_xxxx.csv)
  uint16_t M_SDWriteCounter; // 書き込みカウンタ (10 サンプル毎)
  uint32_t M_RunStartTime;   // RUN 開始時刻 (millis())
};

extern GlobalData        G;
extern Adafruit_MAX31855 thermocouple;

void initGlobalData();
void IO_Task();
void Logic_Task();
void UI_Task();
void EEPROM_LoadToGlobal();
void updateAlarmFlags(float currentTemp, float hiThreshold, float loThreshold,
                      float hysteresis, bool& hiFlag, bool& loFlag);
```

### 2. src/Tasks.cpp（新規作成）

`src` フォルダを右クリック → 「**新しいファイル**」→ ファイル名「**Tasks.cpp**」

> **注**: 実装は `src/Tasks.cpp` を参照（約 1500 行）。以下は各タスクの処理の概要です。

```cpp
// ── IO_Task (10ms 周期) ────────────────────────────────────────────────────
// - MAX31855 から 500ms 間隔で readCelsius()。NaN 時は最大 3 回リトライ
// - 1 次遅れフィルタ: D_FilteredPV = D_FilteredPV*(1-α) + rawPV*α
// - ヒステリシス付き HI/LO アラーム判定 → M_HiAlarm / M_LoAlarm 更新
// - BtnA / BtnB / BtnC の立ち上がりエッジ検出 → M_Btn*_Pressed フラグ

// ── Logic_Task (50ms 周期) ────────────────────────────────────────────────
// BtnA: IDLE → RUN → RESULT → IDLE の状態遷移
//       RUN 開始時に統計をリセット、RESULT 遷移時に SD ファイルをクローズ
// BtnB: IDLE で ALARM_SETTING 進入 / RESULT でページ切替 (Page0 ↔ Page1)
// BtnC: ALARM_SETTING で D_HI/LO_ALARM_CURRENT を SETTING_STEP (5°C) 変更
// RUN 中: Welford 法で D_Sum, D_Count, D_M2, D_Max, D_Min を更新
//         10 サンプル毎に SDManager::writeData() で CSV 書き込み
// RESULT 遷移: D_Average, D_StdDev, D_Range を確定 → SDManager::flush()/closeFile()

// ── UI_Task (200ms 周期) ──────────────────────────────────────────────────
// IDLE         : 現在温度 / アラーム設定値 / SD 状態（緑=OK / 赤=エラー）
// RUN          : 現在温度 / サンプル数 / 経過時間 / アラーム状態
// RESULT Page0 : 平均値 / サンプル数
// RESULT Page1 : 標準偏差 / Range / Max / Min
// ALARM_SETTING: HI 閾値 / LO 閾値（BtnB で切替、BtnC で変更、BtnA で保存・終了）
```

### 3. src/main.cpp（内容を置き換え）

既存の `src/main.cpp` の内容を**全て削除**し、以下に置き換え:

> **注**: 以下は実際の `src/main.cpp` の要点を抜粋したものです。完全な実装はソースファイルを参照してください。

```cpp
#include "Global.h"
#include "SDManager.h"

namespace {
  unsigned long T_IO_Last    = 0;
  unsigned long T_Logic_Last = 0;
  unsigned long T_UI_Last    = 0;
}

void setup() {
  M5.begin();
  M5.Power.begin();
  Serial.begin(SERIAL_BAUD_RATE);   // 115200 bps

  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Temperature Eval Tool");
  M5.Lcd.println("Refactored Version");

  initGlobalData();

  // EEPROM からアラーム閾値を読み込む
  EEPROMManager::init(EEPROM_SIZE);
  EEPROM_LoadToGlobal();

  // MAX31855 接続確認（最大 MAX_SETUP_RETRIES=5 回リトライ）
  delay(SETUP_SENSOR_DELAY_MS);     // 200ms: パワーオン安定待ち
  SPI.begin();
  pinMode(MAX31855_CS, OUTPUT);
  digitalWrite(MAX31855_CS, HIGH);
  float testTemp = NAN;
  for (int i = 0; i < MAX_SETUP_RETRIES; ++i) {
    IO_Task();
    testTemp = G.D_FilteredPV;
    if (!isnan(testTemp)) break;
    M5.Lcd.print('.');
    delay(SETUP_RETRY_INTERVAL_MS); // 500ms
  }
  if (isnan(testTemp)) {
    M5.Lcd.println("\nERROR: MAX31855 — Check wiring!");
    // センサ未接続でも継続動作（UI に ERROR 表示）
  } else {
    G.D_FilteredPV = testTemp;      // フィルタ初期値を実測値で設定
    M5.Lcd.println("\nMAX31855 OK");
  }

  // アラームフラグリセット（setup 中の IO_Task 呼び出しで立ったフラグをクリア）
  G.M_HiAlarm = false;
  G.M_LoAlarm = false;

  delay(SETUP_FINAL_DELAY_MS);      // 1000ms
  M5.Lcd.fillScreen(BLACK);

  // SD カード初期化
  SDManager::init();                // CS=TFCARD_CS_PIN (GPIO4)
  G.M_SDReady = SDManager::isReady();
  G.M_SDError = !G.M_SDReady;
}

void loop() {
  const unsigned long now = millis();

  if (now - T_IO_Last >= IO_CYCLE_MS) {
    T_IO_Last = now;
    IO_Task();
  }
  if (now - T_Logic_Last >= LOGIC_CYCLE_MS) {
    T_Logic_Last = now;
    Logic_Task();
  }
  if (now - T_UI_Last >= UI_CYCLE_MS) {
    T_UI_Last = now;
    UI_Task();
  }
}
```

---

## 書き込み手順

### 1. M5Stack を PC に接続

USB Type-C ケーブルで M5Stack と PC を接続します。

### 2. ビルド（コンパイル）

1. 画面下部の **✓アイコン**（Build）をクリック
2. **ターミナル**に `[SUCCESS]` と表示されるまで待つ（約30秒〜1分）

**エラーが出た場合**: 「トラブルシューティング」を参照

### 3. アップロード（書き込み）

1. 画面下部の **→アイコン**（Upload）をクリック
2. ターミナルに `[SUCCESS]` と表示されたら**書き込み完了**

**💡 ヒント**: ボード選択やシリアルポート選択は不要です。PlatformIO が自動検出します。

---

## 動作確認

### 1. 初回起動確認

書き込み完了後、M5Stack が自動的に再起動します。

以下を確認:

1. 画面に「**Temperature Eval Tool**」「**Refactored Version**」と表示される
2. 「**Checking MAX31855...**」→「**MAX31855 OK**」と表示される
3. 「**STATE: IDLE**」と表示される
4. 「**Temp: XX.X C**」に室温（20〜30℃程度）が表示される
5. SD カードが正しく挿入されている場合、IDLE 画面に SD インジケーターが緑色で表示される

### 2. 状態遷移の確認

1. M5Stack 正面下部の**左ボタン（BtnA）**を押す
2. 「**STATE: RUN**」に切り替わる
3. 「**Samples:**」の数値がカウントアップすることを確認（SD 記録が自動開始）
4. もう一度 **BtnA** を押す
5. 「**STATE: RESULT**」に切り替わり、「**Average: XX.X C**」が表示される
6. **BtnB（中央ボタン）**を押す → 2ページ目に切り替わり、標準偏差・Range・Max/Min が表示される
7. もう一度 **BtnA** を押す
8. 「**STATE: IDLE**」に戺ることを確認

### 3. 平均値計算の確認

1. 熱電対を室温に安定させる（5分以上放置）
2. **BtnA** を押して **RUN** 開始
3. **30秒以上待つ**（サンプル数が **600以上** に到達）
4. **BtnA** を押して **RESULT** に切り替え
5. 表示された**平均値**と、IDLE時の**瞬時値**の差が **±0.5℃以内**であることを確認

**これで動作確認は完了です。**

温度の精度確認は `detailed_spec_hw.md` の「精度確認」を参照してください。

---

## カスタマイズガイド

### パラメータ調整

`include/Global.h` の定数を変更することで、動作をカスタマイズできます。

| パラメータ              | デフォルト | 影響                         |
| ------------------ | ----- | -------------------------- |
| **IO_CYCLE_MS**    | 10ms  | IOポーリング周期。小さくすると応答性向上、負荷増加 |
| **LOGIC_CYCLE_MS** | 50ms  | サンプリング周期（50ms = 20サンプル/秒）  |
| **UI_CYCLE_MS**    | 200ms | 画面更新周期。小さくすると表示がなめらか、ちらつく  |
| **FILTER_ALPHA**   | 0.1   | フィルタ係数。小さくするとノイズ除去強化・追従性低下 |
| **HI_ALARM_TEMP**  | 600.0℃ | 上限アラームの初期値（EEPROM に保存される） |
| **LO_ALARM_TEMP**  | 400.0℃ | 下限アラームの初期値（EEPROM に保存される） |
| **ALARM_HYSTERESIS** | 5.0℃ | ヒステリシス幅（ヱラつき誘発防止） |

### フィルタ係数の変更例

**デフォルト（バランス型）**

```cpp
constexpr float FILTER_ALPHA = 0.1f;
```

**ノイズ除去強化版**（追従が遅くなる）

```cpp
constexpr float FILTER_ALPHA = 0.05f;
```

**応答性優先版**（ノイズに敏感になる）

```cpp
constexpr float FILTER_ALPHA = 0.2f;
```

変更後は **Build & Upload** で反映されます。

---

## トラブルシューティング

### ビルド（コンパイル）エラー

| エラーメッセージ                            | 原因           | 対処方法                              |
| ----------------------------------- | ------------ | --------------------------------- |
| `Global.h: No such file`            | ファイル作成漏れ     | `include/Global.h` が存在するか確認       |
| `'Global' has not been declared`    | インクルード漏れ     | `#include "Global.h"` があるか確認      |
| `undefined reference to 'IO_Task'`  | Tasks.cpp未追加 | `src/Tasks.cpp` が存在するか確認          |
| `Adafruit_MAX31855.h: No such file` | ライブラリ未インストール | `platformio.ini` の `lib_deps` を確認 |

### アップロード（書き込み）エラー

| エラーメッセージ                      | 原因         | 対処方法                      |
| ----------------------------- | ---------- | ------------------------- |
| `Serial port is not detected` | USB接続不良    | USBケーブルを抜き差し。CP210xドライバ確認 |
| `Upload failed`               | 書き込みモード未移行 | リセットボタン+BtnA同時押しで書き込みモード  |
| `espcomm_open failed`         | ポート競合      | シリアルモニタを閉じる。別USBポートを試す    |
| データ転送対応ケーブル以外                 | 充電専用ケーブル使用 | データ転送対応USBケーブルに交換         |

### 実行時エラー

| 症状                           | 原因           | 対処方法                            |
| ---------------------------- | ------------ | ------------------------------- |
| `ERROR: MAX31855 not found!` | 配線不良         | `detailed_spec_hw.md` を参照して配線確認 |
| 画面が真っ暗                       | 書き込み失敗       | 再度 Upload を実行                   |
| 温度が異常値                       | センサ故障または配線ミス | ハードウェア仕様書を参照                    |

---

## 技術資料

### アーキテクチャ概要

#### 3層タスク構成

```
loop() が毎回呼ばれる中で、各タスクはタイマーで周期を管理

IO_Task    (10ms)  → センサ読込 → フィルタ → HI/LO アラーム判定 → BtnA/B/C 読込
Logic_Task (50ms)  → 状態遷移 → Welford 統計計算 → SD CSV 書き込み
UI_Task    (200ms) → 画面描画
```

#### 状態遷移図

```
[IDLE] ──BtnA──> [RUN] ──BtnA──> [RESULT] ──BtnA──> [IDLE]
  待機              計測中            統計結果表示
  D_Sum=0          Welford統計累積      Page0: 平均・サンプル数
  D_Count=0        D_Count++            Page1: StdDev/Range/Max/Min
  D_M2=0           SD CSV 記録          (BtnB でページ切替)
    │
    └──BtnB──> [ALARM_SETTING] ──BtnA(SAVE)──> [IDLE]
                アラーム閾値設定
                BtnB: HI/LO 切替
                BtnC: 値 ±5℃ 変更
```

#### データフロー

```
MAX31855 (SPI)
    ↓ 500ms間隔で読取り (IO_Task 内)
D_RawPV (生値)
    ↓ 1 次遅れフィルタ (y[n] = y[n-1]*(1-α) + x[n]*α)
D_FilteredPV (フィルタ後)
    ↓ IO_Task 内・毎回
HI/LO アラーム判定 (ヒステリシス付き)
    ↓ RUN 状態のみ (Logic_Task 内)
Welford 統計累積:
    D_Sum   += D_FilteredPV
    D_Count  += 1
    D_M2  ← Welford差分累積寄与
    D_Max  = max(D_Max, D_FilteredPV)
    D_Min  = min(D_Min, D_FilteredPV)
    (10 サンプル毎) SDManager::writeData() → CSV 1 行追記
    ↓ BtnA 押下で RESULT 遷移
D_Average = D_Sum / D_Count
D_StdDev  = sqrt(D_M2 / D_Count)
D_Range   = D_Max - D_Min
SDManager::flush() + closeFile()
```

### 変数一覧

| 変数名                | 型      | 説明                              |
| ------------------ | ------ | --------------------------------- |
| **D_RawPV**        | float  | センサから読み取った生の温度値 [°C]        |
| **D_FilteredPV**   | float  | フィルタ処理後の温度値（画面表示・積算に使用）   |
| **D_Sum**          | double | 温度の累積値（平均値計算用、64bit）       |
| **D_Count**        | long   | サンプル数                              |
| **D_Average**      | float  | 計算された平均値（RESULT 状態で表示）      |
| **D_Max**          | float  | 計測期間中の最高温度 [°C]           |
| **D_Min**          | float  | 計測期間中の最低温度 [°C]           |
| **D_Range**        | float  | Max - Min (温度変動幅) [°C]           |
| **D_M2**           | double | Welford 法 二乗偏差累積（分散計算用）  |
| **D_StdDev**       | float  | 標準偏差 σ [°C]                      |
| **M_CurrentState** | enum   | 現在の状態（IDLE/RUN/RESULT/ALARM_SETTING） |
| **M_BtnA_Pressed** | bool   | BtnA 押下フラグ（エッジ検出済み）           |
| **M_BtnB_Pressed** | bool   | BtnB 押下フラグ（ページ切替／設定進入）      |
| **M_BtnC_Pressed** | bool   | BtnC 押下フラグ（設定値変更）              |
| **M_ResultPage**   | int    | RESULT 画面ページ (0=平均, 1=統計詳細)   |
| **M_HiAlarm**      | bool   | 上限アラーム中フラグ                     |
| **M_LoAlarm**      | bool   | 下限アラーム中フラグ                     |
| **D_HI_ALARM_CURRENT** | float | 現在の上限閾値 [°C] (EEPROM 保存)  |
| **D_LO_ALARM_CURRENT** | float | 現在の下限閾値 [°C] (EEPROM 保存)  |
| **M_SettingIndex** | int    | 設定モード: 0=HI 側, 1=LO 側             |
| **M_SDReady**      | bool   | SD 検出フラグ                          |
| **M_SDError**      | bool   | SD エラーフラグ                        |
| **M_CurrentDataFile** | char[32] | 現在の CSV ファイル名 (DATA_xxxx.csv) |
| **M_SDWriteCounter** | uint16 | SD 書き込みカウンタ (10 サンプル毎)     |
| **M_RunStartTime** | uint32 | RUN 開始時刻 (millis())                |
| **M_SDBuffer**     | SDData | SD CSV 1 行分データバッファ              |

### タイミング図

```
時刻(ms)    IO_Task  Logic_Task  UI_Task
  0           ●
 10           ●
 20           ●
 30           ●
 40           ●
 50           ●         ●
 60           ●
 ...
200           ●         ●          ●
```

---

**作成**: Shimano
**最終更新**: 2026年3月3日
