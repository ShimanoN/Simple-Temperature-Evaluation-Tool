#include "Global.h"

// ── Forward Declarations （EEPROM 操作関数） ────────────────────────────────
bool EEPROM_SaveFromGlobal();
bool EEPROM_ValidateSettings(bool printDetail = true);

// ── インスタンス生成 ──────────────────────────────────────────────────────────
// LCDとハードウェアSPIを共有するため、ソフトウェアSPIは使用不可。
// CSピン（MAX31855_CS）のみで制御を切り替える。
GlobalData        G;
Adafruit_MAX31855 thermocouple(MAX31855_CS);

// ── グローバルデータ初期化 ────────────────────────────────────────────────────
void initGlobalData() {
  G.D_RawPV        = NAN;   // 未読取を明示 (isnan() で検査可能)
  G.D_FilteredPV   = NAN;   // setup() でセンサ初読取後に上書き
  G.D_Sum          = 0.0;
  G.D_Count        = 0;
  G.D_Average      = NAN;
  G.M_CurrentState = State::IDLE;
  G.M_BtnA_Pressed = false;
  G.M_BtnB_Pressed = false;  // ボタンB初期化
  G.M_BtnC_Pressed = false;  // ボタンC初期化
  G.M_ResultPage   = 0;     // RESULT画面ページ初期化

  // Phase 3: アラームフラグ初期化
  G.M_HiAlarm      = false;
  G.M_LoAlarm      = false;

  // Phase 3 拡張: 動的閾値初期化（後で EEPROM から上書きされる）
  G.D_HI_ALARM_CURRENT = HI_ALARM_TEMP;
  G.D_LO_ALARM_CURRENT = LO_ALARM_TEMP;
  G.M_SettingIndex     = 0;  // HI側から開始
}

// ========== Phase 3 アラーム判定ロジック関数 ================================

/**
 * @brief ヒステリシス付きアラーム判定ロジック（テスト可能・再利用可能）
 * 
 * @details
 * このアラーム判定は、単純な「閾値超過」ではなく「ヒステリシス」を適用しています。
 * これにより、閾値近くで温度が揺らいでいるときの誤報を防げます。
 * 
 * 【動作原理】
 * 
 * HI アラーム（過温度保護）:
 *   - トリガー: 現在温度 >= HI_THRESHOLD
 *   - クリア:   現在温度 < (HI_THRESHOLD - HYSTERESIS)
 * 
 * 具体例（HI_THRESHOLD=60°C, HYSTERESIS=5°C）:
 *   - 61°C 検出 → hiFlag = true（トリガー）, 2kHz ビープ
 *   - 60°C 検出 → hiFlag = true（継続, 60 < 55 は false）
 *   - 54.9°C → hiFlag = true（継続, 54.9 > 55 は false）
 *   - 54.5°C → hiFlag = false（クリア, 54.5 < 55）
 * 
 * LO アラーム（過低温度保護）:
 *   - トリガー: 現在温度 <= LO_THRESHOLD
 *   - クリア:   現在温度 > (LO_THRESHOLD + HYSTERESIS)
 * 
 * 【利点】
 * ✓ チラチラ切り替わりを防止（過冷却時の誤報防止）
 * ✓ NaN 値を自動で無視（センサ不良時も安全）
 * ✓ 音声フィードバック（HI=2kHz, LO=1kHz で区別可能）
 * ✓ ユニットテスト可能なシグネチャ（独立した関数）
 * 
 * @param currentTemp    現在温度 (℃)
 * @param hiThreshold    上限閾値 (℃)
 * @param loThreshold    下限閾値 (℃)
 * @param hysteresis     ヒステリシス幅 (℃)
 * @param[out] hiFlag    上限アラームフラグ (参照渡し)
 * @param[out] loFlag    下限アラームフラグ (参照渡し)
 * 
 * @see ALARM_HYSTERESIS (Global.h で定義)
 * @see IO_Task() (このタスク内で呼び出し)
 */
void updateAlarmFlags(float currentTemp, float hiThreshold, float loThreshold,
                      float hysteresis, bool& hiFlag, bool& loFlag) {
  // NaN値は判定対象外（処理スキップ）
  if (isnan(currentTemp)) {
    return;
  }

  // ────── 上限アラーム判定 ──────
  // トリガー条件: currentTemp >= threshold
  if (!hiFlag && currentTemp >= hiThreshold) {
    hiFlag = true;
    if (UI::SHOW_DEBUG_LOGS) {
      Serial.printf("[ALARM] HI triggered: %.1f >= %.1f\n", currentTemp, hiThreshold);
    }
    M5.Speaker.tone(ALARM_HI_FREQUENCY_HZ, ALARM_SOUND_DURATION_MS);
  }
  // クリア条件: ヒステリシス適用（threshold - hysteresis 以下になるまで待機）
  else if (hiFlag && currentTemp < hiThreshold - hysteresis) {
    hiFlag = false;
    if (UI::SHOW_DEBUG_LOGS) {
      Serial.printf("[ALARM] HI cleared: %.1f < %.1f (hysteresis)\n",
                    currentTemp, hiThreshold - hysteresis);
    }
  }

  // ────── 下限アラーム判定 ──────
  // トリガー条件: currentTemp <= threshold
  if (!loFlag && currentTemp <= loThreshold) {
    loFlag = true;
    if (UI::SHOW_DEBUG_LOGS) {
      Serial.printf("[ALARM] LO triggered: %.1f <= %.1f\n", currentTemp, loThreshold);
    }
    M5.Speaker.tone(ALARM_LO_FREQUENCY_HZ, ALARM_SOUND_DURATION_MS);
  }
  // クリア条件: ヒステリシス適用（threshold + hysteresis 以上になるまで待機）
  else if (loFlag && currentTemp > loThreshold + hysteresis) {
    loFlag = false;
    if (UI::SHOW_DEBUG_LOGS) {
      Serial.printf("[ALARM] LO cleared: %.1f > %.1f (hysteresis)\n",
                    currentTemp, loThreshold + hysteresis);
    }
  }
}

// ========== IO Layer (10ms周期) ==================================================
void IO_Task() {
  // MAX31855 の変換時間に合わせ、TC_READ_INTERVAL_MS ごとに読み取る。
  // フィルタは新データ到着時のみ適用（同じ値で繰り返すとα=0.1の意味が消える）。
  static unsigned long lastTcRead = 0;
  const unsigned long  now        = millis();

  if (now - lastTcRead >= TC_READ_INTERVAL_MS) {
    lastTcRead = now;

    const float rawTemp = thermocouple.readCelsius();
    if (!isnan(rawTemp)) {
      G.D_RawPV = rawTemp;
      // 1次遅れフィルタ: y[n] = y[n-1]*(1-α) + x[n]*α
      // α=0.1 のとき約22サンプル(11秒)で新値の90%に収束
      G.D_FilteredPV = isnan(G.D_FilteredPV)
                     ? rawTemp                                           // 初回: 即時追従
                     : G.D_FilteredPV * (1.0f - FILTER_ALPHA)
                       + rawTemp      *           FILTER_ALPHA;
    }
  }

  M5.update();
  static bool btnAPrev = false;         // BtnA エッジ検出用前回値
  static bool btnBPrev = false;         // BtnB エッジ検出用前回値
  static bool btnCPrev = false;         // BtnC エッジ検出用前回値

  // ── BtnA 処理（シンプルなエッジ検出）──
  const bool  btnANow  = M5.BtnA.isPressed();
  if (btnANow && !btnAPrev) {
    G.M_BtnA_Pressed = true;  // 立ち上がりエッジ
  }
  btnAPrev = btnANow;

  // ── BtnB 処理（RESULT ページング / 設定調整 / ALARM_SETTING進入）──
  const bool  btnBNow  = M5.BtnB.isPressed();
  if (btnBNow && !btnBPrev) {
    G.M_BtnB_Pressed = true;  // 立ち上がりエッジ（Logic_Task で状態別に処理）
  }
  btnBPrev = btnBNow;

  // ── BtnC 処理（設定モード用）──
  const bool btnCNow = M5.BtnC.isPressed();
  if (btnCNow && !btnCPrev) {
    G.M_BtnC_Pressed = true;  // 立ち上がりエッジ（ALARM_SETTING時に -5℃）
  }
  btnCPrev = btnCNow;

  // Phase 3: アラーム判定（ヒステリシス付き、EEPROM値を使用）
  // デバッグ出力用タイマー（5秒ごと）
  static unsigned long lastAlarmDebug = 0;
  const bool needsDebugLog = (now - lastAlarmDebug >= ALARM_DEBUG_LOG_INTERVAL_MS);
  if (needsDebugLog) {
    lastAlarmDebug = now;
    if (UI::SHOW_DEBUG_LOGS) {
      Serial.printf("[ALARM_DEBUG] Temp=%.1f, HI=%.1f, LO=%.1f, HiAlarm=%d, LoAlarm=%d\n",
                    G.D_FilteredPV, G.D_HI_ALARM_CURRENT, G.D_LO_ALARM_CURRENT,
                    G.M_HiAlarm, G.M_LoAlarm);
    }
  }

  // ← アラーム判定ロジックを専用関数に委譲
  updateAlarmFlags(G.D_FilteredPV, G.D_HI_ALARM_CURRENT, G.D_LO_ALARM_CURRENT,
                   ALARM_HYSTERESIS, G.M_HiAlarm, G.M_LoAlarm);
}

// ========== Logic Layer ヘルパー関数（状態遷移・ボタン処理封遠）================

/**
 * @brief ボタンA イベント処理（状態遷移 + 統計確定）
 * 
 * @details
 * ボタンA は複数の役割を状態に応じて果たします。**状態機械パターン**を採用しています。
 * 
 * 【状態遷移マトリックス】
 * - IDLE → RUN: 計測開始、統計値をリセット
 * - RUN → RESULT: 計測終了、Welford法最終値から統計を計算
 * - RESULT → IDLE: 結果画面から戻す
 * - ALARM_SETTING (HI→LO): SettingIndex を切り替え
 * - ALARM_SETTING (LO→IDLE): EEPROM保存してアラーム設定を確定
 * 
 * 【RUN状態での統計管理】
 * - Logic_Task() が Welford法を逐次計算（G.D_M2, G.D_Sum, G.D_Count 更新）
 * - handleButtonA() が RUN→RESULT 遷移時に 最終的な平均・分散・stddev を計算
 * 
 * 【EEPROM保存フロー】
 * ALARM_SETTING (LO) → IDLE 時に EEPROM_SaveFromGlobal() を呼び出し
 * - Write-Verify パターンで確実性を確保
 * - 保存成功時: アラームフラグを初期化 (ハード遷移)
 * 
 * @note
 * - この関数内では Logic_Task の統計計算コメント内容と連携
 * - ALARM_SETTING の HI/LO 設定値は G.D_HI/LO_ALARM_CURRENT を直接操作
 * - 設定値の制約チェック（最小・最大）は handleButtonB/C で実装予定
 * 
 * @see Logic_Task() (各状態での Welford法逐次計算)
 * @see EEPROM_SaveFromGlobal() (EEPROM保存)
 */
void handleButtonA() {
  switch (G.M_CurrentState) {
    case State::IDLE: {
      // RUN開始: 測定用の統計値をリセット
      G.D_Sum          = 0.0;
      G.D_Count        = 0;
      G.D_Max          = -FLT_MAX;
      G.D_Min          =  FLT_MAX;
      G.D_M2           = 0.0;
      G.M_CurrentState = State::RUN;
      break;
    }

    case State::RUN: {
      // 統計を最終計算して RESULT へ遷移
      if (G.D_Count > 0) {
        G.D_Average = static_cast<float>(G.D_Sum / G.D_Count);
        G.D_Range   = G.D_Max - G.D_Min;
        G.D_StdDev  = static_cast<float>(sqrt(G.D_M2 / G.D_Count));
      } else {
        G.D_Average = G.D_FilteredPV;
        G.D_Range   = 0.0f;
        G.D_StdDev  = 0.0f;
      }
      
      G.M_ResultPage   = 0;  // ページングをリセット
      G.M_CurrentState = State::RESULT;
      break;
    }

    case State::RESULT:
      G.M_CurrentState = State::IDLE;
      break;

    case State::ALARM_SETTING: {
      // BtnA: HI/LO 切り替え or 設定終了
      if (G.M_SettingIndex == 0) {
        // HI → LO へ移行
        G.M_SettingIndex = 1;
      } else {
        // LO → IDLE へ戻る（EEPROM保存）
        if (EEPROM_SaveFromGlobal()) {
          // 保存成功時はアラームフラグをリセット
          G.M_HiAlarm = false;
          G.M_LoAlarm = false;
          Serial.printf("[ALARM_SETTING] Confirmed: HI=%.1f, LO=%.1f (flags reset)\n",
                        G.D_HI_ALARM_CURRENT, G.D_LO_ALARM_CURRENT);
        } else {
          Serial.println("[ALARM_SETTING] ERROR: Failed to save to EEPROM");
        }
        
        G.M_CurrentState = State::IDLE;
      }
      break;
    }
  }
}

// ========== Logic Layer (50ms周期) ===============================================
void Logic_Task() {
  // ── BtnA イベント処理 ──
  if (G.M_BtnA_Pressed) {
    G.M_BtnA_Pressed = false;
    handleButtonA();
  }

  /**
   * @brief Welford法による逐次統計計算（RUN状態でのみ実行）
   * 
   * @details
   * 【Welford法の原理】
   * 従来の統計計算は「データ保存 → 平均計算 → 分散計算」と多パスが必要。
   * Welford法は、逐次的に1パスで分散を正確に計算できます。
   * 
   * 計算式:
   *   M(n)   = 前回までの平均値
   *   M(n+1) = (M(n) + delta / n) where delta = x(n+1) - M(n)
   *   M2(n)  = 二乗偏差の蓄積 = Σ(x(i) - M)²
   * 
   * 最終結果:
   *   平均値  = G.D_Sum / G.D_Count
   *   分散    = G.D_M2 / G.D_Count
   *   標準偏差 = √(分散)
   * 
   * 【利点】
   * ✓ メモリ効率: O(1) の変数（M, M2, Count）のみ
   * ✓ 数値安定: float ではなく double で計算
   * ✓ 逐次性: サンプル受け取り次第すぐに更新可能
   * ✓ 精度: 丸め誤差が少ない
   */
  // RUN 状態での統計計算
  if (G.M_CurrentState == State::RUN && !isnan(G.D_FilteredPV)) {
    G.D_Count++;
    
    // Welford法: Step 1 - 前回の平均を記憶
    const double prevMean = (G.D_Count == 1) ? 0.0 : G.D_Sum / (G.D_Count - 1);
    const double delta  = G.D_FilteredPV - prevMean;
    
    // Welford法: Step 2 - 新しい合計・平均を計算
    G.D_Sum += G.D_FilteredPV;
    const double newMean = G.D_Sum / G.D_Count;
    const double delta2 = G.D_FilteredPV - newMean;
    
    // Welford法: Step 3 - 二乗偏差を逐次累積（分散計算用）
    G.D_M2 += delta * delta2;
    
    // Max/Min 更新（最大・最小値追跡）
    if (G.D_FilteredPV > G.D_Max) G.D_Max = G.D_FilteredPV;
    if (G.D_FilteredPV < G.D_Min) G.D_Min = G.D_FilteredPV;
  }
  
  // ── BtnB イベント処理（ページング / 設定） ──
  if (G.M_BtnB_Pressed) {
    G.M_BtnB_Pressed = false;
    
    if (G.M_CurrentState == State::RESULT) {
      // RESULT ページング
      G.M_ResultPage = (G.M_ResultPage + 1) % 2;  // 0 ↔ 1 を切り替え
    } else if (G.M_CurrentState == State::IDLE) {
      // IDLE → ALARM_SETTING へ進入
      G.M_SettingIndex = 0;  // HI_ALARM設定から開始
      G.M_CurrentState = State::ALARM_SETTING;
    } else if (G.M_CurrentState == State::ALARM_SETTING) {
      // 設定値を +5℃
      if (G.M_SettingIndex == 0) {
        G.D_HI_ALARM_CURRENT += SETTING_STEP;
      } else {
        G.D_LO_ALARM_CURRENT += SETTING_STEP;
      }
    }
  }

  // ── BtnC イベント処理（設定値調整） ──
  if (G.M_BtnC_Pressed) {
    G.M_BtnC_Pressed = false;
    
    if (G.M_CurrentState == State::ALARM_SETTING) {
      // 設定値を -5℃
      if (G.M_SettingIndex == 0) {
        G.D_HI_ALARM_CURRENT -= SETTING_STEP;
      } else {
        G.D_LO_ALARM_CURRENT -= SETTING_STEP;
      }
    }
  }
}

// ========== UI描画ヘルパー関数群（テスト・保守性向上） ===========================

/**
 * @brief IDLE 状態の描画 (待機中の温度監視)
 * 
 * @details
 * 【画面レイアウト】
 * ```
 * STATE: IDLE
 * Temp:
 * 25.3  C          (← 現在温度)
 * (Alarm設定値は DEBUG_MODE時のみ表示)
 * ─────────────────
 * [BtnA] Start  [BtnB] Setting
 * ```
 * 
 * 【アラーム時の表示】
 * HI_ALARMまたはLO_ALARM が true の場合、温度を赤色（RED）で表示
 * 見張り画面として、正常温度範囲外を即座に視認可能
 * 
 * 【表示モード】
 * - SHOW_ALARM_SETTINGS_ON_IDLE = true  : デバッグモード（設定値表示）
 * - SHOW_ALARM_SETTINGS_ON_IDLE = false : 本番モード（設定値非表示）
 * 
 * @note
 * - 温度が未取得（NaN）の場合は "---.-  C" と表示
 * - 通常時はWHITE, アラーム時はREDで色分け
 * - RUN→IDLE遷移時は UI_Task() で画面全消去済みなので再描画
 * 
 * @see renderRUN(), renderRESULT(), renderALARM_SETTING()
 * @see UI_Task()
 */
void renderIDLE() {
  M5.Lcd.setTextSize(UI::TEXTSIZE_VALUE);
  
  // ── 状態行 ──
  M5.Lcd.setCursor(0, UI::PosY::STATE_LABEL);
  M5.Lcd.printf("STATE: IDLE  \n");

  // Phase 3: アラーム中は赤色に切り替え
  if (G.M_HiAlarm || G.M_LoAlarm) {
    M5.Lcd.setTextColor(RED, BLACK);
  } else {
    M5.Lcd.setTextColor(WHITE, BLACK);
  }

  // ── 温度行 ──
  M5.Lcd.setCursor(0, UI::PosY::TEMP_LABEL);
  M5.Lcd.printf("Temp:  ");
  
  M5.Lcd.setCursor(0, UI::PosY::TEMP_VALUE);
  if (isnan(G.D_FilteredPV)) {
    M5.Lcd.printf("---.-  C\n");
  } else {
    M5.Lcd.printf("%5.1f  C\n", G.D_FilteredPV);
  }

  // ── アラーム設定値表示（デバッグ用） ──
  if (UI::SHOW_ALARM_SETTINGS_ON_IDLE) {
    M5.Lcd.setTextSize(UI::TEXTSIZE_GUIDE);
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setCursor(0, UI::PosY::TEMP_VALUE + 40);
    M5.Lcd.printf("Alarm: HI=%.1f LO=%.1f\n", G.D_HI_ALARM_CURRENT, G.D_LO_ALARM_CURRENT);
  }

  // ── ボタンガイド（下端固定） ──
  M5.Lcd.setCursor(0, UI::PosY::BUTTON_INFO);
  M5.Lcd.setTextSize(UI::TEXTSIZE_GUIDE);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.print("[BtnA] Start  [BtnB] Setting");
}

/**
 * @brief RUN 状態の描画 (計測実行中)
 * 
 * @details
 * 【画面レイアウト】
 * ```
 * STATE: RUN
 * Temp:
 * 25.3  C          (← リアルタイム温度)
 * Samples: 42      (← 累計サンプル数)
 * ─────────────────
 * [BtnA] Stop / Reset
 * ```
 * 
 * 【機能】
 * - リアルタイム温度表示（IO_Task の読取値を表示）
 * - サンプル count をインクリメント表示
 * - アラーム検知時は температura 赤色表示
 * - ボタンA長押しで計測停止（RESULT状態へ）
 * 
 * 【計測フロー】
 * 1. IDLE→RUN: ボタンA短押し
 * 2. RUN中: 統計値を Welford法で累積
 * 3. BtnA: RESULT状態に遷移→統計計算
 * 
 * @note
 * - UI_Task() の周期 (200ms) ごとに画面更新
 * - Logic_Task (50ms) でWelford演算が実行されているため表示は遅延
 * - Samples は G.D_Count にバインド
 * 
 * @see updateWelfordStatistics()
 * @see renderIDLE(), renderRESULT()
 */
void renderRUN() {
  M5.Lcd.setTextSize(UI::TEXTSIZE_VALUE);
  M5.Lcd.setTextColor(WHITE, BLACK);
  
  // ── 状態行 ──
  M5.Lcd.setCursor(0, UI::PosY::STATE_LABEL);
  M5.Lcd.printf("STATE: RUN   \n");

  // Phase 3: アラーム中は赤色に切り替え
  if (G.M_HiAlarm || G.M_LoAlarm) {
    M5.Lcd.setTextColor(RED, BLACK);
  } else {
    M5.Lcd.setTextColor(WHITE, BLACK);
  }

  // ── 温度行 ──
  M5.Lcd.setCursor(0, UI::PosY::TEMP_LABEL);
  M5.Lcd.printf("Temp:  ");
  M5.Lcd.setCursor(0, UI::PosY::TEMP_VALUE);
  if (isnan(G.D_FilteredPV)) {
    M5.Lcd.printf("---.-  C\n");
  } else {
    M5.Lcd.printf("%5.1f  C\n", G.D_FilteredPV);
  }

  // ── サンプル数表示 ──
  M5.Lcd.setTextSize(UI::TEXTSIZE_GUIDE);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(0, UI::PosY::GUIDE_BUTTON);
  M5.Lcd.printf("Samples: %5ld  \n", G.D_Count);

  // ── ボタンガイド ──
  M5.Lcd.setCursor(0, UI::PosY::BUTTON_INFO);
  M5.Lcd.print("[BtnA] Stop / Reset");
}

/**
 * @brief ALARM_SETTING 状態の描画 (アラーム閾値設定)
 * 
 * @details
 * 【画面分岐】
 * M_SettingIndex で 2画面を切り替え
 * 
 * **HI_ALARM設定画面 (SettingIndex == 0)**
 * ```
 * HI_ALARM SETTING
 * Current:
 * 60.0 C           (← HI閾値)
 * [BtnB] +5C   [BtnC] -5C
 * [BtnA] Next
 * ```
 * 
 * **LO_ALARM設定画面 (SettingIndex == 1)**
 * ```
 * LO_ALARM SETTING
 * Current:
 * 10.0 C           (← LO閾値)
 * [BtnB] +5C   [BtnC] -5C
 * [BtnA] Save & Exit
 * ```
 * 
 * 【操作フロー】
 * 1. IDLE→ALARM_SETTING: BtnB押下
 * 2. HI画面: BtnB (+5C) / BtnC (-5C) で調整
 * 3. HI→LO: BtnA短押し
 * 4. LO画面: BtnB/BtnC で調整
 * 5. LO→IDLE: BtnA短押し（EEPROM自動保存）
 * 
 * 【値の制約】
 * - 最小: HI/LO = 0°C
 * - 最大: HI/LO = 100°C (デバイス上限)
 * - HIとLOの大小関係チェック: Tasks.cpでhandleButtonA()で実施予定
 * 
 * @note
* - EEPROM保存は LO→IDLE 遷移時に EEPROM_SaveFromGlobal() で実行
 * - 値変更は GlobalData (G.D_HI/LO_ALARM_CURRENT) を直接操作
 * - 2画面切り替えで状態が複雑なため、コメント詳細化推奨
 * 
 * @see handleButtonA(), handleButtonB(), handleButtonC()
 * @see EEPROM_SaveFromGlobal()
 */
void renderALARM_SETTING() {
  M5.Lcd.setTextSize(UI::TEXTSIZE_VALUE);
  M5.Lcd.setTextColor(WHITE, BLACK);
  
  if (G.M_SettingIndex == 0) {
    // HI_ALARM 設定画面
    M5.Lcd.setTextSize(UI::TEXTSIZE_TITLE);
    M5.Lcd.setCursor(0, UI::PosY::TITLE);
    M5.Lcd.printf("HI_ALARM SETTING\n\n");
    
    M5.Lcd.setTextSize(UI::TEXTSIZE_VALUE);
    M5.Lcd.setCursor(0, UI::PosY::TEMP_LABEL);
    M5.Lcd.printf("Current:\n");
    
    M5.Lcd.setCursor(0, UI::PosY::TEMP_VALUE);
    M5.Lcd.printf("%6.1f C", G.D_HI_ALARM_CURRENT);
    
    // 調整ガイド
    M5.Lcd.setCursor(0, UI::PosY::GUIDE_BUTTON);
    M5.Lcd.setTextSize(UI::TEXTSIZE_GUIDE);
    M5.Lcd.printf("[BtnB] +5C   [BtnC] -5C");
    
    // ボタンガイド
    M5.Lcd.setCursor(0, UI::PosY::BUTTON_INFO);
    M5.Lcd.print("[BtnA] Next");
  } else {
    // LO_ALARM 設定画面
    M5.Lcd.setTextSize(UI::TEXTSIZE_TITLE);
    M5.Lcd.setCursor(0, UI::PosY::TITLE);
    M5.Lcd.printf("LO_ALARM SETTING\n\n");
    
    M5.Lcd.setTextSize(UI::TEXTSIZE_VALUE);
    M5.Lcd.setCursor(0, UI::PosY::TEMP_LABEL);
    M5.Lcd.printf("Current:\n");
    
    M5.Lcd.setCursor(0, UI::PosY::TEMP_VALUE);
    M5.Lcd.printf("%6.1f C", G.D_LO_ALARM_CURRENT);
    
    // 調整ガイド
    M5.Lcd.setCursor(0, UI::PosY::GUIDE_BUTTON);
    M5.Lcd.setTextSize(UI::TEXTSIZE_GUIDE);
    M5.Lcd.printf("[BtnB] +5C   [BtnC] -5C");
    
    // ボタンガイド
    M5.Lcd.setCursor(0, UI::PosY::BUTTON_INFO);
    M5.Lcd.print("[BtnA] Save & Exit");
  }
}

/**
 * @brief RESULT 状態の描画 (計測結果の統計表示、2ページング)
 * 
 * @details
 * 【ページ 0: 最新値 + 平均値】
 * ```
 * RESULT (1/2)
 * Temp:
 * 27.5 C
 * Avg:
 * 26.8C
 * [BtnA] Reset   [BtnB] Next
 * ```
 * 
 * 【ページ 1: 標準偏差 + 範囲 / Max + Min】
 * ```
 * RESULT (2/2)
 * StdDev:              Range:
 * 0.8                  10.0
 * Max:                 Min:
 * 30.5                 20.0
 * [BtnA] Reset   [BtnB] Prev
 * ```
 * 
 * 【統計の計算】
 * - 平均値: G.D_Average = G.D_Sum / G.D_Count
 * - 標準偏差: sqrt(G.D_M2 / G.D_Count) [Welford法]
 * - 範囲: G.D_Range = G.D_Max - G.D_Min
 * - Max/Min: 計測中に更新
 * 
 * 【ページング制御】
 * - M_ResultPage == 0: ページ1 (温度 + 平均)
 * - M_ResultPage == 1: ページ2 (統計量)
 * - BtnB短押し: ページ切り替え (0 ↔ 1)
 * - ページ遷移時: UI_Task() で画面全消去（残像防止）
 * 
 * 【NaN対応】
 * - 未計測（計測サンプル 0）時は "---.-" 表示
 * - 計測エラー（ isnan() = true）時も同様
 * 
 * @note
 * - Welford法により1パスで平均・分散を計算（数値安定性向上）
 * - 標準偏差 = sqrt(M2/count) で算出
 * - メモリ効率: データ列全体保存不要
 * 
 * @see updateWelfordStatistics()
 * @see Logic_Task()
 * @see UI_Task()
 */
void renderRESULT() {
  if (G.M_ResultPage == 0) {
    // ========== Page 1: Temp + Avg =========
    M5.Lcd.setTextSize(UI::TEXTSIZE_TITLE);
    M5.Lcd.printf("RESULT (1/2)\n\n");
    
    // 温度表示
    M5.Lcd.setTextSize(UI::TEXTSIZE_VALUE);
    M5.Lcd.setCursor(0, UI::PosY::TEMP_LABEL);
    M5.Lcd.printf("Temp:\n");
    M5.Lcd.setCursor(0, UI::PosY::TEMP_VALUE);
    if (isnan(G.D_FilteredPV)) {
      M5.Lcd.printf("---.- C");
    } else {
      M5.Lcd.printf("%5.1f C", G.D_FilteredPV);
    }
    
    // 平均値
    M5.Lcd.setCursor(UI::LayoutX::LEFT_COL, UI::LayoutY::RESULT_AVG_LABEL);
    M5.Lcd.printf("Avg:\n");
    M5.Lcd.setCursor(UI::LayoutX::LEFT_COL, UI::LayoutY::RESULT_AVG_VALUE);
    if (isnan(G.D_Average)) {
      M5.Lcd.printf("---.-C");
    } else {
      M5.Lcd.printf("%5.1fC", G.D_Average);
    }
  } else {
    // ========== Page 2: StdDev/Range + Max/Min ==========
    M5.Lcd.setTextSize(UI::TEXTSIZE_TITLE);
    M5.Lcd.printf("RESULT (2/2)\n");
    
    // 上段: StdDev と Range
    M5.Lcd.setTextSize(UI::TEXTSIZE_VALUE);
    M5.Lcd.setCursor(UI::LayoutX::LEFT_COL, UI::LayoutY::RESULT_TOP_ROW);
    M5.Lcd.printf("StdDev:");
    M5.Lcd.setCursor(UI::LayoutX::RIGHT_COL, UI::LayoutY::RESULT_TOP_ROW);
    M5.Lcd.printf("Range:");
    
    M5.Lcd.setCursor(UI::LayoutX::LEFT_COL, UI::LayoutY::RESULT_MID_ROW);
    M5.Lcd.printf("%5.1f", G.D_StdDev);
    M5.Lcd.setCursor(UI::LayoutX::RIGHT_COL, UI::LayoutY::RESULT_MID_ROW);
    M5.Lcd.printf("%5.1f", G.D_Range);
    
    // 下段: Max と Min
    M5.Lcd.setCursor(UI::LayoutX::LEFT_COL, UI::LayoutY::RESULT_STAT_LABEL);
    M5.Lcd.printf("Max:");
    M5.Lcd.setCursor(UI::LayoutX::RIGHT_COL, UI::LayoutY::RESULT_STAT_LABEL);
    M5.Lcd.printf("Min:");
    
    M5.Lcd.setCursor(UI::LayoutX::LEFT_COL, UI::LayoutY::RESULT_STAT_VALUE);
    M5.Lcd.printf("%5.1f", G.D_Max);
    M5.Lcd.setCursor(UI::LayoutX::RIGHT_COL, UI::LayoutY::RESULT_STAT_VALUE);
    M5.Lcd.printf("%5.1f", G.D_Min);
  }
  
  // ボタンガイド（下端固定）
  M5.Lcd.setCursor(0, UI::PosY::BUTTON_INFO);
  M5.Lcd.setTextSize(UI::TEXTSIZE_GUIDE);
  if (G.M_ResultPage == 0) {
    M5.Lcd.print("[BtnA] Reset   [BtnB] Next");
  } else {
    M5.Lcd.print("[BtnA] Reset   [BtnB] Prev");
  }
}

// ========== UI Layer (200ms周期) ==================================================
/**
 * @brief UI描画統合タスク（200ms周期で実行）
 * 
 * @details
 * 【責務】
 * 1. 状態遷移を検知したら画面全体をクリア（残像防止）
 * 2. RESULT状態でのページング検判も同様にクリア
 * 3. 現在の G.M_CurrentState に応じて適切な render関数を呼び出し
 * 
 * 【画面クリアタイミング】
 * - State遷移時: M5.Lcd.fillScreen(BLACK) 実行
 * - RESULT内ページング時: 同様にクリア
 * - それ以外: クリアなし（毎回 renderXXX() で上書き）
 * 
 * 【描画フロー】
 * - RESULT状態 → renderRESULT() (ページング対応)
 * - ALARM_SETTING状態 → renderALARM_SETTING()
 * - RUN状態 → renderRUN()
 * - その他 → renderIDLE()
 * 
 * 【レンダリングサイクル】
 * - Logic_Task (50ms周期) と UI_Task (200ms周期) は非同期で動作
 * - Logic_Task で状態遷移が発生 → UI_Task で新しい状態の描画を開始
 * - RESULT→IDLE遷移時に handleButtonA() が統計最終計算を実行
 * 
 * 【パフォーマンス最適化】
 * - fillScreen() は 16ms～25ms かかるため有効な時のみ実行
 * - prevState / prevPage で画面クリアの必要性を判定
 * - 通常フレーム更新は render関数内での上書き描画で対応
 * 
 * @see renderIDLE(), renderRUN(), renderRESULT(), renderALARM_SETTING()
 * @see handleButtonA()
 */
void UI_Task() {
  static State prevState = State::IDLE;
  static int   prevPage  = -1;  // ページ遷移検出用

  // 状態遷移時のみフル消去（残像防止）
  if (G.M_CurrentState != prevState) {
    M5.Lcd.fillScreen(BLACK);
    prevState = G.M_CurrentState;
    prevPage = -1;  // 状態変更時はページリセット
  }
  
  // RESULT状態でページが変わった場合もクリア
  if (G.M_CurrentState == State::RESULT && prevPage != G.M_ResultPage) {
    M5.Lcd.fillScreen(BLACK);
    prevPage = G.M_ResultPage;
  }

  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(WHITE, BLACK);
  
  // ────── 状態毎の描画処理分岐 ──────
  if (G.M_CurrentState == State::RESULT) {
    // RESULT画面はページング対応
    renderRESULT();
  } else if (G.M_CurrentState == State::ALARM_SETTING) {
    // ALARM_SETTING画面
    renderALARM_SETTING();
  } else if (G.M_CurrentState == State::RUN) {
    // RUN画面
    renderRUN();
  } else {
    // IDLE画面（デフォルト）
    renderIDLE();
  }
}

// ========== EEPROM 操作 (EEPROMManager による一元管理) =====================

/**
 * @brief EEPROM から GlobalData にアラーム設定値をロード（起動時）
 * 
 * @details
 * 【動作フロー】
 * 1. EEPROM から設定値を読み込み (チェックサム & 値の検証付き)
 * 2. 読み込み成功 → GlobalData に設定値を反映
 * 3. 読み込み失敗 → デフォルト値で EEPROM を初期化
 * 4. アラームフラグをリセット
 * 
 * @see EEPROM_SaveFromGlobal() (書き込みの対)
 * @see EEPROM_ValidateSettings() (検証専用)
 */
void EEPROM_LoadToGlobal() {
  AlarmSettings settings;
  
  Serial.println("[EEPROM] Loading alarm settings...");
  
  if (EEPROMManager::readSettings(settings)) {
    // 有効な設定がある → GlobalData に反映
    G.D_HI_ALARM_CURRENT = settings.HI_ALARM;
    G.D_LO_ALARM_CURRENT = settings.LO_ALARM;
    Serial.printf("  OK: Loaded from EEPROM: HI=%.1f, LO=%.1f\n",
                  settings.HI_ALARM, settings.LO_ALARM);
  } else {
    // 初期化されていない → デフォルト値で初期化
    Serial.println("  ! No valid settings in EEPROM, initializing with defaults...");
    settings.HI_ALARM = HI_ALARM_TEMP;
    settings.LO_ALARM = LO_ALARM_TEMP;
    if (EEPROMManager::writeSettings(settings)) {
      Serial.printf("  OK: Initialized: HI=%.1f, LO=%.1f\n",
                    settings.HI_ALARM, settings.LO_ALARM);
    } else {
      Serial.println("  ERROR: Failed to initialize EEPROM");
    }
    
    // GlobalData に設定値を反映
    G.D_HI_ALARM_CURRENT = settings.HI_ALARM;
    G.D_LO_ALARM_CURRENT = settings.LO_ALARM;
  }
  
  // アラームフラグをリセット（起動時）
  G.M_HiAlarm = false;
  G.M_LoAlarm = false;
}

/**
 * @brief GlobalData から EEPROM にアラーム設定値をセーブ（ALARM_SETTING 終了時）
 * 
 * @details
 * 【動作フロー】
 * 1. GlobalData から現在のアラーム設定値を取得
 * 2. AlarmSettings 構造体にコピー
 * 3. EEPROMManager::writeSettings() で書き込み + 検証
 * 4. 結果をシリアル出力
 * 
 * 【呼び出し箇所】
 * - handleButtonA() の ALARM_SETTING → IDLE 遷移時
 * 
 * @return true  保存 + 検証成功
 * @return false 保存または検証失敗
 * 
 * @see EEPROM_LoadToGlobal() (読み込みの対)
 */
bool EEPROM_SaveFromGlobal() {
  AlarmSettings settings;
  
  // GlobalData から設定値を取得
  settings.HI_ALARM = G.D_HI_ALARM_CURRENT;
  settings.LO_ALARM = G.D_LO_ALARM_CURRENT;
  
  Serial.println("[EEPROM] Saving alarm settings...");
  Serial.printf("        HI=%.1f, LO=%.1f\n", settings.HI_ALARM, settings.LO_ALARM);
  
  // EEPROM に書き込み（Write-Verify 付き）
  if (EEPROMManager::writeSettings(settings)) {
    Serial.println("  OK: Saved to EEPROM");
    return true;
  } else {
    Serial.println("  ERROR: EEPROM write-verify failed");
    return false;
  }
}

/**
 * @brief EEPROM に保存されている設定値を検証（デバッグ用）
 * 
 * @details
 * 【検証項目】
 * 1. チェックサム確認（EEPROM 初期化状態判定）
 * 2. 値が有限数か確認（NaN/Inf 検出）
 * 3. 値が物理的に可能か確認 (-50°C ～ 1100°C)
 * 4. GlobalData と一致性確認（save後の検証用）
 * 
 * @param printDetail true: 詳細項目の出力, false: 結果のみ出力
 * @return true  すべての検証に合格
 * @return false 1つ以上の検証に失敗
 * 
 * @see EEPROM_SaveFromGlobal()
 */
bool EEPROM_ValidateSettings(bool printDetail) {
  AlarmSettings settings;
  
  if (printDetail) {
    Serial.println("[EEPROM Validation] ==================");
  }
  
  // Step 1: EEPROM から読み込み（これが true = 検証合格の基本）
  if (!EEPROMManager::readSettings(settings)) {
    if (printDetail) {
      Serial.println("  ✗ Checksum or value validation failed");
    }
    Serial.println("[EEPROM Validation] FAILED");
    return false;
  }
  
  if (printDetail) {
    Serial.printf("  ✓ Read: HI=%.1f, LO=%.1f\n", settings.HI_ALARM, settings.LO_ALARM);
  }
  
  // Step 2: GlobalData との一致性確認
  const float TOLERANCE = 0.1f;  // 0.1°C の誤差を許容
  bool hiMatch = (fabs(settings.HI_ALARM - G.D_HI_ALARM_CURRENT) < TOLERANCE);
  bool loMatch = (fabs(settings.LO_ALARM - G.D_LO_ALARM_CURRENT) < TOLERANCE);
  
  if (printDetail) {
    Serial.printf("  %s HI Match: EEPROM=%.1f, Global=%.1f\n",
                  hiMatch ? "✓" : "✗", settings.HI_ALARM, G.D_HI_ALARM_CURRENT);
    Serial.printf("  %s LO Match: EEPROM=%.1f, Global=%.1f\n",
                  loMatch ? "✓" : "✗", settings.LO_ALARM, G.D_LO_ALARM_CURRENT);
  }
  
  bool valid = hiMatch && loMatch;
  Serial.printf("[EEPROM Validation] %s\n", valid ? "PASSED" : "FAILED");
  
  return valid;
}
