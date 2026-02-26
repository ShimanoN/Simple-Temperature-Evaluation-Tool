#include <unity.h>
#include "MeasurementCore.h"

void test_initial_state(void) {
  MeasurementCore c;
  TEST_ASSERT_EQUAL(MeasurementCore::IDLE, c.getState());
  TEST_ASSERT_EQUAL(0, c.getCount());
}

void test_run_and_accumulate(void) {
  MeasurementCore c;
  c.onButtonPress(); // IDLE->RUN
  TEST_ASSERT_EQUAL(MeasurementCore::RUN, c.getState());

  c.tick(10.0f);
  c.tick(20.0f);
  TEST_ASSERT_EQUAL(2, c.getCount());
  TEST_ASSERT_EQUAL_DOUBLE(30.0, c.getSum());
}

void test_result_average(void) {
  MeasurementCore c;
  c.onButtonPress(); // RUN
  c.tick(5.0f);
  c.tick(15.0f);
  c.onButtonPress(); // RUN->RESULT
  TEST_ASSERT_EQUAL(MeasurementCore::RESULT, c.getState());
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 10.0f, c.getAverage());
}

void test_full_cycle(void) {
  MeasurementCore c;
  c.onButtonPress(); // RUN
  c.tick(2.0f);
  c.onButtonPress(); // RESULT
  TEST_ASSERT_EQUAL(MeasurementCore::RESULT, c.getState());
  c.onButtonPress(); // RESULT->IDLE
  TEST_ASSERT_EQUAL(MeasurementCore::IDLE, c.getState());
}

// ========== Phase 3: updateAlarmFlags() テスト構成情報 ===========================
// 
// 注: updateAlarmFlags() は Global.h で宣言されており、
//    platformio test -e native 環境との統合に複雑性があります。
// 
// 推奨テスト方法:
// 1️⃣ 動作検証テスト（推奨）
//    - M5Stack にアップロード後、シリアルモニタで出力を確認
//    - IDLE 画面で温度を上限/下限に調整して、アラーム動作を確認
//    - TROUBLESHOOTING.md 「症状1」を参照
//
// 2️⃣ ユニットテスト（将来対応）
//    - updateAlarmFlags() を独立した単体テスト化する
//    - Global.h への依存を最小化
//    - ビルドツールチェーン全体を見直し
//
// 【テストケース一覧】
// TC1: HI アラーム トリガー（temp >= threshold, 2kHz音）
// TC2: HI アラーム クリア（temp < threshold - hysteresis）
// TC3: LO アラーム トリガー（temp <= threshold, 1kHz音）
// TC4: LO アラーム クリア（temp > threshold + hysteresis）
// TC5: 同時アラーム（HI と LO が同時に発火）
// TC6: NaN 入力は判定対象外（処理スキップ）
// TC7: 無限大入力（実装依存の動作）
// TC8: ちょうど閾値（分岐点での動作確認）
// TC9: ヒステリシス=0（即座にクリア）
// TC10: 大きなヒステリシス（長時間ON状態）
//
// 手動テスト手順 → docs/TROUBLESHOOTING.md「動作検証チェックリスト」参照

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_initial_state);
  RUN_TEST(test_run_and_accumulate);
  RUN_TEST(test_result_average);
  RUN_TEST(test_full_cycle);
  return UNITY_END();
}