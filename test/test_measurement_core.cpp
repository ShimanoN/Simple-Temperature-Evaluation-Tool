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

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_initial_state);
  RUN_TEST(test_run_and_accumulate);
  RUN_TEST(test_result_average);
  RUN_TEST(test_full_cycle);
  return UNITY_END();
}