# 段階2-3 ハンドオーバー チェックリスト

**作成日**: 2026年2月26日  
**ステータス**: 引き継ぎ準備完了  
**次ステップ**: 別チャットで段階2-3を実施

---

## 📝 確認項目（次チャット開始時に確認してください）

### 引き継ぎドキュメント
- [x] HANDOVER_PHASE2-3.md ............. メインハンドオーバードキュメント作成完了
- [x] CODE_REFERENCE.md ............... コード構造・インターフェース リファレンス作成完了
- [x] REFACTORING_PLAN.md ............. 既存（Task 1-10全体計画）
- [x] REFACTORING_PROGRESS.md ......... 既存（Task 1-4完了報告）

### 現在のコードベース状態
- [x] Task 1-4 すべて完了
- [x] ビルド成功: 61.68秒
- [x] アップロード成功: 44.14秒  
- [x] ハードウェア動作確認済み

### 段階2-3実施環境の準備
- [ ] VS Code 1.96以上がインストール済みか
- [ ] PlatformIO CLI が `C:\.platformio\penv\Scripts\platformio.exe` に存在するか
- [ ] M5Stack Basic V2.7 がUSB接続可能か
- [ ] シリアルターミナル（115200 baud）が利用可能か

---

## 🎯 次チャット開始時の実施フロー

### フェーズ1: セットアップ確認（5分）

```
次チャット開始直後の確認:

1. ハンドオーバードキュメントの参照確認
   Q: "3つのハンドオーバードキュメント（HANDOVER_PHASE2-3.md, 
       CODE_REFERENCE.md, 既存のREFACTORING_*.md）を活用して
       段階2-3を進める準備はできていますか？"

2. PlatformIO環境確認
   Q: "platformio test -e native コマンドでユニットテストを実行できる環境は
       整備されていますか？（gcc, arm-none-eabi-gcc など）"

3. ハードウェア確認
   Q: "M5Stack Basic V2.7 は USB接続で認識されていますか？
       platformio device list で確認できますか？"
```

### フェーズ2: 段階2実施（Task 5-7）【推定 2-3時間】

**順序**: Task 5 → Task 6 → Task 7

#### Task 5: CODE_EXPLANATION.md 大幅更新

**作業内容**:
1. `docs/CODE_EXPLANATION.md` を開く
2. 以下を新規追加:
   - アーキテクチャ図（3層タスク）
   - 状態遷移図（4つの状態）
   - アラーム判定フロー（ヒステリシス）
   - Welford法の数式説明
   - EEPROM設計図

**参考資料**: HANDOVER_PHASE2-3.md 「Task 5 詳細」セクション

**検証**: 初心者が15分で理解できるレベルの記述

---

#### Task 6: TROUBLESHOOTING.md 新規作成

**作業内容**:
1. `docs/TROUBLESHOOTING.md` を新規作成
2. 以下の5症状について記載:
   - アラーム不発生（今回経験した、設定が読み込まれていない場合）
   - センサ読み込み失敗（MAX31855 接続エラー）
   - EEPROM書き込み失敗（永続化不可）
   - 設定値が保持されない（電源OFF→ON で初期化）
   - ハードウェア接続エラー（シリアル通信不可）

**形式**: 
```
## 症状: [タイトル]

### 原因候補
- 原因1: 説明
- 原因2: 説明

### 確認方法
- ステップ1
- ステップ2
- ステップ3

### 対処方法
- 対処1
- 対処2
```

**参考資料**: HANDOVER_PHASE2-3.md 「Task 6 詳細」セクション

---

#### Task 7: コード内コメント充実

**作業内容**:
1. Tasks.cpp 内の以下関数にJSDoc追加:
   - updateAlarmFlags() - ヒステリシス説明
   - Welford法関数 - 数式説明

2. EEPROMManager.cpp 内にコメント追加:
   - readSettings() - 検証フロー説明
   - writeSettings() - Write-Verify説明

**参考資料**: HANDOVER_PHASE2-3.md 「Task 7 詳細」セクション

**検証方法**: コード読者が、各関数の目的・アルゴリズムを5分以内に理解可能

---

### フェーズ3: 段階3実施（Task 8-10）【推定 3-4時間】

**順序**: Task 8 → Task 6（追記）→ Task 9 → Task 10

#### Task 8: ユニットテスト充実

**作業内容**:
1. `test/test_measurement_core.cpp` を開く
2. updateAlarmFlags() のテストケース追加:

```cpp
テストケース（最低5個）:
  1. test_alarm_hi_trigger()        // 35℃でトリガー（30℃閾値）
  2. test_alarm_hi_clear()          // ヒステリシスでクリア
  3. test_alarm_lo_trigger()        // 15℃でトリガー（20℃閾値）
  4. test_both_alarms_active()      // 同時アラーム
  5. test_nan_input_no_change()     // NaN無視
```

3. ビルド：`platformio run -e m5stack`
4. テスト実行：`platformio test -e native`

**期待結果**: すべてのテストケースが PASS

**参考資料**: HANDOVER_PHASE2-3.md 「Task 8 詳細」セクション, CODE_REFERENCE.md

---

#### Task 9: 動作検証チェックリスト

**作業内容**:
1. M5Stack に最新ファームウェアをアップロード
2. チェックリスト（10項目以上）を実機で確認:
   - 起動ログ表示
   - IDLE画面表示
   - アラーム判定動作
   - 設定モード動作
   - EEPROM永続性

**期待結果**: すべてのチェック項目が OK

**参考資料**: HANDOVER_PHASE2-3.md 「Task 9 詳細」セクション

---

#### Task 10: パフォーマンス測定

**作業内容**:
1. main.cpp に測定ロジック追加:
   - IO_Task, Logic_Task, UI_Task の実行時間計測
   - 5秒ごとに Serial 出力

2. 30分間の連続実行でデータ収集

3. 結果を REFACTORING_PROGRESS.md に記録:
   ```
   ## パフォーマンス測定結果（Task 10）
   
   IO_Task実行時間: 平均X.XXms (目標: <5ms)
   Logic_Task実行時間: 平均Y.YYms (目標: <3ms)
   UI_Task実行時間: 平均Z.ZZms (目標: <50ms)
   ```

**期待結果**: すべてのタスクが目標値以下

**参考資料**: HANDOVER_PHASE2-3.md 「Task 10 詳細」セクション

---

## 📊 推定進捗タイムテーブル

| フェーズ | 実施内容 | 推定時間 | 累積 |
|:---:|:---|:---:|:---:|
| **準備** | 環境確認・ドキュメント読み込み | 15分 | 15分 |
| **段階2** | Task 5-7（ドキュメント充実） | 2-3h | 2:15-3:15h |
| **段階3① ** | Task 8-9（テスト・検証） | 2-3h | 4:15-6:15h |
| **段階3②** | Task 10（パフォーマンス） | 1h | 5:15-7:15h |
| **総合時間** | 全タスク完了 | **5-7h** | - |

**推奨実施方法**:
- 高品質優先で、複数回セッションに分割
- ドキュメント作成後にテスト実施（テスト駆動開発の観点から）
- パフォーマンス測定は優先度低め（最後に実施）

---

## ⚠️ 重要な注意事項

### 段階2実施時
1. **ドキュメント品質** - 初心者向け、わかりやすさを優先
2. **図の追加** - Mermaid で状態遷移図・アーキテクチャ図を作成
3. **コメント記述** - コード内のコメントは日本語OK、説明は詳細に

### 段階3実施時
1. **テスト環境** - `platformio test -e native` は gcc依存
   - 環境構築問題が発生したら、一時的にスキップ可能（代わりに動作検証に注力）

2. **ハードウェア動作** - M5Stack での実機テスト必須
   - シリアルモニタで [Setup] ログが出力されることを確認

3. **パフォーマンス測定** - 連続30分実行が必要
   - 不安定な場合は、段階2実施後に再実施推奨

---

## 🔗 文書関連図（このドキュメント体系）

```
HANDOVER_PHASE2-3.md (このファイル)
  ├─ 概要・方針
  ├─ 実施計画（Task 5-10 詳細指示）
  └─ チェックリスト

CODE_REFERENCE.md
  ├─ Global.h 定義
  ├─ EEPROMManager クラス詳細
  ├─ Tasks.cpp 関数一覧
  ├─ 状態機械定義
  └─ ビルド・デプロイコマンド

REFACTORING_PLAN.md（既存）
  └─ 全体10タスク計画

REFACTORING_PROGRESS.md（既存）
  └─ Task 1-4 完了報告
      → Task 5-10 結果を追記予定
```

---

## ✅ 最終確認（引き継ぎ前）

### 次チャット開始前に確認すること

```
□ このドキュメント体系を読み込めたか
□ HANDOVER_PHASE2-3.md の内容を理解したか
□ CODE_REFERENCE.md を参照用ブックマークに登録したか
□ ビルド環境が正常に機能するか（最後ビルド成功: 61.68s）
□ M5Stack とシリアル通信が可能か
```

### 次チャット開始時の質問

次チャットを始めるときに、以下のように伝えてください：

```
「段階2-3を実施します。以下の引き継ぎドキュメントを参照します：
 - HANDOVER_PHASE2-3.md（実施計画）
 - CODE_REFERENCE.md（コード参照）
 
準備状況:
✅ ビルド環境OK（最後成功: 61.68s）
✅ M5Stack ハードウェアOK（USB接続可）
✅ ドキュメント読み込み完了

実施予定:
- Phase 2: Task 5-7（ドキュメント充実）→ 2-3h
- Phase 3: Task 8-10（テスト・検証）→ 3-4h

推奨実施順序: Task 5 → 6 → 7 → 8 → 9 → 10

よろしくお願いします。」
```

---

## 🎓 学習・参考資料（オプション）

### Welford法（Phase 2で実装済み）
- 参考: https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
- 利点: 数値的安定性、オンライン計算、低メモリ占有

### ESP32 EEPROM
- 参考: https://github.com/espressif/arduino-esp32/blob/master/libraries/EEPROM
- 特性: 4KB, 耐久性 100,000サイクル

### M5Stack LCD制御 (ILI9342C)
- 解像度: 320×240ピクセル
- 色深度: 16-bit (RGB565)
- 参考: M5Stack API documentation

---

## 📞 トラブル時の対応フロー

### ビルドエラーが発生した場合
```
1. 最後に修正したファイルを確認（Git diff）
2. CODE_REFERENCE.md で関数シグネチャを確認
3. コンパイルエラーメッセージで修正箇所を特定
4. 修正後 `platformio run -e m5stack` で再ビルド
```

### テストが FAIL した場合
```
1. `platformio test -e native` の出力を確認
2. テストケースの想定値と実装を照合
3. updateAlarmFlags() の ロジックをトレース
4. 必要に応じて Serial.printf() でデバッグ
```

### ハードウェア接続エラー
```
1. `platformio device list` で M5Stack を確認
2. USB ドライバを再インストール
3. ウィンドウズのデバイスマネージャで COM ポート確認
4. platformio.ini の [env:m5stack] upload_port を確認
```

---

**ハンドオーバー完了日**: 2026年2月26日  
**次段階**: GitHub Copilot による段階2-3実施  
**予想実施期間**: 5-7時間（高品質優先ペース）

