# 繝代ヵ繧ｩ繝ｼ繝槭Φ繧ｹ貂ｬ螳壹ぎ繧､繝会ｼ・ask 10・・

**蟇ｾ雎｡**: M5Stack Core (ESP32)  
**逶ｮ逧・*: 繧ｷ繧ｹ繝・Β蜈ｨ菴薙・蜻ｨ譛溽音諤ｧ縺ｨCPU雋闕ｷ繧貞ｮ滓ｸｬ  
**螳溯｡檎腸蠅・*: 繧ｷ繝ｪ繧｢繝ｫ繝｢繝九ち + 繧ｿ繧､繝繧ｹ繧ｿ繝ｳ繝苓ｧ｣譫・ 
**謗ｨ螳壽園隕∵凾髢・*: 1 譎る俣・域ｸｬ螳・+ 隗｣譫撰ｼ・ 

---

## 識 貂ｬ螳夂岼讓・

繧ｷ繧ｹ繝・Β險ｭ險域凾縺ｮ逶ｮ讓吝捉譛溘ｒ驕疲・縺励※縺・ｋ縺狗｢ｺ隱搾ｼ・

| 繧ｿ繧ｹ繧ｯ | 險ｭ險亥捉譛・| 逶ｮ讓吶Ξ繧､繝・Φ繧ｷ | 險ｱ螳ｹ蟷・|
|-------|--------|------------|------|
| **IO_Task** | 10 ms | < 5 ms | 竓・0% |
| **Logic_Task** | 50 ms | < 30 ms | 竓・0% |
| **UI_Task** | 200 ms | < 100 ms | 竓・0% |
| **Main Loop Total** | 10 ms | < 10 ms | 竓・0% |

**蜿り・*: [CODE_EXPLANATION.md](./../code/CODE_EXPLANATION.md) 竊・Section 10.3 繧ｿ繧ｹ繧ｯ蜻ｨ譛溘ち繧､繝繝ｩ繧､繝ｳ

---

## 投 繝・・繧ｿ蜿朱寔譁ｹ豕・

### **譁ｹ豕・1: 繧ｷ繝ｪ繧｢繝ｫ繧ｿ繧､繝繧ｹ繧ｿ繝ｳ繝苓ｧ｣譫撰ｼ域耳螂ｨ・・*

#### Step 1: 螳溯｣・さ繝ｼ繝牙､画峩・井ｸ譎ら噪・・

繝輔ぃ繧､繝ｫ: [`src/Tasks.cpp`](../src/Tasks.cpp)  
髢｢謨ｰ: `IO_Task()`, `Logic_Task()`, `UI_Task()`

蜷・ち繧ｹ繧ｯ髢句ｧ区凾縺ｫ**繝槭う繧ｯ繝ｭ遘堤ｲｾ蠎ｦ縺ｮ繧ｿ繧､繝繧ｹ繧ｿ繝ｳ繝・*繧偵す繝ｪ繧｢繝ｫ蜃ｺ蜉・

```cpp
/**縲慎ask 10 諤ｧ閭ｽ貂ｬ螳壹さ繝ｼ繝峨鯛ｻ荳譎ら噪縺ｪ繧､繝ｳ繧ｹ繝医Ν繝｡繝ｳ繝・・繧ｷ繝ｧ繝ｳ*/

// 繝輔ぃ繧､繝ｫ蜀帝ｭ縺ｫ霑ｽ蜉
static unsigned long task_start_time = 0;

// IO_Task() 蜀・・貂ｬ貂ｩ逶ｴ蠕後↓謖ｿ蜈･
void IO_Task() {
  task_start_time = micros();  // 繝槭う繧ｯ繝ｭ遘偵〒險倬鹸
  
  // 譌｢蟄倥・貂ｩ蠎ｦ隱ｭ縺ｿ霎ｼ縺ｿ繧ｳ繝ｼ繝・..
  float currentTemp = readTemperature();
  
  unsigned long task_duration = micros() - task_start_time;
  Serial.printf("[PERF_IO] %lu us\n", task_duration);  // 繝槭う繧ｯ繝ｭ遘貞腰菴阪〒蜃ｺ蜉・
}

// Logic_Task() 縺ｧ繧ょ酔讒・
void Logic_Task() {
  task_start_time = micros();
  
  // 譌｢蟄倥Ο繧ｸ繝・け...
  updateAlarmFlags(...);
  
  unsigned long task_duration = micros() - task_start_time;
  Serial.printf("[PERF_LOGIC] %lu us\n", task_duration);
}

// UI_Task() 縺ｧ繧ょ酔讒・
void UI_Task() {
  task_start_time = micros();  
  
  // 譌｢蟄倥・逕ｻ髱｢譖ｴ譁ｰ...
  updateDisplay();
  
  unsigned long task_duration = micros() - task_start_time;
  Serial.printf("[PERF_UI] %lu us\n", task_duration);
}

// main() 繝ｫ繝ｼ繝玲ｧ閭ｽ繧よｸｬ螳夲ｼ医が繝励す繝ｧ繝ｳ・・
 unsigned long loop_start = micros();
 // ... all 3 tasks ...
 unsigned long loop_duration = micros() - loop_start;
 Serial.printf("[PERF_LOOP] %lu us\n", loop_duration);
```

#### Step 2: 繝輔ぃ繝ｼ繝繧ｦ繧ｧ繧｢繝薙Ν繝・& 繧｢繝・・繝ｭ繝ｼ繝・

```bash
platformio run -e m5stack --target upload
```

#### Step 3: 繧ｷ繝ｪ繧｢繝ｫ繝｢繝九ち蜃ｺ蜉帙く繝｣繝励メ繝｣

蜃ｺ蜉帑ｾ・
```
[PERF_IO] 3200 us
[PERF_LOGIC] 18540 us
[PERF_UI] 45300 us
[PERF_LOOP] 67890 us
[PERF_IO] 3150 us
[PERF_LOGIC] 18600 us
[PERF_UI] 45200 us
[PERF_LOOP] 68000 us
...・・00蝗樔ｻ･荳翫く繝｣繝励メ繝｣・・
```

**繧ｭ繝｣繝励メ繝｣譁ｹ豕・*:
```bash
# 繧ｹ繝医Μ繝ｼ繝蜃ｺ蜉帙ｒ20遘帝俣繝輔ぃ繧､繝ｫ縺ｫ菫晏ｭ・
Ctrl+C 縺ｧ蛛懈ｭ｢蠕後∽ｻ･荳九〒遒ｺ隱・
C:\.platformio\penv\Scripts\platformio.exe device monitor -p COM3 -b 115200 > perf_log.txt
# 20遘貞ｾ・ｩ・
# Ctrl+C 縺ｧ邨ゆｺ・
```

---

### **譁ｹ豕・2: Python 繧ｹ繧ｯ繝ｪ繝励ヨ縺ｧ縺ｮ閾ｪ蜍戊ｧ｣譫・*

#### 繝輔ぃ繧､繝ｫ: `scripts/analyze_performance.py`

```python
#!/usr/bin/env python3
"""
繝代ヵ繧ｩ繝ｼ繝槭Φ繧ｹ繝ｭ繧ｰ隗｣譫舌せ繧ｯ繝ｪ繝励ヨ
菴ｿ逕ｨ豕・ python analyze_performance.py perf_log.txt
"""

import re
import sys
from statistics import mean, stdev, median

def parse_log(filename):
    """繧ｷ繝ｪ繧｢繝ｫ繝ｭ繧ｰ縺九ｉ [PERF_*] 陦後ｒ謚ｽ蜃ｺ"""
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
                    results['io'].append(int(match.group(1)) / 1000)  # ﾎｼs 竊・ms
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
    """邨ｱ險郁ｨ育ｮ・""
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
    """繝ｬ繝昴・繝亥・蜉・""
    print("=" * 70)
    print("繝代ヵ繧ｩ繝ｼ繝槭Φ繧ｹ貂ｬ螳夂ｵ先棡 (蜊倅ｽ・ ms)")
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
            print(f"\n縲須name}縲・)
            print(f"  繧ｵ繝ｳ繝励Ν謨ｰ: {s['count']}")
            print(f"  蟷ｳ蝮・      {s['mean']:.3f} ms (逶ｮ讓・ < {target} ms)")
            print(f"  荳ｭ螟ｮ蛟､:    {s['median']:.3f} ms")
            print(f"  譛蟆丞､:    {s['min']:.3f} ms")
            print(f"  譛螟ｧ蛟､:    {s['max']:.3f} ms (髯千阜: {limit} ms)")
            print(f"  讓呎ｺ門￥蟾ｮ:  {s['stdev']:.3f} ms")
            
            # 蛻､螳・
            if s['max'] > limit:
                print(f"  笞・・WARNING: 譛螟ｧ蛟､縺悟宛髯・{limit}ms)繧定ｶ・℃")
            elif s['mean'] > target:
                print(f"  笞・・CAUTION: 蟷ｳ蝮・′逶ｮ讓・{target}ms)繧定ｶ・℃")
            else:
                print(f"  笨・PASS: 逶ｮ讓咎＃謌・)
        else:
            print(f"\n縲須name}縲・繝・・繧ｿ縺ｪ縺・)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("菴ｿ逕ｨ豕・ python analyze_performance.py <logfile>")
        sys.exit(1)
    
    logfile = sys.argv[1]
    results = parse_log(logfile)
    stats = analyze(results)
    print_report(stats)
```

**螳溯｡梧婿豕・*:
```bash
cd "c:\gemini\Simple Temperature Evaluation Tool"
python scripts/analyze_performance.py perf_log.txt
```

---

## 嶋 貂ｬ螳壼ｮ滓命謇矩・

### **貅門ｙ繝輔ぉ繝ｼ繧ｺ**

1. **螳牙ｮ夂ｨｼ蜒阪∪縺ｧ蠕・ｩ・*
   - 繝輔ぃ繝ｼ繝繧ｦ繧ｧ繧｢繧｢繝・・繝ｭ繝ｼ繝牙ｾ後・蛻・俣縺ｮ蠕・囑
   - ESP32 繧ｭ繝｣繝・す繝･縺ｨ蜻ｨ霎ｺ貂ｩ蠎ｦ縺悟ｮ牙ｮ壼喧

2. **蝓ｺ貅匁擅莉ｶ縺ｮ險ｭ螳・*
   - 迺ｰ蠅・ｸｩ蠎ｦ: 螳､貂ｩ (20ﾂｰC-25ﾂｰC)
   - 髮ｻ貅・ USB 繝輔Ν (5V 2A 莉･荳・
   - 騾壻ｿ｡: 豕｢迚ｹ邇・115200 bps (蝗ｺ螳・

### **貂ｬ螳壹ヵ繧ｧ繝ｼ繧ｺ**

```bash
# 繧ｹ繝・ャ繝・1: 諤ｧ閭ｽ險域ｸｬ繧ｳ繝ｼ繝・繧・main() 縺ｫ邨・∩霎ｼ縺ｿ (荳願ｨ伜盾辣ｧ)

# 繧ｹ繝・ャ繝・2: 繝薙Ν繝・& 繧｢繝・・繝ｭ繝ｼ繝・
pio run -e m5stack --target upload

# 繧ｹ繝・ャ繝・3: 繧ｷ繝ｪ繧｢繝ｫ繝｢繝九ち縺ｧ 30遘抵ｽ・0遘偵く繝｣繝励メ繝｣
pio device monitor -p COM3 -b 115200

# 繧ｹ繝・ャ繝・4: 繝ｭ繧ｰ繝輔ぃ繧､繝ｫ縺ｫ菫晏ｭ・
# ・・ontrol + C 縺ｧ蜃ｺ蜉帙ｒ蛛懈ｭ｢縺励∝・譫千畑繝・・繝ｫ縺ｫ蜈･蜉幢ｼ・
```

### **蛻・梵繝輔ぉ繝ｼ繧ｺ**

```bash
# Python 繧ｹ繧ｯ繝ｪ繝励ヨ縺ｧ閾ｪ蜍戊ｧ｣譫・
python scripts/analyze_performance.py perf_log.txt

# 縺ｾ縺溘・謇句虚蛻・梵・・xcel/Google Sheets 縺ｸ繧ｨ繧ｯ繧ｹ繝昴・繝茨ｼ・
# 繧ｽ繝ｼ繝・竊・邨ｱ險磯未謨ｰ (AVERAGE, MAX, STDEV_S)
```

---

## 投 譛溷ｾ・＆繧後ｋ貂ｬ螳夂ｵ先棡

### **豁｣蟶ｸ蜍穂ｽ懈凾縺ｮ螳滓ｸｬ蛟､**

| Task | 逶ｮ讓・| 譛溷ｾ・ｮ滓ｸｬ | 螳滓ｸｬ邨先棡 |
|------|------|---------|--------|
| **IO_Task** | < 5ms | 3.2-4.5ms | _____ |
| **Logic_Task** | < 30ms | 18.5-27.3ms | _____ |
| **UI_Task** | < 100ms | 45.2-95.8ms | _____ |
| **Main Loop** | < 10ms | 9.5-10.8ms | _____ |

### **螳滓ｸｬ縺檎岼讓吶ｒ雜・℃縺吶ｋ蝣ｴ蜷・*

| 蜴溷屏 | 蜈・・| 蟇ｾ蜃ｦ譁ｹ豕・|
|-----|------|--------|
| **繧ｻ繝ｳ繧ｵ隱ｭ縺ｿ霎ｼ縺ｿ驕・ｻｶ** | IO_Task > 5ms | MAX31855 騾壻ｿ｡繧ｭ繝｣繝・す繝･讀懆ｨ・|
| **險育ｮ苓ｲ闕ｷ** | Logic_Task > 30ms | Welford豕輔・繧ｭ繝｣繝・す繝･譛驕ｩ蛹・|
| **逕ｻ髱｢譖ｴ譁ｰ** | UI_Task > 100ms | M5Stack LCD 繝峨Λ繧､繝宣≦蟒ｶ遒ｺ隱・|
| **WiFi 蟷ｲ貂・* | Loop 繧ｸ繝・ち繝ｼ > 2ms | 繧ｹ繝ｪ繝ｼ繝苓ｨｭ螳壹・繝√Ε繝阪Ν螟画峩 |

隧ｳ邏ｰ: [TROUBLESHOOTING.md](./../troubleshooting/TROUBLESHOOTING.md) "逞・憾8: 繝懊ち繝ｳ蜿榊ｿ懊′驕・＞"

---

## 溌 隧ｳ邏ｰ貂ｬ螳夲ｼ医が繝励す繝ｧ繝ｳ・・

### **CPU Utilization・・PIFFS繧｢繧ｯ繧ｻ繧ｹ譎ゑｼ・*

```cpp
// EEPROM 隱ｭ縺ｿ譖ｸ縺肴凾縺ｮ諤ｧ閭ｽ
static unsigned long eeprom_access_time = 0;

void readSettings() {
  eeprom_access_time = micros();
  // ... EEPROM隱ｭ縺ｿ霎ｼ縺ｿ ...
  Serial.printf("[PERF_EEPROM_READ] %lu us\n", 
                 micros() - eeprom_access_time);
}

void writeSettings() {
  eeprom_access_time = micros();
  // ... EEPROM譖ｸ縺崎ｾｼ縺ｿ ...
  Serial.printf("[PERF_EEPROM_WRITE] %lu us\n", 
                 micros() - eeprom_access_time);
}
```

**譛溷ｾ・､**:
- EEPROM 隱ｭ縺ｿ霎ｼ縺ｿ: 100-500 ﾎｼs
- EEPROM 譖ｸ縺崎ｾｼ縺ｿ: 50-100 ms ・医ヵ繝ｩ繝・す繝･遒ｺ螳壼性繧・・

### **繝｡繝｢繝ｪ菴ｿ逕ｨ驥・*

```cpp
// free heap 縺ｮ霑ｽ霍｡
void debugMemory() {
  Serial.printf("[PERF_HEAP] Free: %d bytes\n", 
                 ESP.getFreeHeap());
}
```

**譛溷ｾ・､**:
- Free Heap: > 50 KB ・・5Stack Core 譛蟆擾ｼ・

---

## 搭 貂ｬ螳夂ｵ先棡繝・Φ繝励Ξ繝ｼ繝・

**貂ｬ螳壽律譎・*: ________________  
**繝輔ぃ繝ｼ繝繧ｦ繧ｧ繧｢迚・*: Phase 3 Refactored  
**迺ｰ蠅・ｸｩ蠎ｦ**: ____ﾂｰC縺励※  
**髮ｻ貅・*: USB / Battery (___V)  

### 貂ｬ螳夂ｵ先棡

```
縲蝕O_Task縲・
 蟷ｳ蝮・ ____ms, 譛蟆・ ____ms, 譛螟ｧ: ____ms, ﾏ・ ____ms
 蛻､螳・ 笨・PASS / 笞・・CAUTION / 笶・FAIL

縲伸ogic_Task縲・
 蟷ｳ蝮・ ____ms, 譛蟆・ ____ms, 譛螟ｧ: ____ms, ﾏ・ ____ms
 蛻､螳・ 笨・PASS / 笞・・CAUTION / 笶悟､ｱ謨・FAIL

縲振I_Task縲・
 蟷ｳ蝮・ ____ms, 譛蟆・ ____ms, 譛螟ｧ: ____ms, ﾏ・ ____ms
 蛻､螳・ 笨・PASS / 笞・・CAUTION / 笶・FAIL

縲信ain Loop縲・
 蟷ｳ蝮・ ____ms, 譛蟆・ ____ms, 譛螟ｧ: ____ms, ﾏ・ ____ms
 蛻､螳・ 笨・PASS / 笞・・CAUTION / 笶・FAIL

縲食eap Memory縲・
 譛蟆・Free: ____KB
 蛻､螳・ 笨・OK / 笞・・LOW
```

### 險ｻ險・

蜴溷屏蛻・梵繝ｻ謾ｹ蝟・署譯・

---

## 売 蜿榊ｾｩ謾ｹ蝟・ヵ繝ｭ繝ｼ

```
貂ｬ螳・竊・蛻・梵 竊・蛻､螳・
  竊・     竊・     竊・
 螳滓命   邨ｱ險・ PASS/FAIL
  竊・     竊・     竊・
  笏披楳笏笏笏笏竊・譛驕ｩ蛹匁署譯・
         (蠢・ｦ√↓蠢懊§縺ｦ)
```

**繧ゅ＠謾ｹ蝟・′蠢・ｦ√↑蝣ｴ蜷・*:
1. [TROUBLESHOOTING.md](./../troubleshooting/TROUBLESHOOTING.md) 縺ｧ蜴溷屏險ｺ譁ｭ
2. 繧ｳ繝ｼ繝画怙驕ｩ蛹・(萓・ 繧ｭ繝｣繝・す繝ｳ繧ｰ, 驕・ｻｶ險育ｮ・
3. 蜀肴ｸｬ螳・竊・蜉ｹ譫懃｢ｺ隱・

---

## 統 髢｢騾｣繝峨く繝･繝｡繝ｳ繝・

- **繧ｷ繧ｹ繝・Β繧｢繝ｼ繧ｭ繝・け繝√Ε**: [CODE_EXPLANATION.md](./../code/CODE_EXPLANATION.md) (Section 10 蜈ｨ驛ｨ)
- **繧ｿ繧ｹ繧ｯ蜻ｨ譛溯ｨｭ險・*: [CODE_EXPLANATION.md](./../code/CODE_EXPLANATION.md) (Section 10.3 繧ｿ繧ｹ繧ｯ蜻ｨ譛溘ち繧､繝繝ｩ繧､繝ｳ)
- **繝・・繝ｫ繝√ぉ繝ｼ繝ｳ**: [pio.ini](../platformio.ini)  
- **繝ｭ繧ｮ繝ｳ繧ｰ螳溯｣・*: [Tasks.cpp](../src/Tasks.cpp#L1)

---

## 笨・讀懆ｨｼ繝√ぉ繝・け繝ｪ繧ｹ繝・

繝代ヵ繧ｩ繝ｼ繝槭Φ繧ｹ貂ｬ螳壹ｒ螳御ｺ・＠縺溘ｉ:

- [ ] 繝ｭ繧ｰ繝輔ぃ繧､繝ｫ 繧・繧ｭ繝｣繝励メ繝｣・・0遘剃ｻ･荳奇ｼ・
- [ ] Python 繧ｹ繧ｯ繝ｪ繝励ヨ縺ｧ邨ｱ險郁ｨ育ｮ・
- [ ] 繝ｬ繝昴・繝域紛逅・ｼ医ユ繝ｳ繝励Ξ繝ｼ繝井ｽｿ逕ｨ・・
- [ ] 蜈ｨ繧ｿ繧ｹ繧ｯ縲娯怛 逶ｮ讓咎＃謌舌咲｢ｺ隱・
- [ ] 邨先棡繧・docs/ 縺ｫ菫晏ｭ・
  - `PERFORMANCE_RESULT_2025-01-17.txt`
  - `perf_log_2025-01-17.txt`

---

**險倩ｿｰ譌･**: 2025-01-17 (Phase 3 refactoring)  
**谺｡迚・*: 螳滓ｸｬ蠕後∝ｮ滄圀縺ｮ謨ｰ蛟､縺ｨ蟇ｾ遲悶ｒ霑ｽ險・


