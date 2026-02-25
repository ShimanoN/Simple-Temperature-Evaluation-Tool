#pragma once

#include <M5Stack.h>
#include <Adafruit_MAX31855.h>
#include <cmath>   // sqrt, isnan など数学関数用
#include <cfloat>  // FLT_MAX, FLT_MIN など

// ── ピン定義 ──────────────────────────────────────────────────────────────────
// ハードウェアSPI (SCK=GPIO18, MISO=GPIO19) でLCDとバス共有。
// MAX31855 の CS ピンのみ個別に割り当て。
constexpr uint8_t MAX31855_CS = 5;

// ── タイマー周期 [ms] ─────────────────────────────────────────────────────────
// millis() のオーバーフローは unsigned 演算の性質で自動吸収。
constexpr unsigned long IO_CYCLE_MS         =  10UL;  // IO層    : センサ読取・ボタン入力
constexpr unsigned long LOGIC_CYCLE_MS      =  50UL;  // Logic層 : 状態遷移・演算
constexpr unsigned long UI_CYCLE_MS         = 200UL;  // UI層    : 画面描画
constexpr unsigned long TC_READ_INTERVAL_MS = 500UL;  // MAX31855 変換完了待ち間隔

// ── フィルタ定数 ──────────────────────────────────────────────────────────────
constexpr float FILTER_ALPHA = 0.1f;  // 1次遅れフィルタ係数 (0.0〜1.0)

// ── 状態定義 ──────────────────────────────────────────────────────────────────
// enum class により名前がグローバル名前空間に漏れない (State::IDLE のようにアクセス)
enum class State : uint8_t {
  IDLE,    // 待機中
  RUN,     // 計測中
  RESULT   // 結果表示中
};

// ── グローバルデータ構造体 ─────────────────────────────────────────────────────
// 命名規則 (PLC対応):
//   D_ → データレジスタ相当 (PLC の D デバイス)
//   M_ → 内部リレー相当     (PLC の M デバイス)
struct GlobalData {
  // データレジスタ群
  float  D_RawPV;        // 生の温度測定値 [°C]
  float  D_FilteredPV;   // フィルタ後の温度値 [°C]
  double D_Sum;          // 積算値 (平均計算用)
  long   D_Count;        // サンプル数
  float  D_Average;      // 平均温度 [°C]

  // Phase 2: 統計機能
  float  D_Max;          // 計測期間中の最高温度 [°C]
  float  D_Min;          // 計測期間中の最低温度 [°C]
  float  D_Range;        // Max - Min（温度変動幅）[°C]
  double D_M2;           // Welford法 二乗偏差累積
  float  D_StdDev;       // 標準偏差 σ（ばらつきの大きさ）[°C]

  // 内部リレー群
  State  M_CurrentState;  // 現在の状態
  bool   M_BtnA_Pressed;  // ボタンA 立ち上がりエッジ検出フラグ
  bool   M_BtnB_Pressed;  // ボタンB 立ち上がりエッジ検出フラグ (ページング用)
  int    M_ResultPage;    // RESULT画面のページ番号（0 or 1）
  // M_BtnA_Prev は IO_Task の実装詳細のため static ローカル変数へ移動
};
// ── 外部宣言 ──────────────────────────────────────────────────────────────────
// 実体は Tasks.cpp で確保
extern GlobalData        G;
extern Adafruit_MAX31855 thermocouple;

// ── 関数宣言 ──────────────────────────────────────────────────────────────────
void initGlobalData();  // グローバルデータ初期化 (Tasks.cpp)
void IO_Task();
void Logic_Task();
void UI_Task();

