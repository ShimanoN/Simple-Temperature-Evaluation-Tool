#pragma once

#include <EEPROM.h>
#include <cstdint>
#include <cmath>

// ── EEPROM アドレス・サイズ定義 ────────────────────────────────────────────────
constexpr uint16_t EEPROM_SIZE         = 4096;        // ESP32 標準サイズ
constexpr uint16_t ALARM_SETTINGS_ADDR = 0;           // AlarmSettings保存位置
constexpr uint8_t  EEPROM_CHECKSUM     = 0xA5;        // 初期化済みマーク
constexpr size_t   ALARM_SETTINGS_SIZE = 9;           // 4B + 4B + 1B

// ── AlarmSettings 構造体（EEPROM保存用） ──────────────────────────────────────
struct AlarmSettings {
  float  HI_ALARM;   // offset: 0  (float型 4Byte)
  float  LO_ALARM;   // offset: 4  (float型 4Byte)
  uint8_t checksum;  // offset: 8  初期化済み判定用
};

/**
 * @class EEPROMManager
 * @brief EEPROM操作を集約管理するクラス
 * 
 * EEPROM読み書き、検証、初期化をすべて担当
 * テストお容易性・再利用性を高める目的で独立化
 */
class EEPROMManager {
public:
  /**
   * @brief EEPROM初期化
   * @param size EEPROM サイズ（通常は EEPROM_SIZE で呼び出し）
   */
  static void init(size_t size = EEPROM_SIZE);

  /**
   * @brief EEPROMから設定値を読み込む
   * @param settings [out] 読み込んだ設定
   * @return true: 読み込み成功、false: 無効またはチェックサム不一致
   * 
   * 実装:
   * - チェックサム検証
   * - NaN/Inf値のフィルタリング
   * - 値の範囲チェック（-50℃〜1100℃）
   */
  static bool readSettings(AlarmSettings& settings);

  /**
   * @brief EEPROMに設定値を書き込む
   * @param settings 書き込む設定
   * @return true: 書き込み成功（検証済み）、false: 失敗
   * 
   * 実装:
   * - チェックサム付きで書き込み
   * - EEPROM.commit() で永続化
   * - 書き込み検証ループ（失敗リトライなし、即座に結果を返す）
   */
  static bool writeSettings(const AlarmSettings& settings);

  /**
   * @brief EEPROMのストレージ使用状況をシリアル出力
   * @brief デバッグ用
   */
  static void printDebugInfo();

private:
  /**
   * @brief 値がfiniteであるか検証（NaN・Infを除外）
   */
  static bool isValueValid(float value);

  /**
   * @brief 値が許容範囲内であるか検証
   * @param value 検証値
   * @param minVal 最小値
   * @param maxVal 最大値
   */
  static bool isValueInRange(float value, float minVal, float maxVal);
};
