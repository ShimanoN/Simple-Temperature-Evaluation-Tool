---
title: Simple Temperature Evaluation Tool
subtitle: M5Stack Basic V2.7 × MAX31855 熱電対温度計測システム
tagline: "リアルタイム計測 ／ 統計解析 ／ SD カード記録 ／ アラーム機能"
meta: "ESP32  /  C++ (Arduino)  /  PlatformIO"
date: "v1.0.0  ｜  2026 年 3 月"
# --- Project-specific variables ---
# These are accessible in any slide via {{key}} notation.
# If hardware model changes, update here only — all slides update automatically.
mcu: M5Stack Basic V2.7
sensor: MAX31855
total_cost: "¥9,570"
output: simple_temperature_tool.pptx
---

## slide_01 | layout: title

## slide_02 | layout: toc
items:
  - "1.  プロジェクト概要"
  - "2.  ハードウェア構成"
  - "3.  まとめ"

## slide_03 | layout: section_divider
section_num: "Section 1"
section_title: プロジェクト概要
description: 開発背景・目的・解決した課題

## slide_04 | layout: bullet
title: 開発の背景と目的
section: "Section 1  プロジェクト概要"
bullets:
  - "現場で手軽に使える熱電対温度ロガーが高価（市販品 ¥30,000〜）"
  - "{{mcu}} + K 型熱電対による自作でコストを大幅削減（{{total_cost}}）"
  - "リアルタイム表示・SD 記録・アラームを単一デバイスで実現"
  - "PlatformIO によるモジュール設計でコードの保守性を確保"
highlight: "{{total_cost}} の自作ツールで ¥30,000 超の市販品と同等機能を実現"
sub_text: "スタンドアロン動作・EEPROM 設定保存・統計解析（平均・σ・Max/Min）まで完全実装"

## slide_05 | layout: dual_table
title: "コストメリット — 自作 vs. 市販品"
section: "Section 1  プロジェクト概要"
left_label: "{{title}} 品目別コスト"
left_rows:
  - [部品, 購入先目安, 価格（税込）]
  - [M5Stack Basic V2.7, Amazon, "¥7,990"]
  - [MAX31855 ブレークアウト, 秋月電子, "¥980"]
  - ["K型熱電対 (1m)", Amazon 等, "¥700"]
  - ["MicroSD (16GB)", 量販店, "¥600"]
  - [ジャンパワイヤ, 秋月電子, "¥200"]
  - [合計, —, "{{total_cost}}"]
left_col_widths: [2.9, 1.5, 1.4]
left_highlight_last: true
right_label: "市販温度ロガーとの価格比較"
right_rows:
  - [カテゴリ, 代表製品, 概算価格]
  - [本ツール（自作）, "{{mcu}} + {{sensor}}", "{{total_cost}}"]
  - [標準製品, USB データロガー, "¥30,000〜¥50,000"]
  - [プロフェッショナル, 産業用データロガー, "¥100,000〜¥250,000+"]
right_col_widths: [2.8, 2.18, 1.5]
right_highlight_row: 1
highlight: "標準製品の 1/3.1 の価格で同等機能を実現"
sub_highlight: "本ツール {{total_cost}} vs 標準ロガー ¥30,000〜 → 最大 ¥20,000 以上削減"
checklist:
  - "✓ SD 自動記録  ✓ LCD リアルタイム表示  ✓ 上下限アラーム（ビープ音）"
  - "✓ スタンドアロン動作  ✓ EEPROM 設定保存  ✓ 統計解析（平均・σ・Max/Min）"

## slide_06 | layout: section_divider
section_num: "Section 2"
section_title: ハードウェア構成
description: "{{mcu}} と {{sensor}} の SPI 接続"

## slide_07 | layout: two_column
title: ハードウェア構成概要
section: "Section 2  ハードウェア"
left_title: 主要コンポーネント
left_items:
  - "{{mcu}}（ESP32 内蔵、LCD、ボタン）"
  - "{{sensor}} K 型熱電対デジタルコンバーター"
  - "K 型熱電対（1m、-200〜1350°C 対応）"
  - "MicroSD カード（CSV ログ記録用）"
right_title: インターフェース仕様
right_items:
  - "通信: SPI（LCD と {{sensor}} でバス共有）"
  - "電源: USB-C 5V（M5Stack 内蔵バッテリー対応）"
  - "表示: 2.0\" IPS LCD 320×240 px（TFT_eSPI）"
  - "設定保存: ESP32 内蔵フラッシュ仮想 EEPROM"
note: "EEPROM: 外部配線不要。Preferences ライブラリで上下限しきい値・単位を永続保存。"

## slide_08 | layout: table
title: "SPI バス共有（LCD + {{sensor}}）"
section: "Section 2  ハードウェア"
columns: [GPIO, 信号名, 接続先, 備考]
col_widths: [2.0, 2.5, 4.0, 4.1]
rows:
  - ["GPIO 18", "SCK (SPI Clock)",      "{{sensor}} CLK", "LCD と共有"]
  - ["GPIO 19", "MISO (SPI Data in)",   "{{sensor}} DO",  "LCD と共有"]
  - ["GPIO 23", "MOSI (SPI Data out)",  "(LCD のみ)",   "{{sensor}} は MOSI 不要"]
  - ["GPIO 5",  "CS (Chip Select)",     "{{sensor}} CS",  "MAX31855_CS 定数"]
  - ["GPIO 4",  "CS (SD Card)",         "MicroSD CS",   "TFCARD_CS_PIN 定数"]
warning: "SD カードの CS は GPIO4 (TFCARD_CS_PIN) を明示指定すること"
warning_code: "SD.begin(TFCARD_CS_PIN, SPI, 40000000);  // GPIO5 デフォルトでは {{sensor}} と衝突！"
note: "EEPROM: ESP32 内蔵フラッシュを仮想 EEPROM として使用（外部配線なし）"

## slide_09 | layout: bullet
title: まとめ
section: まとめ
bullets:
  - "{{total_cost}} で構築した熱電対温度計測システム（市販品比 1/3.1 の価格）"
  - "SD 自動記録・LCD リアルタイム表示・上下限アラームを完全実装"
  - "PlatformIO × C++ によるモジュール設計で保守性・拡張性を確保"
  - "EEPROM 永続設定・統計解析（σ・Max/Min）まで実装済み"
highlight: "組込み C++ / ESP32 / センサーインターフェース設計の実践経験を実証"
