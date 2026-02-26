#ifndef TASKS_H
#define TASKS_H

// ========== IO Layer (10ms周期) =================================================
void IO_Task();

// ========== Logic Layer (50ms周期) =================================================
void Logic_Task();
void handleButtonA();
void handleButtonB();
void handleButtonC();
void updateWelfordStatistics();

// ========== UI Layer (200ms周期) =================================================
void UI_Task();

/**
 * @brief IDLE状態の描画
 * @details リアルタイム温度表示 + 設定値（デバッグ用）
 */
void renderIDLE();

/**
 * @brief RUN状態の描画（計測中）
 * @details リアルタイム温度 + サンプル数を表示
 */
void renderRUN();

/**
 * @brief RESULT状態の描画（ページング対応）
 * @details Page 0: 最新値 + 平均値, Page 1: 標準偏差 + 範囲 / Max + Min
 */
void renderRESULT();

/**
 * @brief ALARM_SETTING状態の描画
 * @details HI_ALARM設定画面 or LO_ALARM設定画面, BtnB: +5C, BtnC: -5C
 */
void renderALARM_SETTING();

// ========== 初期化関数 =====================================================
void initTasks();
float getInitialTemperature();  // センサー接続確認用

// ========== EEPROM操作（一元管理） =====================================================
/**
 * @brief EEPROM → GlobalData: 起動時に設定値をロード
 */
void EEPROM_LoadToGlobal();

/**
 * @brief GlobalData → EEPROM: ALARM_SETTING終了時に設定値をセーブ
 * @return true: 保存 + 検証成功, false: 保存または検証失敗
 */
bool EEPROM_SaveFromGlobal();

/**
 * @brief EEPROM設定値の検証（デバッグ用）
 * @param printDetail: true=詳細出力, false=結果のみ出力
 * @return true: 検証合格, false: 検証失敗
 */
bool EEPROM_ValidateSettings(bool printDetail = true);

#endif
