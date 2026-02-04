# 簡易温度評価ツール - 拡張計画（Phase 2以降）

**参照元**: basic_spec.md  
**対象**: Phase 1 完了後の機能拡張

---

## Phase 1 の完了条件

Phase 2 に進む前に、以下が完了していることを確認してください:

- [ ] ハードウェア組み立て完了
- [ ] プログラム書き込み完了
- [ ] 動作確認完了（状態遷移・平均値計算）
- [ ] 精度確認完了（±2℃以内）
- [ ] 現場運用を開始し、基本機能に問題がないことを確認

---

## Phase 2: 統計機能の追加

### 追加機能

| 機能 | 内容 |
|---|---|
| **標準偏差（σ）** | 温度のばらつきを数値化 |
| **最大値（Max）** | 計測期間中の最高温度 |
| **最小値（Min）** | 計測期間中の最低温度 |
| **レンジ（Range）** | Max - Min（温度変動幅） |

### 表示イメージ

```
STATE: RESULT
Temp: 540.2 C

Average: 540.1 C
Std Dev: 1.2 C
Max: 542.3 C
Min: 538.5 C
Range: 3.8 C

[BtnA] Reset
```

### 実装方針

- **ハードウェア変更**: なし（ソフトウェア更新のみ）
- **追加変数**: `D_Max`, `D_Min`, `D_SumSq`（標準偏差計算用）
- **UI変更**: RESULT画面のレイアウト変更
- **計算式**: 
  - 標準偏差 σ = sqrt((ΣX² / n) - (ΣX / n)²)
  - レンジ = Max - Min

### 開発難易度

**低**（Phase 1のコード構造を活用すれば容易に実装可能）

---

## Phase 3: データ保存機能

### 追加機能

| 機能 | 内容 |
|---|---|
| **SDカード保存** | 測定データをCSV形式で保存 |
| **ファイル形式** | 日時・温度・平均値・標準偏差等をCSV出力 |
| **ファイル名** | `YYMMDD_HHMMSS.csv`（測定開始時刻） |

### CSVフォーマット例

```csv
DateTime,Temp_C,State,Samples,Average,StdDev
2025-01-15 14:30:00,540.2,RUN,100,540.1,1.2
2025-01-15 14:30:01,540.3,RUN,120,540.2,1.1
...
```

### 必要な追加部品

| 部品 | 型番/仕様 | 単価 | 購入先 |
|---|---|---:|---|
| microSDカード | 4GB以上 Class10 | 約500円 | Amazon |

**注意**: M5Stack Basic V2.7 には標準でSDカードスロットが搭載されています。

### 実装方針

- **ライブラリ**: Arduino標準の `SD.h` を使用
- **保存タイミング**: 
  - RUN開始時にファイル作成
  - 10サンプル毎にCSV書き込み
  - RESULT遷移時にファイルクローズ
- **エラー処理**: SDカード未挿入時の警告表示

### 開発難易度

**中**（SD操作の経験があれば容易）

---

## Phase 4: Bluetooth データ転送（オプション）

### 追加機能

| 機能 | 内容 |
|---|---|
| **BLE通信** | スマートフォンやPCへリアルタイムデータ送信 |
| **プロトコル** | BLE GATT（汎用プロファイル） |
| **用途** | 離れた場所からのモニタリング |

### 実装方針

- **ライブラリ**: ESP32標準の `BLEServer.h` を使用
- **通信内容**: 温度・状態・サンプル数をJSON形式で送信
- **受信側**: 専用スマホアプリまたはPC用モニタツール

### 開発難易度

**高**（BLE通信の実装経験が必要）

---

## Phase 5: マルチチャンネル対応

### 追加機能

| 機能 | 内容 |
|---|---|
| **複数点同時測定** | MAX31855を複数接続し、最大4点同時測定 |
| **チャンネル切替** | BtnB/BtnCでチャンネル切替表示 |

### 必要な追加部品

| 部品 | 数量 | 単価 | 合計 |
|---|:---:|---:|---:|
| MAX31855モジュール | 3個 | 904円 | 2,712円 |
| K型熱電対 | 3本 | 5,290円 | 15,870円 |
| ジャンパワイヤ | 15本 | - | 約500円 |
| **合計** | | | **約19,082円** |

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

## 実装優先度

| Phase | 優先度 | 理由 |
|---|:---:|---|
| **Phase 2（統計機能）** | **高** | 現場ニーズが高く、実装が容易 |
| **Phase 3（SD保存）** | **中** | データ蓄積が必要な場合に有用 |
| **Phase 4（Bluetooth）** | **低** | 高温環境では接近が困難な場合のみ |
| **Phase 5（マルチ）** | **低** | コスト増・複雑化を考慮 |

---

## Phase 2 実装ガイド（参考）

Phase 2 を実装する場合、以下の変更が必要です。

### 1. Global.h への追加

```cpp
struct GlobalData {
  // 既存の変数
  float  D_RawPV;
  float  D_FilteredPV;
  double D_Sum;
  long   D_Count;
  float  D_Average;

  // Phase 2 追加変数
  double D_SumSq;      // 二乗和（標準偏差計算用）
  float  D_Max;        // 最大値
  float  D_Min;        // 最小値
  float  D_StdDev;     // 標準偏差
  float  D_Range;      // レンジ

  // 既存の変数
  State  M_CurrentState;
  bool   M_BtnA_Pressed;
  bool   M_BtnA_Prev;
};
```

### 2. Logic_Task の変更

```cpp
// IDLE → RUN 遷移時
case STATE_IDLE:
  G.D_Sum    = 0.0;
  G.D_SumSq  = 0.0;  // 追加
  G.D_Count  = 0;
  G.D_Max    = -999.0;  // 追加
  G.D_Min    = 999.0;   // 追加
  G.M_CurrentState = STATE_RUN;
  break;

// RUN → RESULT 遷移時
case STATE_RUN:
  if (G.D_Count > 0) {
    G.D_Average = G.D_Sum / G.D_Count;
    // 標準偏差計算
    double variance = (G.D_SumSq / G.D_Count) - (G.D_Average * G.D_Average);
    G.D_StdDev = sqrt(variance);
    // レンジ計算
    G.D_Range = G.D_Max - G.D_Min;
  }
  G.M_CurrentState = STATE_RESULT;
  break;

// 積算処理（RUN状態のみ）
if (G.M_CurrentState == STATE_RUN) {
  if (!isnan(G.D_FilteredPV)) {
    G.D_Sum += G.D_FilteredPV;
    G.D_SumSq += (G.D_FilteredPV * G.D_FilteredPV);  // 追加
    G.D_Count++;
    // 最大値・最小値更新
    if (G.D_FilteredPV > G.D_Max) G.D_Max = G.D_FilteredPV;
    if (G.D_FilteredPV < G.D_Min) G.D_Min = G.D_FilteredPV;
  }
}
```

### 3. UI_Task の変更

```cpp
if (G.M_CurrentState == STATE_RESULT) {
  M5.Lcd.setTextSize(1);  // 小さいフォントに変更
  M5.Lcd.print("Average: "); M5.Lcd.print(G.D_Average, 1); M5.Lcd.println(" C");
  M5.Lcd.print("Std Dev: "); M5.Lcd.print(G.D_StdDev, 2); M5.Lcd.println(" C");
  M5.Lcd.print("Max: ");     M5.Lcd.print(G.D_Max, 1);    M5.Lcd.println(" C");
  M5.Lcd.print("Min: ");     M5.Lcd.print(G.D_Min, 1);    M5.Lcd.println(" C");
  M5.Lcd.print("Range: ");   M5.Lcd.print(G.D_Range, 1);  M5.Lcd.println(" C");
}
```

---

## 今後の検討事項

### ハードウェア面

- バッテリーモジュール追加（現場の移動範囲拡大）
- 外部ディスプレイ対応（大画面表示）
- 防塵・防滴ケース（過酷環境対応）

### ソフトウェア面

- Wi-Fi経由でのデータアップロード
- クラウド連携（データ解析・レポート生成）
- アラート機能（設定温度超過時の警告）

### 運用面

- 定期校正スケジュールの自動化
- 測定履歴のトレーサビリティ管理
- 作業手順のマニュアル化

---

## まとめ

Phase 1 で基本機能を確立し、現場で実際に使用してフィードバックを得ることが重要です。  
Phase 2 以降の拡張は、**現場のニーズ**と**開発リソース**を考慮して優先順位を決定してください。

特に **Phase 2（統計機能）** は実装が容易で効果が高いため、Phase 1 完了後すぐに着手することを推奨します。

---

**作成**: 生産技術開発チーム  
**最終更新**: 2025年
