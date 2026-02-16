#include "Global.h"
#include <SPI.h>

unsigned long T_IO_Last    = 0;
unsigned long T_Logic_Last = 0;
unsigned long T_UI_Last    = 0;

// グローバルデータの初期化
void initGlobalData() {
  G.M_CurrentState  = STATE_IDLE;
  G.D_FilteredPV    = 0.0;
  G.D_Sum           = 0.0;
  G.D_Count         = 0;
  G.D_Average       = 0.0;  // 初期化漏れ修正
  G.M_BtnA_Pressed  = false;
  G.M_BtnA_Prev     = false;
}

void setup() {
  M5.begin();
  M5.Power.begin();
  Serial.begin(115200);
  Serial.println("Setup start");
  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Temperature Eval Tool");
  M5.Lcd.println("Refactored Version");
  M5.Lcd.println("");
  Serial.println("Checking MAX31855...");

  delay(1000);

  // 初期化
  initGlobalData();

  // MAX31855接続確認
  float testTemp = NAN;
  const int MAX_RETRY = 5;
  for (int i = 0; i < MAX_RETRY; ++i) {
    testTemp = thermocouple.readCelsius();
    // ★SPI復元: ソフトウェアSPIがLCD用ピンを壊すので再初期化
    SPI.begin(18, 19, 23, -1);
    Serial.print("MAX try "); Serial.print(i); Serial.print(" -> ");
    if (isnan(testTemp)) Serial.println("nan"); else Serial.println(testTemp, 3);
    if (!isnan(testTemp)) break;
    M5.Lcd.print('.');
    delay(500);
  }

  if (isnan(testTemp)) {
    Serial.println("MAX31855 not found after retries");
    M5.Lcd.println();
    M5.Lcd.println("ERROR: MAX31855 not found!");
    M5.Lcd.println("Check wiring, VCC (3.3V), GND, CS/SCK/MISO pins");
    // センサ未接続でもシステムは継続動作させる（UI に ERROR 表示）
    G.D_FilteredPV = NAN;
  } else {
    // フィルタ初期値を実測値で初期化 (収束時間短縮)
    G.D_FilteredPV = testTemp;
    M5.Lcd.println("MAX31855 OK");
    Serial.print("MAX31855 OK: "); Serial.println(testTemp, 3);
  }
  delay(1000);
  // SPI復元後に画面クリアして通常UI開始
  SPI.begin(18, 19, 23, -1);
  M5.Lcd.fillScreen(BLACK);
  Serial.println("Setup complete - entering loop");
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
