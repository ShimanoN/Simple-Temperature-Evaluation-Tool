# 簡易温度評価ツール - 詳細仕様書（ソフトウェア編）

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
[env:m5stack-core-esp32]
platform = espressif32
board = m5stack-core-esp32
framework = arduino
upload_speed = 921600
monitor_speed = 115200
lib_deps =
    m5stack/M5Stack@^0.4.6
    adafruit/Adafruit MAX31855 library@^1.4.2
```

4. 保存後、画面下部に「**PlatformIO: Installing dependencies...**」と表示される
5. インストール完了まで待つ（約2〜3分、初回のみ）

**💡 ヒント**: ライブラリが自動インストールされない場合は、画面下部の「**ホームアイコン**」→「**Libraries**」から手動でインストールできます。

---

## プロジェクト構成

セットアップ完了後、以下のファイル構成を作成します。

```
temp_eval_tool/
├── platformio.ini          ← 上記で編集済み
├── include/
│   └── Global.h            ← 新規作成（次のセクション参照）
├── src/
│   ├── main.cpp            ← デフォルトで存在（内容を置き換え）
│   └── Tasks.cpp           ← 新規作成（次のセクション参照）
└── lib/                    ← ライブラリ（自動管理）
```

---

## ソースコードの作成

以下の3ファイルを作成・編集します。

### 1. include/Global.h（新規作成）

`include` フォルダを右クリック → 「**新しいファイル**」→ ファイル名「**Global.h**」

```cpp
#ifndef GLOBAL_H
#define GLOBAL_H

#include <M5Stack.h>
#include <Adafruit_MAX31855.h>

// --- ピン定義 ---
#define MAX31855_CS   5    // チップセレクトのみ使用（ハードウェアSPIでLCDとバス共有）
// SCK=GPIO18, MISO=GPIO19 はハードウェアSPIが自動管理

// --- タイマー周期 (ms) ---
constexpr unsigned long IO_CYCLE_MS    = 10UL;   // IO層: センサ読取、ボタン入力
constexpr unsigned long LOGIC_CYCLE_MS = 50UL;   // Logic層: 状態遷移、演算
constexpr unsigned long UI_CYCLE_MS    = 200UL;  // UI層: 画面描画

// --- フィルタ定数 ---
constexpr float FILTER_ALPHA = 0.1f;  // 1次遅れフィルタ係数 (0.0-1.0)

// --- 状態定義 ---
enum State {
  STATE_IDLE,
  STATE_RUN,
  STATE_RESULT
};

// --- グローバルデータ ---
struct GlobalData {
  // データレジスタ群
  float  D_RawPV;        // 生の温度測定値
  float  D_FilteredPV;   // フィルタ後の温度値
  double D_Sum;          // 積算値 (平均計算用)
  long   D_Count;        // サンプル数
  float  D_Average;      // 計算された平均温度

  // 内部リレー群
  State  M_CurrentState;   // 現在の状態
  bool   M_BtnA_Pressed;   // ボタンA立ち上がりエッジ検出
  bool   M_BtnA_Prev;      // ボタンA前回値 (エッジ検出用)
};

// 外部参照宣言
extern GlobalData G;
extern Adafruit_MAX31855 thermocouple;

// タスク関数宣言
void IO_Task();
void Logic_Task();
void UI_Task();

#endif
```

### 2. src/Tasks.cpp（新規作成）

`src` フォルダを右クリック → 「**新しいファイル**」→ ファイル名「**Tasks.cpp**」

```cpp
#include "Global.h"

// グローバル変数の実体
GlobalData G;
Adafruit_MAX31855 thermocouple(MAX31855_CS);

// ========== IO Layer (10ms周期) ==========
void IO_Task() {
  G.D_RawPV = thermocouple.readCelsius();

  if (!isnan(G.D_RawPV)) {
    // 1次遅れフィルタ: y[n] = y[n-1] * (1-α) + x[n] * α
    G.D_FilteredPV = (G.D_FilteredPV * (1.0f - FILTER_ALPHA)) + (G.D_RawPV * FILTER_ALPHA);
  }

  M5.update();
  bool btnNow = M5.BtnA.isPressed();
  if (btnNow && !G.M_BtnA_Prev) {
    G.M_BtnA_Pressed = true;
  }
  G.M_BtnA_Prev = btnNow;
}

// ========== Logic Layer (50ms周期) ==========
void Logic_Task() {
  if (G.M_BtnA_Pressed) {
    G.M_BtnA_Pressed = false;

    switch (G.M_CurrentState) {
      case STATE_IDLE:
        G.D_Sum           = 0.0;
        G.D_Count         = 0;
        G.M_CurrentState  = STATE_RUN;
        break;

      case STATE_RUN:
        if (G.D_Count > 0) {
          G.D_Average = G.D_Sum / G.D_Count;
        } else {
          G.D_Average = G.D_FilteredPV;
        }
        G.M_CurrentState = STATE_RESULT;
        break;

      case STATE_RESULT:
        G.M_CurrentState = STATE_IDLE;
        break;
    }
  }

  if (G.M_CurrentState == STATE_RUN) {
    if (!isnan(G.D_FilteredPV)) {
      G.D_Sum += G.D_FilteredPV;
      G.D_Count++;
    }
  }
}

// ========== UI Layer (200ms周期) ==========
void UI_Task() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);

  M5.Lcd.print("STATE: ");
  switch (G.M_CurrentState) {
    case STATE_IDLE:   M5.Lcd.println("IDLE");   break;
    case STATE_RUN:    M5.Lcd.println("RUN");    break;
    case STATE_RESULT: M5.Lcd.println("RESULT"); break;
  }
  M5.Lcd.println();

  M5.Lcd.print("Temp: ");
  if (isnan(G.D_FilteredPV)) {
    M5.Lcd.println("ERROR");
  } else {
    M5.Lcd.print(G.D_FilteredPV, 1);
    M5.Lcd.println(" C");
  }
  M5.Lcd.println();

  if (G.M_CurrentState == STATE_RUN) {
    M5.Lcd.print("Samples: ");
    M5.Lcd.println(G.D_Count);
  } else if (G.M_CurrentState == STATE_RESULT) {
    M5.Lcd.print("Average: ");
    M5.Lcd.print(G.D_Average, 1);
    M5.Lcd.println(" C");
  }

  M5.Lcd.setCursor(0, 220);
  M5.Lcd.setTextSize(1);
  M5.Lcd.println("[BtnA] Start/Stop/Reset");
}
```

### 3. src/main.cpp（内容を置き換え）

既存の `src/main.cpp` の内容を**全て削除**し、以下に置き換え:

```cpp
#include "Global.h"

unsigned long T_IO_Last    = 0;
unsigned long T_Logic_Last = 0;
unsigned long T_UI_Last    = 0;

// グローバルデータの初期化
void initGlobalData() {
  G.M_CurrentState  = STATE_IDLE;
  G.D_FilteredPV    = 0.0;
  G.D_Sum           = 0.0;
  G.D_Count         = 0;
  G.D_Average       = 0.0;
  G.M_BtnA_Pressed  = false;
  G.M_BtnA_Prev     = false;
}

void setup() {
  M5.begin();
  M5.Power.begin();
  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Temperature Eval Tool");
  M5.Lcd.println("Phase 1 - Starting...");
  M5.Lcd.println("");
  M5.Lcd.println("Checking MAX31855...");

  delay(1000);

  // 初期化
  initGlobalData();

  // MAX31855接続確認
  float testTemp = thermocouple.readCelsius();
  if (isnan(testTemp)) {
    M5.Lcd.println("ERROR: MAX31855 not found!");
    M5.Lcd.println("Check wiring!");
    while(1) { delay(1000); }
  }

  // フィルタ初期値を実測値で初期化 (収束時間短縮)
  G.D_FilteredPV = testTemp;

  M5.Lcd.println("MAX31855 OK");
  delay(1000);
  M5.Lcd.fillScreen(BLACK);
}

void loop() {
  unsigned long now = millis();

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

1. 画面に「**Temperature Eval Tool**」と表示される
2. 「**Checking MAX31855...**」→「**MAX31855 OK**」と表示される
3. 「**STATE: IDLE**」と表示される
4. 「**Temp: XX.X C**」に室温（20〜30℃程度）が表示される

### 2. 状態遷移の確認

1. M5Stack 正面下部の**左ボタン（BtnA）**を押す
2. 「**STATE: RUN**」に切り替わる
3. 「**Samples:**」の数値がカウントアップすることを確認
4. もう一度 **BtnA** を押す
5. 「**STATE: RESULT**」に切り替わり、「**Average: XX.X C**」が表示される
6. もう一度 **BtnA** を押す
7. 「**STATE: IDLE**」に戻ることを確認

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

IO_Task    (10ms)  → センサ読込 → フィルタ → ボタン読込
Logic_Task (50ms)  → 状態遷移 → 積算（RUN中のみ）
UI_Task    (200ms) → 画面描画
```

#### 状態遷移図

```
[IDLE] ──BtnA──> [RUN] ──BtnA──> [RESULT] ──BtnA──> [IDLE]
  待機              計測中            平均値表示
  D_Sum=0          D_Sum積算         D_Average表示
  D_Count=0        D_Count++         (固定値)
```

#### データフロー

```
MAX31855 (SPI)
    ↓ 10ms周期
D_RawPV (生値)
    ↓ ローパスフィルタ
D_FilteredPV (フィルタ後)
    ↓ RUN状態のみ
D_Sum += D_FilteredPV
D_Count++
    ↓ BtnA押下でRESULT遷移
D_Average = D_Sum / D_Count
```

### 変数一覧

| 変数名                | 型      | 説明                      |
| ------------------ | ------ | ----------------------- |
| **D_RawPV**        | float  | センサから読み取った生の温度値         |
| **D_FilteredPV**   | float  | フィルタ処理後の温度値（画面表示・積算に使用） |
| **D_Sum**          | double | 温度の累積値（平均値計算用、64bit）    |
| **D_Count**        | long   | サンプル数（32bit整数）          |
| **D_Average**      | float  | 計算された平均値（RESULT状態で表示）   |
| **M_CurrentState** | enum   | 現在の状態（IDLE/RUN/RESULT）  |
| **M_BtnA_Pressed** | bool   | ボタン押下フラグ（エッジ検出済み）       |
| **M_BtnA_Prev**    | bool   | ボタン前回状態（エッジ検出用）         |

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
**最終更新**: 2026年2月4日
