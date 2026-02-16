#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <M5Stack.h>
#include "Global.h"

// UI層: LCD画面表示を管理
class DisplayManager {
public:
  DisplayManager();
  
  void begin();  // 初期化
  void update(State state, float temperature, int count, float average);  // 画面更新
  
private:
  State lastState_;
  
  void clearScreen();
  void drawStateLabel(State state);
  void drawTemperature(float temperature);
  void drawSampleCount(int count);
  void drawAverage(float average);
  void drawButtonHelp();
};

#endif
