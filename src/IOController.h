#ifndef IOCONTROLLER_H
#define IOCONTROLLER_H

#include <Adafruit_MAX31855.h>

// IO層: センサー読み取りとボタン検出を管理
class IOController {
public:
  IOController(uint8_t csPin, float filterAlpha);
  
  void begin();  // 初期化
  void tick();   // 10ms周期で呼び出し
  
  float getFilteredTemperature() const { return filteredPV_; }
  bool isButtonAPressed();  // A ボタンの立ち上がりエッジを検出（1回だけtrue）
  
private:
  Adafruit_MAX31855 thermocouple_;
  float filterAlpha_;
  float rawPV_;
  float filteredPV_;
  unsigned long lastReadTime_;
  bool btnAPrev_;
  bool btnAPressed_;
};

#endif
