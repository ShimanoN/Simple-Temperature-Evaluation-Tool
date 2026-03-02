# Simple Temperature Evaluation Tool (v1.0.0)

M5Stack Core2 / M5StackCoreS3 + K型熱電対 MAX31855を使用した、**現場用高精度温度評価・データ記録ツール**です。

ヒートガンやチャンバーから流れる温度変化を **リアルタイムで計測**し、統計値（平均・標準偏差・最大/最小値）を算出・CSV記録します。生産技術現場での温度評価・品質保証業務を効率化します。

---

## 🎯 主な特徴

| 機能 | 説明 |
|:---|:---|
| **リアルタイム計測** | K型熱電対＋MAX31855で高精度温度読取（±1℃相当） |
| **統計計算（Welford法）** | 平均値・標準偏差を計測中にリアルタイム更新 |
| **SD カード記録** | CSV形式でデータを自動保存（複数ラン対応） |
| **アラーム機能** | 高温/低温閾値設定・超過検知（LCD表示） |
| **EEPROM設定保存** | アラーム値をハード内に永続保存 |
| **多段階UI** | IDLE → RUN → RESULT 状態遷移でわかりやすく操作 |

---

## 📊 システム構成

```
┌─────────────────────────────────────────┐
│      M5Stack Core2 / CoreS3             │
│  - ESP32（計算・制御・UI表示）          │
│  - 320×240 LCD タッチスクリーン          │
│  - ボタン×3（操作）                      │
└────┬────────────────────────────────────┘
     │ I2C
     ├──→ EEPROM（設定保存）
     │
     │ SPI
     ├──→ MAX31855 ← K型熱電対
     │    （温度読み込み）
     │
     └──→ SD カードスロット
          （CSV出力）
```

### ハードウェア仕様
- **本体**: M5Stack Core2 / CoreS3
- **温度センサ**: K型熱電対 + MAX31855 I2C デジタル変換ボード
- **ストレージ**: MicroSD カード（最大32GB、FAT32推奨）
- **接続**: USB-C（M5Stack給電・ファームウェア書き込み）

---

## 🚀 クイックスタート（5分）

### 1. ハードウェア準備
```
K型熱電対 → MAX31855ボード → M5Stack（SPI接続）
             ↑ I2C（EEPROM も共用）
```

詳細は [ハードウェア仕様書](docs/specs/detailed_spec_hw.md) 参照。

### 2. ファームウェア書き込み
```bash
cd "Simple Temperature Evaluation Tool"
platformio run --target upload --environment m5stack
```

### 3. 初期化
M5Stack起動時に自動的にSD・EEPROMを初期化します。  
LCD表示で「`SD Ready`」(GREEN) が見えればOK。

### 4. 基本操作
- **[BtnA] 短押し**: 状態遷移（IDLE → RUN → RESULT → IDLE）
- **IDLE**: 現在温度表示、アラーム値確認
- **RUN**: 計測中（毎秒リアルタイム更新）
- **RESULT**: 計測結果表示（データ自動SD保存済み）

📖 詳細操作は [セットアップガイド](docs/guides/QUICKSTART_SESSION4.md) 参照。

---

## 📋 ドキュメント（わかりやすさ重視）

| ドキュメント | 対象 | 内容 |
|:---|:---|:---|
| [セットアップガイド](docs/guides/QUICKSTART_SESSION4.md) | **全員向け** | 初回環境構築～初期操作 |
| [実装仕様書](docs/code/IMPLEMENTATION_GUIDE.md) | **開発者向け** | アーキテクチャ・コード詳細・API |
| [ハンドオーバーガイド](docs/guides/FINAL_HANDOVER_GUIDE.md) | **新人向け** | 背景・全体図・修正方法 |
| [ハードウェア仕様](docs/specs/detailed_spec_hw.md) | **組立者向け** | 配線図・GPIO・I2C設定 |
| [ファイル構成・不具合対応](docs/troubleshooting/TROUBLESHOOTING.md) | **運用者向け** | よくある質問・エラー対応 |
| [変更履歴](CHANGELOG.md) | **全員向け** | バージョン・主要改修 |

---

## 📈 使用シーン例

### 💡 例：ヒートガン → チャンバー → ヒートエアの温度測定

```
1. 準備フェーズ
   ├─ M5Stack起動 → IDLE画面で現在温度確認
   └─ アラーム値をEEPROM設定（HI=60°C, LO=20°C）

2. 計測フェーズ
   ├─ ヒートガン→チャンバー内部のエアに接触
   ├─ [BtnA]短押し → RUN開始
   ├─ 約10秒～数分間、温度の上昇・下降を観察
   └─ [BtnA]短押し → RESULT終了（自動CSV保存）

3. 分析フェーズ
   ├─ RESULT画面で平均値・標準偏差確認
   ├─ MicroSDから CSV をPCに転送
   └─ Excel等で詳細グラフ化・記録
```

**CSV記録例:**
```
ElapsedSec, Temp_C, State, Samples, Average_C, StdDev_C, Max_C, Min_C, HI_ALARM, LO_ALARM
0,         20.7,   RUN,   10,      20.7,       0.0,      20.8,  20.6,  0,        0
0,         21.3,   RUN,   12,      21.0,       0.3,      21.3,  20.6,  0,        0
...（以下、計測期間中の毎行に記録）
```

---

## 🔧 開発環境

- **OS**: Windows 10/11
- **エディタ**: VS Code 1.96+
- **フレームワーク**: PlatformIO IDE for VSCode
- **ボード対応**: esp32 (M5Stack環境)
- **言語**: C++ (Arduino互換)
- **ビルド時間**: 約13秒 / アップロード: 約44秒

### インストール（初回のみ）
```bash
# VS Code に PlatformIO Extension をインストール
# 本リポジトリをクローン
git clone https://github.com/your-org/Simple-Temperature-Evaluation-Tool.git
cd Simple\ Temperature\ Evaluation\ Tool

# ビルド確認
platformio run -e m5stack

# M5Stack をUSB接続して書き込み
platformio run --target upload --environment m5stack
```

---

## 📊 テスト・品質

✅ **Phase 4 統合テスト PASS（2026年3月）**
- 初期化テスト: 4/4 PASS
- RUN状態テスト: 5/5 PASS
- RESULT状態テスト: 3/3 PASS
- UI表示テスト: 5/5 PASS
- CSV形式検証: 7/7 PASS
- エラーハンドリング: 5/5 PASS
- パフォーマンス: 4/4 PASS

詳細は [テスト結果ドキュメント](docs/future/PHASE4_INTEGRATION_TEST_RESULTS.md) 参照。

---

## ⚠️ 既知制限・今後の拡張

| 項目 | 現状 | 今後の可能性 |
|:---|:---|:---|
| **タイムスタンプ** | 経過秒のみ（相対時刻） | RTC統合で絶対時刻記録可能 |
| **クラウド連携** | なし | AWS IoT / Google Cloud等への自動送信 |
| **グラフ表示** | なし（CSV→Excel分析） | M5Stack上でリアルタイムグラフ表示可能 |
| **温度範囲** | -50～+150°C（K型理論値） | 拡張可能（別センサ選定） |
| **電源** | USB-C給電のみ | バッテリーパック対応可能 |

---

## 🤝 開発・保守情報

### ファイル構成
```
Simple Temperature Evaluation Tool/
├── src/                          # ソースコード（Phase 4実装）
│   ├── main.cpp                  # エントリーポイント・状態遷移
│   ├── Tasks.cpp/h               # 定期タスク（IO, Logic, UI）
│   ├── SDManager.cpp/h           # SD操作・CSV書き込み
│   ├── EEPROMManager.cpp/h       # EEPROM読み書き
│   ├── DisplayManager.cpp/h      # LCD UI表示
│   ├── IOController.cpp/h        # I/O制御
│   └── MeasurementCore.cpp/h     # 計測・統計計算（Welford法）
├── include/                      # ヘッダファイル
├── test/                         # ユニットテスト
├── docs/                         # ドキュメント
│   ├── guides/                   # セットアップ・ハンドオーバー
│   ├── specs/                    # ハードウェア・ソフトウェア仕様
│   ├── code/                     # コード説明・実装ガイド
│   ├── troubleshooting/          # トラブル対応
│   └── future/                   # 今後改善案・テスト結果
├── platformio.ini                # PlatformIO設定
└── README.md                     # このファイル
```

---

## 📝 ライセンス

MIT License - 詳細は [LICENSE](LICENSE) 参照。  
作成者・開発関係者のクレジットは [CREDITS.md](docs/CREDITS.md) に記載。

---

## 📧 サポート・質問

- **不具合報告**: GitHub Issues で詳細ログとともに報告ください
- **機能リクエスト**: GitHub Discussions で議論可能です
- **引き継ぎ・保守**: 開発者向けドキュメント [FINAL_HANDOVER_GUIDE.md](docs/guides/FINAL_HANDOVER_GUIDE.md) 参照

---

**Last Updated**: 2026年3月2日 (v1.0.0 Release)  
**Status**: ✅ Production Ready
