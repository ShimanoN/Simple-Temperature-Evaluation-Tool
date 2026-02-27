# Phase 4 実装計画書：SDカードデータ保存

**対象**: 計測データの microSD カードへの CSV 形式保存機能  
**難易度**: 中（RTC方針決定がポイント）  
**推定実装期間**: 2-3日（高品質優先）  
**作成日**: 2026年2月26日

---

## Ⅰ 実装概要

### 目的

RUN状態での温度計測データを microSD カードに CSV 形式で自動記録し、事後分析・精度検証を可能にする。

### 追加機能

| 機能 | 内容 |
|:---|:---|
| **CSV保存** | 計測データを microSD に自動記録 |
| **タイムスタンプ** | ファイル名に日時（RTC要）または相対時間を記録 |
| **データ項目** | 経過時間・温度・状態・サンプル数・統計情報など |
| **エラー通知** | SD カード未検出・書き込み失敗を LCD 表示 |

### 実装方針（層別責任）

```
IO_Task    → SD カード書き込み実行（外部デバイス通信 = IO層）
Logic_Task → 書き込みトリガー管理（フラグON/OFF制御）
UI_Task    → エラー状態の LCD 表示
```

---

## Ⅱ 前提条件決定

### ⚠️ RTC（リアルタイムクロック）方針決定が必須

M5Stack Basic V2.7 には RTC が搭載されていません。  
**実装開始前に以下のいずれかを確定すること：**

| 対応策 | コスト | 難易度 | 選択 |
|:---|:---:|:---:|:---:|
| **（推奨）DS3231 RTC 追加** | 約300〜500円 | 低 | ☐ |
| WiFi + NTP 時刻同期 | 0円 | 中〜高 | ☐ |
| millis() 相対時間のみ（暫定） | 0円 | 低 | ☐ |

**推奨理由**: 
- DS3231 は I²C 接続で実装が簡単
- ライブラリが豊富（RTClib など）
- WiFi依存なし・確実性が高い
- M5Stack の GPIO ポート拡張可能

### 選定案

**本計画では ☑ DS3231 RTC 追加 を前提に進める**

---

## Ⅲ 詳細設計

### 3.1 データフォーマット

#### CSV ヘッダ行

```csv
ElapsedSec,Temp_C,State,Samples,Average_C,StdDev_C,Max_C,Min_C,HI_ALARM,LO_ALARM
```

#### CSV データ行（計測中に逐次追記）

```csv
0,540.2,RUN,1,540.2,0.0,540.2,540.2,false,false
1,540.3,RUN,2,540.25,0.05,540.3,540.2,false,false
5,540.1,RUN,10,540.15,0.08,540.4,539.9,false,false
120,540.1,RESULT,240,540.1,1.2,542.3,538.5,false,false
```

**1行の構成**:
- **ElapsedSec**: RUN開始からの経過秒数（millis() からの相対時間）
- **Temp_C**: 現在の測定温度（℃）
- **State**: 現在の状態（"RUN", "RESULT"等）
- **Samples**: 取得サンプル数
- **Average_C**: 現在の平均温度
- **StdDev_C**: 標準偏差
- **Max_C**: 最大温度
- **Min_C**: 最小温度
- **HI_ALARM**: 上限アラーム中フラグ
- **LO_ALARM**: 下限アラーム中フラグ

### 3.2 ファイル命名規則

#### 方案A：RTC使用時（推奨）

```
YYYYMMDD_HHMMSS.csv
例: 20260226_143025.csv (2026年2月26日 14:30:25 で RUN開始)
```

#### 方案B：相対時間×連番

```
LOG_NNN.csv (NNN = 001, 002, ...)
例: LOG_001.csv, LOG_002.csv
```

**本計画では方案A（RTC）を前提に進める**

### 3.3 ファイルライフサイクル

```
① RUN開始
   → ファイルオープン or 新規作成 (ファイル名は RTC から生成)
   → ヘッダ行書き込み
   
② RUN中（定期的に）
   → バッファに計測データを蓄積（毎サンプル書くと IO 負荷大）
   → 10サンプルごとに SD カードへフラッシュ
   
③ RESULT遷移（RUN終了）
   → 統計サマリー行を追記
   → ファイルクローズ（確認書き込み）
   
④ IDLE 復帰
   → ファイルハンドル初期化
```

### 3.4 メモリ効率化

**書き込みバッファ方式**:
```cpp
struct SDBuffer {
  char data[256];  // 1行分のCSVデータ
  bool pending;    // フラッシュ待ち状態
};
```

- **毎サンプル書き込み**: IO 負荷大、オーバーヘッドが大きい
- **バッファ蓄積**: 10〜20行ごとにまとめて書き込み（推奨）
- **RAM使用量**: 1〜2KB（M5Stack は 327KB 余裕有）

### 3.5 エラーハンドリング

| 条件 | 対応 | LCD表示 |
|:---|:---|:---|
| **SD 未検出** | ファイル生成失敗 → フラグ立て | "SD Card Not Found" |
| **書き込み失敗** | リトライ 3回 → 失敗ログ出力 | "SD Write Error" |
| **容量不足** | ファイルクローズ・エラー通知 | "SD Full" |
| **ファイルオープン失敗** | 状態を RUN → IDLE に強制遷移 | "Cannot Save Data" |

---

## Ⅳ 実装タスク分解（10段階）

### フェーズ 1: 環境構築・設計確認（タスク 1-2）

#### **【タスク 1】Phase 4 計画書作成・前提条件決定**
- **内容**: 本計画書作成・RTC 方針確定
- **成果物**: 本ドキュメント + RTC 購入リスト
- **時間**: 1h
- **完了条件**: 
  - [ ] 計画書完成
  - [ ] RTC（DS3231）購入確定 or 代替案決定

#### **【タスク 2】RTC + SD ハードウェア確認**
- **内容**: 既存 M5Stack の microSD スロット確認、RTC の I²C 接続仕様確認
- **成果物**: ハードウェア接続図、ピン割り当て表
- **時間**: 0.5h
- **完了条件**:
  - [ ] M5Stack の microSD スロット位置確認
  - [ ] RTC の I²C ピン（SDA=GPIO21, SCL=GPIO22）確認
  - [ ] SPI 共有ルール確認（LCD との競合なし）

### フェーズ 2: コア実装（タスク 3-8）

#### **【タスク 3】Global.h 拡張**
- **内容**: SD カード関連の定数・構造体追加
- **追加項目**:
  ```cpp
  // SD カード定数
  constexpr const char* SD_MOUNT_POINT = "/sd";
  constexpr uint32_t SD_BUFFER_SIZE = 256;
  constexpr uint16_t SD_WRITE_INTERVAL = 10;  // 10サンプルごと
  
  // RTC 定数
  constexpr uint8_t RTC_I2C_ADDR = 0x68;
  
  // SDData 構造体
  struct SDData {
    uint32_t elapsedSeconds;
    float temperature;
    const char* state;
    uint32_t sampleCount;
    float averageTemp;
    float stdDev;
    float maxTemp;
    float minTemp;
    bool hiAlarm;
    bool loAlarm;
  };
  
  // GlobalData に追加
  struct GlobalData {
    // ... 既存フィールド ...
    
    // Phase 4: SD カード関連
    bool M_SDReady;           // SD カード検出フラグ
    bool M_SDError;           // SD エラーフラグ
    char M_CurrentDataFile[32];  // 現在のファイル名
    SDData M_SDBuffer;        // バッファ（1行分）
    uint16_t M_SDWriteCounter;  // 書き込みカウンタ
    uint32_t M_RunStartTime;  // RUN開始時刻（millis）
  };
  ```
- **時間**: 1h
- **完了条件**:
  - [ ] Global.h にコメント付きで追加
  - [ ] コンパイルエラーなし

#### **【タスク 4】RTC ドライバ統合**
- **内容**: RTClib ライブラリの platformio.ini 追加、I²C 初期化
- **作業**:
  - [ ] platformio.ini に `lib_deps = adafruit/RTClib` 追加
  - [ ] main.cpp setup() に RTC 初期化コード追加
  - [ ] 時刻取得テスト（シリアル出力で確認）
- **時間**: 1h
- **完了条件**:
  - [ ] ビルド成功
  - [ ] RTC から現在時刻取得可能（シリアルで確認）

#### **【タスク 5】SDManager クラス実装**
- **内容**: EEPROMManager と同パターンで SD 操作を集約
- **ファイル**: `src/SDManager.cpp`, `include/SDManager.h`
- **関数群**:
  ```cpp
  class SDManager {
  public:
    // 初期化・終了
    static bool init();
    static bool begin();
    static void end();
    
    // ファイル操作
    static bool createNewFile(const char* filename);
    static bool writeHeader();
    static bool writeData(const SDData& data);
    static bool flush();
    static bool closeFile();
    
    // 状態確認
    static bool isReady();
    static bool hasError();
    static const char* getLastError();
  };
  ```
- **エラーハンドリ**:
  - [ ] SD 未検出時の処理
  - [ ] 書き込み失敗時のリトライ
  - [ ] ファイルクローズ失敗の処理
- **時間**: 2h
- **完了条件**:
  - [ ] SDManager 実装完成
  - [ ] 全関数コンパイル成功
  - [ ] エラーハンドリング実装済み

#### **【タスク 6】IO_Task 修正**
- **内容**: 計測データを SDBuffer に蓄積、定期的に SD へ書き込み
- **修正内容**:
  ```cpp
  // RUN 中毎回
  if (G.M_CurrentState == State::RUN) {
    // バッファに現在データを詰める
    G.M_SDBuffer.elapsedSeconds = (millis() - G.M_RunStartTime) / 1000;
    G.M_SDBuffer.temperature = G.D_FilteredPV;
    G.M_SDBuffer.state = "RUN";
    // ... その他フィールド ...
    
    // 10サンプルごとに SD へ書き込み
    G.M_SDWriteCounter++;
    if (G.M_SDWriteCounter >= SD_WRITE_INTERVAL) {
      SDManager::writeData(G.M_SDBuffer);
      G.M_SDWriteCounter = 0;
    }
  }
  ```
- **時間**: 1h
- **完了条件**:
  - [ ] ビルド成功
  - [ ] 計測中のバッファ更新動作確認

#### **【タスク 7】Logic_Task 修正**
- **内容**: SD ファイルのオープン・クローズ制御
- **修正内容**:
  ```cpp
  case State::IDLE:
    // ...既存コード...
    
    // SD 初期化（毎回は最小化）
    if (!G.M_SDReady) {
      G.M_SDReady = SDManager::init();
    }
    break;
    
  case State::RUN:
    // RUN 開始時：ファイルオープン・ヘッダ書き込み
    if (first_entry_to_run) {
      snprintf(G.M_CurrentDataFile, 32, "%04d%02d%02d_%02d%02d%02d.csv",
               RTClib year, month, day, hour, minute, second);
      if (SDManager::createNewFile(G.M_CurrentDataFile)) {
        SDManager::writeHeader();
        G.M_RunStartTime = millis();
      } else {
        G.M_SDError = true;
      }
    }
    break;
    
  case State::RESULT:
    // RESULT 遷移時：ファイルクローズ
    SDManager::flush();
    SDManager::closeFile();
    break;
  ```
- **時間**: 1.5h
- **完了条件**:
  - [ ] ビルド成功
  - [ ] RUN開始時にファイル作成確認
  - [ ] RESULT移行時にファイルクローズ確認

#### **【タスク 8】UI_Task 修正**
- **内容**: SD エラー状態の LCD 表示
- **修正内容**:
  ```cpp
  // IDLE 画面：SD 状態表示
  if (G.M_SDError) {
    M5.Lcd.setTextColor(RED, BLACK);
    M5.Lcd.printf("SD Error: %s\n", SDManager::getLastError());
  } else if (G.M_SDReady) {
    M5.Lcd.setTextColor(GREEN, BLACK);
    M5.Lcd.printf("SD Ready\n");
  }
  
  // RUN 画面：SD 書き込み中の状態表示（軽量）
  if (G.M_CurrentState == State::RUN && G.M_SDWriteCounter == 0) {
    M5.Lcd.printf("...SD\n");
  }
  ```
- **時間**: 0.5h
- **完了条件**:
  - [ ] ビルド成功
  - [ ] LCD に SD 状態が表示される

### フェーズ 3: テスト・検証（タスク 9-10）

#### **【タスク 9】統合ビルド・動作確認**
- **内容**: 全修正をマージしてビルド・基本動作確認
- **テスト項目**:
  - [ ] ビルド成功（Warning ゼロ）
  - [ ] M5Stack に書き込み成功
  - [ ] 起動時 RTC 時刻取得確認（シリアル出力）
  - [ ] IDLE 画面で SD 状態表示確認
  - [ ] RUN 開始でファイル作成確認（microSD を PC で確認）
  - [ ] RUN 中にデータ記録確認（エクスプローラで CSVファイルサイズ増加）
  - [ ] RESULT へ移行でファイルクローズ確認
  - [ ] CSV ファイルを PC で開いて内容確認（ヘッダ・データ行OK）
- **時間**: 2h
- **完了条件**:
  - [ ] CSV ファイルが microSD に正常に生成される
  - [ ] データ行が正しくフォーマットされている
  - [ ] エラー時の画面表示が機能している

#### **【タスク 10】運用ガイド・ドキュメント作成**
- **内容**: 
  - Phase 4 完了レポート（SESSION形式）
  - CSV 記録の分析方法ガイド
  - トラブルシューティング（SD Not Found時の対処等）
  - ハードウェア接続図（RTC I²C 配線）
- **成果物**:
  - [ ] PHASE4_HARDWARE_GUIDE.md（RTC接続手順）
  - [ ] PHASE4_CSV_ANALYSIS_GUIDE.md（CSV分析方法）
  - [ ] SESSION5_COMPLETION_REPORT.md（完了報告書）
  - [ ] future_plan.md 更新（Phase 5 への準備）
- **時間**: 1.5h
- **完了条件**:
  - [ ] 3つのガイドドキュメント完成
  - [ ] future_plan.md に Phase 4 完了マーク

---

## Ⅴ 実装スケジュール（推定）

| フェーズ | タスク | 推定時間 | 実績 |
|:---:|:---|:---:|:---|
| **フェーズ 1** | 1-2 | 1.5h | |
| **フェーズ 2** | 3-8 | 8.5h | |
| **フェーズ 3** | 9-10 | 3.5h | |
| **小計** |  | **13.5h** | |
| **余裕** | テスト・修正 | 2h | |
| **合計** |  | **15.5h** | |

**トータル推定**: 2営業日（フルタイム）or 3〜4営業日（兼務）

---

## Ⅵ リスク・検討事項

### リスク R1: RTC ハードウェア入手⏱️

**影響**: RTC が手元に到着しないと実装を開始できない  
**対策**:
- 事前に Amazon で発注（1-2営業日で到着）
- 代替案: 相対時間（millis）で一次実装し、後で RTC に差し替え

### リスク R2: SD カード SPI 共有による競合

**影響**: LCD と SD がハードウェア SPI を共有するため、CS ピン管理ミスで片方が動作不可に  
**対策**:
- M5Stack 標準の SD ライブラリ使用（既に SPI 管理済み）
- LCD との排他制御は既に実装されている（M5Stack IoT により）

### リスク R3: CSV データ行が大きすぎて、バッファオーバーフロー

**影響**: 256 バイトのバッファを超えると、データ破損 or 書き込み失敗  
**対策**:
- バッファサイズを 512 バイトに増加（RAM 余裕十分）
- スプリント フォーマットでサイズ事前確認

### 検討事項 C1: SD カード容量・保持期間

**現在**: 4〜8GB microSD  
**計測例**:
```
1行あたり 150 バイト（CSV)
1000行（≈計測15分）: 150KB
1日間（96000サンプル）: 14.4MB
1年間: 5.3GB
```

→ **4GB で約 9ヶ月分計測可能**。実用的。

---

## Ⅶ 次フェーズへの準備

### Phase 5（マルチチャンネル）との連携

CSVフォーマットを拡張可能な設計に：
```csv
ElapsedSec,Temp_CH1_C,Temp_CH2_C,Temp_CH3_C,State,...
```

→ Phase 4 CSVヘッダは後方互換性を保ったまま拡張可能

---

## Ⅷ 参考資料

- **RTClib ガイド**: https://adafruit-circuitpython-rtclib.readthedocs.io/
- **M5Stack SD API**: https://docs.m5stack.com/
- **CSV フォーマット RFC4180**: https://tools.ietf.org/html/rfc4180

---

**作成者**: GitHub Copilot  
**作成日**: 2026年2月26日  
**レビュー待ち**: Shimano
