#include "Global.h"

// タイマー用変数（このファイルだけで使うので extern しない）
unsigned long T_IO_Last    = 0;
unsigned long T_Logic_Last = 0;
unsigned long T_UI_Last    = 0;

void setup() {
  M5.begin();
  M5.Power.begin();
  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Temperature Eval Tool");
  M5.Lcd.println("Refactored Version");
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
    while(1) { delay(1000); }
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
