#pragma once

// MeasurementCore: ロジック層（状態遷移・積算・平均）を UI/IO から分離した純粋ロジック
// - グローバル依存を持たないためユニットテストが可能

class MeasurementCore {
public:
  enum State { IDLE = 0, RUN = 1, RESULT = 2 };

  MeasurementCore();

  // ボタン押下イベント（立ち上がり）を通知
  void onButtonPress();

  // 周期的に呼ぶ（RUN 中は filteredPV を積算）
  void tick(float filteredPV);

  // 状態クエリ
  State getState() const;

  // 集計データの取得
  double getSum() const;
  long getCount() const;
  float getAverage() const;

  // リセット（テスト用）
  void reset();

private:
  State   m_state;
  double  m_sum;
  long    m_count;
  float   m_average;
};