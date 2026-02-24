#include "Global.h"

namespace {
  unsigned long T_IO_Last    = 0;
  unsigned long T_Logic_Last = 0;
  unsigned long T_UI_Last    = 0;
}

// ── setup ────────────────────────────────────────────────────────────────────
void setup() {
  M5.begin();
  M5.Power.begin();
  Serial.begin(115200);
  Serial.println("=== Setup start ===");

  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Temperature Eval Tool");
  M5.Lcd.println("Refactored Version");
  M5.Lcd.println();

  initGlobalData();

<<<<<<< HEAD
  // MAX31855接続確認
  // IOControllerが内部的に500msごとに読み取るため、複数回チェック
=======
  // MAX31855 接続確認 (最大5回リトライ)
  // MAX31855 はパワーオン後最低 100ms の安定待ちが必要（データシート p.1）
  delay(200);
  Serial.println("Checking MAX31855...");
  constexpr int MAX_RETRY = 5;
>>>>>>> b45dc54 (refactor: グローバルデータの初期化と状態管理の改善、MAX31855接続確認のメッセージを修正)
  float testTemp = NAN;
  for (int i = 0; i < MAX_RETRY; ++i) {
<<<<<<< HEAD
    // g_ioにtickを呼び出して温度を読み取らせる
    IO_Task();
    testTemp = getInitialTemperature();
    Serial.print("MAX try "); Serial.print(i); Serial.print(" -> ");
    if (isnan(testTemp)) Serial.println("nan"); else Serial.println(testTemp, 3);
=======
    testTemp = thermocouple.readCelsius();
    Serial.printf("  try %d -> %s\n", i,
                  isnan(testTemp) ? "NAN" : String(testTemp, 3).c_str());
>>>>>>> b45dc54 (refactor: グローバルデータの初期化と状態管理の改善、MAX31855接続確認のメッセージを修正)
    if (!isnan(testTemp)) break;
    M5.Lcd.print('.');
    delay(500);
  }

  if (isnan(testTemp)) {
    Serial.println("ERROR: MAX31855 not found after retries");
    M5.Lcd.println();
    M5.Lcd.println("ERROR: MAX31855");
    M5.Lcd.println("Check wiring!");
    // センサ未接続でも動作継続 (UI に ---.- C を表示)
  } else {
    G.D_FilteredPV = testTemp;  // フィルタ初期値を実測値で設定 (収束時間短縮)
    M5.Lcd.println();
    M5.Lcd.println("MAX31855 OK");
    Serial.printf("MAX31855 OK: %.3f C\n", testTemp);
  }

  delay(1000);
  M5.Lcd.fillScreen(BLACK);
  Serial.println("=== Setup complete ===");
}

// ── loop ─────────────────────────────────────────────────────────────────────
void loop() {
  const unsigned long now = millis();

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

