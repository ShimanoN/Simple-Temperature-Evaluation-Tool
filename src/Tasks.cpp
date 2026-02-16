#include "Global.h"

// 実体の作成（ここで場所を確保する）
GlobalData G;
Adafruit_MAX31855 thermocouple(MAX31855_SCK, MAX31855_CS, MAX31855_MISO);

// ========== IO Layer (10ms周期) ==========
void IO_Task() {
  G.D_RawPV = thermocouple.readCelsius();

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
    Serial.println("IO_Task - BtnA edge detected");
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
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);

  // Ensure high-contrast text
  M5.Lcd.setTextColor(WHITE, BLACK);

  // Debug: report UI update over serial
  Serial.print("UI_Task - State:");
  switch (G.M_CurrentState) {
    case STATE_IDLE:   Serial.print("IDLE");   break;
    case STATE_RUN:    Serial.print("RUN");    break;
    case STATE_RESULT: Serial.print("RESULT"); break;
  }
  Serial.print(" Temp:");
  if (isnan(G.D_FilteredPV)) Serial.println("nan"); else Serial.println(G.D_FilteredPV, 2);

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
