#ifndef GLOBAL_H
#define GLOBAL_H

#include <M5Stack.h>
#include <Adafruit_MAX31855.h>

// --- ピン定義 ---
#define MAX31855_CS   5    // チップセレクトのみ使用（ハードウェアSPIでLCDとバス共有）
// SCK=GPIO18, MISO=GPIO19 はハードウェアSPIが自動管理

// --- タイマー周期 (ms) ---
// millis()オーバーフロー対応済み (unsigned演算の性質を利用)
constexpr unsigned long IO_CYCLE_MS    = 10UL;   // IO層: センサ読取、ボタン入力
constexpr unsigned long LOGIC_CYCLE_MS = 50UL;   // Logic層: 状態遷移、演算
constexpr unsigned long UI_CYCLE_MS    = 200UL;  // UI層: 画面描画

// --- フィルタ定数 ---
constexpr float FILTER_ALPHA = 0.1f;  // 1次遅れフィルタ係数 (0.0-1.0)

// --- 状態定義（セレクタスイッチ） ---
enum State {
  STATE_IDLE,
  STATE_RUN,
  STATE_RESULT
};

// --- グローバルデータ（構造体バインダー） ---
// PLC対応表:
//   D_系 → データレジスタ (PLC の D デバイス相当)
//   M_系 → 内部リレー (PLC の M デバイス相当)
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

// 他のファイルから見えるように宣言（実体はどこか1つのファイルで作る）
extern GlobalData G;
extern Adafruit_MAX31855 thermocouple;

// タスク関数の予約票
void IO_Task();
void Logic_Task();
void UI_Task();

#endif
