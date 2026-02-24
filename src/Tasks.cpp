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
  static bool btnPrev = false;          // エッジ検出用前回値 (IO_Task の実装詳細)
  const bool  btnNow  = M5.BtnA.isPressed();
  if (btnNow && !btnPrev) {
    G.M_BtnA_Pressed = true;  // 立ち上がりエッジ
  }
  btnPrev = btnNow;
}

// ========== Logic Layer (50ms周期) ===============================================
void Logic_Task() {
  if (G.M_BtnA_Pressed) {
    G.M_BtnA_Pressed = false;

    switch (G.M_CurrentState) {
      case State::IDLE:
        G.D_Sum          = 0.0;
        G.D_Count        = 0;
        G.M_CurrentState = State::RUN;
        break;

      case State::RUN:
        G.D_Average = (G.D_Count > 0)
                    ? static_cast<float>(G.D_Sum / G.D_Count)
                    : G.D_FilteredPV;   // サンプルなし時は現在値 (NAN の場合もある)
        G.M_CurrentState = State::RESULT;
        break;

      case State::RESULT:
        G.M_CurrentState = State::IDLE;
        break;
    }
  }

  if (G.M_CurrentState == State::RUN && !isnan(G.D_FilteredPV)) {
    G.D_Sum += G.D_FilteredPV;
    G.D_Count++;
  }
}

// ========== UI Layer (200ms周期) =================================================
void UI_Task() {
  static State prevState = State::IDLE;

  // 状態遷移時のみフル消去（残像防止）
  if (G.M_CurrentState != prevState) {
    M5.Lcd.fillScreen(BLACK);
    prevState = G.M_CurrentState;
  }

  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, BLACK);

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
  switch (G.M_CurrentState) {
    case State::RUN:
      M5.Lcd.printf("Samples: %5ld  \n", G.D_Count);
      break;
    case State::RESULT:
      if (isnan(G.D_Average)) {
        M5.Lcd.printf("Average: ---.-C\n");
      } else {
        M5.Lcd.printf("Average: %5.1fC\n", G.D_Average);
      }
      break;
    default:
      M5.Lcd.printf("               \n");
      break;
  }

  // ── ボタンガイド（下端固定） ──
  M5.Lcd.setCursor(0, 220);
  M5.Lcd.setTextSize(1);
  M5.Lcd.print("[BtnA] Start / Stop / Reset");
}
