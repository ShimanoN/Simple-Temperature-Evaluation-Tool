# PPTX 自動生成ワークフローガイド

> **目的**: `仕様書 → Python → 実行` のパイプラインを安定して回すためのリファレンス。  
> **テンプレート**: [`tools/pptx_template.py`](../../tools/pptx_template.py)

---

## 1. ワークフロー全体像

```
仕様書 (PPTX_SPEC.md)
    │  ↓ 内容を参照して
ProjectConfig を更新 ← 型番・価格・タイトル等の可変値をここに集約
    │  ↓
スライド関数を追加・修正 (slide01 〜 slideNN)
    │  ↓
python create_presentation.py
    │  ↓
output_presentation.pptx  ✅
```

---

## 2. 新規プロジェクトの始め方

```bash
# 1. テンプレートをコピー
cp tools/pptx_template.py docs/<プロジェクト名>/create_presentation.py

# 2. テンプレートを開いて ProjectConfig を編集（次節参照）

# 3. スライド関数を追加（4節参照）

# 4. 実行
python docs/<プロジェクト名>/create_presentation.py
```

---

## 3. ProjectConfig — 変数の一元管理

**最重要ルール**: プロジェクト固有の値（型番・価格・タイトルなど）は **ProjectConfig の中にだけ** 書く。スライド関数の中に直接書かない。

```python
class ProjectConfig:
    TITLE           = "Simple Temperature Evaluation Tool"
    HARDWARE_MCU    = "M5Stack Basic V2.7"   # ← ここだけ変えれば全スライドに反映
    HARDWARE_SENSOR = "MAX31855"
    COST_TOTAL      = "¥9,570"
    OUTPUT_FILE     = "output.pptx"
```

**スライド関数での参照方法:**

```python
# ❌ 悪い例 — ハードコード
add_textbox(s, "M5Stack Basic V2.7 × MAX31855", ...)

# ✅ 良い例 — ProjectConfig 参照
add_textbox(s, f"{ProjectConfig.HARDWARE_MCU} × {ProjectConfig.HARDWARE_SENSOR}", ...)
```

> **なぜ重要か**: 前回のセッションで `Core2` と `Basic V2.7` の不一致が複数箇所で発生した。  
> ProjectConfig を単一の真実の源泉にすることで、型番変更が 1 行の修正で全スライドに反映される。

---

## 4. スライド関数の追加パターン

```python
def slide03(prs):
    s = _blank_slide(prs)
    header_bar(s, "スライドのタイトル", "Section X  セクション名")
    footer_bar(s, 3)   # ← ページ番号

    # コンテンツをここに追加（4.1 〜 4.3 節参照）
```

### 4.1 テキストボックス

```python
add_textbox(s, "テキスト内容",
            Inches(X), Inches(Y), Inches(幅), Inches(高さ),
            size=Pt(フォントサイズ), color=C_NAVY, bold=True, font=FONT_JP,
            align=PP_ALIGN.CENTER)
```

### 4.2 塗りつぶしボックス

```python
add_rect(s, Inches(X), Inches(Y), Inches(幅), Inches(高さ), C_NAVY)
# 枠線あり:
add_rect(s, Inches(X), Inches(Y), Inches(幅), Inches(高さ), C_RED_LT, C_RED, line_width=1.5)
```

### 4.3 テーブル — **座標計算が必要**

テーブルを使う場合は、テーブルの終了位置を必ず計算してから次の要素を配置する。

```python
# ① 定数を先に定義
TABLE_START_Y = 1.6    # テーブル上端 (inches)
TABLE_ROW_H   = 0.40   # 行高 (inches)

rows = [
    ["ヘッダ1", "ヘッダ2"],
    ["データ A", "値 A"],
    ["データ B", "値 B"],
    ["データ C", "値 C"],
]
col_widths = [Inches(6.0), Inches(6.63)]

# ② テーブルを配置
make_table(s, CX, Inches(TABLE_START_Y), CW, rows, col_widths,
           row_height=Inches(TABLE_ROW_H))

# ③ テーブル終了位置を計算してから次の要素を配置  ← ここが重要!
next_y = safe_y_after_table(TABLE_START_Y, len(rows), TABLE_ROW_H, margin_inches=0.15)

add_rect(s, CX, Inches(next_y), CW, Inches(0.8), C_NAVY)
add_textbox(s, "テーブルの下が絶対に重ならないボックス",
            Inches(0.5), Inches(next_y + 0.1), ..., align=PP_ALIGN.CENTER)
```

---

## 5. 座標計算リファレンス

### スライドの座標系

```
(0, 0) ─────────────────────────── (13.33", 0)
  │   ヘッダーバー (高さ 1.0")      │
  │                                  │
  │   コンテンツエリア               │  ← CX=0.35", CY=1.1"
  │   (y=1.1" 〜 y=6.85")          │
  │                                  │
  │   フッターバー (y=6.9")         │
(0, 7.5") ─────────────────── (13.33", 7.5")
```

- **y は上から下に増加**
- **コンテンツの最下端**: y = 6.85"（これを超えるとフッターと重なる）

### テーブル終了位置の計算式

$$
\text{テーブル終了 y} = \text{開始 y} + \text{行数} \times \text{行高}
$$

| 開始 y | 行数 | 行高 | 終了 y |
|--------|------|------|--------|
| 1.50"  |  7行 | 0.40" | **4.30"** |
| 1.55"  |  6行 | 0.45" | **4.25"** |
| 1.50"  |  4行 | 0.40" | **3.10"** |

**使い方**: 終了 y + マージン（推奨 0.1" 〜 0.15"）が次の要素の開始 y になる。

### よく使うサイズ

| 要素 | 推奨行高 | 備考 |
|------|----------|------|
| 通常テーブル | 0.40" | コンパクトで読みやすい |
| 詳細テーブル | 0.45" 〜 0.50" | 4列以上のとき |
| ヘッダーバー | 1.0" | 固定 |
| フォントサイズ参考 | Pt(12)〜Pt(24) | タイトル: 24、本文: 12〜14 |

---

## 6. よくあるミスと対処法

### ❌ ミス 1: テーブルとブロックが重なる

**原因**: テーブルの終了位置を手計算せず、固定値で y を指定した。

**対策**: `safe_y_after_table()` を必ず使う（5節参照）。

---

### ❌ ミス 2: 型番・価格の不一致

**原因**: 型番を各スライド関数に直接書いたため、変更漏れが発生した。

**対策**: `ProjectConfig` に書いてスライド関数では `ProjectConfig.HARDWARE_MCU` で参照する。

---

### ❌ ミス 3: コンテンツがフッターにはみ出す

**原因**: 末尾の要素の y + 高さが 6.85" を超えている。

**対策**:
1. テーブルの行高を少し小さくする（0.45" → 0.40"）
2. ボックスの高さを削減する
3. コンテンツを 2 カラムに分割する

---

### ❌ ミス 4: PowerShell での git commit 失敗

**原因**: commit メッセージに `¥` 等の特殊文字が入ると PowerShell が誤解析する。

**対策**: commit メッセージはシングルクォートで囲む、または ASCII のみにする。

```powershell
# ✅ 安全
git commit -m 'fix: update hardware model and layout fixes'

# ❌ 危険（¥ が特殊文字として解析されることがある）
git commit -m "fix: M5Stack ¥6,600 → ¥7,990"
```

---

## 7. AI モデル使い分けガイド

| フェーズ | タスク例 | 推奨モデル | 理由 |
|----------|---------|-----------|------|
| **構想・設計** | スライド構成の決定、レイアウト設計 | **Sonnet** | 複雑な判断・設計力が必要 |
| **座標計算** | テーブル終了位置、レイアウト調整 | **Sonnet** | 精度が重要 |
| **実装（定型部分）** | ヘルパー関数、ボイラープレート生成 | **Haiku** | 単純繰り返し、コスト最適 |
| **仕様書確認・修正** | 誤字・型番の修正 | **Haiku** | 定型的な検索・置換 |
| **日本語ドキュメント** | README、ガイド文書の執筆 | **Sonnet** | 表現品質が重要 |
| **Git 操作** | add / commit / push | **Haiku** | 定型作業 |

---

## 8. チェックリスト（修正時）

毎回の修正前後に確認する。

```
[ ] ProjectConfig の型番・価格が最新か？
[ ] 修正した型番が全スライドに反映されているか？（grep で確認）
[ ] テーブルを変更した場合、safe_y_after_table() で次の要素の y を再計算したか？
[ ] 末尾要素の y + 高さ ≤ 6.85"（フッターとの重なり確認）
[ ] python create_presentation.py を実行して PPTX が生成されるか確認
[ ] git commit メッセージはシングルクォートか ASCII のみか？
```

---

## 9. ディレクトリ構造

```
プロジェクトルート/
├── tools/
│   └── pptx_template.py          ← 新規プレゼン作成の出発点
├── docs/
│   ├── guides/
│   │   └── PPTX_WORKFLOW_GUIDE.md ← このファイル
│   ├── PPTX_SPEC.md               ← スライド仕様書
│   └── career/
│       ├── create_presentation_v2.py  ← 実際の生成スクリプト
│       └── Simple_Temp_Tool_Overview_v2.pptx  ← 出力ファイル
```

---

*最終更新: 2026 年 3 月*
