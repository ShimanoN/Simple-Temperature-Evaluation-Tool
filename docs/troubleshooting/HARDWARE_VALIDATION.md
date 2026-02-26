# ハードウェア検証ガイド（Task 9）

**対象**: M5Stack Core (ESP32)  
**目標**: 完成された全機能の動作確認  
**実施内容**: シリアルモニタ + 実際の温度計・アラーム発火・UI動作確認  
**所要時間**: 1.5 時間  

---

## Ⅰ 準備チェック

### 1. ファームウェアの書き込み

```bash
# ビルド＆アップロード
platformio run -e m5stack --target upload

# または VS Code タスク
PlatformIO Upload (m5stack)
```

### 2. シリアルモニタの起動

```bash
# Windows PowerShell
C:\.platformio\penv\Scripts\platformio.exe device monitor -p COM3 -b 115200

# または VS Code PlatformIO Icon → Device → Monitor
```

**期待される出力ログ**:
```
[Setup] Initializing EEPROM...
[Setup] Reading stored settings...
[Setup] Alarm flags reset before entering main loop
[IO] Temperature: 25.3℃
[Logic] Average: 25.30℃, Variance: 0.00
[UI] Display refresh
```

---

## Ⅱ 検証チェックリスト

### **チェック 1：起動シーケンス**

**実施手順**:
1. M5Stack を電源供給（USB または バッテリー）
2. シリアルモニタを確認

**期待結果**:
- [ ] [Setup] ログが表示
- [ ] EEPROM 初期化メッセージ
- [ ] 設定値の読み込み確認
- [ ] メインループ開始待機状態

**失敗時対応**: [TROUBLESHOOTING.md](./../troubleshooting/TROUBLESHOOTING.md) で「セットアップ失敗」参照

---

### **チェック 2：温度センサー読み込み**

**実施手順**:
1. シリアルモニタで [IO] Temperature な行を確認
2. 温度値が 20℃～ 30℃ の範囲か確認
3. 30 秒間、変動がないか（安定性）確認

**期待結果**:
- [ ] 出力：`[IO] Temperature: XX.X℃`
- [ ] 値が有理的な範囲（0℃～ 60℃）
- [ ] 小数第一位まで表示
- [ ] 約 100ms ごとに更新

**チェック方法**:
```
温度計 or 別のセンサーで実測値を比較
誤差範囲：±1℃ 以内が目安
```

**失敗時対応**: 
- センサー接続確認：[detailed_spec_hw.md](./../specs/detailed_spec_hw.md) の接続図
- MAX31855 通信確認：SPI ライン確認

---

### **チェック 3：統計処理（Welford アルゴリズム）**

**実施手順**:
1. 温度が同じ値を 5 分以上保つ（または固定値セット）
2. シリアルモニタで Average と Variance を確認

**期待結果**:
- [ ] `[Logic] Average: XX.XX℃`
- [ ] `[Logic] Variance: 0.00` or 小さい値
- [ ] 5 分後：分散が 0 に接近
- [ ] 統計値が更新される

**テスト環境例**:
- 室温で 5～10 分放置
- または温度をで 25℃ に調整

**失敗時対応**: [detailed_spec_sw.md](./../specs/detailed_spec_sw.md)「アルゴリズム」参照

---

### **チェック 4：アラーム機能**

**実施手順**:

#### ① 高温アラーム
1. EEPROM で `SETPOINT_HIGH` を 30℃ に設定
2. ドライヤーを使い温度を加熱
3. 30℃ に達したら、アラーム発火確認

```
アラーム発火時の期待ログ：
[Alarm] HIGH temperature detected!
[UI] Alarm LED flashing
```

**確認項目**:
- [ ] ログに Alarm メッセージ
- [ ] LED が点灯 or 点滅（LCDに表示）
- [ ] ブザー（ある場合）が鳴る

#### ② 低温アラーム
1. `SETPOINT_LOW` を 10℃ に設定
2. 冷蔵庫例で冷却
3. 10℃ 以下で発火確認

```
[Alarm] LOW temperature detected!
```

#### ③ ヒステリシス確認
- アラーム状態から除去
- 温度が設定値の 1℃ 内側に戻る確認
- アラームが解除

**失敗時対応**: [TROUBLESHOOTING.md](./../troubleshooting/TROUBLESHOOTING.md)「アラーム不動作」参照

---

### **チェック 5：UI・LCD 表示**

**実施手順**:
1. M5Stack 画面を確認
2. 以下情報が表示か確認

**期待表示内容**:
```
┌────────────────────┐
│ Temp Eval Tool v2 │
│ Current: 25.30℃   │
│ Avg: 25.30℃       │
│ Min/Max: 25/25℃   │
│ Alarm: OFF         │
│ [M5] [PRV] [NXT]   │
└────────────────────┘
```

**チェック項目**:
- [ ] 温度値（現在値）表示
- [ ] 平均値表示
- [ ] アラーム状態表示
- [ ] ボタンラベル表示（M5/PRV/NXT）
- [ ] 5 秒ごとに更新

**失敗時対応**: [TROUBLESHOOTING.md](./../troubleshooting/TROUBLESHOOTING.md)「画面表示異常」参照

---

### **チェック 6：ボタン操作**

**実施手順**:

#### ① M5 ボタン（左）
- 短押：メニュー or 設定画面へ
- 長押（3 秒以上）：リセット or 再起動

#### ② PRV ボタン（中央前面）
- 前の画面 or メニュー上移動

#### ③ NXT ボタン（右）
- 次の画面 or メニュー下移動

**期待動作**:
- [ ] ボタン押下時にログ出力：`[UI] Button pressed: M5`
- [ ] 画面が切り替わる
- [ ] 反応速度：100ms 以内

**失敗時対応**: [TROUBLESHOOTING.md](./../troubleshooting/TROUBLESHOOTING.md)「ボタン反応なし」参照

---

### **チェック 7：EEPROM 永続化**

**実施手順**:
1. 設定値を変更（例：`SETPOINT_HIGH` を 35℃ に）
2. M5Stack を再起動（電源OFF→ON）
3. 設定値が保存されているか確認

**期待結果**:
- [ ] 再起動後、設定値が復元
- [ ] ログに `[Setup] Reading stored settings...`
- [ ] 変更が消えていない

**確認方法**:
```
シリアルモニタで EEPROM 読み込みログ確認、
または画面で設定表示を確認
```

**失敗時対応**: [TROUBLESHOOTING.md](./../troubleshooting/TROUBLESHOOTING.md)「EEPROM 不安定」参照

---

### **チェック 8：全機能統合テスト**

**テストシナリオ**:
1. 室温で 5 分運用
2. ドライヤーで 30℃ まで加熱
3. 中断（アラーム発火）
4. 冷却して 20℃ に戻す
5. アラーム解除確認

**期待ログシーケンス**:
```
[IO] Temperature: 25.5℃
[Logic] Average: 25.50℃, Variance: 0.01
[IO] Temperature: 28.3℃
[IO] Temperature: 30.1℃
[Alarm] HIGH temperature detected!
[UI] Alarm state: ON
[IO] Temperature: 29.2℃
[IO] Temperature: 28.9℃
[IO] Temperature: 28.5℃
[Alarm] Alarm cleared
[UI] Alarm state: OFF
```

**チェック項目**:
- [ ] 全ログが時系列で出力
- [ ] アラーム発火・解除タイミング正確
- [ ] UI 表示が同期
- [ ] エラーログなし

---

## Ⅲ 検証完了チェックシート

| 項目 | 状態 | 実施日時 | 備考 |
|------|------|---------|------|
| 起動シーケンス | ✅/⚠️/❌ | __________ | |
| 温度センサー | ✅/⚠️/❌ | __________ | |
| 統計処理 | ✅/⚠️/❌ | __________ | |
| 高温アラーム | ✅/⚠️/❌ | __________ | |
| 低温アラーム | ✅/⚠️/❌ | __________ | |
| ヒステリシス | ✅/⚠️/❌ | __________ | |
| LCD 表示 | ✅/⚠️/❌ | __________ | |
| ボタン操作 | ✅/⚠️/❌ | __________ | |
| EEPROM 永続化 | ✅/⚠️/❌ | __________ | |
| 統合テスト | ✅/⚠️/❌ | __________ | |

---

## Ⅳ トラブルシューティング

問題が発生した場合は、以下を確認：

| 症状 | 原因の可能性 | 対応 |
|------|----------|------|
| シリアル出力なし | 接続 or ドライバ | [TROUBLESHOOTING.md](./../troubleshooting/TROUBLESHOOTING.md)「Issue 1」参照 |
| 温度値が異常 | センサー故障 or SPI エラー | [detailed_spec_hw.md](./../specs/detailed_spec_hw.md)「MAX31855」確認 |
| アラーム発火しない | ソフト設定 or エラーハンドリング | [CODE_EXPLANATION.md](./../code/CODE_EXPLANATION.md) Section 7 参照 |
| ボタン反応なし | GPIO 設定 or ハードウェア | [detailed_spec_hw.md](./../specs/detailed_spec_hw.md)「ボタン配線」確認 |

---

## Ⅴ 関連ドキュメント

- **ハードウェア仕様**: [detailed_spec_hw.md](./../specs/detailed_spec_hw.md)
- **ソフトウェア説明**: [CODE_EXPLANATION.md](./../code/CODE_EXPLANATION.md)
- **トラブルシューティング**: [TROUBLESHOOTING.md](./../troubleshooting/TROUBLESHOOTING.md)
- **パフォーマンス**: [PERFORMANCE.md](./../troubleshooting/PERFORMANCE.md)

---

**最終更新**: 2026年2月26日


