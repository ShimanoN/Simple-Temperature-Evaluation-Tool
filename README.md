# Simple Temperature Evaluation Tool (Phase 1)
M5Stack Basic V2.7 + K型熱電対を使用した、現場用簡易温度評価ツール。

## 概要
チャンバーや金型等の温度を計測し、特定期間の「平均値」をその場で算出・表示します。
生産技術の現場での温度評価作業を効率化するために設計されました。

## システム構成
- **本体**: M5Stack Basic V2.7
- **センサ**: K型熱電対 + MAX31855 K型熱電対アンプ
- **通信**: SPI (GPIO 18, 23, 5)

## 使い方
1. **IDLE**: 待機状態。現在温度が表示されます。
2. **RUN**: [BtnA]で計測開始。積算とサンプルカウントを行います。
3. **RESULT**: [BtnA]で計測終了。期間内の平均温度を固定表示します。
4. **RESET**: [BtnA]でIDLEに戻ります。

## セットアップ
詳細は `docs/` フォルダ内の各仕様書を参照してください。
- [組み立て・配線 (Hardware)](docs/detailed_spec_hw.md)
- [開発環境・書き込み (Software)](docs/detailed_spec_sw.md)

## 開発環境
- VSCode + PlatformIO IDE
- Framework: Arduino (ESP32)

---
Developed by Shimano / Powered by Antigravity.
