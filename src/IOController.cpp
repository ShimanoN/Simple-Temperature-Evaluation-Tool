#include "IOController.h"
#include <M5Stack.h>
#include <cmath>

IOController::IOController(uint8_t csPin, float filterAlpha)
  : thermocouple_(csPin),
    filterAlpha_(filterAlpha),
    rawPV_(NAN),
    filteredPV_(0.0f),
    lastReadTime_(0),
    btnAPrev_(false),
    btnAPressed_(false)
{}

void IOController::begin() {
  filteredPV_ = 0.0f;
  rawPV_ = NAN;
  lastReadTime_ = 0;
  btnAPrev_ = false;
  btnAPressed_ = false;
}

void IOController::tick() {
  // 温度読み取りは500msに1回
  unsigned long now = millis();
  if (now - lastReadTime_ >= 500) {
    lastReadTime_ = now;
    rawPV_ = thermocouple_.readCelsius();
  }

  // 1次遅れフィルタ: y[n] = y[n-1] * (1-α) + x[n] * α
  if (!std::isnan(rawPV_)) {
    filteredPV_ = (filteredPV_ * (1.0f - filterAlpha_)) + (rawPV_ * filterAlpha_);
  }

  // ボタン A の立ち上がりエッジ検出
  M5.update();
  bool btnNow = M5.BtnA.isPressed();
  if (btnNow && !btnAPrev_) {
    btnAPressed_ = true;
  }
  btnAPrev_ = btnNow;
}

bool IOController::isButtonAPressed() {
  bool pressed = btnAPressed_;
  btnAPressed_ = false;  // 読み取り後にクリア
  return pressed;
}

bool IOController::isTemperatureValid() const {
  return !std::isnan(rawPV_);
}

float IOController::getRawTemperature() const {
  return rawPV_;
}
