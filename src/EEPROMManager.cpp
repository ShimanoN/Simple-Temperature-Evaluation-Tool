#include "EEPROMManager.h"
#include <Arduino.h>

// ========== EEPROMManager 実装 ====================================================

void EEPROMManager::init(size_t size) {
  EEPROM.begin(size);
  Serial.println("[EEPROMManager] Initialized");
}

/**
 * @brief EEPROM から設定値を読み込み（複数段階の検証付き）
 * 
 * @details
 * 読み込み検証フロー:
 * 1️⃣ チェックサム確認: EEPROM[8] == 0xA5 ?
 *    → NO: 未初期化状態、デフォルト値を使用すべき
 * 2️⃣ 有限値チェック: isfinite(HI) && isfinite(LO) ?
 *    → NO: NaN/Inf が記録されている（腐敗状態）
 * 3️⃣ 範囲チェック: -50°C ≦ value ≦ 1100°C ?
 *    → NO: 物理的に不可能な値（腐敗 or 誤書き込み）
 * 
 * 【ESP32 EEPROM の特性】
 * - 容量: 4KB (デフォルト)
 * - 耐久性: 100,000 サイクル（大容量タイプ）
 * - 初期化: Arduino IDE 経由で消去可能
 * - 特注: setFlash() で永続化
 * 
 * @param[out] settings 読み込み先の設定構造体
 * @return true  検証に成功し、有効な設定値を設定
 * @return false チェックサム無効 or 値が不正
 * 
 * @see writeSettings() (書き込みの対になる処理)
 * @see isValueValid(), isValueInRange() (検証ロジック)
 */
bool EEPROMManager::readSettings(AlarmSettings& settings) {
  // EEPROMから設定値を読み込む（チェックサム検証付き）
  
  // オフセット: 0-3: HI_ALARM, 4-7: LO_ALARM, 8: checksum
  settings.HI_ALARM = EEPROM.readFloat(ALARM_SETTINGS_ADDR + 0);
  settings.LO_ALARM = EEPROM.readFloat(ALARM_SETTINGS_ADDR + 4);
  uint8_t readChecksum = EEPROM.read(ALARM_SETTINGS_ADDR + 8);
  
  // 検証Step1: チェックサム確認
  if (readChecksum != EEPROM_CHECKSUM) {
    // チェックサム不一致 → 未初期化
    return false;
  }
  
  // 検証Step2: チェックサム OK → 値の妥当性チェック（NaN/Inf検出）
  if (!isValueValid(settings.HI_ALARM) || !isValueValid(settings.LO_ALARM)) {
    Serial.printf("[EEPROMManager] Value is NaN/Inf: HI=%f, LO=%f\n",
                  settings.HI_ALARM, settings.LO_ALARM);
    return false;
  }
  
  // 検証Step3: 値の範囲チェック（-50℃〜1100℃を許容）
  if (!isValueInRange(settings.HI_ALARM, -50.0f, 1100.0f) ||
      !isValueInRange(settings.LO_ALARM, -50.0f, 1100.0f)) {
    Serial.printf("[EEPROMManager] Value out of range: HI=%.1f, LO=%.1f\n",
                  settings.HI_ALARM, settings.LO_ALARM);
    return false;
  }
  
  // すべての検証を通過
  return true;
}

/**
 * @brief EEPROM への書き込み（Write-Verify 機構付き）
 * 
 * @details
 * 書き込み手順:
 * 1️⃣ 値をEEPROMに書き込み（HI, LO, Checksum）
 * 2️⃣ EEPROM.commit() でフラッシュメモリに確定
 * 3️⃣ 50ms 待機（EEPROM の書き込み安定化期待）
 * 4️⃣ 読み込んで検証（write-verify）
 * 
 * 【なぜ検証が必要？】
 * EEPROM の書き込み中に電源喪失 → データ破損リスク
 * 稀に EEPROM.commit() が失敗（ハードウェアエラー）
 * → 検証することで確実性を高めます
 * 
 * 【許容誤差】
 * ±0.01°C の誤差は許容（float 精度の限界）
 * それ以上に値が変わっていれば、書き込み失敗と判定
 * 
 * 【タイムアウト】
 * 書き込み通常: 10ms 以内
 * 安定化待機: 50ms (安全マージン)
 * 検証: 数ms
 * 合計: ~60ms
 * 
 * @param settings       書き込む設定値
 * @return true  書き込み + 検証成功
 * @return false 検証失敗（実際の値が期待値と不一致）
 * 
 * @see readSettings() (検証で readSettings() を呼び出す)
 */
bool EEPROMManager::writeSettings(const AlarmSettings& settings) {
  // EEPROMに設定値を書き込む（チェックサムも含む）
  
  EEPROM.writeFloat(ALARM_SETTINGS_ADDR + 0, settings.HI_ALARM);
  EEPROM.writeFloat(ALARM_SETTINGS_ADDR + 4, settings.LO_ALARM);
  EEPROM.write(ALARM_SETTINGS_ADDR + 8, EEPROM_CHECKSUM);
  
  // フラッシュメモリに確定
  EEPROM.commit();
  Serial.printf("[EEPROMManager::write] Wrote: HI=%.1f, LO=%.1f\n", 
                settings.HI_ALARM, settings.LO_ALARM);
  
  // Write-Verify: 書き込み検証（読み込んで値をチェック）
  delay(50);  // EEPROM安定待ち
  AlarmSettings verify;
  if (readSettings(verify)) {
    // チェックサムが有効（初期化済み）
    // 値が一致しているか確認
    const float HI_DIFF = fabs(verify.HI_ALARM - settings.HI_ALARM);
    const float LO_DIFF = fabs(verify.LO_ALARM - settings.LO_ALARM);
    
    if (HI_DIFF < 0.01f && LO_DIFF < 0.01f) {
      Serial.println("[EEPROMManager::write] Verification OK");
      return true;  // 書き込み成功
    } else {
      Serial.printf("[EEPROMManager::write] Value mismatch: HI_DIFF=%.3f, LO_DIFF=%.3f\n", 
                    HI_DIFF, LO_DIFF);
      return false;
    }
  } else {
    Serial.println("[EEPROMManager::write] Checksum verification failed");
    return false;
  }
}

void EEPROMManager::printDebugInfo() {
  Serial.println("[EEPROMManager] Debug Info:");
  Serial.printf("  EEPROM_SIZE: %u bytes\n", EEPROM_SIZE);
  Serial.printf("  ALARM_SETTINGS_ADDR: 0x%04X\n", ALARM_SETTINGS_ADDR);
  Serial.printf("  ALARM_SETTINGS_SIZE: %u bytes\n", ALARM_SETTINGS_SIZE);
  
  AlarmSettings current;
  if (readSettings(current)) {
    Serial.printf("  Current Settings: HI=%.1f, LO=%.1f\n", 
                  current.HI_ALARM, current.LO_ALARM);
  } else {
    Serial.println("  Current Settings: INVALID");
  }
}

// ========== Private Implementation ================================================

bool EEPROMManager::isValueValid(float value) {
  return isfinite(value);
}

bool EEPROMManager::isValueInRange(float value, float minVal, float maxVal) {
  return (value >= minVal && value <= maxVal);
}
