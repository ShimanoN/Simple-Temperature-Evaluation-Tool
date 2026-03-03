# 簡易温度評価ツール - 基本仕様書

**プロジェクト**: 温度評価ツール v1.0.0  
**開発環境**: PlatformIO (VSCode拡張)  
**バージョン**: v1.0.0 (リリース済み / 2026-03-02)

---

## このドキュメントの役割

プロジェクト全体の構成・購入部品・開発フローをまとめたサマリー。  
詳細な手順は以下を参照:

| ドキュメント                          | 内容                                  |
| --------------------------------- | ----------------------------------- |
| **basic_spec.md**（本ファイル）         | 全体把握・購入部品・開発環境概要                  |
| **detailed_spec_hw.md**           | ハードウェア: 組み立て・配線・現場運用・保守           |
| **detailed_spec_sw.md**           | ソフトウェア: 環境構築・コード構成・書き込み・動作確認      |
| **docs/code/CODE_EXPLANATION.md** | コード設計の意図・アーキテクチャ解説・トラブルシューティング  |
| **docs/code/IMPLEMENTATION_GUIDE.md** | 詳細アーキテクチャ・API 実装ガイド           |
| **docs/KNOWN_ISSUES_AND_FUTURE_PLANS.md** | 既知問題と将来拡張計画                  |
| **docs/guides/FINAL_HANDOVER_GUIDE.md** | 新人向けハンドオーバーガイド（学習ロードマップ付き）|

---

## プロジェクト概要

M5Stack Basic V2.7 + K型熱電対による簡易温度計測ツール。  
生産現場（チャンバー・金型・槽）の温度を計測し平均値を表示。

**測定範囲**: -200〜1350℃（K型熱電対使用時）  
**想定用途**: 540℃チャンバー測定（目標600℃対応）  
**実装済み機能（v1.0.0）**:
- リアルタイム温度表示・計測開始/停止
- Welford 法による統計計算（平均・標準偏差・最大/最小・レンジ）
- HI/LO アラーム（ヒステリシス付き、閾値設定は EEPROM 保存）
- microSD カード CSV 記録（DATA_0000.csv 形式で自動採番）
- 3ボタン対応（BtnA: 状態遷移、BtnB: ページ切替/設定進入、BtnC: 設定値変更）

**設計上の注意**:
- シングルチャネル専用（マルチチャネル設計は廃止）
- SD カード CS ピン = GPIO4（`TFCARD_CS_PIN`）、MAX31855 CS ピン = GPIO5
  （両者を別 GPIO に固定して SPI バス衝突を回避）

---

## システム構成

```
[K型熱電対 600℃対応]
    │
    └─ T+/T- 端子（ねじ止め）
          │
    [MAX31855モジュール]
    （オスピンヘッダ付き完成品）
          │
          └─ SPI通信 5線
                │
          オス-メス ジャンパワイヤ
                │
          [Proto Module]
          （メスピン穴）
                │
                └─ スタック接続
                      │
                [M5Stack Basic V2.7]
                      │
                   LCD表示
                   BtnA操作
```

**動作**: BtnA で計測開始・停止・結果確認、BtnB でページ切替/アラーム設定進入、BtnC でアラーム閾値調整

---

## 購入部品リスト

総額: **約17,082円**

| No.    | 部品名                  | 型番/仕様              | 単価           | 購入先    |
|:------:| -------------------- | ------------------ | ------------:| ------ |
| 1      | M5Stack Basic V2.7   | M5Stack-BASIC-V2.7 | 7,990円       | Amazon |
| 2      | M5Stack Proto Module | 純正品                | 1,198円       | Amazon |
| 3      | MAX31855モジュール        | オスピンヘッダ付き完成品       | 904円         | Amazon |
| 4      | K型熱電対 600℃対応         | MSNDFL1.6-30-F2-Y  | 5,290円       | MISUMI |
| 5      | ジャンパワイヤ（オス-メス）       | 20cm 10本セット        | 700円         | Amazon |
| 6      | microSD カード          | 32GB以下、Class 10以上  | 〜1,000円     | Amazon |
| **合計** |                      |                    | **約17,082円** |        |

### 購入時の注意点

- **M5Stack Basic**: 必ず **V2.7** を購入（V2.6以前は画面仕様が異なる）
- **Proto Module**: **純正品**を選ぶ（互換品は接続不良の可能性あり）
- **MAX31855**: 「**オスピンヘッダ付き**」完成品を選ぶ（ハンダ付け不要）
  
      **注意**: モジュールによってはピン表記が `Vin` / `3VO` / `GND` / `DO` / `CS` / `CLK` のように異なります。信号名の対応は `DO`=MISO, `CLK`=SCK, `CS`=CS です。電源は基本的に **3.3V** を使用してください（誤って 5V を与えると破損する場合があります）。
- **ジャンパワイヤ**: 必ず「**オス-メス**」タイプを購入（メス-メスではない）
- **K型熱電対**: 測定対象温度に応じて選定（本仕様は600℃対応品）
- **microSD カード**: **FAT32 フォーマット**済み推奨。M5Stack 本体内蔵の microSD スロットに差し込む（追加配線不要）。

### 必要な工具

- **ドライバー**（熱電対端子のねじ止め用）のみ

**ハンダごて・ハンダ・ニッパーは不要です**（全て完成品を使用）

---

## 開発環境

### ソフトウェア

- **VSCode** + **PlatformIO IDE** 拡張機能
- フレームワーク: Arduino (ESP32)
- ライブラリ: M5Stack, Adafruit MAX31855 library

詳細なセットアップ手順は `detailed_spec_sw.md` を参照。

### プロジェクト構成

```
temp_eval_tool/
├── platformio.ini            # PlatformIO 設定
├── include/
│   ├── Global.h              # 共通型・定数・GlobalData 構造体
│   ├── Tasks.h               # タスク関数宣言
│   ├── EEPROMManager.h       # EEPROM 操作クラス
│   └── SDManager.h           # SD カード操作クラス
├── src/
│   ├── main.cpp              # setup / loop
│   ├── Tasks.cpp             # IO / Logic / UI タスク実装
│   ├── DisplayManager.h/.cpp # UI 表示管理クラス
│   ├── IOController.h/.cpp   # IO 制御クラス（テスト用）
│   ├── MeasurementCore.h/.cpp# 計測ロジッククラス（テスト用）
│   ├── EEPROMManager.cpp     # EEPROM 実装
│   └── SDManager.cpp         # SD カード実装
├── test/
│   └── test_measurement_core.cpp  # ユニットテスト
└── docs/
    ├── specs/
    │   ├── basic_spec.md              # 本ファイル（全体概要）
    │   ├── detailed_spec_hw.md        # ハードウェア仕様
    │   └── detailed_spec_sw.md        # ソフトウェア仕様
    ├── code/
    │   ├── CODE_EXPLANATION.md        # コード設計指針
    │   └── IMPLEMENTATION_GUIDE.md    # 詳細実装ガイド
    ├── guides/
    │   └── FINAL_HANDOVER_GUIDE.md    # 引継ぎガイド
    └── troubleshooting/
        └── TROUBLESHOOTING.md         # トラブルシューティング
```

---

## 開発フロー

```
1. 部品購入
      ↓
2. ハードウェア組み立て      → detailed_spec_hw.md
   （配線・スタッキング・接続確認）
      ↓
3. 開発環境セットアップ      → detailed_spec_sw.md
   （PlatformIO・ライブラリ）
      ↓
4. コード書き込み            → detailed_spec_sw.md
   （Build & Upload）
      ↓
5. 動作確認                  → detailed_spec_sw.md
   （起動・状態遷移・平均値計算）
      ↓
6. 精度確認                  → detailed_spec_hw.md
   （温度計との比較・応答性確認）
      ↓
7. 現場運用                  → detailed_spec_hw.md
   （測定手順・安全対策）
```

---

## アーキテクチャ概要

### 3層タスク構成

| タスク名           | 周期    | 役割                 |
| -------------- | ----- | ------------------ |
| **IO_Task**    | 10ms  | センサ読取・フィルタ処理・ボタン入力 |
| **Logic_Task** | 50ms  | 状態遷移・Welford統計計算（平均/標準偏差/Max/Min）・アラーム判定 |
| **UI_Task**    | 200ms | LCD画面描画            |

### 状態遷移

```
[IDLE] ──BtnA──> [RUN] ──BtnA──> [RESULT] ──BtnA──> [IDLE]
  待機              計測中            統計結果表示
    ↑                                    │
    └── BtnB ──> [ALARM_SETTING] ────────┘
                  アラーム閾値設定
                  (BtnB=HI/LO切替, BtnC=値変更)
```

詳細は `detailed_spec_sw.md` の「技術資料」を参照。

---

## 次のステップ

1. **部品を購入**したら → `detailed_spec_hw.md` で組み立て
2. **組み立て完了**したら → `detailed_spec_sw.md` で環境構築・書き込み
3. **動作確認完了**したら → `detailed_spec_hw.md` で現場運用開始
4. **既知問題・将来計画を確認**するなら → `docs/KNOWN_ISSUES_AND_FUTURE_PLANS.md`

---

**作成**: Shimano Takumi
**最終更新**: 2026年3月3日
