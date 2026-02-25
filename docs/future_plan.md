# 簡易温度評価ツール - 拡張計画（Phase 2以降）

**参照元**: basic_spec.md  
**対象**: Phase 1 完了後の機能拡張  
**最終更新**: 2026年2月25日

---

## 📋 実装状況

| Phase | 内容 | 状態 | 完了日 |
|:---:|:---|:----:|:-------:|
| 1 | 基本機能 | ✅ 完了 | 2026年2月24日 |
| **2** | **統計機能（ばらつき表示）** | **✅ 完了** | **2026年2月25日** |
| 3 | 閾値アラーム | 未着手 | — |
| 4 | SDカードデータ保存 | 未着手 | — |
| 5 | マルチチャンネル対応 | 未着手 | — |

---

## Phase 1 の完了条件

Phase 2 に進む前に、以下が完了していることを確認してください:

- [ ] ハードウェア組み立て完了
- [ ] プログラム書き込み完了
- [ ] 動作確認完了（状態遷移・平均値計算）
- [ ] 精度確認完了（±2℃以内）
- [ ] 現場運用を開始し、基本機能に問題がないことを確認

---

## 実装優先度と難易度

| Phase | 内容             | ハードウェア追加     | 開発難易度 | 優先度 | 完了状態 |
|:-----:| -------------- |:----------:|:-----:|:---:|:----:|
| **2** | 統計機能（ばらつき表示）  | なし         | 低     | ★★★ | ✅ |
| **3** | 閾値アラーム         | なし         | 低〜中   | ★★★ | — |
| **4** | SDカードデータ保存     | RTCモジュール推奨  | 中     | ★★☆ | — |
| **5** | マルチチャンネル対応     | MAX31855×3追加 | 中〜高 | ★☆☆ | — |

---

## Phase 2: 統計機能（ばらつき表示） ✅ 完了

### ✨ 実装概要

**状態**: 🟢 **完了** (2026年2月25日)

**目的**: RUN状態で計測したデータから統計情報を計算し、ページング機能で大きく見やすく表示する。

### 追加機能（実装済）

| 変数名        | 内容                      | 初期化方法 |
| ---------- | ----------------------- | -------- |
| `D_Max`    | 計測期間中の最高温度 [°C]         | `-FLT_MAX` |
| `D_Min`    | 計測期間中の最低温度 [°C]         | `+FLT_MAX` |
| `D_Range`  | Max − Min（温度変動幅） [°C]    | 自動計算 |
| `D_StdDev` | 標準偏差 σ（ばらつきの大きさ） [°C]   | Welford法で計算 |
| `M_ResultPage` | RESULT画面のページ番号 (0/1) | 0（リセット時） |
| `M_BtnB_Pressed` | ボタンB エッジ検出フラグ | false |

### RESULT 画面実装仕様

#### **Page 1/2: 温度と平均値**

```
RESULT (1/2)

Temp:
540.1 C

Avg:
540.1C

[BtnA] Reset   [BtnB] Next
```

**特徴**:
- textSize(2) で統一（全コンテンツ） ← UI改善で実装
- ラベル + 値の2段構成で見やすく
- BtnB で次ページへ遷移

#### **Page 2/2: 詳細統計情報**

```
RESULT (2/2)

StdDev:         Range:
1.2             3.8

Max:            Min:
542.3           538.5

[BtnA] Reset   [BtnB] Prev
```

**特徴**:
- 左右2列レイアウトで情報を効率的に表示
- textSize(2) で統一（全コンテンツ） ← UI改善で実装
- BtnB で前ページへ遷移

### 実装変更箇所

#### Global.h への追加（`GlobalData` 構造体）

```cpp
struct GlobalData {
  // データレジスタ群（既存）
  float  D_RawPV;
  float  D_FilteredPV;
  double D_Sum;
  long   D_Count;
  float  D_Average;

  // Phase 2 追加（実装済）
  float  D_Max;          // 最大値（RUN開始時 -FLT_MAX で初期化）
  float  D_Min;          // 最小値（RUN開始時 +FLT_MAX で初期化）
  float  D_Range;        // レンジ = Max - Min
  double D_M2;           // Welford法 二乗偏差累積
  float  D_StdDev;       // 標準偏差

  // 内部リレー群
  State  M_CurrentState; // 現在の状態
  bool   M_BtnA_Pressed; // ボタンA 立ち上がりエッジ検出フラグ
  bool   M_BtnB_Pressed; // ボタンB 立ち上がりエッジ検出フラグ（ページング用）
  int    M_ResultPage;   // RESULT画面のページ番号（0 or 1）
};
```

#### Tasks.cpp の変更（実装済）

**IO_Task での BtnB イベント検出:**
```cpp
M5.update();
static bool btnAPrev = false;
static bool btnBPrev = false;

const bool  btnANow  = M5.BtnA.isPressed();
if (btnANow && !btnAPrev) {
  G.M_BtnA_Pressed = true;
}
btnAPrev = btnANow;

const bool  btnBNow  = M5.BtnB.isPressed();
if (btnBNow && !btnBPrev) {
  G.M_BtnB_Pressed = true;  // ページング用
}
btnBPrev = btnBNow;
```

**Logic_Task での IDLE→RUN 初期化と RESULT 統計計算:**
```cpp
case State::IDLE:
  G.D_Sum          = 0.0;
  G.D_Count        = 0;
  G.D_Max          = -FLT_MAX;   // Phase 2 追加
  G.D_Min          =  FLT_MAX;   // Phase 2 追加
  G.D_M2           = 0.0;        // Phase 2 追加
  G.M_ResultPage   = 0;          // ページングをリセット
  G.M_CurrentState = State::RUN;
  break;

case State::RUN:
  G.D_Average = (G.D_Count > 0)
              ? static_cast<float>(G.D_Sum / G.D_Count)
              : G.D_FilteredPV;
  
  if (G.D_Count > 0) {
    G.D_Range = G.D_Max - G.D_Min;
    G.D_StdDev = static_cast<float>(sqrt(G.D_M2 / G.D_Count));
  } else {
    G.D_Range = 0.0f;
    G.D_StdDev = 0.0f;
  }
  
  G.M_ResultPage   = 0;
  G.M_CurrentState = State::RESULT;
  break;
```

**RUN 中の Welford 法による逐次更新:**
```cpp
if (G.M_CurrentState == State::RUN && !isnan(G.D_FilteredPV)) {
  G.D_Count++;
  const double prevMean = (G.D_Count == 1) ? 0.0 : G.D_Sum / (G.D_Count - 1);
  const double delta  = G.D_FilteredPV - prevMean;
  
  G.D_Sum += G.D_FilteredPV;
  const double newMean = G.D_Sum / G.D_Count;
  const double delta2 = G.D_FilteredPV - newMean;
  
  G.D_M2 += delta * delta2;  // 二乗偏差を逐次累積
  
  if (G.D_FilteredPV > G.D_Max) G.D_Max = G.D_FilteredPV;
  if (G.D_FilteredPV < G.D_Min) G.D_Min = G.D_FilteredPV;
}

// BtnB イベント処理（ページング）
if (G.M_BtnB_Pressed) {
  G.M_BtnB_Pressed = false;
  if (G.M_CurrentState == State::RESULT) {
    G.M_ResultPage = (G.M_ResultPage + 1) % 2;  // 0 ↔ 1 を切り替え
  }
}
```
// BtnB イベント処理（ページング）
if (G.M_BtnB_Pressed) {
  G.M_BtnB_Pressed = false;
  if (G.M_CurrentState == State::RESULT) {
    G.M_ResultPage = (G.M_ResultPage + 1) % 2;  // 0 ↔ 1 を切り替え
  }
}
```

**UI_Task での RESULT 画面実装:**

ページ遷移時の画面クリアと2ページレイアウト実装により、統一感のある表示を実現：

```cpp
static State prevState = State::IDLE;
static int   prevPage  = -1;

// 状態遷移時のクリア
if (G.M_CurrentState != prevState) {
  M5.Lcd.fillScreen(BLACK);
  prevState = G.M_CurrentState;
  prevPage = -1;
}

// ページ遷移時もクリア
if (G.M_CurrentState == State::RESULT && prevPage != G.M_ResultPage) {
  M5.Lcd.fillScreen(BLACK);
  prevPage = G.M_ResultPage;
}

// Page 1
if (G.M_ResultPage == 0) {
  M5.Lcd.setTextSize(1);
  M5.Lcd.printf("RESULT (1/2)\n\n");
  
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 40);
  M5.Lcd.printf("Temp:\n");
  M5.Lcd.setCursor(0, 70);
  M5.Lcd.printf("%5.1f C", G.D_FilteredPV);  // isnan() チェック省略
  
  M5.Lcd.setCursor(0, 120);
  M5.Lcd.printf("Avg:\n");
  M5.Lcd.setCursor(0, 150);
  M5.Lcd.printf("%5.1fC", G.D_Average);
}
// Page 2
else {
  M5.Lcd.setTextSize(1);
  M5.Lcd.printf("RESULT (2/2)\n");
  
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 30);
  M5.Lcd.printf("StdDev:");
  M5.Lcd.setCursor(160, 30);
  M5.Lcd.printf("Range:");
  
  M5.Lcd.setCursor(0, 60);
  M5.Lcd.printf("%5.1f", G.D_StdDev);
  M5.Lcd.setCursor(160, 60);
  M5.Lcd.printf("%5.1f", G.D_Range);
  
  M5.Lcd.setCursor(0, 110);
  M5.Lcd.printf("Max:");
  M5.Lcd.setCursor(160, 110);
  M5.Lcd.printf("Min:");
  
  M5.Lcd.setCursor(0, 140);
  M5.Lcd.printf("%5.1f", G.D_Max);
  M5.Lcd.setCursor(160, 140);
  M5.Lcd.printf("%5.1f", G.D_Min);
}

// ボタンガイド
M5.Lcd.setCursor(0, 220);
M5.Lcd.setTextSize(1);
if (G.M_ResultPage == 0) {
  M5.Lcd.print("[BtnA] Reset   [BtnB] Next");
} else {
  M5.Lcd.print("[BtnA] Reset   [BtnB] Prev");
}
```

### UI 改善（実装済）

**ページング機能の追加**:
- BtnB でページ切り替え（0 ↔ 1）
- RESULT 画面を2ページに分割
- ページ遷移時に画面を完全クリア

**フォントサイズの統一**: 
- ラベル・値の区別をなくし、すべてコンテンツを **textSize(2)** で統一
- 視覚的統一感を実現

**レイアウト最適化**:
- ページ1: Temp と Avg を中心に表示
- ページ2: 4つの統計情報を2列×2行で効率的に表示
- setCursor() で位置を正確に制御

### 開発難易度

**低** — ハードウェア変更なし。純粋なソフトウェア追加。

---

## Phase 3: 閾値アラーム

### 追加機能

| 機能         | 内容                              |
| ---------- | ------------------------------- |
| **上限アラーム** | 温度が `HI_ALARM_TEMP` を超えた場合に警告   |
| **下限アラーム** | 温度が `LO_ALARM_TEMP` を下回った場合に警告  |
| **アラーム出力** | ビープ音（スピーカー）＋ LCD 赤色表示           |
| **ヒステリシス** | 一度アラームが鳴ったら一定幅戻るまで再鳴動を抑制（誤報防止） |

### 実装方針の選択

#### Step 1（推奨）: コンパイル時定数で実装

```cpp
// Global.h に追加
constexpr float HI_ALARM_TEMP    = 600.0f;  // 上限 [°C]
constexpr float LO_ALARM_TEMP    = 400.0f;  // 下限 [°C]
constexpr float ALARM_HYSTERESIS =   5.0f;  // ヒステリシス幅 [°C]
```

変更のたびに再コンパイルが必要だが、UI 実装が不要なため最もシンプル。  
まずここで動作確認してから、Step 2（実行時設定）へ進むことを推奨。

#### Step 2（将来）: BtnB / BtnC で実行時設定

BtnA は状態遷移に使用済み。BtnB で値アップ、BtnC で値ダウン、  
または BtnA 長押しで「設定モード」に入る設計を検討する。

> **注意**: M5Stack の内蔵スピーカーは音量が小さく、チャンバー近辺では  
> 聞こえない可能性が高い。**LCD の赤色表示を必ず併用すること。**

### 実装変更箇所

#### Global.h の `GlobalData` に追加

```cpp
bool   M_HiAlarm;  // 上限アラーム中フラグ
bool   M_LoAlarm;  // 下限アラーム中フラグ
```

#### Tasks.cpp（IO_Task 末尾にアラーム判定を追加）

```cpp
if (!isnan(G.D_FilteredPV)) {
  // 上限アラーム（ヒステリシス付き）
  if (!G.M_HiAlarm && G.D_FilteredPV >= HI_ALARM_TEMP) {
    G.M_HiAlarm = true;
    M5.Speaker.tone(2000, 500);  // 2kHz, 500ms
  } else if (G.M_HiAlarm && G.D_FilteredPV < HI_ALARM_TEMP - ALARM_HYSTERESIS) {
    G.M_HiAlarm = false;
  }
  // 下限アラーム（同様）
  if (!G.M_LoAlarm && G.D_FilteredPV <= LO_ALARM_TEMP) {
    G.M_LoAlarm = true;
    M5.Speaker.tone(1000, 500);
  } else if (G.M_LoAlarm && G.D_FilteredPV > LO_ALARM_TEMP + ALARM_HYSTERESIS) {
    G.M_LoAlarm = false;
  }
}
```

#### Tasks.cpp（UI_Task の温度行の色切り替え）

```cpp
if (G.M_HiAlarm || G.M_LoAlarm) {
  M5.Lcd.setTextColor(RED, BLACK);
} else {
  M5.Lcd.setTextColor(WHITE, BLACK);
}
```

### 開発難易度

**低〜中** — ハードウェア変更なし。ヒステリシス値の現場確認がポイント。

---

## Phase 4: SDカードデータ保存

### 追加機能

| 機能        | 内容                                     |
| --------- | -------------------------------------- |
| **CSV保存** | 測定データを microSD に CSV 形式で書き出し            |
| **ファイル名** | `YYMMDD_HHMMSS.csv`（要RTC）または連番 `LOG_NNN.csv` |
| **保存内容**  | 経過時間・温度・状態・サンプル数・平均・標準偏差など              |

### ⚠️ タイムスタンプ問題（要事前決定）

M5Stack Basic V2.7 には **RTC（リアルタイムクロック）が搭載されていない**。  
実装前に以下のいずれかを選択すること：

| 対応策                   | コスト       | 難易度 | 備考                           |
| --------------------- | --------- | --- | ---------------------------- |
| **DS3231 RTC 追加**（推奨） | 約300〜500円 | 低   | I²C 接続、ライブラリ豊富               |
| WiFi + NTP 時刻同期       | 0円        | 中〜高 | 現場に WiFi 環境が必要               |
| millis() 相対時間のみ（暫定）   | 0円        | 低   | 「計測開始から何秒経過か」のみ記録。後でRTC追加に備えた設計推奨 |

### 必要な追加部品（DS3231 を選ぶ場合）

| 部品              | 仕様            | 単価       | 購入先    |
| --------------- | ------------- | --------: | ------ |
| microSD カード     | 4GB以上 Class10 | 約500円    | Amazon |
| DS3231 RTC モジュール | CR2032 電池付き | 約300〜500円 | Amazon |

> M5Stack Basic V2.7 には microSD スロットが搭載済み（追加スロット不要）。  
> SD カードの SPI は LCD と同バスを共有するため、CS ピン管理に注意。

### CSV フォーマット例

```csv
ElapsedSec,Temp_C,State,Samples,Average_C,StdDev_C,Max_C,Min_C
0,540.2,RUN,1,540.2,0.0,540.2,540.2
1,540.3,RUN,2,540.25,0.05,540.3,540.2
120,540.1,RESULT,240,540.1,1.2,542.3,538.5
```

### 実装方針（タスク分担）

```
IO_Task    → SD カードへの書き込み実行（外部デバイス通信 = IO 層の責任）
Logic_Task → 書き込みトリガー管理（例: 10サンプルごとにフラグ ON）
UI_Task    → SD エラー状態の表示
```

- RUN 開始時: ファイルオープン・ヘッダ行書き込み
- RUN 中: バッファが溜まったら書き出し（毎サンプル書くと IO 負荷大）
- RESULT 遷移時: 統計行を追記してファイルクローズ

### 開発難易度

**中** — RTC方針の決定と SD 初期化エラー処理が実装のポイント。

---

## Phase 5: マルチチャンネル対応

### 追加機能

| 機能          | 内容                      |
| ----------- | ----------------------- |
| **複数点同時測定** | MAX31855を複数接続し、最大4点同時測定 |
| **チャンネル切替** | BtnB/BtnCでチャンネル切替表示     |

### 必要な追加部品

| 部品            | 数量  | 単価     | 合計           |
| ------------- |:---:| ------:| ------------:|
| MAX31855モジュール | 3個  | 904円   | 2,712円       |
| K型熱電対         | 3本  | 5,290円 | 15,870円      |
| ジャンパワイヤ       | 15本 | -      | 約500円        |
| **合計**        |     |        | **約19,082円** |

### 実装方針

- **GPIO割り当て**: 
  - CH1: CS=GPIO5
  - CH2: CS=GPIO15
  - CH3: CS=GPIO2
  - CH4: CS=GPIO12
  - SCK/MISOは共通
- **UI変更**: チャンネル表示追加
- **データ構造**: 配列化（`D_FilteredPV[4]`等）

### 開発難易度

**中〜高**（GPIO管理とUI設計の複雑化）

---

## 今後の検討事項（未確定）

- **Wi-Fi + クラウド連携**: データのリモートモニタリング・CSV 自動アップロード
- **Bluetooth (BLE)**: スマートフォンへのリアルタイム表示。ただし高温環境への接近が困難な現場では活用シーン限定的
- **外部ディスプレイ**: 大画面表示（距離が離れた場所からの視認性向上）
- **防塵・防滴ケース**: 過酷環境対応

---

## 📝 更新履歴

| 日付 | 更新内容 | 実施者 |
|:---:|:---|:---|
| 2026年2月24日 | Phase 1 完了 | Shimano |
| 2026年2月25日 | **Phase 2 完了**（統計機能 + UI改善実装） | GitHub Copilot |

---

**作成**: Shimano  
**最終更新**: 2026年2月25日  
**最終実装**: GitHub Copilot (Phase 2)
