# CHANGELOG

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.0.0] - 2026-03-02

### 🎉 Release: Production Ready

**Status**: ✅ Phase 4 Integration Test PASS (全33テスト PASS)

#### Added

- **Core Measurement Features**
  - K型熱電対でのリアルタイム温度計測（MAX31855経由）
  - Welford法による統計計算（平均・標準偏差）
  - Max/Min値の自動追跡
  - 高精度計測（±1℃相当）

- **Data Storage**
  - CSV形式でのSD記録（自動ファイル名付け: DATA_0000.csv, DATA_0001.csv, ...）
  - 複数ラン対応（リセットなしでファイルカウンタ自動増加）
  - メモリ効率化（10サンプル毎の書き込み）
  - CRLF改行でExcel互換

- **Alarm System**
  - 高温/低温閾値設定（EEPROM保存）
  - リアルタイムアラーム検知・LCD表示
  - CSV記録（HI_ALARM, LO_ALARM列）

- **Hardware Integration**
  - EEPROM対応（アラーム値永続保存）
  - MicroSD カード対応（最大32GB推奨）
  - I2C/SPI通信安定化
  - マルチタスク実装（IO/Logic/UI周期管理）

- **User Interface**
  - 3状態UI（IDLE → RUN → RESULT）
  - リアルタイム計測中の画面更新（200ms周期）
  - SD状態表示（GREEN/RED）
  - 計測結果の即座表示

- **Error Handling**
  - SD初期化失敗時のフォールバック
  - MAX31855未応答時の復帰処理
  - EEPROM読込失敗時のデフォルト値使用
  - ハードウェア故障時でもファームウェア動作継続

- **Documentation**
  - [セットアップガイド](docs/guides/QUICKSTART_SESSION4.md) - 初回構築
  - [ハンドオーバーガイド](docs/guides/FINAL_HANDOVER_GUIDE.md) - 新人向け（4週間学習ロードマップ付き）
  - [実装仕様書](docs/code/IMPLEMENTATION_GUIDE.md) - 詳細アーキテクチャ・API
  - [ハードウェア仕様](docs/specs/detailed_spec_hw.md) - 配線・GPIO
  - [トラブルシューティング](docs/troubleshooting/TROUBLESHOOTING.md)
  - [統合テスト結果](docs/future/PHASE4_INTEGRATION_TEST_RESULTS.md)

#### Fixed

- **Phase 3 → Phase 4 Migration**
  - SD書き込みタイミング最適化（D_Count >= 2 → D_Count >= 10）
    - Welford計算の確実な完了待機 ✅
  - RUN中のAverage/StdDev計算
    - Logic_Task内で毎周期計算 → CSV出力前にリアルタイム値を反映 ✅
  - CSV形式の整合性確認完了 ✅

- **Known Issues Resolved**
  - Multi-run stability: 5ラン連続実行でCrash/Hang無し ✅
  - Memory leak: 長時間実行でメモリ増加無し ✅
  - Performance: パフォーマンス: Flash 31.0% / RAM 7.2% 以内 ✅

#### Performance

- **Build Time**: ≈ 13 seconds (PlatformIO optimized)
- **Upload Time**: ≈ 44 seconds (ESP32 UART)
- **Runtime Performance**:
  - IO_Task: 0.5ms avg / 2ms peak (10ms周期)
  - Logic_Task: 1ms avg / 3ms peak (50ms周期)
  - UI_Task: 5ms avg / 10ms peak (200ms周期)
  - Total footprint: 406KB Flash / 23KB RAM

#### Testing

**✅ Phase 4 Integration Test: ALL PASS (33/33)**

| カテゴリ | テスト数 | PASS | 進捗 |
|:---|:---:|:---:|:---|
| I: 初期化テスト | 4 | 4 | 100% ✅ |
| D: RUN状態テスト | 5 | 5 | 100% ✅ |
| C: RESULT テスト | 3 | 3 | 100% ✅ |
| U: UI 表示テスト | 5 | 5 | 100% ✅ |
| F: CSV フォーマット検証 | 7 | 7 | 100% ✅ |
| E: エラーハンドリング | 5 | 5 | 100% ✅ |
| P: パフォーマンス | 4 | 4 | 100% ✅ |

詳細: [PHASE4_INTEGRATION_TEST_RESULTS.md](docs/future/PHASE4_INTEGRATION_TEST_RESULTS.md)

#### Breaking Changes

- None (초版 릴리스)

#### Deprecated

- None

---

## [0.1.0-beta] - 2026-02-27

### Early Access / Phase 4 Development

- Phase 3 リファクタリング完了
- CSV書き込みバグ修正
- Welford法実装
- 統合テスト開始

---

## [0.0.1] - 2025-XX-XX

### Initial Development (Phase 1-2)

- Basic temperature measurement
- Simple UI with IDLE/RUN/RESULT states
- EEPROM integration (alarm thresholds)
- SD storage foundation

---

## 📋 Known Limitations (v1.0.0)

### 機能制限

| 項目 | 現状 | 理由 | 改善案 |
|:---|:---|:---|:---|
| **タイムスタンプ** | 相対時刻（経過秒）のみ | RTC統合コスト & 引き継ぎ時間制約 | DS3231 I2C RTC統合 (3～5日) |
| **クラウド連携** | なし | WiFi認証の複雑性 | AWS IoT Core / Azure IoT Hub (1週間) |
| **グラフ表示** | LCD非対応（CSV→Excel分析） | M5Stack画面の解像度制限 | M5StackCoreS3 TFT Card使用 (3日) |
| **複数センサ対応** | シングルチャネルのみ | ハード設計の再考が必要 | I2C Mux対応 (2日) |
| **電源** | USB-C給電のみ | バッテリーIC的な設計がない | リチウム電池+充電管理IC (5日) |

### 温度計測仕様

- **温度範囲**: -50～+150℃ (K型理論値、MAX31855仕様)
- **精度**: ±1℃相当 (MAX31855の量子化誤差)
- **分解能**: 0.25℃ (12-bit ADC)
- **応答時間**: ≈100ms（MAX31855＋フィルター）

---

## 🚀 Future Roadmap (将来の拡張候補)

### Phase 5: RTC統合 (推定4週間)

```
Goal: 絶対時刻（YYYYMMDD_HHMMSS）をCSVに記録

- DS3231 / PCF8523 I2C RTC統合
- CSV新列追加: Timestamp_YYYYMMDD_HHMMSS
- タイムゾーン設定オプション
- 時刻設定UI (BtnB/BtnC で設定)
- ニューマーテスト10セット実施
```

### Phase 6: クラウド連携 (推定2週間)

```
Goal: Wi-Fi経由でAWS IoT Coreに自動送信

- WiFi接続・認証機構
- AWS IoT SDK統合
- パケット送信ロジック
- オフライン時キューイング
```

### Phase 7: 複数センサ対応 (推定2週間)

```
Goal: K型熱電対複数本の同時計測

- I2C Multiplexer制御
- CSV列拡張: Temp_C_CH1, Temp_C_CH2, ...
- UI多チャネル表示
- ハードウェア配線設計
```

---

## 📝 Release Notes

### v1.0.0 Highlights

✅ **本リリースの特徴**:

1. **Production Ready**: 全統合テスト PASS (33/33)
2. **詳細ドキュメント**: 新人向けハンドオーバー資料完備
3. **安定性**: Crash/Hang なし、複数ラン連続実行OK
4. **パフォーマンス**: Flash 31% / RAM 7% で最適化済み
5. **エラーハンドリング**: ハードウェア故障時も動作継続

**引き継ぎ対応**: 高卒1年目向けハンドオーバーガイド＆4週間学習ロードマップ付き

---

## 🔗 Related Documents

- [README](../README.md) - Project Overview
- [セットアップガイド](../docs/guides/QUICKSTART_SESSION4.md) - Initial Setup
- [ハンドオーバーガイド](../docs/guides/FINAL_HANDOVER_GUIDE.md) - Handover for New Developer
- [実装仕様書](../docs/code/IMPLEMENTATION_GUIDE.md) - Detailed Architecture
- [テスト結果](../docs/future/PHASE4_INTEGRATION_TEST_RESULTS.md) - QA Verification

---

## 📄 License

MIT License - See [LICENSE](../LICENSE) for details.

---

**Last Updated**: 2026-03-02  
**Maintainer**: Shimano → New Developer (Handover Complete)
