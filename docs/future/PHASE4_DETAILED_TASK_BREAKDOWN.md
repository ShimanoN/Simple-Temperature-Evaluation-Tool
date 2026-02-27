# Phase 4 詳細タスク分解表

**対象**: 全実装タスクの細粒度分解  
**作成日**: 2026年2月26日  

---

## タスク 1：Phase 4 計画書作成・前提条件決定

**親タスク**: フェーズ 1:1  
**推定時間**: 1h  
**完了条件**: RTC 方針確定

### サブタスク

| # | 内容 | 完了基準 |
|:---|:---|:---|
| 1-1 | PHASE4_IMPLEMENTATION_PLAN.md 作成 | ドキュメント完成 |
| 1-2 | PHASE4_ARCHITECTURE_DESIGN.md 作成 | アーキテクチャ設計書完成 |
| 1-3 | RTC 方針決定（DS3231）通知 | 方針を Shimano と確認 |
| 1-4 | Amazon RTC 発注（翌営業日配送） | 注文確認メール |
| 1-5 | future_plan.md に Phase 4 欄追加 | 進捗マーク更新 |

---

## タスク 2：RTC + SD ハードウェア確認

**親タスク**: フェーズ 1:2  
**推定時間**: 0.5h  
**完了条件**: 接続仕様確認・ハードウェア接続図完成

### サブタスク

| # | 内容 | 確認方法 |
|:---|:---|:---|
| 2-1 | M5Stack Basic V2.7 の microSD スロット位置確認 | M5Stack 本体で確認（側面に記載） |
| 2-2 | RTC DS3231 の I²C ピン確認 | データシート p.1 確認 |
| 2-3 | I²C アドレス 0x68 確認 | RTClib ライブラリで確認 |
| 2-4 | GPIO21(SDA), GPIO22(SCL) の利用状況確認 | Global.h で他の利用なし確認 |
| 2-5 | M5Stack 電源から 3.3V 取得可能確認 | M5.Power.h で確認 |
| 2-6 | ハードウェア接続図ドキュメント作成 | PHASE4_HARDWARE_GUIDE.md 作成 |

---

## タスク 3：Global.h 拡張

**親タスク**: フェーズ 2:3  
**推定時間**: 1h  
**完了条件**: コンパイルエラーなし（Warning も 0）

### サブタスク

| # | 内容 | チェック項目 |
|:---|:---|:---|
| 3-1 | SD カード定数追加（Global.h） | `constexpr` キーワード確認 |
| 3-2 | RTC 定数追加（I²C アドレス等） | 値の妥当性確認 |
| 3-3 | SDData 構造体定義 | メンバ変数の型・初期化 |
| 3-4 | GlobalData に M_SDReady 等フィールド追加 | 既存フィールドとの命名規則確認 |
| 3-5 | initGlobalData() に新フィールド初期化を追加 | Tasks.cpp で修正 |
| 3-6 | Global.h を include するファイル再コンパイル | `platformio run -e m5stack` 実行 |
| 3-7 | 警告・エラーメッセージがない確認 | ビルド出力確認 |

**修正対象ファイル**:
- `include/Global.h` (定数・構造体追加)
- `src/Tasks.cpp` (initGlobalData() 修正)

---

## タスク 4：RTC ドライバ統合

**親タスク**: フェーズ 2:4  
**推定時間**: 1h  
**完了条件**: RTC から現在時刻取得可能（シリアル確認）

### サブタスク

| # | 内容 | 検証方法 |
|:---|:---|:---|
| 4-1 | platformio.ini に `adafruit/RTClib@^2.1.1` 追加 | ビルド時にライブラリダウンロード確認 |
| 4-2 | main.cpp setup() に `#include "RTClib.h"` 追加 | コンパイル確認 |
| 4-3 | RTC インスタンス作成 (`RTC_DS3231 rtc;`) | グローバル変数として Tasks.cpp に追加 |
| 4-4 | setup() に Wire.begin() 追加（I²C バス初期化） | GPIO21/22 指定確認 |
| 4-5 | setup() に rtc.begin() 追加 | I²C アドレス 0x68 デバイス確認 |
| 4-6 | 現在時刻を serial に出力する仮のコード追加 | Serial.printf で日時出力 |
| 4-7 | ビルド・アップロード実行 | `platformio run --target upload -e m5stack` |
| 4-8 | シリアルモニタで RTC からの時刻出力確認 | `20260226 14:30:25` の形式で表示確認 |

**修正対象ファイル**:
- `platformio.ini` (lib_deps 追加)
- `src/main.cpp` (Wire.begin(), rtc.begin() 追加)
- `src/Tasks.cpp` (RTC インスタンス作成)

---

## タスク 5：SDManager クラス実装

**親タスク**: フェーズ 2:5  
**推定時間**: 2h  
**完了条件**: 全関数のコンパイル成功・エラーハンドリング実装完了

### サブタスク

| # | 内容 | チェック項目 |
|:---|:---|:---|
| 5-1 | `include/SDManager.h` ファイル作成 | クラス定義・関数宣言含む |
| 5-2 | `src/SDManager.cpp` ファイル作成 | 骨組みコピー |
| 5-3 | `init()` 関数実装 | microSD 検出・ファイルシステム初期化 |
| 5-4 | `createNewFile()` 関数実装 | ファイル新規作成・上書き防止 |
| 5-5 | `writeHeader()` 関数実装 | CSV ヘッダ行簡記込み |
| 5-6 | `writeData()` 関数実装 | SDData から CSV フォーマット生成・書き込み |
| 5-7 | `flush()` 関数実装 | バッファフラッシュ）書き込み確認 |
| 5-8 | `closeFile()` 関数実装 | ファイルクローズ・エラーチェック |
| 5-9 | `isReady()` 関数実装 | SD 検出フラグ返却 |
| 5-10 | `hasError()` 関数実装 | エラーフラグ返却 |
| 5-11 | `getLastError()` 関数実装 | エラーメッセージ（char*）返却 |
| 5-12 | エラーハンドリング実装 | リトライ・エラーログ出力 |
| 5-13 | main.cpp に SDManager::init() を setup() 内で呼び出し | EEPROM の後に実行 |
| 5-14 | ビルド実行 | `platformio run -e m5stack` |
| 5-15 | Warning・Error 確認 | コンパイル出力レビュー |

**新規ファイル**:
- `include/SDManager.h`
- `src/SDManager.cpp`

**修正対象ファイル**:
- `src/main.cpp` (setup() に SDManager::init() 呼び出し追加)

---

## タスク 6：IO_Task 修正

**親タスク**: フェーズ 2:6  
**推定時間**: 1h  
**完了条件**: ビルド成功・バッファ更新ロジック動作確認

### サブタスク

| # | 内容 | テスト方法 |
|:---|:---|:---|
| 6-1 | RUN状態判定 block を特定（IO_Task 内） | コード位置確認 |
| 6-2 | M_SDBuffer フィールド蓄積ロジック追加 | elapsedSeconds 等を計算 |
| 6-3 | M_SDWriteCounter インクリメント実装 | カウンタ >= 10 で write 呼び出し |
| 6-4 | バッファをリセット（カウンタ=0） | write() 後の処理 |
| 6-5 | RTC 時刻取得ロジック実装（millis比較） | UTC 時刻の代わりに経過秒を使用 |
| 6-6 | エラーハンドリング（write 失敗時） | M_SDError フラグセット |
| 6-7 | ビルド実行 | `platformio run -e m5stack` |
| 6-8 | M5Stack への書き込み実行 | `platformio run --target upload` |
| 6-9 | シリアルモニタでバッファ更新ログ確認 | (オプション: debug ログ出力) |

**修正対象ファイル**:
- `src/Tasks.cpp` (IO_Task 関数内)

---

## タスク 7：Logic_Task 修正

**親タスク**: フェーズ 2:7  
**推定時間**: 1.5h  
**完了条件**: ビルド成功・ファイルライフサイクル動作確認

### サブタスク

| # | 内容 | テスト方法 |
|:---|:---|:---|
| 7-1 | IDLE 状態の block で SD 初期化チェック | 最初 1回のみ実行 |
| 7-2 | RUN 状態への遷移検行（state machine） | first_entry_to_run フラグ追加 |
| 7-3 | RTC から現在日時取得→ファイル名生成 | snprintf() で YYYYMMDD_HHMMSS.csv 形式 |
| 7-4 | SDManager::createNewFile() 呼び出し | ファイル作成成功判定 |
| 7-5 | SDManager::writeHeader() 呼び出し | ヘッダ行書き込み |
| 7-6 | M_RunStartTime = millis() で RUN開始時刻記録 | IO_Task の elapsed 計算用 |
| 7-7 | RESULT 状態への遷移検行 | first_entry_to_result フラグ追加 |
| 7-8 | 統計データ行の最終追記（SDManager::writeData） | State="RESULT" で記録 |
| 7-9 | SDManager::flush() + closeFile() 実行 | ファイルクローズ |
| 7-10 | エラー時の M_SDError フラグセット | UI に通知 |
| 7-11 | ビルド実行 | `platformio run -e m5stack` |
| 7-12 | M5Stack への書き込み実行 | `platformio run --target upload` |
| 7-13 | BtnA押下で RUN → microSD にファイル作成確認 | PC で microSD の内容確認 |
| 7-14 | RUN → RESULT へボタン操作でファイルクローズ確認 | CSV ファイルが確定 |

**修正対象ファイル**:
- `src/Tasks.cpp` (Logic_Task 関数内)

---

## タスク 8：UI_Task 修正

**親タスク**: フェーズ 2:8  
**推定時間**: 0.5h  
**完了条件**: ビルド成功・SD エラー表示動作確認

### サブタスク

| # | 内容 | テスト方法 |
|:---|:---|:---|
| 8-1 | IDLE 画面に SD 状態表示コード追加 | `M_SDReady`, `M_SDError` フラグ確認 |
| 8-2 | SD 未検出時: "SD Not Found" 赤色表示 | SD が接続されていない場合 |
| 8-3 | SD 正常時: "SD Ready" 緑色表示 | SD が接続されている場合 |
| 8-4 | RUN 画面に SD 書き込み中インジケータ表示（軽量） | M_SDWriteCounter==0 で "...SD" |
| 8-5 | エラー時: "SD Error: xxx" 赤色表示（メッセージ含む） | SDManager::getLastError() を使用 |
| 8-6 | ビルド実行 | `platformio run -e m5stack` |
| 8-7 | M5Stack への書き込み実行 | `platformio run --target upload` |
| 8-8 | IDLE 画面で SD 状態が表示されることを確認 | 液晶で視認 |

**修正対象ファイル**:
- `src/Tasks.cpp` (UI_Task 関数内)

---

## タスク 9：統合ビルド・動作確認

**親タスク**: フェーズ 3:9  
**推定時間**: 2h  
**完了条件**: CSV ファイル生成・データ記録確認

### テストケース詳細

| # | テスト項目 | 実行方法 | 期待イメージ結果 | 合格基準 |
|:---|:---|:---|:---|:---|
| 9-1 | ビルド成功 | `platformio run -e m5stack` | "SUCCESS" メッセージ | Error/Warning ゼロ |
| 9-2 | 書き込み成功 | `platformio run --target upload` | アップロード完了 | ExitCode=0 |
| 9-3 | M5Stack 起動確認 | 本体を起動 | "Temperature Eval Tool" 画面表示 | IDLE 画面表示 |
| 9-4 | shireial ログで RTC 時刻確認 | シリアルモニタ開く | 日時出力（20260226 14:30:25） | 正しい日時 |
| 9-5 | IDLE 画面で SD 状態確認 | 液晶表示確認 | "SD Ready" or "SD Not Found" | 接続状態に応じた表示 |
| 9-6 | RUN 開始でファイル作成確認 | BtnA 押す + microSD PC 確認 | YYYYMMDD_HHMMSS.csv ファイル存在 | ファイル名形式正しい |
| 9-7 | CSV ヘッダ行確認 | ファイルをテキストエディタで開く | "ElapsedSec,Temp_C,..." | ヘッダ行格式正しい |
| 9-8 | RUN 中データ記録確認 | 計測 1分間 → ファイルサイズ増加 | ファイルサイズ増加（150KB〜） | サイズ > 0 |
| 9-9 | CSV データ行形式確認 | PC で CSV を開く | "0,540.2,RUN,1,..." | カンマ区切り形式正しい |
| 9-10 | RESULT 遷移でファイルクローズ確認 | BtnA 押下→RESULT | ファイルハンドルクローズ | ファイル閉じられた（PC で再オープン可能） |
| 9-11 | フラッシュ使用率確認 | ビルド出力 | "Flash: xxx% used" | <50% |
| 9-12 | RAM 使用率確認 | ビルド出力 | "RAM: xxx% used" | <20% |
| 9-13 | SD エラー時の画面確認 | SD カード抜く→RUN試行 | "SD Error" 表示 | 赤色で警告表示 |

**トラブル対応**:
- **SD が検出されない**: ファイルシステム初期化エラー → SDManager::init() をデバッグ
- **CSV ヘッダ行が重複**: file.open() 上書き権限確認
- **データ行が記録されない**: IO_Task の M_SDWriteCounter ロジック確認

---

## タスク 10：ドキュメント作成

**親タスク**: フェーズ 3:10  
**推定時間**: 1.5h  
**完了条件**: 3つのドキュメント完成

### サブタスク

| # | 内容 | 成果物 | 要素 |
|:---|:---|:---|:---|
| 10-1 | PHASE4_HARDWARE_GUIDE.md 作成 | ハードウェア接続ガイド | RTC I²C 配線図・セットアップ手順 |
| 10-2 | PHASE4_CSV_ANALYSIS_GUIDE.md 作成 | CSV 分析ガイド | データの読み方・Excel/GnuPlot でのグラフ化方法 |
| 10-3 | SESSION5_COMPLETION_REPORT.md 作成 | 完了報告書（Session 5 想定） | タスク完了・ビルド結果・テスト結果 |
| 10-4 | future_plan.md を更新 | Phase 4 進捗更新 | ✅ 完了マーク、Phase 5 への準備記載 |
| 10-5 | CODE_REFERENCE.md を更新（オプション） | コード参照ドキュメント | SDManager 関数リファレンス |

**出力ファイル例**:
```
docs/
├─ guides/
│  └─ PHASE4_HARDWARE_GUIDE.md (新規)
│  └─ PHASE4_CSV_ANALYSIS_GUIDE.md (新規)
├─ sessions/
│  └─ SESSION5_COMPLETION_REPORT.md (新規)
└─ future/
   └─ future_plan.md (更新)
```

---

## タスク依存関係グラフ

```
タスク 1 (計画)
  └─→ タスク 2 (ハードウェア確認)
       └─→ タスク 3 (Global.h 拡張)
            └─→ タスク 4 (RTC 統合) ┐
                 └───────────────────┤
                                     ├─→ タスク 5 (SDManager) ┐
タスク 4 (RTC) ┐                    │                         │
               └────────────────────┤                         ├─→ タスク 9 (統合テスト)
                                    ├─→ タスク 6 (IO修正)     │     └─→ タスク 10 (ドキュメント)
                                    ├─→ タスク 7 (Logic修正) ┤
                                    └─→ タスク 8 (UI修正)   ┘
```

---

## 進捗トラッキング用チェックリスト

### フェーズ 1 チェック（計画・環境構築）

- [ ] 1-1: PHASE4_IMPLEMENTATION_PLAN.md 作成
- [ ] 1-2: PHASE4_ARCHITECTURE_DESIGN.md 作成
- [ ] 1-3: RTC 方針確定
- [ ] 1-4: RTC 発注
- [ ] 1-5: future_plan.md 更新
- [ ] 2-1 ～ 2-6: ハードウェア確認完了

### フェーズ 2 チェック（コア実装）

- [ ] 3-1 ～ 3-7: Global.h 拡張
- [ ] 4-1 ～ 4-8: RTC ドライバ統合
- [ ] 5-1 ～ 5-15: SDManager 実装
- [ ] 6-1 ～ 6-9: IO_Task 修正
- [ ] 7-1 ～ 7-14: Logic_Task 修正
- [ ] 8-1 ～ 8-8: UI_Task 修正

### フェーズ 3 チェック（検証・完了）

- [ ] 9-1 ～ 9-13: 統合テスト全項目
- [ ] 10-1 ～ 10-5: ドキュメント完成

---

**最終更新**: 2026年2月26日  
**推定総時間**: 15.5h（高品質優先）
