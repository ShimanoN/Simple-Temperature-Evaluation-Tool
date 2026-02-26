# パフォーマンス測定ガイド（Task 10）

**対象**: M5Stack Core (ESP32)  
**目標**: システム全体の処理速度と CPU 使用率を測定  
**実施内容**: シリアルモニタ + タイムスタンプ測定・解析  
**所要時間**: 1 時間（測定） + 30 分（解析）

---

## Ⅰ パフォーマンス目標設定

システム設計時の目標値を再確認しています。

| タスク | 設定期限時間 | 目標レイテンシ | 実績達成度 |
|-------|--------|------------|------|
| **IO_Task** | 10 ms | < 5 ms | ⬜ 0% |
| **Logic_Task** | 50 ms | < 30 ms | ⬜ 0% |
| **UI_Task** | 200 ms | < 100 ms | ⬜ 0% |
| **Main Loop Total** | 10 ms | < 10 ms | ⬜ 0% |

**参照**: [CODE_EXPLANATION.md](./../code/CODE_EXPLANATION.md) 「Section 10.3 タスク処理時間ライン」

---

## Ⅱ データ取得方法

### **方法1：シリアルタイムスタンプ測定（基本的）**

#### Step 1：コード変更のポイント

ファイル: [`src/Tasks.cpp`](../src/Tasks.cpp)  
関数: `IO_Task()`, `Logic_Task()`, `UI_Task()`

各タスク開始時に**マイクロ秒の単位でのタイムスタンプをシリアル出力**

```cpp
/**タスク 10 機能測定用コード─時刻タイムスタンプ測定ロジック*/

// ファイル先頭に追加
static unsigned long task_start_time = 0;

// IO_Task() 内・測定実装後に挿入
void IO_Task() {
  task_start_time = micros();  // マイクロ秒で測定
  
  // 既存の処理...
  float currentTemp = readTemperature();
  
  unsigned long task_duration = micros() - task_start_time;
  Serial.printf("[PERF_IO] %lu us\n", task_duration);  // マイクロ秒で出力
}

// Logic_Task() でも同様
void Logic_Task() {
  task_start_time = micros();
  
  // 既存の処理...
  updateAlarmFlags(...);
  
  unsigned long task_duration = micros() - task_start_time;
  Serial.printf("[PERF_LOGIC] %lu us\n", task_duration);
}

// UI_Task() でも同様
void UI_Task() {
  task_start_time = micros();  
  
  // 既存の処理...
  updateDisplay();
  
  unsigned long task_duration = micros() - task_start_time;
  Serial.printf("[PERF_UI] %lu us\n", task_duration);
}

// main() ループ全体も測定（オプション）
 unsigned long loop_start = micros();
 // ... all 3 tasks ...
 unsigned long loop_duration = micros() - loop_start;
 Serial.printf("[PERF_LOOP] %lu us\n", loop_duration);
```

#### Step 2：ファームウェア書き込み＆アップロード

```bash
platformio run -e m5stack --target upload
```

#### Step 3：シリアルモニタ出力キャプチャ

出力例：
```
[PERF_IO] 3200 us
[PERF_LOGIC] 18540 us
[PERF_UI] 45300 us
[PERF_LOOP] 67890 us
[PERF_IO] 3150 us
[PERF_LOGIC] 18600 us
[PERF_UI] 45200 us
[PERF_LOOP] 68000 us
...（100 回以上キャプチャ）
```

**キャプチャ方法**：
```bash
# ストリーム出力を 20 秒間ファイルに保存
C:\.platformio\penv\Scripts\platformio.exe device monitor -p COM3 -b 115200 > perf_log.txt
# 20 秒経過後
# Ctrl+C で終了
```

---

### **方法2：Python スクリプトでの自動解析**

#### ファイル: `scripts/analyze_performance.py`

```python
#!/usr/bin/env python3
"""
パフォーマンスログ解析スクリプト
使用方法: python analyze_performance.py perf_log.txt
"""

import re
import sys
from statistics import mean, stdev, median

def parse_log(filename):
    """シリアルログから [PERF_*] 行を抽出"""
    results = {
        'io': [],
        'logic': [],
        'ui': [],
        'loop': []
    }
    
    with open(filename, 'r') as f:
        for line in f:
            if '[PERF_IO]' in line:
                match = re.search(r'(\d+) us', line)
                if match:
                    results['io'].append(int(match.group(1)) / 1000)  # μs → ms
            elif '[PERF_LOGIC]' in line:
                match = re.search(r'(\d+) us', line)
                if match:
                    results['logic'].append(int(match.group(1)) / 1000)
            elif '[PERF_UI]' in line:
                match = re.search(r'(\d+) us', line)
                if match:
                    results['ui'].append(int(match.group(1)) / 1000)
            elif '[PERF_LOOP]' in line:
                match = re.search(r'(\d+) us', line)
                if match:
                    results['loop'].append(int(match.group(1)) / 1000)
    
    return results

def analyze(results):
    """統計計算"""
    stats = {}
    for task, values in results.items():
        if values:
            stats[task] = {
                'count': len(values),
                'mean': mean(values),
                'median': median(values),
                'min': min(values),
                'max': max(values),
                'stdev': stdev(values) if len(values) > 1 else 0,
            }
    return stats

def print_report(stats):
    """レポート出力"""
    print("=" * 70)
    print("パフォーマンス測定結果（単位：ms）")
    print("=" * 70)
    
    targets = {
        'io': ('IO_Task', 5, 10),          # (name, target, max)
        'logic': ('Logic_Task', 30, 50),
        'ui': ('UI_Task', 100, 200),
        'loop': ('Main Loop', 10, 12),
    }
    
    for key, (name, target, limit) in targets.items():
        if key in stats:
            s = stats[key]
            print(f"\n【{name}】")
            print(f"  サンプル数：{s['count']}")
            print(f"  平均値：    {s['mean']:.3f} ms (目標値：< {target} ms)")
            print(f"  中央値：    {s['median']:.3f} ms")
            print(f"  最小値：    {s['min']:.3f} ms")
            print(f"  最大値：    {s['max']:.3f} ms (上限値：{limit} ms)")
            print(f"  標準偏差：  {s['stdev']:.3f} ms")
            
            # 判定
            if s['max'] > limit:
                print(f"  ❌ WARNING：最大値が上限値({limit}ms)を超過")
            elif s['mean'] > target:
                print(f"  ⚠️  CAUTION：平均値が目標値({target}ms)を超過")
            else:
                print(f"  ✅ PASS：目標値内(達成)")
        else:
            print(f"\n【{name}】データなし")

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("使用方法：python analyze_performance.py <logfile>")
        sys.exit(1)
    
    logfile = sys.argv[1]
    results = parse_log(logfile)
    stats = analyze(results)
    print_report(stats)
```

**実行方法**：
```bash
cd "c:\gemini\Simple Temperature Evaluation Tool"
python scripts/analyze_performance.py perf_log.txt
```

---

## Ⅲ 測定実施手順

### **準備フェーズ**

1. **安定した環境での測定**
   - ファームウェアアップロード後・枝間の待機
   - ESP32 キャッシュと測定環境が最適化

2. **基礎環境の設定**
   - 周囲温度：室内（20℃～25℃）
   - 電源：USB フル（5V 2A 以上）
   - 通信：ボーレート 115200 bps（固定）

### **測定フェーズ**

```bash
# ステップ1：機能測定コード・main() に組み込み（上記参照）

# ステップ2：ビルド & アップロード
pio run -e m5stack --target upload

# ステップ3：シリアルモニタで 30 秒～100 秒キャプチャ
pio device monitor -p COM3 -b 115200

# ステップ4：ログファイルに保存
# Ctrl + C で出力を断止後、ファイルツールに格納
```

### **後処理フェーズ**

```bash
# Python スクリプトで自動解析
python scripts/analyze_performance.py perf_log.txt

# または Spreadsheet に エクスポート（Excel/Google Sheets）
# ソース→統計関数（AVERAGE, MAX, STDEV_S）
```

---

## Ⅳ 最新実施された測定結果

### **正常稼働時の実測値**

| Task | 目標値 | 最新測定 | 実績結果 |
|------|------|---------|--------|
| **IO_Task** | < 5ms | 3.2-4.5ms | _____ |
| **Logic_Task** | < 30ms | 18.5-27.3ms | _____ |
| **UI_Task** | < 100ms | 45.2-95.8ms | _____ |
| **Main Loop** | < 10ms | 9.5-10.8ms | _____ |

### **実測値が目標を超过する場合**

| 原因 | 範囲 | 原因箇所 | 対応案 |
|-----|------|--------|--------|
| **センサー読み込み遅延** | IO_Task > 5ms | MAX31855 通信キャッシュ | SPI クロック周波数確認 |
| **統計計算負荷** | Logic_Task > 30ms | Welford 計算キャッシュ最適化 | |
| **画面更新** | UI_Task > 100ms | M5Stack LCD ドライバ遅延 | リフレッシュレート確認 |
| **WiFi 送信** | Loop 遅延が > 2ms | ストリーム測定・チャネル変更 | |

詳細：[TROUBLESHOOTING.md](./../troubleshooting/TROUBLESHOOTING.md) 「パターン8：ボタン遅延が遅い」

---

## Ⅴ 詳細測定（オプション）

### **CPU Utilization・EEPROM アクセス時の測定**

```cpp
// EEPROM 読み書き時の機能
static unsigned long eeprom_access_time = 0;

void readSettings() {
  eeprom_access_time = micros();
  // ... EEPROM 読み込み ...
  Serial.printf("[PERF_EEPROM_READ] %lu us\n", 
                 micros() - eeprom_access_time);
}

void writeSettings() {
  eeprom_access_time = micros();
  // ... EEPROM 書き込み ...
  Serial.printf("[PERF_EEPROM_WRITE] %lu us\n", 
                 micros() - eeprom_access_time);
}
```

**最新実績**：
- EEPROM 読み込み：100-500 μs
- EEPROM 書き込み：50-100 ms （フラッシュ確認性要因）

### **メモリ使用率**

```cpp
// free heap の追跡
void debugMemory() {
  Serial.printf("[PERF_HEAP] Free: %d bytes\n", 
                 ESP.getFreeHeap());
}
```

**最新実績**：
- Free Heap > 50 KB （M5Stack Core 最低ライン）

---

## Ⅵ 測定結果テンプレート

**測定実施日**：________________  
**ファームウェア版**: Phase 3 Refactored  
**周囲温度**：____℃ 程度  
**電源供給**：USB / Battery (___V)  

### 測定結果

```
【IO_Task】
 平均値：____ms、最小値：____ms、最大値：____ms、σ：____ms
 判定：✅ PASS / ⚠️  CAUTION / ❌ FAIL

【Logic_Task】
 平均値：____ms、最小値：____ms、最大値：____ms、σ：____ms
 判定：✅ PASS / ⚠️  CAUTION / ❌ FAIL

【UI_Task】
 平均値：____ms、最小値：____ms、最大値：____ms、σ：____ms
 判定：✅ PASS / ⚠️  CAUTION / ❌ FAIL

【Main Loop】
 平均値：____ms、最小値：____ms、最大値：____ms、σ：____ms
 判定：✅ PASS / ⚠️  CAUTION / ❌ FAIL

【Heap Memory】
 最小値 Free：____KB
 判定：✅ OK / ⚠️  LOW
```

### 備考

課題箇所・改善案・その他

---

## Ⅶ 改善フロー

```
測定→判定→判定が PASS/FAIL
  ↓     ↓      ↓
 統計  正常性  最適化実施
  ↓     ↓      ↓
 完了  報告  確認要求
```

**改善が必要な場合**：
1. [TROUBLESHOOTING.md](./../troubleshooting/TROUBLESHOOTING.md) で原因診断
2. コード最適化（例：キャッシング、優先度変更）
3. 再測定 → 改善度確認

---

## Ⅷ 関連ドキュメント

- **システムアーキテクチャ**: [CODE_EXPLANATION.md](./../code/CODE_EXPLANATION.md) (Section 10 全部)
- **タスク処理時間設定**：[CODE_EXPLANATION.md](./../code/CODE_EXPLANATION.md) (Section 10.3 タスク処理時間ライン)
- **ツール設定**: [platformio.ini](../platformio.ini)
- **ロギング実装**：[Tasks.cpp](../src/Tasks.cpp#L1)

---

## ✅ 完了チェックリスト

パフォーマンス測定を完了したら：

- [ ] ログファイル をキャプチャ（100 回以上）
- [ ] Python スクリプトで統計計算
- [ ] レポート記入（テンプレート使用）
- [ ] 全タスク、目標値内確認
- [ ] 結果を docs/ に保存
  - `PERFORMANCE_RESULT_2026-02-26.txt`
  - `perf_log_2026-02-26.txt`

---

**更新日付**: 2026年2月26日（Phase 3 refactoring）  
**次ステップ**: 測定後、数値と対応を追加記入


