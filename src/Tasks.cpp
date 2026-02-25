#include "Global.h"

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
  G.M_ResultPage   = 0;     // RESULT画面ページ初期化
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
  
  const bool  btnANow  = M5.BtnA.isPressed();
  if (btnANow && !btnAPrev) {
    G.M_BtnA_Pressed = true;  // 立ち上がりエッジ
  }
  btnAPrev = btnANow;
  
  const bool  btnBNow  = M5.BtnB.isPressed();
  if (btnBNow && !btnBPrev) {
    G.M_BtnB_Pressed = true;  // 立ち上がりエッジ（ページング用）
  }
  btnBPrev = btnBNow;
}

// ========== Logic Layer (50ms周期) ===============================================
void Logic_Task() {
  if (G.M_BtnA_Pressed) {
    G.M_BtnA_Pressed = false;

    switch (G.M_CurrentState) {
      case State::IDLE:
        G.D_Sum          = 0.0;
        G.D_Count        = 0;
        G.D_Max          = -FLT_MAX;  // Phase 2: 最大値を最小に初期化
        G.D_Min          =  FLT_MAX;  // Phase 2: 最小値を最大に初期化
        G.D_M2           = 0.0;       // Phase 2: Welford法 二乗偏差をリセット
        G.M_CurrentState = State::RUN;
        break;

      case State::RUN:
        G.D_Average = (G.D_Count > 0)
                    ? static_cast<float>(G.D_Sum / G.D_Count)
                    : G.D_FilteredPV;   // サンプルなし時は現在値 (NAN の場合もある)
        
        // Phase 2: RUN→RESULT 遷移時に統計を計算
        if (G.D_Count > 0) {
          G.D_Range = G.D_Max - G.D_Min;
          G.D_StdDev = static_cast<float>(sqrt(G.D_M2 / G.D_Count));
        } else {
          G.D_Range = 0.0f;
          G.D_StdDev = 0.0f;
        }
        
        G.M_ResultPage   = 0;  // ページングをリセット
        G.M_CurrentState = State::RESULT;
        break;

      case State::RESULT:
        G.M_CurrentState = State::IDLE;
        break;
    }
  }

  if (G.M_CurrentState == State::RUN && !isnan(G.D_FilteredPV)) {
    G.D_Count++;
    
    // Welford法: 数値的に安定した逐次更新
    const double prevMean = (G.D_Count == 1) ? 0.0 : G.D_Sum / (G.D_Count - 1);
    const double delta  = G.D_FilteredPV - prevMean;
    
    G.D_Sum += G.D_FilteredPV;
    const double newMean = G.D_Sum / G.D_Count;
    const double delta2 = G.D_FilteredPV - newMean;
    
    G.D_M2 += delta * delta2;  // 二乗偏差を逐次累積
    
    // Max/Min 更新
    if (G.D_FilteredPV > G.D_Max) G.D_Max = G.D_FilteredPV;
    if (G.D_FilteredPV < G.D_Min) G.D_Min = G.D_FilteredPV;
  }
  
  // BtnB イベント処理（ページング用）
  if (G.M_BtnB_Pressed) {
    G.M_BtnB_Pressed = false;
    if (G.M_CurrentState == State::RESULT) {
      G.M_ResultPage = (G.M_ResultPage + 1) % 2;  // 0 ↔ 1 を切り替え
    }
  }
}

// ========== UI Layer (200ms周期) =================================================
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
  
  // Phase 2の拡張: RESULT画面は2ページに分割して大きく表示
  if (G.M_CurrentState == State::RESULT) {
    if (G.M_ResultPage == 0) {
      // ========== Page 1: Temp + Avg（最小限の表示）==========
      M5.Lcd.setTextSize(1);
      M5.Lcd.printf("RESULT (1/2)\n\n");
      
      // 温度表示（大きく）
      M5.Lcd.setTextSize(3);
      M5.Lcd.setCursor(10, 30);
      if (isnan(G.D_FilteredPV)) {
        M5.Lcd.printf("---.- C");
      } else {
        M5.Lcd.printf("%5.1f C", G.D_FilteredPV);
      }
      
      // 平均値ラベルと値
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(0, 100);
      M5.Lcd.printf("Avg:\n");
      M5.Lcd.setCursor(0, 140);
      if (isnan(G.D_Average)) {
        M5.Lcd.printf("---.-C");
      } else {
        M5.Lcd.printf("%5.1fC", G.D_Average);
      }
    } else {
      // ========== Page 2: StdDev + Max/Min + Range ==========
      M5.Lcd.setTextSize(1);
      M5.Lcd.printf("RESULT (2/2)\n");
      
      // StdDev - ラベルを大きく、値を超大型
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(0, 30);
      M5.Lcd.printf("StdDev:");
      M5.Lcd.setTextSize(3);
      M5.Lcd.setCursor(20, 60);
      M5.Lcd.printf("%5.1f C", G.D_StdDev);
      
      // Max と Min - ラベルを大きく、値は中型
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(0, 115);
      M5.Lcd.printf("Max:");
      M5.Lcd.setCursor(150, 115);
      M5.Lcd.printf("Min:");
      
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(10, 145);
      M5.Lcd.printf("%5.1fC", G.D_Max);
      M5.Lcd.setCursor(160, 145);
      M5.Lcd.printf("%5.1fC", G.D_Min);
      
      // Range - ラベルを大きく、値を超大型
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(0, 180);
      M5.Lcd.printf("Range:");
      M5.Lcd.setTextSize(3);
      M5.Lcd.setCursor(20, 210);
      M5.Lcd.printf("%5.1f C", G.D_Range);
    }
    
    // ボタンガイド（下端固定）
    M5.Lcd.setCursor(0, 220);
    M5.Lcd.setTextSize(1);
    if (G.M_ResultPage == 0) {
      M5.Lcd.print("[BtnA] Reset   [BtnB] Next");
    } else {
      M5.Lcd.print("[BtnA] Reset   [BtnB] Prev");
    }

  } else {
    // IDLE/RUN 状態は textSize(2) で表示
    M5.Lcd.setTextSize(2);
    
    // ── 状態行 ──
    const char* stateLabel = "      ";
    switch (G.M_CurrentState) {
      case State::IDLE:   stateLabel = "IDLE  "; break;
      case State::RUN:    stateLabel = "RUN   "; break;
      case State::RESULT: stateLabel = "RESULT"; break;
    }
    M5.Lcd.printf("STATE: %s\n\n", stateLabel);

    // ── 温度行 ──
    if (isnan(G.D_FilteredPV)) {
      M5.Lcd.printf("Temp:  ---.-  C\n\n");
    } else {
      M5.Lcd.printf("Temp:  %5.1f  C\n\n", G.D_FilteredPV);
    }

    // ── 状態別情報行 ──
    if (G.M_CurrentState == State::RUN) {
      M5.Lcd.printf("Samples: %5ld  \n", G.D_Count);
    } else {
      M5.Lcd.printf("               \n");
    }

    // ── ボタンガイド（下端固定） ──
    M5.Lcd.setCursor(0, 220);
    M5.Lcd.setTextSize(1);
    M5.Lcd.print("[BtnA] Start / Stop / Reset");
  }
}
