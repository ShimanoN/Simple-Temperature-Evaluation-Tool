# Phase 4 アーキテクチャ設計書

**対象**: SDカードデータ保存機能の詳細設計  
**作成日**: 2026年2月26日  

---

## Ⅰ ハードウェア構成図

### 現在の構成（Phase 1-3）

```
┌─────────────────────────────────────┐
│        M5Stack Basic V2.7           │
│  ┌─────────────────────────────────┤
│  │  ESP32-D0WD (Dual-core 240MHz)  │
│  │  ├─ GPIO5: MAX31855 CS           │
│  │  ├─ GPIO18(SCK): SPI Clock       │
│  │  ├─ GPIO19(MISO): SPI Data In    │
│  │  ├─ GPIO23(MOSI): SPI Data Out   │
│  │  └─ GPIO2,12,15: (予約)          │
│  │                                  │
│  │  Built-in:                       │
│  │  ├─ TFT LCD 320x240 pixels       │
│  │  ├─ microSD Card Reader (SPI)    │
│  │  ├─ Speaker (GPIO 25)            │
│  │  └─ Button×3 (BtnA/B/C)         │
│  │                                  │
│  │  EEPROM (4KB)                    │
│  └─────────────────────────────────┤
│                                     │
│  I²C Bus (SDA=GPIO21, SCL=GPIO22)   │  ← Phase 4 で活用
│  └─ RTC DS3231 (0x68)               │  ← 新規追加
│  └─ (将来) 温度センサ等              │
│                                     │
└─────────────────────────────────────┘

外部接続：
MAX31855 ─(SPI:CS=GPIO5)─→ M5Stack
RTC DS3231 ─(I²C:SDA/SCL)─→ M5Stack

存在するリソース：
✓ microSD Card Slot
✓ I²C Bus (GPIO21/22)
✓ SPI Bus (共有: LCD + MAX31855 + microSD)
```

### Phase 4 での追加（RTC）

```
新規：DS3231 RTC モジュール

┌─ DS3231 ─────┐
│ VCC (3.3V)   ├─→ M5Stack 3.3V
│ GND          ├─→ M5Stack GND
│ SDA          ├─→ M5Stack GPIO21
│ SCL          ├─→ M5Stack GPIO22
│ INT (不使用)  │
│ CR2032 (付属) │ (バックアップ電池)
└──────────────┘

接続方法：
1. ジャンパワイヤ 4本（VCC/GND/SDA/SCL）
2. M5Stack の I²C ポート拡張板またはブレッドボード活用
3. 熱電対・MAX31855 と同じく M5Stack の側面に固定（3M養生テープ）
```

---

## Ⅱ ソフトウェアアーキテクチャ

### クラス図

```cpp
┌────────────────────────────────────────┐
│         GlobalData (Global.h)          │
│  ──────────────────────────────────────  │
│  D_FilteredPV: float                   │
│  D_Count: long                         │
│  D_Sum: double                         │
│  ...（既存データ）...                   │
│  ────────────────────────────────────── │
│  ★Phase 4 追加：                       │
│  M_SDReady: bool                       │
│  M_SDError: bool                       │
│  M_CurrentDataFile: char[32]           │
│  M_SDBuffer: SDData                     │
│  M_SDWriteCounter: uint16_t            │
│  M_RunStartTime: uint32_t              │
└────────────────────────────────────────┘
                  △
                  │ 利用
                  │
┌────────────────────────────────────────┐
│      SDManager (SDManager.h)           │
│  ────────────────────────────────────── │
│  static methods:                       │
│    init()         : bool               │
│    begin()        : bool               │
│    createNewFile(filename) : bool      │
│    writeHeader()  : bool               │
│    writeData(SDData) : bool            │
│    flush()        : bool               │
│    closeFile()    : bool               │
│    isReady()      : bool               │
│    hasError()     : bool               │
│    getLastError() : const char*        │
└────────────────────────────────────────┘

┌────────────────────────────────────────┐
│      M5Stack SD Library (内部)         │
│  microSD Interface (SPI)               │
└────────────────────────────────────────┘
```

### 層別責任モデル

```
┌─────────────────────────────────────────┐
│         UI_Task (200ms 周期)            │
│  ┌───────────────────────────────────┐  │
│  │ • 画面描画（IDLE/RUN/RESULT）      │  │
│  │ • SD エラー状態表示               │  │
│  │ • 状態遷移ビジュアル               │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
                  △ 読取
                  │ (M_SDReady, M_SDError)
                  │
┌─────────────────────────────────────────┐
│      Logic_Task (50ms 周期)      ★      │
│  ┌───────────────────────────────────┐  │
│  │ • 状態遷移制御（IDLE↔RUN↔RESULT）  │  │
│  │ ★ SD ファイルオープン (RUN開始時)  │  │
│  │ ★ SD ファイルクローズ (RESULT時)  │  │
│  │ • アラーム判定トリガー             │  │
│  │ • 統計計算トリガー                 │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
          △ 制御信号
          │ 設定 (M_SDBuffer)
          │ フラグ管理
          │
┌─────────────────────────────────────────┐
│        IO_Task (10ms 周期)      ★       │
│  ┌───────────────────────────────────┐  │
│  │ • MAX31855 温度読取               │  │
│  │ • ボタン入力検出                 │  │
│  │ ★ SD カード書き込み実行           │  │
│  │ ★ RTC 時刻取得                   │  │
│  │ • アラーム音声出力                │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
          △
          │ ハードウェア操作
          │
┌─────────────────────────────────────────┐
│      Drivers & Peripherals              │
│  ├─ MAX31855 (Adafruit ライブラリ)     │
│  ├─ M5Stack SD (M5Stack ライブラリ)    │
│  ├─ RTC DS3231 (RTClib ライブラリ)    │
│  └─ M5Stack LCD/Speaker (M5.h)        │
└─────────────────────────────────────────┘
```

---

## Ⅲ データフロー図（RUN 状態）

```
┌──────────────────────────────────────┐
│  IO_Task (10ms周期)                   │
│                                       │
│  1. MAX31855 読取                     │
│     └─→ D_FilteredPV (フィルタ後値)   │
│                                       │
│  2. RTC 時刻取得                      │
│     └─→ currentTime (構造体)         │
│                                       │
│  3. バッファ蓄積                      │
│     ├─ M_SDBuffer.temperature ← D_FilteredPV
│     ├─ M_SDBuffer.elapsedSeconds ← RUN開始から経過秒
│     └─ ... その他フィールド ...       │
│                                       │
│  4. カウンタ インクリメント             │
│     M_SDWriteCounter++                │
│     if (M_SDWriteCounter >= 10) {    │
│       SDManager::writeData()          │
│       M_SDWriteCounter = 0            │
│     }                                 │
└──────────────────────────────────────┘
           △
           │ 毎 10ms
           │
┌──────────────────────────────────────┐
│  Logic_Task (50ms周期)                │
│                                       │
│  • 統計計算トリガー確認               │
│  • 状態遷移ロジック                   │
│  • RUN→RESULT 時：統計行追記トリガー  │
└──────────────────────────────────────┘
           △
           │ 状態フラグ監視
           │
┌──────────────────────────────────────┐
│  UI_Task (200ms周期)                  │
│                                       │
│  • 温度表示 (D_FilteredPV)            │
│  • 状態表示 ("RUN")                   │
│  • SD 中」表示（書き込み中）           │
│  • 統計情報表示（計測中）              │
└──────────────────────────────────────┘
           △
           │
┌──────────────────────────────────────┐
│  microSD Card (SPI経由)               │
│                                       │
│  ファイル例: 20260226_143025.csv      │
│  ────────────────────────────────────  │
│  ElapsedSec,Temp_C,State,...         │
│  0,540.2,RUN,1,...                  │
│  1,540.3,RUN,2,...                  │
│  ...（毎10サンプルごと追記）          │
│                                       │
│  ファイルサイズ: 計測15分で 150KB     │
└──────────────────────────────────────┘
```

---

## Ⅳ 初期化シーケンス図（setup()）

```
[main.cpp setup()]
    ↓
[1] M5.begin()              ← LCD, Speaker, Button 初期化
    ↓
[2] Serial.begin(115200)    ← シリアル通信開始
    ↓
[3] initGlobalData()        ← グローバル変数初期化
    ↓
[4] EEPROMManager::init()   ← EEPROM 初期化
    ↓
[5] EEPROM_LoadToGlobal()   ← アラーム設定値読み込み
    ↓
[6] ★ EEPROMManager::init() （or SDManager::begin()）
    │  ├─ microSD 検出判定
    │  ├─ SD カードファイルシステム初期化
    │  └─ M_SDReady フラグ設定
    ↓
[7] ★ Wire.begin(SDA=21, SCL=22)  ← I²C バス初期化（RTC用）
    ↓
[8] ★ RTC ds3231 初期化
    │  ├─ I²C アドレス 0x68 にデバイス確認
    │  ├─ 時刻取得
    │  └─ シリアル出力で確認
    ↓
[9] MAC31855 確認（既存）
    ↓
[10] delay(1000) + setup完了メッセージ表示
    ↓
[loop()] へ移行
```

---

## Ⅴ ファイル操作の詳細フロー（RUN 開始～終了）

### ① RUN 開始時（Logic_Task）

```cpp
case State::RUN:
  if (first_entry_to_run) {
    // ファイル名生成（RTC から）
    DateTime now = rtc.now();
    snprintf(G.M_CurrentDataFile, 32,
             "%04d%02d%02d_%02d%02d%02d.csv",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());
    
    // ファイル作成・オープン
    if (SDManager::createNewFile(G.M_CurrentDataFile)) {
      // ヘッダ行書き込み
      SDManager::writeHeader();  // CSV ヘッダ
      G.M_RunStartTime = millis();
      G.M_SDError = false;
    } else {
      G.M_SDError = true;  // LCD に表示される
    }
    first_entry_to_run = false;
  }
  break;
```

### ② RUN 中（IO_Task）毎 10ms

```cpp
if (G.M_CurrentState == State::RUN) {
  // バッファに現在データを詰める
  G.M_SDBuffer.elapsedSeconds = (millis() - G.M_RunStartTime) / 1000;
  G.M_SDBuffer.temperature = G.D_FilteredPV;
  G.M_SDBuffer.state = "RUN";
  G.M_SDBuffer.sampleCount = G.D_Count;
  G.M_SDBuffer.averageTemp = G.D_Average;
  G.M_SDBuffer.stdDev = G.D_StdDev;
  G.M_SDBuffer.maxTemp = G.D_Max;
  G.M_SDBuffer.minTemp = G.D_Min;
  G.M_SDBuffer.hiAlarm = G.M_HiAlarm;
  G.M_SDBuffer.loAlarm = G.M_LoAlarm;
  
  // 10サンプルごとに SD へ書き込み
  G.M_SDWriteCounter++;
  if (G.M_SDWriteCounter >= SD_WRITE_INTERVAL) {
    if (!SDManager::writeData(G.M_SDBuffer)) {
      G.M_SDError = true;
    }
    G.M_SDWriteCounter = 0;
  }
}
```

⏱️ **タイミング**: RUN 中は毎 10ms ごとに呼ばれるため、
実質 100ms ごと（SD 書き込みは 10 サンプルごと = 100ms）

### ③ RESULT 遷移時（Logic_Task）

```cpp
case State::RESULT:
  if (first_entry_to_result) {
    // 統計情報の最終行を追記
    G.M_SDBuffer.state = "RESULT";
    SDManager::writeData(G.M_SDBuffer);  // 最終サマリー行
    
    // ファイルをフラッシュして確認
    if (!SDManager::flush() || !SDManager::closeFile()) {
      G.M_SDError = true;
    }
    first_entry_to_result = false;
  }
  break;
```

---

## Ⅵ CSV ファイル例

### ヘッダ行（1行目）

```csv
ElapsedSec,Temp_C,State,Samples,Average_C,StdDev_C,Max_C,Min_C,HI_ALARM,LO_ALARM
```

### データ行（2行目以降）

```csv
0,540.2,RUN,1,540.20,0.00,540.2,540.2,false,false
1,540.3,RUN,2,540.25,0.05,540.3,540.2,false,false
2,540.4,RUN,3,540.30,0.08,540.4,540.2,false,false
...
10,540.1,RUN,11,540.25,0.10,540.4,540.1,false,true
...
120,540.0,RESULT,240,540.10,1.20,542.3,538.5,false,false
```

**データ例の説明**:
- `ElapsedSec=10`: RUN開始から10秒経過
- `Temp_C=540.1`: 現在の温度 540.1°C
- `State=RUN`: 計測中
- `Samples=11`: 累計11サンプル取得
- `Average_C=540.25`: 平均 540.25°C
- `StdDev_C=0.10`: 標準偏差 0.10°C（ばらつき小）
- `Max_C=540.4`: 最高 540.4°C
- `Min_C=540.1`: 最低 540.1°C
- `HI_ALARM=false`: 上限アラームなし
- `LO_ALARM=true`: 下限アラーム発生中

---

## Ⅶ エラーハンドリングツリー

```
┌─────────────────────────────────────┐
│    SD カードエラー決定木             │
└─────────────────────────────────────┘

[setup() 時点]
  ├─ SD 検出失敗
  │  ├─ 対応: M_SDReady = false
  │  ├─ LCD : "SD Not Found" (IDLE 画面)
  │  └─ 動作: メジャー機能は動作継続（SD未使用）
  │
  └─ SD 検出成功
     └─ EEPROM キャッシュに SD 状態保存？（検討）

[RUN 開始時]
  ├─ ファイル作成失敗
  │  ├─ 原因: SD 容量不足, ファイルシステムエラー等
  │  ├─ 対応: M_SDError = true, ファイル操作中止
  │  ├─ LCD: "Cannot Save Data" (IDLE/RUN 画面)
  │  └─ 動作: 計測は継続、CSV 記録のみスキップ
  │
  └─ ファイル作成成功
     ├─ ヘッダ行書き込み

[RUN 中（毎サンプル）]
  ├─ バッファ蓄積 [成功]
  │  └─ カウンタ増加
  │
  └─ 10サンプルごとの書き込み
     ├─ 成功 → M_SDError = false
     └─ 失敗
        ├─ リトライ 1回
        ├─ 再度失敗 → M_SDError = true
        ├─ LCD: "SD Write Error"
        └─ 動作: バッファをリセット、以降スキップ

[RESULT 遷移時]
  ├─ 最終データ行書き込み
  ├─ ファイルフラッシュ
  ├─ ファイルクローズ
  └─ 失敗時: M_SDError = true（但し RUN 記録は保持）

[IDLE 復帰時]
  └─ ファイルハンドルリセット
```

---

## Ⅷ メモリマップ（ESP32 内部）

```
※参考値（実装前の推定）

┌──────────────────────────────────┐
│   Flash Memory (1.3MB)           │
├──────────────────────────────────┤
│ • Program Code              30% │
│ • Libraries (M5, Adafruit)  20% │
│ • Const Data (strings etc)   5% │
│ ★ Phase 4 Add: SD+RTC lib  +3% │
│ • Free Space               42% │
│                                │
│ 推定最終: Flash 33% / 1.3MB    │
└──────────────────────────────────┘

┌──────────────────────────────────┐
│   RAM (SRAM) (328KB)             │
├──────────────────────────────────┤
│ • Global Variables           8% │
│ • Stack (Tasks)              2% │
│ • Heap (Malloc)              2% │
│ ★ Phase 4 Add: Buffers      +0.5% │
│ • Free Space                87.5% │
│                                │
│ 推定最終: RAM 12.5% / 328KB    │
└──────────────────────────────────┘

┌──────────────────────────────────┐
│   EEPROM (4KB) [相対アドレス0]   │
├──────────────────────────────────┤
│ Offset 0x00-0x0B               │
│  • HI_ALARM_TEMP   (4B)        │
│  • LO_ALARM_TEMP   (4B)        │
│  • Checksum        (1B)        │
│                                │
│ Offset 0x0C-0xFFF              │
│  • Free Space      (4083B)     │
│  ★ Phase 4では未使用           │
│    （将来: ログヘッダ等）       │
└──────────────────────────────────┘

結論: メモリに十分な余裕あり
```

---

## Ⅸ ビルド依存関係

### platformio.ini への追加

```ini
[env:m5stack]
platform = espressif32
board = m5stack-core-esp32
framework = arduino

lib_deps =
  m5stack/M5Stack@^0.4.5
  adafruit/Adafruit MAX31855 library@^1.1.0
  adafruit/RTClib@^2.1.1          ← ★ Phase 4 新規追加

upload_speed = 921600
```

### Include Graph

```
main.cpp
├─ Global.h
│  ├─ M5Stack.h (M5フレームワーク)
│  ├─ Adafruit_MAX31855.h (MAX31855センサ)
│  ├─ EEPROMManager.h （Phase 3）
│  └─ cmath, cfloat （標準ライブラリ）
│
├─ Tasks.cpp
│  ├─ Global.h (上記)
│  ├─ SDManager.h （★ Phase 4 新規）
│  │  ├─ FS.h (ファイルシステム)
│  │  ├─ SD.h (SD カード)
│  │  └─ RTClib.h （★ RTClib ライブラリ）
│  └─ （その他既存分）
```

---

## Ⅹ テスト可能性設計

### ユニットテスト対象

SDManager に対するテストケース例：
```cpp
# test/test_sdmanager.cpp

void test_createNewFile() {
  // ファイル作成成功テスト
  ASSERT_TRUE(SDManager::createNewFile("TEST.csv"));
}

void test_writeHeader() {
  // CSV ヘッダ書き込みテスト
  ASSERT_TRUE(SDManager::writeHeader());
}

void test_writeData() {
  // データ行書き込みテスト
  SDData data = {0, 540.0, "RUN", 1, 540.0, 0.0, 540.0, 540.0, false, false};
  ASSERT_TRUE(SDManager::writeData(data));
}

void test_closeFile() {
  // ファイルクローズテスト
  ASSERT_TRUE(SDManager::closeFile());
}
```

### 統合テスト項目

1. **ハードウェア統合**:
   - RTC 時刻取得 → シリアル出力確認
   - SD ファイルシステム初期化 → PC で確認
   
2. **エンドツーエンド**:
   - RUN 開始 → ファイル作成 → データ記録 → RESULT → ファイルクローズ
   - CSV ファイルを PC で開いて、データ形式確認

3. **エラーハンドリング**:
   - SD 未検出時の動作
   - 書き込み失敗時のリトライ
   - 領域不足時のエラー通知

---

**設計者**: GitHub Copilot  
**設計日**: 2026年2月26日
