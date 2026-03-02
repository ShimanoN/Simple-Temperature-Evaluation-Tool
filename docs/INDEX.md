#  ドキュメント完全索引

**最終更新**: 2026年2月（v1.0.0 Release）  
**ステータス**:  Production Ready  開発終了最終版

---

##  ファイル一覧（最終構成）

###  ルートドキュメント

| # | ファイル | 説明 | 優先度 |
|---|---------|------|--------|
| 1 | [README.md](README.md) | ドキュメント総合ナビゲーション |  |
| 2 | [INDEX.md](INDEX.md) | このファイル（全索引） |  |
| 3 | [INTEGRATION_TEST_RESULTS.md](INTEGRATION_TEST_RESULTS.md) | 統合テスト結果 33/33 PASS |  |
| 4 | [KNOWN_ISSUES_AND_FUTURE_PLANS.md](KNOWN_ISSUES_AND_FUTURE_PLANS.md) | 既知制限将来計画 (Phase 5-9) |  |
| 5 | [CHANGELOG.md](CHANGELOG.md) | 技術的変更履歴 (2026-02-26〜27) |  |
| 6 | [UI_LAYOUT_DESIGN.md](UI_LAYOUT_DESIGN.md) | LCD 画面レイアウト設計書 |  |

###  仕様書 (specs/)

| # | ファイル | 説明 | 優先度 |
|---|---------|------|--------|
| 7 | [specs/basic_spec.md](specs/basic_spec.md) | 全体概要部品一覧操作方法 |  |
| 8 | [specs/detailed_spec_hw.md](specs/detailed_spec_hw.md) | ハードウェア詳細（配線組み立て精度） |  |
| 9 | [specs/detailed_spec_sw.md](specs/detailed_spec_sw.md) | ソフトウェア詳細（環境構築API運用） |  |

###  コード解説 (code/)

| # | ファイル | 説明 | 優先度 |
|---|---------|------|--------|
| 10 | [code/CODE_EXPLANATION.md](code/CODE_EXPLANATION.md) | 設計思想アーキテクチャ解説（なぜそう作ったか） |  |
| 11 | [code/IMPLEMENTATION_GUIDE.md](code/IMPLEMENTATION_GUIDE.md) | API仕様GlobalDataタスク実装詳細 |  |

###  ガイド (guides/)

| # | ファイル | 説明 | 優先度 |
|---|---------|------|--------|
| 12 | [guides/FINAL_HANDOVER_GUIDE.md](guides/FINAL_HANDOVER_GUIDE.md) | 引き継ぎガイド（初心者向け8部構成） |  |

###  トラブルシューティング (troubleshooting/)

| # | ファイル | 説明 | 優先度 |
|---|---------|------|--------|
| 13 | [troubleshooting/TROUBLESHOOTING.md](troubleshooting/TROUBLESHOOTING.md) | 症状別デバッグ手順（7症状722行） |  |
| 14 | [troubleshooting/HARDWARE_VALIDATION.md](troubleshooting/HARDWARE_VALIDATION.md) | ハードウェア動作確認手順 |  |
| 15 | [troubleshooting/PERFORMANCE.md](troubleshooting/PERFORMANCE.md) | パフォーマンス測定検証方法 |  |

---

##  キーワード別索引

### 「温度センサ / MAX31855」
- [specs/detailed_spec_hw.md](specs/detailed_spec_hw.md)  配線CS ピン設定
- [code/CODE_EXPLANATION.md](code/CODE_EXPLANATION.md)  読み取りフロー
- [troubleshooting/TROUBLESHOOTING.md](troubleshooting/TROUBLESHOOTING.md)  センサエラー症状

### 「EEPROM / アラーム設定」
- [specs/detailed_spec_sw.md](specs/detailed_spec_sw.md)  EEPROM 設定値保存
- [code/IMPLEMENTATION_GUIDE.md](code/IMPLEMENTATION_GUIDE.md)  EEPROMManager API
- [troubleshooting/TROUBLESHOOTING.md](troubleshooting/TROUBLESHOOTING.md)  設定値リセット症状

### 「SD カード / CSV」
- [code/IMPLEMENTATION_GUIDE.md](code/IMPLEMENTATION_GUIDE.md)  SDManager APIフォーマット仕様
- [INTEGRATION_TEST_RESULTS.md](INTEGRATION_TEST_RESULTS.md)  D-01〜D-05 テスト結果
- [troubleshooting/TROUBLESHOOTING.md](troubleshooting/TROUBLESHOOTING.md)  SD 未保存症状

### 「Welford法 / 統計」
- [code/CODE_EXPLANATION.md](code/CODE_EXPLANATION.md)  設計思想
- [code/IMPLEMENTATION_GUIDE.md](code/IMPLEMENTATION_GUIDE.md)  実装詳細
- [guides/FINAL_HANDOVER_GUIDE.md](guides/FINAL_HANDOVER_GUIDE.md)  わかりやすい解説

### 「セットアップ / 環境構築」
- [specs/detailed_spec_sw.md](specs/detailed_spec_sw.md)  PlatformIO セットアップ手順
- [guides/FINAL_HANDOVER_GUIDE.md](guides/FINAL_HANDOVER_GUIDE.md)  Step-by-step 初期設定

### 「タスク / アーキテクチャ」
- [code/IMPLEMENTATION_GUIDE.md](code/IMPLEMENTATION_GUIDE.md)  IO_Task / Logic_Task / UI_Task
- [code/CODE_EXPLANATION.md](code/CODE_EXPLANATION.md)  3タスク設計の理由

### 「アラーム / ヒステリシス」
- [specs/basic_spec.md](specs/basic_spec.md)  アラーム機能概要
- [code/IMPLEMENTATION_GUIDE.md](code/IMPLEMENTATION_GUIDE.md)  アラームロジック
- [guides/FINAL_HANDOVER_GUIDE.md](guides/FINAL_HANDOVER_GUIDE.md)  フロー図

### 「拡張 / 将来計画」
- [KNOWN_ISSUES_AND_FUTURE_PLANS.md](KNOWN_ISSUES_AND_FUTURE_PLANS.md)  Phase 5-9 ロードマップ

---

##  ドキュメント統計（v1.0.0 最終）

| カテゴリー | ファイル数 |
|----------|-----------|
| ルートドキュメント | 6個 |
| 仕様書 (specs/) | 3個 |
| コード解説 (code/) | 2個 |
| ガイド (guides/) | 1個 |
| トラブルシューティング (troubleshooting/) | 3個 |
| **合計** | **15個** |

---

##  用途別スタートガイド

### 引き継ぎ担当者（初めて触る人）
1. [guides/FINAL_HANDOVER_GUIDE.md](guides/FINAL_HANDOVER_GUIDE.md)  ここから（全8パート）

### ハードウェアのトラブル
1. [troubleshooting/HARDWARE_VALIDATION.md](troubleshooting/HARDWARE_VALIDATION.md)
2. [troubleshooting/TROUBLESHOOTING.md](troubleshooting/TROUBLESHOOTING.md)

### ソフトウェアのバグ修正
1. [troubleshooting/TROUBLESHOOTING.md](troubleshooting/TROUBLESHOOTING.md)
2. [code/CODE_EXPLANATION.md](code/CODE_EXPLANATION.md)
3. [code/IMPLEMENTATION_GUIDE.md](code/IMPLEMENTATION_GUIDE.md)

### 将来の機能追加を検討
1. [KNOWN_ISSUES_AND_FUTURE_PLANS.md](KNOWN_ISSUES_AND_FUTURE_PLANS.md)
2. [code/IMPLEMENTATION_GUIDE.md](code/IMPLEMENTATION_GUIDE.md)  拡張ポイント

---

**最終更新**: v1.0.0  開発終了全ドキュメント最終版
