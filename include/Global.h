#pragma once

#include <M5Stack.h>
#include <Adafruit_MAX31855.h>
#include <EEPROM.h>  // EEPROM設定保存用
#include <cmath>   // sqrt, isnan など数学関数用
#include <cfloat>  // FLT_MAX, FLT_MIN など
#include "EEPROMManager.h"  // EEPROM操作集約

// ── ピン定義 ──────────────────────────────────────────────────────────────────
// ハードウェアSPI (SCK=GPIO18, MISO=GPIO19) でLCDとバス共有。
// MAX31855 の CS ピンのみ個別に割り当て。
constexpr uint8_t MAX31855_CS = 5;

// ── タイマー周期 [ms] ─────────────────────────────────────────────────────────
// millis() のオーバーフローは unsigned 演算の性質で自動吸収。
constexpr unsigned long IO_CYCLE_MS         =  10UL;  // IO層    : センサ読取・ボタン入力
constexpr unsigned long LOGIC_CYCLE_MS      =  50UL;  // Logic層 : 状態遷移・演算
constexpr unsigned long UI_CYCLE_MS         = 200UL;  // UI層    : 画面描画
constexpr unsigned long TC_READ_INTERVAL_MS = 500UL;  // MAX31855 変換完了待ち間隔

// ── フィルタ定数 ──────────────────────────────────────────────────────────────
constexpr float FILTER_ALPHA = 0.1f;  // 1次遅れフィルタ係数 (0.0〜1.0)
// ── UI表示定数（液晶座標・テキストサイズ）────────────────────────────────────
namespace UI {
  // テキストサイズ定義
  constexpr uint8_t TEXTSIZE_TITLE    = 1;   // タイトル・ラベル用（小）
  constexpr uint8_t TEXTSIZE_VALUE    = 2;   // 値表示用（大）
  constexpr uint8_t TEXTSIZE_GUIDE    = 1;   // ボタンガイド用（小）
  
  // 液晶座標（Y座標: ピクセル）
  namespace PosY {
    constexpr uint16_t TITLE         = 0;    // 画面最上部
    constexpr uint16_t STATE_LABEL   = 10;   // 状態表示
    constexpr uint16_t TEMP_LABEL    = 50;   // 温度ラベル
    constexpr uint16_t TEMP_VALUE    = 90;   // 温度値
    constexpr uint16_t GUIDE_BUTTON  = 140;  // ボタン操作ガイド
    constexpr uint16_t BUTTON_INFO   = 220;  // 下部ボタン情報（固定）
  }
  
  // 液晶のMax座標
  constexpr uint16_t LCD_WIDTH       = 320;  // 横ピクセル
  constexpr uint16_t LCD_HEIGHT      = 240;  // 縦ピクセル
  
  // デバッグ表示用オプション
  constexpr bool SHOW_ALARM_SETTINGS_ON_IDLE = true;  // IDLE画面でアラーム設定値表示
  constexpr bool SHOW_DEBUG_LOGS              = true;  // シリアル詳細ログ出力
}
// ── Phase 3: アラーム定数 ────────────────────────────────────────────────────
constexpr float HI_ALARM_TEMP    = 600.0f;  // 上限 [°C]
constexpr float LO_ALARM_TEMP    = 400.0f;  // 下限 [°C]
constexpr float ALARM_HYSTERESIS =   5.0f;  // ヒステリシス幅 [°C]
constexpr float SETTING_STEP     =   5.0f;  // 設定時の調整幅 [°C]

// ── EEPROM 設定 ────────────────────────────────────────────────────────────────
// AlarmSettings 構造体と関連定数は EEPROMManager.h で定義

// ── 初期化・シリアル通信定数 ──────────────────────────────────────────────────
// シリアルボーレート (UART通信速度)
constexpr uint32_t SERIAL_BAUD_RATE = 115200UL;

// 初期化時の MAX31855 センサ安定化・リトライ設定
constexpr unsigned long SETUP_SENSOR_DELAY_MS      = 200UL;   // パワーオン後の初期化待ち
constexpr unsigned long SETUP_RETRY_INTERVAL_MS    = 500UL;   // リトライ間隔
constexpr unsigned long SETUP_FINAL_DELAY_MS       = 1000UL;  // 最終確認後の待機時間
constexpr int           MAX_SETUP_RETRIES          = 5;       // 最大リトライ回数

// ── アラーム音声設定（Speaker制御）────────────────────────────────────────────
// HI/LO アラームは異なる周波数で区別可能
constexpr uint16_t ALARM_HI_FREQUENCY_HZ  = 2000U;  // 上限アラーム: 2kHz（高い音）
constexpr uint16_t ALARM_LO_FREQUENCY_HZ  = 1000U;  // 下限アラーム: 1kHz（低い音）
constexpr uint16_t ALARM_SOUND_DURATION_MS = 500U;  // アラーム音継続時間 (ms)

// ── デバッグ・監視タイマー ────────────────────────────────────────────────────
// IO_Task 内での定期的なアラーム状態ログ出力
constexpr unsigned long ALARM_DEBUG_LOG_INTERVAL_MS = 5000UL;  // 5秒ごとにデバッグ出力

// ── LCD 座標定数（UI描画の高度な制御）─────────────────────────────────────────
namespace UI {
  // LCD描画座標（ピクセル: RESULT ページ2 の複数列レイアウト用）
  namespace LayoutX {
    constexpr uint16_t LEFT_COL   = 0;     // 左列（StdDev, Max等）
    constexpr uint16_t RIGHT_COL  = 160;   // 右列（Range, Min等）
  }
  
  namespace LayoutY {
    constexpr uint16_t RESULT_AVG_LABEL  = 120;   // RESULT Page1: 平均値ラベル行
    constexpr uint16_t RESULT_AVG_VALUE  = 150;   // RESULT Page1: 平均値表示行
    constexpr uint16_t RESULT_TOP_ROW    = 30;    // RESULT Page2: 上段（StdDev/Range）
    constexpr uint16_t RESULT_MID_ROW    = 60;    // RESULT Page2: 中段（値表示）
    constexpr uint16_t RESULT_STAT_LABEL = 110;   // RESULT Page2: 下段ラベル（Max/Min）
    constexpr uint16_t RESULT_STAT_VALUE = 140;   // RESULT Page2: 下段値
  }
}

// ── 状態定義 ──────────────────────────────────────────────────────────────────
// enum class により名前がグローバル名前空間に漏れない (State::IDLE のようにアクセス)
enum class State : uint8_t {
  IDLE,           // 待機中
  RUN,            // 計測中
  RESULT,         // 結果表示中
  ALARM_SETTING   // アラーム閾値設定中（Phase 3拡張）
};

// ── グローバルデータ構造体 ─────────────────────────────────────────────────────
// 命名規則 (PLC対応):
//   D_ → データレジスタ相当 (PLC の D デバイス)
//   M_ → 内部リレー相当     (PLC の M デバイス)
struct GlobalData {
  // データレジスタ群
  float  D_RawPV;        // 生の温度測定値 [°C]
  float  D_FilteredPV;   // フィルタ後の温度値 [°C]
  double D_Sum;          // 積算値 (平均計算用)
  long   D_Count;        // サンプル数
  float  D_Average;      // 平均温度 [°C]

  // Phase 2: 統計機能
  float  D_Max;          // 計測期間中の最高温度 [°C]
  float  D_Min;          // 計測期間中の最低温度 [°C]
  float  D_Range;        // Max - Min（温度変動幅）[°C]
  double D_M2;           // Welford法 二乗偏差累積
  float  D_StdDev;       // 標準偏差 σ（ばらつきの大きさ）[°C]

  // 内部リレー群
  State  M_CurrentState;  // 現在の状態
  bool   M_BtnA_Pressed;  // ボタンA 立ち上がりエッジ検出フラグ
  bool   M_BtnB_Pressed;  // ボタンB 立ち上がりエッジ検出フラグ (ページング・設定進入用)
  bool   M_BtnC_Pressed;  // ボタンC 立ち上がりエッジ検出フラグ（設定用：-値）
  int    M_ResultPage;    // RESULT画面のページ番号（0 or 1）

  // Phase 3: アラーム機能
  bool   M_HiAlarm;       // 上限アラーム中フラグ
  bool   M_LoAlarm;       // 下限アラーム中フラグ

  // Phase 3 拡張: 動的アラーム閾値（EEPROM保存）
  float  D_HI_ALARM_CURRENT;  // 現在の上限値 [°C]（EEPROM読み込み値）
  float  D_LO_ALARM_CURRENT;  // 現在の下限値 [°C]（EEPROM読み込み値）
  int    M_SettingIndex;      // 設定モード時：0=HI側, 1=LO側
  
  // M_BtnA_Prev は IO_Task の実装詳細のため static ローカル変数へ移動
};
// ── 外部宣言 ──────────────────────────────────────────────────────────────────
// 実体は Tasks.cpp で確保
extern GlobalData        G;
extern Adafruit_MAX31855 thermocouple;

// ── 関数宣言 ──────────────────────────────────────────────────────────────────
void initGlobalData();  // グローバルデータ初期化 (Tasks.cpp)
void IO_Task();
void Logic_Task();
void UI_Task();

// EEPROM初期化ラッパー（Tasks.cpp で実装）
// EEPROMManager を使用してEEPROMからGlobalDataに設定値を反映
void EEPROM_LoadToGlobal();

// アラーム判定ロジック（テスト可能）
void updateAlarmFlags(float currentTemp, float hiThreshold, float loThreshold,
                      float hysteresis, bool& hiFlag, bool& loFlag);

