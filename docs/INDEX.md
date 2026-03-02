# 📑 ドキュメント完全索引

**最終更新**: 2026年3月2日（v1.0.0 Release）  
**用途**: ドキュメント検索・カテゴリー別閲覧用  
**ステータス**: ✅ Production Ready - 全ドキュメント整備完了  

---

## 🔍 キーワード検索

### 「温度」に関連するドキュメント
- [specs/basic_spec.md](specs/basic_spec.md) - 温度測定の仕様
- [specs/detailed_spec_hw.md](specs/detailed_spec_hw.md#温度-max31855-の精度・応答)
- [code/CODE_EXPLANATION.md](code/CODE_EXPLANATION.md) - 温度フィルタリング処理
- [troubleshooting/PERFORMANCE.md](troubleshooting/PERFORMANCE.md) - 温度測定精度測定

### 「センサ」に関連するドキュメント
- [specs/detailed_spec_hw.md](specs/detailed_spec_hw.md#max31855-温度センサ仕様)
- [troubleshooting/HARDWARE_VALIDATION.md](troubleshooting/HARDWARE_VALIDATION.md)
- [specs/basic_spec.md](specs/basic_spec.md#参考資料-max31855-仕様)

### 「EEPROM」に関連するドキュメント
- [specs/detailed_spec_sw.md](specs/detailed_spec_sw.md#eeprom-設定値保存)
- [code/CODE_EXPLANATION.md](code/CODE_EXPLANATION.md#eeprom-操作フロー)
- [code/CODE_REFERENCE.md](code/CODE_REFERENCE.md) - EEPROMManager API

### 「アラーム」に関連するドキュメント
- [specs/basic_spec.md](specs/basic_spec.md#アラーム機能)
- [code/CODE_EXPLANATION.md](code/CODE_EXPLANATION.md#アラーム判定-hysteresis)
- [troubleshooting/TROUBLESHOOTING.md](troubleshooting/TROUBLESHOOTING.md#アラームが-off-にならない)

### 「UI / ボタン」に関連するドキュメント
- [specs/basic_spec.md](specs/basic_spec.md#操作方法)
- [specs/detailed_spec_sw.md](specs/detailed_spec_sw.md#操作フロー)
- [code/CODE_EXPLANATION.md](code/CODE_EXPLANATION.md#状態遷移-state-machine)
- [guides/STAGE_4_REFACTORING_HANDOVER.md](guides/STAGE_4_REFACTORING_HANDOVER.md) - UI分割詳細

### 「統計」に関連するドキュメント
- [code/CODE_EXPLANATION.md](code/CODE_EXPLANATION.md#welford法による逐次統計計算)
- [specs/detailed_spec_sw.md](specs/detailed_spec_sw.md#統計値計算-平均-標準偏差)
- [troubleshooting/PERFORMANCE.md](troubleshooting/PERFORMANCE.md) - 処理時間測定

### 「セットアップ / 開発環境」に関連するドキュメント
- [specs/detailed_spec_sw.md](specs/detailed_spec_sw.md#開発環境セットアップ)
- [guides/QUICKSTART_SESSION4.md](guides/QUICKSTART_SESSION4.md)
- [troubleshooting/TROUBLESHOOTING.md](troubleshooting/TROUBLESHOOTING.md#開発環境-セットアップエラー)

### 「エラー / デバッグ」に関連するドキュメント
- [troubleshooting/TROUBLESHOOTING.md](troubleshooting/TROUBLESHOOTING.md) ⭐ **最優先**
- [troubleshooting/HARDWARE_VALIDATION.md](troubleshooting/HARDWARE_VALIDATION.md)
- [troubleshooting/PERFORMANCE.md](troubleshooting/PERFORMANCE.md)

### 「リファクタリング / コード改造」に関連するドキュメント
- [guides/STAGE_4_REFACTORING_HANDOVER.md](guides/STAGE_4_REFACTORING_HANDOVER.md)
- [sessions/SESSION4_COMPLETION_REPORT.md](sessions/SESSION4_COMPLETION_REPORT.md)
- [code/CODE_EXPLANATION.md](code/CODE_EXPLANATION.md#stage-2-b-code-quality-refactoring)

### 「拡張 / 次フェーズ」に関連するドキュメント
- [future/future_plan.md](future/future_plan.md)
- [future/PHASE3_EXTENSION_PLAN.md](future/PHASE3_EXTENSION_PLAN.md)

---

## 📂 ファイル別詳細情報

### **仕様書**

#### 1. `specs/basic_spec.md` ⭐ **最初に読むべき**
- **概要**: プロジェクト基本仕様
- **内容**: プロジェクト目的、機能一覧、操作方法、用語集
- **読む時間**: 10分
- **対象**: 全員必読

#### 2. `specs/detailed_spec_hw.md`
- **概要**: ハードウェア詳細仕様
- **内容**: 組み立て手順、各部品の選定理由、精度・応答確認
- **読む時間**: 20分
- **対象**: ハードウェア改造予定者、トラブル解決担当

#### 3. `specs/detailed_spec_sw.md`
- **概要**: ソフトウェア詳細仕様
- **内容**: セットアップ、アルゴリズム説明、API仕様、運用方法
- **読む時間**: 25分
- **対象**: ソフトウェア開発者、保守担当者

---

### **コード解説**

#### 4. `code/CODE_EXPLANATION.md` ⭐ **コード理解の必読書**
- **概要**: コード完全解説書
- **内容**: 変数・型の選定理由、制御フロー、アルゴリズム詳細、Stage別進展
- **読む時間**: 45分
- **対象**: コード改造・バグ修正担当者、新規学習者

#### 5. `code/CODE_REFERENCE.md`
- **概要**: API リファレンス
- **内容**: 全関数の宣言、構造体定義、定数一覧
- **読む時間**: 随時参照
- **対象**: 実装中のコーダー

---

### **開発ガイド**

#### 6. `guides/QUICKSTART_SESSION4.md` ⭐ **急ぎの場合はコレ**
- **概要**: 30秒で開発準備、5分で開始
- **内容**: 環境確認、最低限の手順、チェックリスト
- **読む時間**: 5分
- **対象**: 時間がない開発者、急いでいる人

#### 7. `guides/STAGE_4_REFACTORING_HANDOVER.md`
- **概要**: Task 3-5 実装ガイド
- **内容**: UI分割、EEPROM独立化、ドキュメント充実の詳細手順
- **読む時間**: 30分
- **対象**: コードリファクタリング予定者

#### 8. `guides/HANDOVER_CHECKLIST.md`
- **概要**: 開発チェックリスト
- **内容**: ビルド検証、動作確認、Git操作、デプロイ手順
- **読む時間**: 随時確認
- **対象**: リリース前確認担当

---

### **トラブルシューティング / 検証**

#### 9. `troubleshooting/TROUBLESHOOTING.md` ⭐ **エラー時の必須ガイド**
- **概要**: よくあるエラーと対処法
- **内容**: コンパイルエラー、実行時エラー、表示異常、通信エラー等
- **読む時間**: 10分 (問題に応じて随時)
- **対象**: トラブル発生時全員

#### 10. `troubleshooting/HARDWARE_VALIDATION.md`
- **概要**: ハードウェア動作確認手順
- **内容**: センサ動作確認、配線チェック、信号測定手順
- **読む時間**: 15分
- **対象**: ハードウェア問題診断担当

#### 11. `troubleshooting/PERFORMANCE.md`
- **概要**: パフォーマンス測定・最適化
- **内容**: 処理時間測定、メモリ使用量、精度測定結果
- **読む時間**: 15分
- **対象**: 性能改善担当、ボトルネック調査者

---

### **セッション履歴 / 完了報告**

#### 12. `sessions/SESSION4_COMPLETION_REPORT.md` ⭐ **最新のセッション**
- **概要**: Session 4 完了報告
- **内容**: Task 1-5 全体成果、ビルド結果、技術的価値、次ステップ
- **読む時間**: 15分
- **対象**: 現在の開発状況把握、次フェーズ計画

#### 13. `sessions/STAGE2_3_COMPLETION_SUMMARY.md`
- **概要**: Stage 2-3 完了報告
- **内容**: 計測精度向上・EEPROM・ドキュメント確定の成果
- **読む時間**: 15分
- **対象**: 過去の実装状況参考用

#### 14. `sessions/SESSION3_FINAL_REPORT.md`
- **概要**: Session 3 完了報告（参考資料）
- **内容**: Task 1-2 の詳細、ビルド検証過程
- **読む時間**: 10分
- **対象**: 過去の実装参考用

---

### **拡張計画 / 学習**

#### 15. `future/future_plan.md`
- **概要**: 将来の拡張・改善案
- **内容**: Stage 3-4 計画、新機能アイデア、優先度検討
- **読む時間**: 10分
- **対象**: 長期計画担当者、拡張予定者

#### 16. `future/PHASE3_EXTENSION_PLAN.md`
- **概要**: Web UI・ログ機能の詳細計画
- **内容**: API設計、ログフォーマット、Web UI仕様
- **読む時間**: 15分
- **対象**: Web機能開発予定者

#### 17. `future/LEARNING_LOG_PLC_to_CPP.md`
- **概要**: PLC→C++ 学習ログ
- **内容**: 言語移行の工夫、デバッグテクニック、ベストプラクティス
- **読む時間**: 随時
- **対象**: PLC出身者、学習参考用

---

### **アーカイブ / 参考資料**

#### 18. `_legacy/HANDOVER_PHASE2-3.md` (削除済み)
- **慎重**: Phase 2-3 はすでに完了。参考用のみ
- **用途**: 過去の引き継ぎ記録、方法論の参考

#### 19. ~~`REFACTORING_PLAN.md`~~ / ~~`REFACTORING_PROGRESS.md`~~ (削除済み)
- **注意**: Phase 2-B のリファクタリングはすでに完了
- **用途**: 過去の計画プロセス参考用

---

## 📊 ドキュメント統計

| カテゴリー | ファイル数 | 優先度 |
|----------|-----------|--------|
| 仕様書 | 3個 | ⭐⭐⭐ |
| コード解説 | 2個 | ⭐⭐⭐ |
| 開発ガイド | 3個 | ⭐⭐⭐ |
| トラブルシューティング | 3個 | ⭐⭐⭐ |
| セッション履歴 | 3個 | ⭐⭐ |
| 拡張計画 | 3個 | ⭐⭐ |
| 参考資料 | 3個 | ⭐ |
| **合計** | **20個** | |

---

## 🎯 用途別推奨ドキュメント読み順

### **新規プロジェクト参画者向け**
1. README.md (このナビゲーション)
2. [specs/basic_spec.md](specs/basic_spec.md) (全体像)
3. [specs/detailed_spec_hw.md](specs/detailed_spec_hw.md) + [specs/detailed_spec_sw.md](specs/detailed_spec_sw.md) (詳細)
4. [code/CODE_EXPLANATION.md](code/CODE_EXPLANATION.md) (コード理解)
5. [guides/QUICKSTART_SESSION4.md](guides/QUICKSTART_SESSION4.md) (開発開始)

**所要時間**: 3-4時間

### **バグ修正・小規模改造向け**
1. [troubleshooting/TROUBLESHOOTING.md](troubleshooting/TROUBLESHOOTING.md) (問題特定)
2. [code/CODE_EXPLANATION.md](code/CODE_EXPLANATION.md) の該当箱所 (原因理解)
3. コード修正・テスト
4. [guides/HANDOVER_CHECKLIST.md](guides/HANDOVER_CHECKLIST.md) (確認)

**所要時間**: 1-2時間

### **大規模リファクタリング・拡張向け**
1. [code/CODE_EXPLANATION.md](code/CODE_EXPLANATION.md) (現在設計)
2. [guides/STAGE_4_REFACTORING_HANDOVER.md](guides/STAGE_4_REFACTORING_HANDOVER.md) (最新実装方法)
3. [sessions/SESSION4_COMPLETION_REPORT.md](sessions/SESSION4_COMPLETION_REPORT.md) (現在状況)
4. [future/future_plan.md](future/future_plan.md) / [future/PHASE3_EXTENSION_PLAN.md](future/PHASE3_EXTENSION_PLAN.md) (次計画)

**所要時間**: 4-6時間

---

## 💡 ドキュメント使用上の注意

1. **複数ファイルが関連している場合**
   - 必ず **「参照」セクション** を確認してから次のファイルを読む
   - 情報が重複している場合がある（これは意図的です。学習効果向上のため）

2. **セッション履歴について** ⭐ **重要**
   - 過去セッションの情報は → [_navigation/SESSION_HISTORY.md](_navigation/SESSION_HISTORY.md) で管理
   - セッション報告は時系列で整理されています

3. **廃止ドキュメントについて** ⚠️ **重要**
   - 以下のファイルは **計画実施完了済み** のため廃止されました：
     - ❌ REFACTORING_PLAN.md
     - ❌ REFACTORING_PROGRESS.md
     - ⚠️ HANDOVER_PHASE2-3.md
   - 詳細は → [reports/_LEGACY_REFERENCE_ONLY.md](reports/_LEGACY_REFERENCE_ONLY.md)

4. **更新日時に注意**
   - 日付が古いファイルは、新しいセッション報告書で更新されている可能性あり
   - 最新状況は **SESSION4_COMPLETION_REPORT.md** を確認

5. **実装予定の場合**
   - コード実装前に **CODE_EXPLANATION.md** で設計思想を確認すべし
   - 実装後は **HANDOVER_CHECKLIST.md** で最終確認

6. **質問・提案がある場合**
   - ドキュメント内容に矛盾や不明な点があれば、報告してください
   - 改善提案も大歓迎です

---

**ドキュメントの品質が、プロジェクトの成功を左右します。**

---

**最終更新**: 2026年2月26日  
**整理状況**: ✅ ドキュメント統廃合完了 & セッション履歴体系化完了

