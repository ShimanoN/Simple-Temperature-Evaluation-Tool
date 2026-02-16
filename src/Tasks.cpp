#include "Global.h"

// 実体の作成（ここで場所を確保する）
GlobalData G;
// ★ハードウェアSPIを使用（LCDとバス共有、CSピンのみで切り替え）
// ソフトウェアSPI(ビットバンギング)はLCDのSPIバスを破壊するため使用不可
Adafruit_MAX31855 thermocouple(MAX31855_CS);

// ========== IO Layer (10ms周期) ==========
void IO_Task() {
  // 温度読み取りは500msに1回（MAX31855の変換時間に合わせて）
  static unsigned long lastTcRead = 0;
  unsigned long now = millis();

  if (now - lastTcRead >= 500) {
    lastTcRead = now;
    G.D_RawPV = thermocouple.readCelsius();
  }

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

#include "MeasurementCore.h"

// ========== Logic Layer (50ms周期) ==========
static MeasurementCore g_measure;

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
  // 状態変化を検出して画面をクリア
  static State prevState = STATE_IDLE;
  if (G.M_CurrentState != prevState) {
    M5.Lcd.fillScreen(BLACK);
    prevState = G.M_CurrentState;
  }



  // 画面描画（状態変化時のクリア後に描画）
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, BLACK);

  M5.Lcd.print("STATE: ");
  switch (G.M_CurrentState) {
    case STATE_IDLE:   M5.Lcd.println("IDLE  ");   break;
    case STATE_RUN:    M5.Lcd.println("RUN   ");   break;
    case STATE_RESULT: M5.Lcd.println("RESULT");   break;
  }
  M5.Lcd.println();

  M5.Lcd.print("Temp: ");
  if (isnan(G.D_FilteredPV)) {
    M5.Lcd.println("ERROR   ");
  } else {
    M5.Lcd.print(G.D_FilteredPV, 1);
    M5.Lcd.println(" C      ");
  }
  M5.Lcd.println();

  if (G.M_CurrentState == STATE_RUN) {
    M5.Lcd.print("Samples: ");
    M5.Lcd.print(G.D_Count);
    M5.Lcd.println("      ");
  } else if (G.M_CurrentState == STATE_RESULT) {
    M5.Lcd.print("Average: ");
    M5.Lcd.print(G.D_Average, 1);
    M5.Lcd.println(" C      ");
  } else {
    M5.Lcd.println("                    ");
  }

  M5.Lcd.setCursor(0, 220);
  M5.Lcd.setTextSize(1);
  M5.Lcd.println("[BtnA] Start/Stop/Reset");
}
