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
