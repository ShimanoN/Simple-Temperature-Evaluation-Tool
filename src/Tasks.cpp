#include "Global.h"
#include "IOController.h"
#include "MeasurementCore.h"
#include "DisplayManager.h"

// 実体の作成（ここで場所を確保する）
GlobalData G;

// 各レイヤーのコントローラー
static IOController g_io(MAX31855_CS, FILTER_ALPHA);
static MeasurementCore g_measure;
static DisplayManager g_display;

// ========== IO Layer (10ms周期) ==========
void IO_Task() {
  g_io.tick();
  
  // グローバル変数に結果を同期（後方互換性のため）
  G.D_FilteredPV = g_io.getFilteredTemperature();
  
  if (g_io.isButtonAPressed()) {
    G.M_BtnA_Pressed = true;
  }
}

// ========== Logic Layer (50ms周期) ==========

void Logic_Task() {
  // MeasurementCore に処理を委譲
  if (G.M_BtnA_Pressed) {
    G.M_BtnA_Pressed = false;
    g_measure.onButtonPress();
  }

  // 周期 tick（RUN 中なら積算される）
  g_measure.tick(G.D_FilteredPV);

  // MeasurementCore の状態をグローバルに反映
  switch (g_measure.getState()) {
    case MeasurementCore::IDLE:
      G.M_CurrentState = STATE_IDLE; break;
    case MeasurementCore::RUN:
      G.M_CurrentState = STATE_RUN; break;
    case MeasurementCore::RESULT:
      G.M_CurrentState = STATE_RESULT; break;
  }

  // 集計値を同期
  G.D_Sum     = g_measure.getSum();
  G.D_Count   = g_measure.getCount();
  G.D_Average = g_measure.getAverage();
}

// ========== UI Layer (200ms周期) ==========
void UI_Task() {
  g_display.update(
    G.M_CurrentState,
    G.D_FilteredPV,
    G.D_Count,
    G.D_Average
  );
}

// ========== 初期化 ==========
void initTasks() {
  g_io.begin();
  g_display.begin();
}

float getInitialTemperature() {
  // 初期化時のセンサ存在チェックにはフィルタ後の値ではなく
  // 生の読み取り値の有効性を用いる。フィルタは初期値を非NaNにするため
  // 誤検出を招くため、最初の有効なreadCelsius()が得られるまでNaNを返す。
  if (g_io.isTemperatureValid()) {
    return g_io.getRawTemperature();
  }
  return NAN;
}
