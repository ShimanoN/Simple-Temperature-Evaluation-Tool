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

  // Debug: print raw reading
  Serial.print("IO_Task - RawPV: ");
  if (isnan(G.D_RawPV)) Serial.println("nan"); else Serial.println(G.D_RawPV, 3);

  if (!isnan(G.D_RawPV)) {
    // 1次遅れフィルタ: y[n] = y[n-1] * (1-α) + x[n] * α
    G.D_FilteredPV = (G.D_FilteredPV * (1.0f - FILTER_ALPHA)) + (G.D_RawPV * FILTER_ALPHA);
  }

  M5.update();
  bool btnNow = M5.BtnA.isPressed();
  if (btnNow && !G.M_BtnA_Prev) {
    G.M_BtnA_Pressed = true;
    Serial.println("*** IO_Task - BtnA EDGE DETECTED! Flag set to TRUE ***");
  }
  // Debug: print raw button states (A,B,C)
  Serial.print("Btns A B C: ");
  Serial.print(M5.BtnA.isPressed()); Serial.print(' ');
  Serial.print(M5.BtnB.isPressed()); Serial.print(' ');
  Serial.print(M5.BtnC.isPressed());
  Serial.print(" | BtnA_Pressed flag: ");
  Serial.println(G.M_BtnA_Pressed ? "TRUE" : "false");
  G.M_BtnA_Prev = btnNow;
}

// ========== Logic Layer (50ms周期) ==========
void Logic_Task() {
  if (G.M_BtnA_Pressed) {
    Serial.println("*** Logic_Task - Processing button press! ***");
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
          // サンプル数0の場合は現在値を使用
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
  // 状態変化を検出して画面をクリア
  static State prevState = STATE_IDLE;
  if (G.M_CurrentState != prevState) {
    M5.Lcd.fillScreen(BLACK);
    prevState = G.M_CurrentState;
  }

  // Debug: report UI update over serial
  Serial.print("UI_Task - State:");
  switch (G.M_CurrentState) {
    case STATE_IDLE:   Serial.print("IDLE");   break;
    case STATE_RUN:    Serial.print("RUN");    break;
    case STATE_RESULT: Serial.print("RESULT"); break;
  }
  Serial.print(" Temp:");
  if (isnan(G.D_FilteredPV)) Serial.println("nan"); else Serial.println(G.D_FilteredPV, 2);

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
  Serial.println("UI_Task - drawn to LCD");
}
