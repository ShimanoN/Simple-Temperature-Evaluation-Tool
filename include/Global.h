#ifndef GLOBAL_H
#define GLOBAL_H

#include <M5Stack.h>
#include <Adafruit_MAX31855.h>

// --- ピン定義 ---
#define MAX31855_CS   5
#define MAX31855_SCK  18
#define MAX31855_MISO 23

// --- タイマー周期 ---
#define IO_CYCLE    10
#define LOGIC_CYCLE 50
#define UI_CYCLE    200

// --- 状態定義（セレクタスイッチ） ---
enum State {
  STATE_IDLE,
  STATE_RUN,
  STATE_RESULT
};

// --- グローバルデータ（構造体バインダー） ---
struct GlobalData {
  float  D_RawPV;
  float  D_FilteredPV;
  double D_Sum;
  long   D_Count;
  float  D_Average;

  State  M_CurrentState;
  bool   M_BtnA_Pressed;
  bool   M_BtnA_Prev;
};

// 他のファイルから見えるように宣言（実体はどこか1つのファイルで作る）
extern GlobalData G;
extern Adafruit_MAX31855 thermocouple;

// タスク関数の予約票
void IO_Task();
void Logic_Task();
void UI_Task();

#endif
