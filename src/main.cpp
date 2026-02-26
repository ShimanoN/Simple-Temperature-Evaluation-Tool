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
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.println("=== Setup start ===");

  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Temperature Eval Tool");
  M5.Lcd.println("Refactored Version");
  M5.Lcd.println();

  initGlobalData();

  // Phase 3拡張: EEPROM 初期化と設定値読み込み
  Serial.println("Initializing EEPROM...");
  EEPROMManager::init(EEPROM_SIZE);
  EEPROM_LoadToGlobal();  // EEPROMから設定値をロード
  Serial.printf("  HI_ALARM: %.1f C, LO_ALARM: %.1f C\n", 
                G.D_HI_ALARM_CURRENT, G.D_LO_ALARM_CURRENT);

  // MAX31855 接続確認 (最大MAX_SETUP_RETRIES回リトライ)
  // MAX31855 はパワーオン後最低 100ms の安定待ちが必要（データシート p.1）
  delay(SETUP_SENSOR_DELAY_MS);
  Serial.println("Checking MAX31855...");
  float testTemp = NAN;
  for (int i = 0; i < MAX_SETUP_RETRIES; ++i) {
    // g_ioにtickを呼び出して温度を読み取らせる
    IO_Task();
    testTemp = G.D_FilteredPV;
    Serial.printf("  try %d -> %s\n", i,
                  isnan(testTemp) ? "NAN" : String(testTemp, 3).c_str());
    if (!isnan(testTemp)) break;
    M5.Lcd.print('.');
    delay(SETUP_RETRY_INTERVAL_MS);
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

  delay(SETUP_FINAL_DELAY_MS);
  M5.Lcd.fillScreen(BLACK);
  
  // ⚠️ 重要: setup中のIO_Task呼び出しで立てられたアラームフラグをリセット
  // loop()での正常な判定を確保
  G.M_HiAlarm = false;
  G.M_LoAlarm = false;
  Serial.println("[Setup] Alarm flags reset before entering main loop");
  
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

