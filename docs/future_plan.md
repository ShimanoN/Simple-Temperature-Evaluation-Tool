# 簡易温度評価ツール - 拡張計画（Phase 2以降）

**参照元**: basic_spec.md  
**対象**: Phase 1 完了後の機能拡張  
**最終更新**: 2026年2月24日

---

## Phase 1 の完了条件

Phase 2 に進む前に、以下が完了していることを確認してください:

- [ ] ハードウェア組み立て完了
- [ ] プログラム書き込み完了
- [ ] 動作確認完了（状態遷移・平均値計算）
- [ ] 精度確認完了（±2℃以内）
- [ ] 現場運用を開始し、基本機能に問題がないことを確認

---

## 実装優先度まとめ

| Phase | 内容             | ハードウェア追加     | 開発難易度 | 優先度 |
|:-----:| -------------- |:----------:|:-----:|:---:|
| **2** | 統計機能（ばらつき表示）  | なし         | 低     | ★★★ |
| **3** | 閾値アラーム         | なし         | 低〜中   | ★★★ |
| **4** | SDカードデータ保存     | RTCモジュール推奨  | 中     | ★★☆ |
| **5** | マルチチャンネル対応     | MAX31855×3追加 | 中〜高 | ★☆☆ |

---

## Phase 2: 統計機能（ばらつき表示）

### 追加機能

| 変数名        | 内容                      |
| ---------- | ----------------------- |
| `D_Max`    | 計測期間中の最高温度 [°C]         |
| `D_Min`    | 計測期間中の最低温度 [°C]         |
| `D_Range`  | Max − Min（温度変動幅） [°C]    |
| `D_StdDev` | 標準偏差 σ（ばらつきの大きさ） [°C]   |

### RESULT 画面イメージ

```
STATE: RESULT

Temp:  540.2 C

Avg:   540.1 C
StdDev:  1.2 C
Max:   542.3 C
Min:   538.5 C
Range:   3.8 C

[BtnA] Reset
```

> **注意**: 現在の `textSize(2)` では行数が入りきらないため、  
> RESULT 画面のみ `textSize(1)` に切り替えるか、BtnB でページ送りを実装する。

### 標準偏差の計算方式

`ΣX²` 方式（`D_SumSq / n - avg²`）は大きな数同士の減算で浮動小数点誤差が出やすい。  
**Welford's online algorithm** を推奨（逐次更新で数値的に安定）：

```
delta  = x - mean_prev
mean  += delta / n
M2    += delta * (x - mean_new)
σ      = sqrt(M2 / n)
```

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

  // Phase 2 追加
  float  D_Max;     // 最大値（RUN開始時 -FLT_MAX で初期化）
  float  D_Min;     // 最小値（RUN開始時 +FLT_MAX で初期化）
  float  D_Range;   // レンジ = Max - Min
  double D_M2;      // Welford法 二乗偏差累積
  float  D_StdDev;  // 標準偏差

  // 内部リレー群（既存）
  State  M_CurrentState;
  bool   M_BtnA_Pressed;
};
```

#### Tasks.cpp の変更（Logic_Task）

```cpp
// IDLE → RUN 遷移時のリセット
case State::IDLE:
  G.D_Sum          = 0.0;
  G.D_Count        = 0;
  G.D_Max          = -FLT_MAX;   // Phase 2 追加
  G.D_Min          =  FLT_MAX;   // Phase 2 追加
  G.D_M2           = 0.0;        // Phase 2 追加
  G.M_CurrentState = State::RUN;
  break;

// RUN → RESULT 遷移時の統計計算
case State::RUN:
  if (G.D_Count > 0) {
    G.D_Average = static_cast<float>(G.D_Sum / G.D_Count);
    G.D_Range   = G.D_Max - G.D_Min;
    G.D_StdDev  = static_cast<float>(sqrt(G.D_M2 / G.D_Count));
  }
  G.M_CurrentState = State::RESULT;
  break;

// RUN状態の積算処理
if (G.M_CurrentState == State::RUN && !isnan(G.D_FilteredPV)) {
  G.D_Count++;
  const double delta  = G.D_FilteredPV - G.D_Sum / (G.D_Count - 1 + (G.D_Count == 1));
  G.D_Sum    += G.D_FilteredPV;
  const double delta2 = G.D_FilteredPV - G.D_Sum / G.D_Count;
  G.D_M2     += delta * delta2;  // Welford 更新
  if (G.D_FilteredPV > G.D_Max) G.D_Max = G.D_FilteredPV;
  if (G.D_FilteredPV < G.D_Min) G.D_Min = G.D_FilteredPV;
}
```

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

**作成**: Shimano  
**最終更新**: 2026年2月24日
