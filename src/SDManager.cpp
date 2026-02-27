#include "SDManager.h"

// ── 静的メンバー変数の実装 ─────────────────────────────────────────────────────
bool   SDManager::s_sdReady       = false;
bool   SDManager::s_fileOpen      = false;
File   SDManager::s_currentFile;
char   SDManager::s_lastError[64] = {0};
char   SDManager::s_lineBuffer[256] = {0};

// ================================ 実装部分 ====================================

/**
 * @brief SDカードの初期化・マウント
 */
bool SDManager::init() {
  // SD インターフェースの初期化
  // ⚠️ 重要: M5Stack の SD カード CS は GPIO 4 (TFCARD_CS_PIN)。
  // SD.begin() を引数なしで呼ぶと ESP32 デフォルトの SS=GPIO5 が使われ、
  // MAX31855_CS(GPIO5) と衝突して温度読み取りが破損する。
  // M5.begin() と同じ設定 (TFCARD_CS_PIN, SPI, 40MHz) を明示的に指定する。
  if (!SD.begin(TFCARD_CS_PIN, SPI, 40000000)) {
    setError("SD initialization failed");
    s_sdReady = false;
    Serial.printf("[SDManager] SD initialization failed\n");
    return false;
  }

  s_sdReady = true;
  Serial.printf("[SDManager] SD card initialized successfully (CS=GPIO%d)\n", TFCARD_CS_PIN);
  
  // 診断: SD カード容量情報をシリアル出力
  uint64_t cardSize = SD.cardSize();
  Serial.printf("[SDManager] Card size: %.2f MB\n", (float)cardSize / (1024.0 * 1024.0));

  return true;
}

/**
 * @brief SDカード接続確認（マウント再試行）
 */
bool SDManager::begin() {
  // 既にマウント済みなら OK
  if (s_sdReady) {
    return true;
  }

  // マウント再試行
  return init();
}

/**
 * @brief 新規 CSV ファイルの作成・オープン
 */
bool SDManager::createNewFile(const char* filename) {
  // SD 未初期化なら失敗
  if (!s_sdReady) {
    setError("SD not ready");
    return false;
  }

  // ファイルが既に開いていればクローズ
  if (s_fileOpen) {
    closeFile();
  }

  // ファイルを書き込みモードで作成
  s_currentFile = SD.open(filename, FILE_WRITE);
  
  if (!s_currentFile) {
    Serial.printf("[SDManager] Failed to create file: %s\n", filename);
    setError("Cannot create file");
    return false;
  }

  s_fileOpen = true;
  Serial.printf("[SDManager] File created: %s\n", filename);

  return true;
}

/**
 * @brief CSV ヘッダ行の書き込み
 */
bool SDManager::writeHeader() {
  if (!s_fileOpen) {
    setError("File not open");
    return false;
  }

  // ヘッダ行フォーマット
  const char* header = "ElapsedSec,Temp_C,State,Samples,Average_C,StdDev_C,Max_C,Min_C,HI_ALARM,LO_ALARM\r\n";
  
  // ファイルへ書き込み
  size_t written = s_currentFile.write((uint8_t*)header, strlen(header));
  
  if (written != strlen(header)) {
    Serial.printf("[SDManager] Header write failed: wrote %d of %d bytes\n", 
                  written, strlen(header));
    setError("Header write failed");
    return false;
  }

  Serial.printf("[SDManager] Header written (%d bytes)\n", written);

  // ヘッダは即時フラッシュして確実に保存
  s_currentFile.flush();
  Serial.printf("[SDManager] Header flushed to card\n");

  return true;
}

/**
 * @brief CSV データ行の書き込み
 */
bool SDManager::writeData(const SDData& data) {
  if (!s_fileOpen) {
    setError("File not open");
    return false;
  }

  // CSV フォーマット生成
  const char* csvLine = formatCSVLine(data);
  
  // ファイルへ書き込み
  size_t written = s_currentFile.write((uint8_t*)csvLine, strlen(csvLine));
  
  if (written != strlen(csvLine)) {
    Serial.printf("[SDManager] Data write failed: wrote %d of %d bytes\n", 
                  written, strlen(csvLine));
    setError("Data write failed");
    return false;
  }

  // 成功時は即時フラッシュしてログ出力
  s_currentFile.flush();
    // SD書き込み後にSPIバスをリセット
    SPI.end();
    SPI.begin();
  Serial.printf("[SDManager] Data written (%d bytes): %s", written, csvLine);
  return true;
}

/**
 * @brief 内部バッファを SD カードへフラッシュ
 */
bool SDManager::flush() {
  if (!s_fileOpen) {
    // ファイルが開いていなければ OK（何もしない）
    return true;
  }

  // SD カードバッファをフラッシュ
  s_currentFile.flush();

  Serial.printf("[SDManager] Buffer flushed\n");

  return true;
}

/**
 * @brief 開いているファイルをクローズ
 */
bool SDManager::closeFile() {
  if (!s_fileOpen) {
    // ファイルが開いていなければ OK
    return true;
  }

  // flush() してからクローズ
  flush();
  s_currentFile.close();
  s_fileOpen = false;

  Serial.printf("[SDManager] File closed\n");

  return true;
}

/**
 * @brief ファイルをクローズして終了
 */
void SDManager::end() {
  closeFile();
  SD.end();
  s_sdReady = false;
  Serial.printf("[SDManager] SD interface closed\n");
}

/**
 * @brief SD カード検出状態の確認
 */
bool SDManager::isReady() {
  return s_sdReady;
}

/**
 * @brief エラー状態の確認
 */
bool SDManager::hasError() {
  // s_lastError に何か入っていたらエラー
  return (s_lastError[0] != '\0');
}

/**
 * @brief 最後に発生したエラーメッセージを取得
 */
const char* SDManager::getLastError() {
  if (s_lastError[0] == '\0') {
    return "No error";
  }
  return s_lastError;
}

// ================================ 内部メソッド ====================================

/**
 * @brief エラーメッセージの設定（内部用）
 */
void SDManager::setError(const char* message) {
  if (message == nullptr) {
    s_lastError[0] = '\0';
    return;
  }

  // メッセージをコピー（バッファオーバーフロー防止）
  strncpy(s_lastError, message, sizeof(s_lastError) - 1);
  s_lastError[sizeof(s_lastError) - 1] = '\0';

  Serial.printf("[SDManager] Error: %s\n", s_lastError);
}

/**
 * @brief 内部バッファから CSV 行フォーマットを生成
 * 
 * @details
 * SDData 構造体を CSV フォーマットに変換します。
 * 浮動小数点数は小数第1位（%.1f）で出力します。
 * 
 * フォーマット例：
 * 0,540.2,RUN,1,540.2,0.0,540.2,540.2,false,false\r\n
 */
const char* SDManager::formatCSVLine(const SDData& data) {
  // アラーム状態を文字列に変換
  const char* hiAlarmStr = data.hiAlarm ? "true" : "false";
  const char* loAlarmStr = data.loAlarm ? "true" : "false";

  // CSV 行を生成
  // NaN 値は 0.0 としてしまうと不正なデータに見えるため、
  // テキスト 'NaN' を出力して後処理で判別しやすくする。
  const char* tempStr = isnan(data.temperature) ? "NaN" : nullptr;
  const char* avgStr  = isnan(data.averageTemp) ? "NaN" : nullptr;
  const char* sdStr   = isnan(data.stdDev) ? "NaN" : nullptr;
  const char* maxStr  = isnan(data.maxTemp) ? "NaN" : nullptr;
  const char* minStr  = isnan(data.minTemp) ? "NaN" : nullptr;

  if (tempStr) {
    snprintf(s_lineBuffer, sizeof(s_lineBuffer),
             "%u,%s,%s,%u,%s,%s,%s,%s,%s,%s\r\n",
             data.elapsedSeconds,
             tempStr,
             data.state,
             data.sampleCount,
             avgStr ? avgStr : "NaN",
             sdStr  ? sdStr  : "NaN",
             maxStr ? maxStr : "NaN",
             minStr ? minStr : "NaN",
             hiAlarmStr,
             loAlarmStr);
  } else {
    // 温度が数値なら他も数値フォーマットで出力（小数第1位）
    snprintf(s_lineBuffer, sizeof(s_lineBuffer),
             "%u,%.1f,%s,%u,%.1f,%.1f,%.1f,%.1f,%s,%s\r\n",
             data.elapsedSeconds,
             data.temperature,
             data.state,
             data.sampleCount,
             isnan(data.averageTemp) ? 0.0f : data.averageTemp,
             isnan(data.stdDev) ? 0.0f : data.stdDev,
             isnan(data.maxTemp) ? 0.0f : data.maxTemp,
             isnan(data.minTemp) ? 0.0f : data.minTemp,
             hiAlarmStr,
             loAlarmStr);
  }

  return s_lineBuffer;
}
