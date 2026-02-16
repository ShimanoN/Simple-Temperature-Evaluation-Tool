#include "DisplayManager.h"
#include <cmath>

DisplayManager::DisplayManager()
  : lastState_(STATE_IDLE)
{}

void DisplayManager::begin() {
  M5.Lcd.fillScreen(BLACK);
  lastState_ = STATE_IDLE;
}

void DisplayManager::update(State state, float temperature, int count, float average) {
  // 状態変化時に画面をクリア
  if (state != lastState_) {
    clearScreen();
    lastState_ = state;
  }

  // 画面描画
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, BLACK);

  // 状態ラベル
  drawStateLabel(state);
  M5.Lcd.println();

  // 温度表示
  drawTemperature(temperature);
  M5.Lcd.println();

  // サンプル数または平均値
  if (state == STATE_RUN) {
    drawSampleCount(count);
  } else if (state == STATE_RESULT) {
    drawAverage(average);
  } else {
    M5.Lcd.println("                    ");
  }

  // ボタン説明
  drawButtonHelp();
}

void DisplayManager::clearScreen() {
  M5.Lcd.fillScreen(BLACK);
}

void DisplayManager::drawStateLabel(State state) {
  M5.Lcd.print("STATE: ");
  switch (state) {
    case STATE_IDLE:   M5.Lcd.println("IDLE  ");   break;
    case STATE_RUN:    M5.Lcd.println("RUN   ");   break;
    case STATE_RESULT: M5.Lcd.println("RESULT");   break;
  }
}

void DisplayManager::drawTemperature(float temperature) {
  M5.Lcd.print("Temp: ");
  if (std::isnan(temperature)) {
    M5.Lcd.println("ERROR   ");
  } else {
    M5.Lcd.print(temperature, 1);
    M5.Lcd.println(" C      ");
  }
}

void DisplayManager::drawSampleCount(int count) {
  M5.Lcd.print("Samples: ");
  M5.Lcd.print(count);
  M5.Lcd.println("      ");
}

void DisplayManager::drawAverage(float average) {
  M5.Lcd.print("Average: ");
  M5.Lcd.print(average, 1);
  M5.Lcd.println(" C      ");
}

void DisplayManager::drawButtonHelp() {
  M5.Lcd.setCursor(0, 220);
  M5.Lcd.setTextSize(1);
  M5.Lcd.println("[BtnA] Start/Stop/Reset");
}
