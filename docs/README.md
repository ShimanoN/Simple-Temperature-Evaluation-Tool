#  Simple Temperature Evaluation Tool - ドキュメント総合ガイド

**プロジェクト**: M5Stack 温度評価ツール  
**バージョン**: v1.0.0  リリース完了（開発終了）  
**最終更新**: 2026年2月  
**ビルド状態**:  SUCCESS  Flash 31.0% (406KB) / RAM 7.2% (23KB)

**CHANGELOG**: See [CHANGELOG.md](CHANGELOG.md) for technical change history.  
**Release Notes**: See the [project root README](../README.md) for v1.0.0 release overview.

---

##  ドキュメントを探す

### 「何をしたいか」で選ぶ

| 目的 | 最初に読むファイル |
|------|------------------|
|  **プロジェクトを引き継いで使いたい** | [guides/FINAL_HANDOVER_GUIDE.md](guides/FINAL_HANDOVER_GUIDE.md)  |
|  **ハードウェアを組み立てたい** | [specs/detailed_spec_hw.md](specs/detailed_spec_hw.md) |
|  **開発環境をセットアップしたい** | [specs/detailed_spec_sw.md](specs/detailed_spec_sw.md) |
|  **コードの設計思想を理解したい** | [code/CODE_EXPLANATION.md](code/CODE_EXPLANATION.md) |
|  **APIや実装詳細を調べたい** | [code/IMPLEMENTATION_GUIDE.md](code/IMPLEMENTATION_GUIDE.md) |
|  **エラーを解決したい** | [troubleshooting/TROUBLESHOOTING.md](troubleshooting/TROUBLESHOOTING.md) |
|  **テスト結果を確認したい** | [INTEGRATION_TEST_RESULTS.md](INTEGRATION_TEST_RESULTS.md) |
|  **将来の拡張候補を知りたい** | [KNOWN_ISSUES_AND_FUTURE_PLANS.md](KNOWN_ISSUES_AND_FUTURE_PLANS.md) |

---

##  ドキュメント一覧

###  入口ドキュメント

| ファイル | 説明 | 読む時間 |
|---------|------|--------|
| [INDEX.md](INDEX.md) | 全ドキュメント一覧 | 2分 |
| [KNOWN_ISSUES_AND_FUTURE_PLANS.md](KNOWN_ISSUES_AND_FUTURE_PLANS.md) | v1.0.0 既知制限と将来計画 | 15分 |
| [INTEGRATION_TEST_RESULTS.md](INTEGRATION_TEST_RESULTS.md) | 33/33 PASS 統合テスト結果 | 10分 |
| [CHANGELOG.md](CHANGELOG.md) | 技術的変更履歴 | 5分 |

###  仕様書

| ファイル | 説明 | 読む時間 |
|---------|------|--------|
| [specs/basic_spec.md](specs/basic_spec.md) |  プロジェクト全体概要部品一覧 | 10分 |
| [specs/detailed_spec_hw.md](specs/detailed_spec_hw.md) | ハードウェア詳細 (配線組み立て) | 20分 |
| [specs/detailed_spec_sw.md](specs/detailed_spec_sw.md) | ソフトウェア詳細 (環境構築動作確認) | 25分 |
| [UI_LAYOUT_DESIGN.md](UI_LAYOUT_DESIGN.md) | LCD 画面レイアウト設計 | 10分 |

###  コード解説

| ファイル | 説明 | 読む時間 |
|---------|------|--------|
| [code/CODE_EXPLANATION.md](code/CODE_EXPLANATION.md) |  設計思想「なぜそう作ったか」 | 45分 |
| [code/IMPLEMENTATION_GUIDE.md](code/IMPLEMENTATION_GUIDE.md) | API仕様グローバル構造体タスク実装 | 30分 |

###  ガイド

| ファイル | 説明 | 読む時間 |
|---------|------|--------|
| [guides/FINAL_HANDOVER_GUIDE.md](guides/FINAL_HANDOVER_GUIDE.md) |  引き継ぎガイド (初心者向け推奨) | 60分 |

###  トラブルシューティング

| ファイル | 説明 | 読む時間 |
|---------|------|--------|
| [troubleshooting/TROUBLESHOOTING.md](troubleshooting/TROUBLESHOOTING.md) |  症状別デバッグ手順 (7症状) | 10分 |
| [troubleshooting/HARDWARE_VALIDATION.md](troubleshooting/HARDWARE_VALIDATION.md) | ハードウェア動作確認手順 | 15分 |
| [troubleshooting/PERFORMANCE.md](troubleshooting/PERFORMANCE.md) | パフォーマンス測定検証方法 | 15分 |

---

##  引き継ぎ担当者向けスタートガイド

1. **まずここから**  プロジェクト root の [README.md](../README.md) を読む（5分）
2. **環境構築**  [specs/detailed_spec_sw.md](specs/detailed_spec_sw.md)（25分）
3. **ハードウェア確認**  [specs/detailed_spec_hw.md](specs/detailed_spec_hw.md)（20分）
4. **コード理解**  [code/CODE_EXPLANATION.md](code/CODE_EXPLANATION.md)（45分）
5. **詳細仕様**  [code/IMPLEMENTATION_GUIDE.md](code/IMPLEMENTATION_GUIDE.md)（30分）
6. **問題が起きたら**  [troubleshooting/TROUBLESHOOTING.md](troubleshooting/TROUBLESHOOTING.md)

総所要時間: 約 2〜3 時間

---

##  v1.0.0 実装済み機能

| 機能 | 状態 |
|------|------|
| MAX31855 K型熱電対 読み取り |  |
| Welford法 リアルタイム統計（平均標準偏差最大最小） |  |
| ヒステリシス付き HI/LO アラーム |  |
| MicroSD CSV ロギング（バッファリング） |  |
| EEPROM アラーム設定永続化 |  |
| M5Stack LCDリアルタイム表示 |  |
| 3タスク設計 (IO 10ms / Logic 50ms / UI 200ms) |  |
| 統合テスト 33/33 PASS |  |

---

##  問題があった場合

1. **エラーが出ている**  [troubleshooting/TROUBLESHOOTING.md](troubleshooting/TROUBLESHOOTING.md)
2. **ハードウェアが怪しい**  [troubleshooting/HARDWARE_VALIDATION.md](troubleshooting/HARDWARE_VALIDATION.md)
3. **コードの動きがわからない**  [code/CODE_EXPLANATION.md](code/CODE_EXPLANATION.md)
4. **APIを調べたい**  [code/IMPLEMENTATION_GUIDE.md](code/IMPLEMENTATION_GUIDE.md)
5. **仕様を確認したい**  [specs/](specs/)

---

##  更新履歴

| 日付 | 内容 |
|------|------|
| 2026-02-27 | v1.0.0 リリース / 統合テスト 33/33 PASS |
| 2026-02-27 | docs/ 最終整理（旧開発過程ドキュメント削除） |
| 2026-02-27 | FINAL_HANDOVER_GUIDE.md / IMPLEMENTATION_GUIDE.md 新規作成 |
| 2026-02-27 | KNOWN_ISSUES_AND_FUTURE_PLANS.md 新規作成 |
| 2026-02-27 | Welford計算SD書き込みタイミング修正 |
| 2026-02-26 | リファクタリング完了 (EEPROM独立化UI分割定数化) |
