# Session 4 繧ｯ繧､繝・け繧ｹ繧ｿ繝ｼ繝医ぎ繧､繝・

**蜑榊屓螳御ｺ・*: Task 1-2 (Magic Number螳壽焚蛹・+ 繝懊ち繝ｳ蜃ｦ逅・未謨ｰ蛹・  
**莉雁屓莠亥ｮ・*: Task 3-5 (UI謠冗判蛻・牡 竊・EEPROM迢ｬ遶句喧 竊・繝峨く繝･繝｡繝ｳ繝亥・螳・  

---

## 笞｡ 30遘偵〒迥ｶ諷区滑謠｡

### **繝薙Ν繝臥｢ｺ隱・*
```bash
cd "c:\gemini\Simple Temperature Evaluation Tool"
C:\.platformio\penv\Scripts\platformio.exe run -e m5stack
# 譛溷ｾ・ SUCCESS (62遘・ | Flash 30.7% | RAM 7.0%
```

### **迴ｾ蝨ｨ縺ｮ騾ｲ謐・*
```
Stage 2-B: Code Quality Refactoring
笏懌楳笏 笨・Task 1: Magic Number螳壽焚蛹・(Global.h 40 陦瑚ｿｽ蜉)
笏懌楳笏 笨・Task 2: Button蜃ｦ逅・未謨ｰ蛹・(handleButtonA/B/C + 4髢｢謨ｰ)
笏懌楳笏 筐｡・・ Task 3: UI謠冗判蛻・牡 (renderXXX()髢｢謨ｰ謨ｴ逅・ 竊・谺｡縺薙％
笏懌楳笏 竢ｳ Task 4: EEPROM迢ｬ遶句喧 (繧､繝ｳ繧ｿ繝ｼ繝輔ぉ繝ｼ繧ｹ謨ｴ逅・
笏披楳笏 竢ｳ Task 5: 繝峨く繝･繝｡繝ｳ繝亥・螳・(JSDoc + 隧ｳ邏ｰ繧ｳ繝｡繝ｳ繝・
```

### **髢｢騾｣繝峨く繝･繝｡繝ｳ繝・*
- 祷 **隧ｳ邏ｰ螳溯｣・ぎ繧､繝・*: STAGE_4_REFACTORING_HANDOVER.md 竊・蠢・ｪｭ
- 搭 **繧ｳ繝ｼ繝牙盾閠・*: CODE_REFERENCE.md
- 識 **蜈ｨ菴楢ｨ育判**: detailed_spec_sw.md

---

## 識 Task 3 螳溯｣・し繝槭Μ繝ｼ・・蛻・〒逅・ｧ｣・・

### **菴輔ｒ縺吶ｋ縺・*
Tasks.cpp 縺ｮ 600+ 陦後・ UI_Task() 繧貞・蜑ｲ縺励※縲・**renderIDLE()**, **renderRUN()**, **renderRESULT()**, **renderALARM_SETTING()** 蛟句挨髢｢謨ｰ縺ｫ謨ｴ逅・

### **螳溯｣・・豬√ｌ**

```
Step 1: 迴ｾ蝨ｨ縺ｮ UI_Task() 繧ｳ繝ｼ繝我ｽ咲ｽｮ繧堤｢ｺ隱・
        笏披楳 Tasks.cpp 陦・380-600

Step 2: renderXXX() 髢｢謨ｰ繧貞句挨縺ｫ螳溯｣・
        笏懌楳 renderIDLE()          (50陦・
        笏懌楳 renderRUN()           (37陦・
        笏懌楳 renderRESULT()        (91陦・
        笏披楳 renderALARM_SETTING() (68陦・

Step 3: UI_Task() 繧堤ｰ｡貎斐↑繝・ぅ繧ｹ繝代ャ繝√↓謨ｴ逅・
        笏披楳 switch譁・〒蜷вenderXXX()繧貞他縺ｳ蜃ｺ縺・

Step 4: 繝薙Ν繝画､懆ｨｼ
        笏披楳 platformio run -e m5stack
```

### **譛溷ｾ・＆繧後ｋ蜉ｹ譫・*
| 鬆・岼 | 謾ｹ蝟・燕 | 謾ｹ蝟・ｾ・|
|------|--------|--------|
| UI_Task()縺ｮ陦梧焚 | 200+ | 15 |
| renderXXX()蟷ｳ蝮・｡梧焚 | - | 45 |
| 蜊倅ｸ雋ｬ莉ｻ蠎ｦ | 菴・| 箝絶ｭ絶ｭ・|
| 繝・せ繝亥ｮｹ譏捺ｧ | 菴・| 箝絶ｭ絶ｭ・|

---

## 剥 繧ｳ繝ｼ繝画歓蜃ｺ繧ｬ繧､繝・

### **遒ｺ隱阪☆繧九さ繝ｼ繝我ｽ咲ｽｮ**

**Tasks.cpp 縺九ｉ迴ｾ蝨ｨ縺ｮ renderXXX() 繧偵さ繝斐・:**

```cpp
// Line ~380
void renderIDLE() {
  // 50陦後⊇縺ｩ・唔DLE迥ｶ諷九・謠冗判蜃ｦ逅・
}

void renderRUN() {
  // 37陦後⊇縺ｩ・啌UN迥ｶ諷九・謠冗判蜃ｦ逅・
}

void renderRESULT() {
  // 91陦後⊇縺ｩ・啌ESULT迥ｶ諷九・謠冗判蜃ｦ逅・ｼ医・繝ｼ繧ｸ繝ｳ繧ｰ蟇ｾ蠢懶ｼ・
}

void renderALARM_SETTING() {
  // 68陦後⊇縺ｩ・哂LARM_SETTING迥ｶ諷九・謠冗判蜃ｦ逅・
}

void UI_Task() {
  // 荳ｻ蜃ｦ逅・↓縺ｾ縺ｨ繧√ｉ繧後※繧・
  switch (G.M_CurrentState) {
    case State::IDLE:
      // renderIDLE() 縺ｮ蜃ｦ逅・′蜀・Κ縺ｫ縺ゅｋ
      
    case State::RUN:
      // renderRUN() 縺ｮ蜃ｦ逅・′蜀・Κ縺ｫ縺ゅｋ
      
    case State::RESULT:
      // renderRESULT() 縺ｮ蜃ｦ逅・′蜀・Κ縺ｫ縺ゅｋ
      
    case State::ALARM_SETTING:
      // renderALARM_SETTING() 縺ｮ蜃ｦ逅・′蜀・Κ縺ｫ縺ゅｋ
  }
}
```

### **螟画峩蠕後・讒矩**

```cpp
// Tasks.h 縺ｫ螳｣險霑ｽ蜉
void renderIDLE();
void renderRUN();
void renderRESULT();
void renderALARM_SETTING();

// Tasks.cpp 縺ｧ縺ｯ髢｢謨ｰ蛻・牡
void renderIDLE() { ... }
void renderRUN() { ... }
void renderRESULT() { ... }
void renderALARM_SETTING() { ... }

void UI_Task() {
  static State prevState = State::IDLE;
  
  if (G.M_CurrentState != prevState) {
    M5.Lcd.fillScreen(BLACK);
    prevState = G.M_CurrentState;
  }
  
  // 遏ｭ縺上↑縺｣縺滂ｼ・
  switch (G.M_CurrentState) {
    case State::IDLE:           renderIDLE(); break;
    case State::RUN:            renderRun(); break;
    case State::RESULT:         renderRESULT(); break;
    case State::ALARM_SETTING:  renderALARM_SETTING(); break;
  }
}
```

---

## 笨・螳溯｣・メ繧ｧ繝・け繝ｪ繧ｹ繝・

### **繧ｹ繝・ャ繝・1: 貅門ｙ**
- [ ] VS Code 縺ｧ繝ｯ繝ｼ繧ｯ繧ｹ繝壹・繧ｹ髢九￥
- [ ] Tasks.cpp 繧定ｪｭ縺ｿ繧・☆縺・憾諷九↓・域釜繧翫◆縺溘∩縺ｪ縺ｩ・・
- [ ] 谿ｵ蜿悶ｊ蟶ｳ・医％縺ｮ繝峨く繝･繝｡繝ｳ繝茨ｼ峨ｒ蜿ら・蜿ｯ閭ｽ縺ｫ

### **繧ｹ繝・ャ繝・2: 繧ｳ繝ｼ繝牙・蜑ｲ**
- [ ] `renderIDLE()` 繧・Tasks.cpp 縺ｫ謚ｽ蜃ｺ
- [ ] `renderRUN()` 繧・Tasks.cpp 縺ｫ謚ｽ蜃ｺ  
- [ ] `renderRESULT()` 繧・Tasks.cpp 縺ｫ謚ｽ蜃ｺ
- [ ] `renderALARM_SETTING()` 繧・Tasks.cpp 縺ｫ謚ｽ蜃ｺ

### **繧ｹ繝・ャ繝・3: UI_Task()繝ｪ繝輔ぃ繧ｯ繧ｿ繝ｪ繝ｳ繧ｰ**
- [ ] UI_Task() 縺ｮ譛ｬ菴薙ｒ遏ｭ縺上∪縺ｨ繧√ｋ
- [ ] 蜷・renderXXX() 縺ｮ蜻ｼ縺ｳ蜃ｺ縺励ｒ switch 譁・↓
- [ ] 繧ｳ繝｡繝ｳ繝域紛逅・

### **繧ｹ繝・ャ繝・4: Tasks.h 譖ｴ譁ｰ**
- [ ] 髢｢謨ｰ螳｣險繧定ｿｽ蜉
  ```cpp
  void renderIDLE();
  void renderRUN();
  void renderRESULT();
  void renderALARM_SETTING();
  ```

### **繧ｹ繝・ャ繝・5: 讀懆ｨｼ**
- [ ] 繝薙Ν繝画・蜉・`platformio run -e m5stack`
- [ ] 隴ｦ蜻翫↑縺・
- [ ] 繝｡繝｢繝ｪ菴ｿ逕ｨ邇・メ繧ｧ繝・け・・lash < 40%, RAM < 10%・・

### **繧ｹ繝・ャ繝・6: 繧ｳ繝溘ャ繝茨ｼ・it・・*
```bash
git add src/Tasks.cpp src/Tasks.h
git commit -m "refactor: UI_Task decomposed into renderXXX() functions (Task 3)"
```

---

## 菅 繧医￥縺ゅｋ繝医Λ繝悶Ν

### **蝠城｡・1: 縲蛍ndeclared identifier 'renderIDLE'縲・*
**蜴溷屏**: Tasks.h 縺ｫ髢｢謨ｰ螳｣險繧貞ｿ倥ｌ縺・ 
**隗｣豎ｺ**: Tasks.h 縺ｫ莉･荳玖ｶｳ縺・
```cpp
void renderIDLE();
void renderRUN();
void renderRESULT();
void renderALARM_SETTING();
```

### **蝠城｡・2: 縲憩xpected ';' before '}'縲・*
**蜴溷屏**: 髢｢謨ｰ螳夂ｾｩ縺ｮ諡ｬ蠑ｧ縺悟粋縺｣縺ｦ縺・↑縺・ 
**隗｣豎ｺ**: 蜷・renderXXX() 縺ｮ譛ｫ蟆ｾ縺ｫ `}` 縺後≠繧九°遒ｺ隱・

### **蝠城｡・3: 繝薙Ν繝牙､ｱ謨励′邯壹￥**
**蟇ｾ蠢・**
```bash
# 繧ｯ繝ｪ繝ｼ繝ｳ繝薙Ν繝・
C:\.platformio\penv\Scripts\platformio.exe run --target clean -e m5stack

# 蜀榊ｺｦ繝薙Ν繝・
C:\.platformio\penv\Scripts\platformio.exe run -e m5stack
```

---

## 答 隧ｳ邏ｰ蜿ら・

- 迫 **Task 3 隧ｳ邏ｰ**: [STAGE_4_REFACTORING_HANDOVER.md](./../guides/STAGE_4_REFACTORING_HANDOVER.md)
- 迫 **繧ｳ繝ｼ繝我ｽ咲ｽｮ蜿ら・**: [CODE_REFERENCE.md](./../code/CODE_REFERENCE.md)
- 迫 **蜈ｨ菴薙・繝ｭ繧ｸ繧ｧ繧ｯ繝域ｦりｦ・*: [detailed_spec_sw.md](./../specs/detailed_spec_sw.md)

---

## 竢ｱ・・莠域Φ譎る俣

- **逅・ｧ｣繝ｻ遒ｺ隱・*: 5 蛻・
- **繧ｳ繝ｼ繝牙・蜑ｲ**: 20 蛻・
- **繝薙Ν繝峨・讀懆ｨｼ**: 10 蛻・
- **繧ｳ繝溘ャ繝・*: 2 蛻・

**蜷郁ｨ・*: 邏・**40-50 蛻・*

---

**髢句ｧ区ｺ門ｙ螳御ｺ・ｼゝask 3 螳溯｣・↓騾ｲ繧薙〒縺上□縺輔＞縲・* 噫


