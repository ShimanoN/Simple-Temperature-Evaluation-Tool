# 簡易温度評価ツール - 基本仕様書

プロジェクト: 温度評価ツール Phase 1  
開発環境: Antigravity (VSCode + Gemini + PlatformIO)  
対象: 生産技術開発チーム内用

---

## このドキュメントの役割

全体の構成・何をどうするかのサマリーをまとめたものです。  
実際の手順や詳細は以下の詳細仕様書を参照してください。

| ドキュメント | 内容 |
|---|---|
| **このファイル** (basic_spec.md) | 全体把握・購入部品・開発環境・フロー |
| detailed_spec_hw.md | ハードウェア: 組み立て・配線・現場運用 |
| detailed_spec_sw.md | ソフトウェア: 開発環境・コード・書き込み・動作確認 |

---

## プロジェクト概要

M5Stack Basic V2.7 + K型熱電対を組み合わせた簡易温度計測ツールです。  
チャンバー・金型・槽など生産現場の温度を計測し、平均値を表示するものです。  
Phase 1では「測定・積算・平均値表示」までを実装します。

測定範囲: -200〜1350℃（K型熱電対使用時）  
想定測定対象: 540℃チャンバー（目標600℃対応）

---

## システム構成

```
[K型熱電対 600℃] ─── T+/T- ─── [MAX31855モジュール] ─── SPI ─── [Proto Module] ─── スタック ─── [M5Stack Basic V2.7]
                                                                                                        │
                                                                                                     LCD表示
                                                                                                     BtnA操作
```

M5Stackの画面で温度リアルタイム表示・BtnAで計測開始・停止・結果確認。

---

## 購入部品リスト

合計: **約16,082円**

| No. | 部品名 | 型番/仕様 | 単価 | 購入先 |
|---|---|---|---|---|
| 1 | M5Stack Basic V2.7 | M5Stack-BASIC-V2.7 | 7,990円 | Amazon |
| 2 | M5Stack Proto Module | Proto Module（純正品） | 1,198円 | Amazon |
| 3 | MAX31855モジュール（オスピン付き） | MAX31855 SPI K-type | 904円 | Amazon |
| 4 | K型熱電対（600℃対応） | MSNDFL1.0-50-F0.3-Y | 5,290円 | MISUMI |
| 5 | ジャンパワイヤ（オス-メス） | 20cm 10本セット | 700円 | Amazon |

購入時のポイント:
- M5Stack Basic は V2.7 を指定（V2.6以前は画面が違う）
- Proto Module は純正品のみ（互換品は接続不良になる）
- MAX31855 は「オスピンヘッダ付き」の完成品を選ぶ
- ジャンパワイヤは「オス-メス」を選ぶ（Proto Module側がメス、MAX31855側がオス）

必要な工具: ドライバー（熱電対端子のねじ止め用）のみ

---

## 開発環境

Antigravity（VSCode + Gemini）を使用し、PlatformIO IDE で書き込みを行います。

開発環境のセットアップ手順や `platformio.ini` の内容は `detailed_spec_sw.md` を参照してください。

---

## 開発フロー

```
1. 部品購入
      ↓
2. ハードウェア組み立て      → detailed_spec_hw.md
      ↓
3. 開発環境セットアップ      → detailed_spec_sw.md
      ↓
4. コード書き込み            → detailed_spec_sw.md
      ↓
5. 動作確認                  → detailed_spec_sw.md → detailed_spec_hw.md（精度確認）
      ↓
6. 現場運用                  → detailed_spec_hw.md
```

---

## Phase 2 予定（ソフトウェア更新のみ）

- 標準偏差・最大値・最小値・レンジの計算と表示
- SDカード保存
- Bluetooth データ転送（オプション）

追加購入: microSDカード（4GB以上 Class10）約500円のみ  
※ Proto Moduleスタック状態でのSD動作確認が事前に必要
