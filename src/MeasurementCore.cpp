#include "MeasurementCore.h"
#include <cmath>

MeasurementCore::MeasurementCore()
  : m_state(IDLE), m_sum(0.0), m_count(0), m_average(NAN) {}

void MeasurementCore::onButtonPress() {
  switch (m_state) {
    case IDLE:
      // RUN開始: 集計初期化
      m_sum = 0.0;
      m_count = 0;
      m_average = NAN;
      m_state = RUN;
      break;

    case RUN:
      // RUN終了 → RESULT に遷移して平均を計算
      if (m_count > 0) {
        m_average = static_cast<float>(m_sum / m_count);
      } else {
        m_average = NAN;
      }
      m_state = RESULT;
      break;

    case RESULT:
      // RESULT → IDLE
      m_state = IDLE;
      break;
  }
}

void MeasurementCore::tick(float filteredPV) {
  if (m_state == RUN) {
    if (!std::isnan(filteredPV)) {
      m_sum += filteredPV;
      ++m_count;
    }
  }
}

MeasurementCore::State MeasurementCore::getState() const { return m_state; }

double MeasurementCore::getSum() const { return m_sum; }

long MeasurementCore::getCount() const { return m_count; }

float MeasurementCore::getAverage() const { return m_average; }

void MeasurementCore::reset() {
  m_state = IDLE;
  m_sum = 0.0;
  m_count = 0;
  m_average = NAN;
}