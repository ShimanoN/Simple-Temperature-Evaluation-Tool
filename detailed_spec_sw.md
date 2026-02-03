# 簡易温度評価ツール - 詳細仕様書（ソフトウェア編）

参照元: basic_spec.md  
関連: detailed_spec_hw.md（組み立て・精度確認・現場運用）  
開発環境: Antigravity (VSCode + Gemini + PlatformIO IDE)

---

## 開発環境セットアップ

### 前提

Antigravity（VSCode + Gemini）が既に導入されている前提です。  
PlatformIO IDE の拡張機能を追加し、プロジェクトを作成します。

### 手順

1. VSCode の拡張機能検索で `PlatformIO IDE` を検索してインストール
2. インストール完了後、左サイドバーに PlatformIO のアイコン（アンテナ形状）が表示されることを確認
3. PlatformIO アイコン → `New Project` をクリック
4. 以下で設定して `Create` をクリック

| 項目 | 設定値 |
|---|---|
| Project Name | temp_eval_tool |
| Board | M5Stack-Core-ESP32 |
| Framework | Arduino |

5. プロジェクト生成後、`platformio.ini` を以下の内容で書き換えて保存

```ini
[env:m5stack]
platform = espressif32
board = m5stack
framework = arduino
upload_speed = 921600
lib_deps =
    M5Stack
    Adafruit MAX31855 library
```

6. 保存後、画面下部に「Libraries are missing」と出る場合は `Install` をクリック
7. ライブラリインストール完了を確認

> これで開発環境は完了です。あとは `src/main.cpp` にコードを貼り付けて書き込むだけです。

---

## プロジェクト構成

```
temp_eval_tool/
├── platformio.ini          ← 上記で編集済み
├── src/
│   └── main.cpp            ← 下記のコードを貼り付ける
├── lib/                    ← 自動管理（操作不要）
├── include/                ← 本プロジェクトでは使用しない
└── .pio/                   ← PlatformIO内部ファイル（操作不要）
```

編集するのは `platformio.ini` と `src/main.cpp` の2ファイルのみです。

---

## ソースコード

`src/main.cpp` の中身を全て削除してから、以下のコードを貼り付けてください。

```cpp
/*
 * 簡易温度評価ツール Phase 1
 * 対象: M5Stack Basic v2.7 + Proto Module + MAX31855
 * 測定範囲: -200〜1350℃（K型熱電対使用時）
 *
 * アーキテクチャ: 3層タスク構成
 *   IO_Task    (10ms)  - センサ読込・フィルタ・ボタン読込
 *   Logic_Task (50ms)  - 状態遷移・積算演算
 *   UI_Task    (200ms) - LCD表示
 *
 * 状態遷移: IDLE → RUN → RESULT → IDLE（BtnA で順次遷移）
 */
#include <M5Stack.h>
#include <Adafruit_MAX31855.h>

// ピン定義（Proto Module経由）
#define MAX31855_CS   5
#define MAX31855_SCK  18
#define MAX31855_MISO 23

// タスク周期定義（ミリ秒）
#define IO_CYCLE    10
#define LOGIC_CYCLE 50
#define UI_CYCLE    200

// 状態定義
enum State {
  STATE_IDLE,
  STATE_RUN,
  STATE_RESULT
};

// グローバル変数
struct {
  float  D_RawPV;        // センサ生値
  float  D_FilteredPV;   // フィルタ後PV（画面表示・積算に使用）
  double D_Sum;          // 積算値（オーバーフロー対策で64bit）
  long   D_Count;        // サンプル数
  float  D_Average;      // 平均値（RESULT状態で表示）

  State  M_CurrentState; // 現在の状態
  bool   M_BtnA_Pressed; // ボタン押下フラグ（エッジ検出済み）
  bool   M_BtnA_Prev;    // ボタン前回状態（エッジ検出用）
} G;

// タイマー変数
unsigned long T_IO_Last    = 0;
unsigned long T_Logic_Last = 0;
unsigned long T_UI_Last    = 0;

// MAX31855インスタンス
Adafruit_MAX31855 thermocouple(MAX31855_SCK, MAX31855_CS, MAX31855_MISO);

// --- 関数宣言 ---
void IO_Task();
void Logic_Task();
void UI_Task();

// ========================================
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
  G.M_CurrentState  = STATE_IDLE;
  G.D_FilteredPV    = 0.0;
  G.D_Sum           = 0.0;
  G.D_Count         = 0;
  G.M_BtnA_Pressed  = false;
  G.M_BtnA_Prev     = false;

  // MAX31855接続確認
  float testTemp = thermocouple.readCelsius();
  if (isnan(testTemp)) {
    M5.Lcd.println("ERROR: MAX31855 not found!");
    M5.Lcd.println("Check wiring!");
    while(1) { delay(1000); }  // 無限ループで停止
  }

  M5.Lcd.println("MAX31855 OK");
  delay(1000);
  M5.Lcd.fillScreen(BLACK);
}

void loop() {
  unsigned long now = millis();

  if (now - T_IO_Last >= IO_CYCLE) {
    T_IO_Last = now;
    IO_Task();
  }
  if (now - T_Logic_Last >= LOGIC_CYCLE) {
    T_Logic_Last = now;
    Logic_Task();
  }
  if (now - T_UI_Last >= UI_CYCLE) {
    T_UI_Last = now;
    UI_Task();
  }
}

// ========== IO Layer (10ms周期) ==========
void IO_Task() {
  // センサ読込（SPI通信）
  G.D_RawPV = thermocouple.readCelsius();

  // ローパスフィルタ（一次遅れ）
  // NaN の場合はフィルタ値を維持する
  if (!isnan(G.D_RawPV)) {
    G.D_FilteredPV = (G.D_FilteredPV * 0.9) + (G.D_RawPV * 0.1);
  }

  // ボタン読込と立ち上がりエッジ検出
  M5.update();
  bool btnNow = M5.BtnA.isPressed();
  if (btnNow && !G.M_BtnA_Prev) {
    G.M_BtnA_Pressed = true;
  }
  G.M_BtnA_Prev = btnNow;
}

// ========== Logic Layer (50ms周期) ==========
void Logic_Task() {
  // 状態遷移
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
        }
        G.M_CurrentState = STATE_RESULT;
        break;

      case STATE_RESULT:
        G.M_CurrentState = STATE_IDLE;
        break;
    }
  }

  // 積算処理（RUN状態のみ）
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

  // 状態表示
  M5.Lcd.print("STATE: ");
  switch (G.M_CurrentState) {
    case STATE_IDLE:   M5.Lcd.println("IDLE");   break;
    case STATE_RUN:    M5.Lcd.println("RUN");    break;
    case STATE_RESULT: M5.Lcd.println("RESULT"); break;
  }
  M5.Lcd.println();

  // 温度表示
  M5.Lcd.print("Temp: ");
  if (isnan(G.D_FilteredPV)) {
    M5.Lcd.println("ERROR");
  } else {
    M5.Lcd.print(G.D_FilteredPV, 1);
    M5.Lcd.println(" C");
  }
  M5.Lcd.println();

  // サンプル数（RUN中）or 平均値（RESULT）
  if (G.M_CurrentState == STATE_RUN) {
    M5.Lcd.print("Samples: ");
    M5.Lcd.println(G.D_Count);
  } else if (G.M_CurrentState == STATE_RESULT) {
    M5.Lcd.print("Average: ");
    M5.Lcd.print(G.D_Average, 1);
    M5.Lcd.println(" C");
  }

  // 操作ガイド
  M5.Lcd.setCursor(0, 220);
  M5.Lcd.setTextSize(1);
  M5.Lcd.println("[BtnA] Start/Stop/Reset");
}
```

---

## 書き込み手順

1. M5Stack を USBケーブル（Type-C）で PC に接続
2. VSCode で左サイドバーの PlatformIO アイコンをクリック
3. QUICK ACCESS の **Build**（✓アイコン）をクリック
4. 画面下部タミナルで `[SUCCESS]` が表示されるまで待つ（約30秒〜1分）
5. **Upload**（→アイコン）をクリック
6. タミナルで `[SUCCESS]` が表示されたら書き込み完了

> ボード選択やシリアルポート選択は不要です。PlatformIO が自動検出します。

### 書き込みトラブルシューティング

| エラー | 対処 |
|---|---|
| `Serial port is not detected` | USBケーブルを抜き差し。CP210xドライバの確認 |
| `Upload failed` | リセットボタンと BtnA を同時に押しながら USB 接続（書き込みモード） |
| `#include <Adafruit_MAX31855.h>: No such file` | ライブラリ未インストール。セットアップの手順6〜7を再確認 |
| コンパイル成功だが書き込めない | USBポートを別のものに変える。データ転送対応ケーブルを使用 |
| `platform "espressif32" is not installed` | PlatformIO → Update を実行してプラットフォームをインストール |

---

## 動作確認

書き込み完了後に以下で確認してください。

1. M5Stack が自動再起動し「Temperature Eval Tool」と表示されることを確認
2. 「STATE: IDLE」と表示されることを確認
3. 「Temp: XX.X C」に室温（20〜30℃程度）が表示されることを確認
4. BtnA を押す → 「STATE: RUN」に切り替わり、Samples の数値がカウントアップすることを確認
5. もう一度 BtnA → 「STATE: RESULT」に切り替わり、平均値が表示されることを確認
6. もう一度 BtnA → 「STATE: IDLE」に戻ることを確認

### 平均値の確認

1. 熱電対を室温に安定させる
2. BtnA で RUN開始
3. 30秒以上待つ（サンプル数 600 以上に到達）
4. BtnA で RESULT に切り替え
5. 平均値と IDLE 時の瞬時値の差が ±0.5℃ 以内であることを確認

温度の精度確認は `detailed_spec_hw.md` の「精度確認」を参照してください。

---

## カスタマイズ

以下のパラメータを変更で調整可能です。

| パラメータ | デフォルト | 影響 |
|---|---|---|
| IO_CYCLE | 10ms | IOポーリング周期。小さくすると応答性向上、負荷増加 |
| LOGIC_CYCLE | 50ms | サンプリング周期（50ms = 20サンプル/秒） |
| UI_CYCLE | 200ms | 画面更新周期。小さくすると表示がなめらかだがちらつく |
| フィルタ係数 | 0.9 / 0.1 | 0.9を大きくすると→ノイズ除去強化・追従性低下 |

フィルタ係数の変更例:

```cpp
// デフォルト: ノイズと追従のバランス
G.D_FilteredPV = (G.D_FilteredPV * 0.9) + (G.D_RawPV * 0.1);

// ノイズ除去強化版（追従が遅くなる）
G.D_FilteredPV = (G.D_FilteredPV * 0.95) + (G.D_RawPV * 0.05);
```

---

## 技術参照

### タスク構成

```
loop() が毎回呼ばれる中で、各タスクはタイマーで周期を管理する

IO_Task    (10ms)  → センサ読込 → フィルタ → ボタン読込
Logic_Task (50ms)  → 状態遷移 → 積算（RUN中のみ）
UI_Task    (200ms) → 画面描画
```

### 状態遷移

```
[IDLE] --BtnA--> [RUN] --BtnA--> [RESULT] --BtnA--> [IDLE]
  待機・リセット     計測・積算        平均値表示
```

### 変数一覧

| 変数 | 型 | 説明 |
|---|---|---|
| D_RawPV | float | センサ生値 |
| D_FilteredPV | float | フィルタ後の値（表示・積算に使用） |
| D_Sum | double | 温度累積値（64bitで長時間計測対応） |
| D_Count | long | サンプル数 |
| D_Average | float | 平均値（RESULT状態で表示） |
| M_CurrentState | enum | 現在の状態 |
| M_BtnA_Pressed | bool | ボタン押下フラグ（エッジ検出済み） |
| M_BtnA_Prev | bool | ボタン前回状態（エッジ検出用） |
