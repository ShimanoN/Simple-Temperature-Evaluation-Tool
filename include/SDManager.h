#pragma once

#include "Global.h"

/**
 * @file SDManager.h
 * @brief microSD カード操作を集約するマネージャー
 * 
 * @details
 * ESP32 のハードウェア SPI を使用して microSD カードに CSV形式でデータを記録します。
 * EEPROMManager と同パターンで、すべてのSD操作を静的メソッドで提供します。
 * 
 * ファイルライフサイクル：
 * 1. RUN開始 → createNewFile() でファイル作成
 * 2. RUN中 → writeData() で逐次データ記録（バッファ蓄積）
 * 3. RESULT遷移 → flush() + closeFile() でファイルクローズ
 * 
 * エラーハンドリング：
 * - SD未検出時：M_SDReady=false を GlobalData に設定
 * - 書き込み失敗時：M_SDError=true を GlobalData に設定
 * - エラーメッセージは getLastError() で取得可能
 */
class SDManager {
public:
  /**
   * @brief SDカードの初期化・マウント
   * 
   * @details
   * microSD ファイルシステムを初期化し、マウントポイント /sd に接続します。
   * 検出失敗時は M_SDReady フラグを false のままとして、
   * 他の機能は継続動作するようにします（graceful degradation）。
   * 
   * @return true : SD初期化・マウント成功
   * @return false : SD検出失敗または初期化失敗
   */
  static bool init();

  /**
   * @brief SDカード接続確認（マウント再試行）
   * 
   * @details
   * setup() の init() 後に1回のみ呼び出し推奨。
   * または、init() 失敗後に再試行する場合に使用。
   * 
   * @return true : マウント成功
   * @return false : マウント失敗
   */
  static bool begin();

  /**
   * @brief 新規 CSV ファイルの作成・オープン
   * 
   * @details
   * ファイル名は YYYYMMDD_HHMMSS.csv 形式で、
   * Logic_Task から RTC の現在時刻を基に生成されたファイル名が渡されます。
   * 
   * 既にファイルが開いている場合は closeFile() してから呼び出してください。
   * 
   * @param filename ファイル名（例："20260226_143025.csv"）
   * @return true : ファイル作成・オープン成功
   * @return false : ファイル操作失敗
   */
  static bool createNewFile(const char* filename);

  /**
   * @brief CSV ヘッダ行の書き込み
   * 
   * @details
   * createNewFile() の直後に呼び出す想定です。
   * ヘッダ行フォーマット：
   * ElapsedSec,Temp_C,State,Samples,Average_C,StdDev_C,Max_C,Min_C,HI_ALARM,LO_ALARM
   * 
   * @return true : ヘッダ行書き込み成功
   * @return false : 書き込み失敗
   */
  static bool writeHeader();

  /**
   * @brief CSV データ行の書き込み
   * 
   * @details
   * SDData 構造体（Global.h で定義）を CSV フォーマットに変換して書き込みます。
   * 実装としては内部バッファに蓄積し、flushBuffer() で物理的に SD へ書き出します。
   * 
   * @param data CSV に記録するデータ（SDData 構造体参照）
   * @return true : バッファに蓄積成功
   * @return false : バッファ溢れか書き込み失敗
   */
  static bool writeData(const SDData& data);

  /**
   * @brief 内部バッファを SD カードへフラッシュ
   * 
   * @details
   * IO_Task で 10 サンプルごとに call される想定です。
   * バッファの内容を物理的に microSD へ書き込み、バッファをクリアします。
   * 
   * @return true : フラッシュ成功
   * @return false : 書き込み失敗
   */
  static bool flush();

  /**
   * @brief 開いているファイルをクローズ
   * 
   * @details
   * 最後に flush() してからクローズします。
   * RESULT 遷移時に Logic_Task から call される想定です。
   * 
   * @return true : クローズ成功
   * @return false : クローズ失敗
   */
  static bool closeFile();

  /**
   * @brief ファイルをクローズして終了
   * 
   * @details
   * クリーンアップ用。通常は closeFile() でOK です。
   */
  static void end();

  /**
   * @brief SD カード検出状態の確認
   * 
   * @return true : SD カード接続・マウント済み
   * @return false : SD 未検出またはマウント失敗
   */
  static bool isReady();

  /**
   * @brief エラー状態の確認
   * 
   * @return true : エラーが発生している
   * @return false : エラーなし
   */
  static bool hasError();

  /**
   * @brief 最後に発生したエラーメッセージを取得
   * 
   * @details
   * エラー発生時に、詳細メッセージ（最大 32 文字程度）を返します。
   * デバッグログには常に出力され、UI に表示する場合は
   * GlobalData::M_SDError フラグを参照して判定してください。
   * 
   * @return const char* : エラーメッセージ（C文字列）
   */
  static const char* getLastError();

private:
  // ── 内部状態管理 ──
  static bool       s_sdReady;              // SD 初期化完了フラグ
  static bool       s_fileOpen;             // ファイルオープン状態
  static File       s_currentFile;          // 現在のファイルハンドル
  static char       s_lastError[64];        // 最後のエラーメッセージ
  static char       s_lineBuffer[256];      // CSV 行バッファ

  /**
   * @brief エラーメッセージの設定（内部用）
   * @param message エラーメッセージ
   */
  static void setError(const char* message);

  /**
   * @brief 内部バッファからCSV行フォーマットを生成
   * @param data SDData 構造体
   * @return 生成された CSV 行（s_lineBuffer）
   */
  static const char* formatCSVLine(const SDData& data);
};
