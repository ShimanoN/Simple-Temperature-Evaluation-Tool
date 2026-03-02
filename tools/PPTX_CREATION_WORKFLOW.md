# PPTX Creation Workflow — AI Reference Guide

> **Purpose**: This document is the single source of truth for AI-assisted PPTX creation.  
> When a user says "PPTX を作りたい、このファイルを参照して", the AI must follow this workflow precisely.
>
> **Scope**: Domain-agnostic — works for technical reports, business proposals, research papers,
> recruitment presentations, product demos, and any other use case.
>
> **Language policy**: AI-internal logic and comments → English. All output content → match user's language.

---

## Overview

```
[Phase 1] AI interviews user → understands presentation purpose and structure
[Phase 2] AI drafts presentation_spec.md → user reviews and approves
[Phase 3] AI runs spec_to_pptx.py → PPTX is generated
[Phase 4] User reviews PPTX → AI iterates if needed
```

**Human responsibilities**: Decide content intent and approve the spec  
**AI responsibilities**: Draft spec.md, run converter, fix any bugs, commit to Git

---

## Phase 1 — Spec Design Interview

When the user requests a new PPTX, ask the following questions **in a single message**
(do not ask one at a time). Group them naturally in the user's language.

### Required information to gather

| Category | Questions |
|----------|-----------|
| **Purpose** | What is this presentation for? (job interview, report, proposal, demo, etc.) |
| **Audience** | Who will see it? (recruiter, client, manager, engineers, investors, etc.) |
| **Message** | What is the single most important thing the audience should take away? |
| **Content** | What sections or topics to cover? Any specific data, comparisons, tables? |
| **Vocabulary** | Are there key terms, names, or values that must appear exactly? (product names, prices, dates) |
| **Slide count** | Rough target (5 slides, 10 slides, etc.) |

### AI behavior in Phase 1

1. Ask all questions in a single message
2. Based on answers, **propose a slide structure** (numbered list with layout type)
3. Confirm with user before writing spec.md
4. If user approves, proceed to Phase 2

### Example slide structure proposal

```
1. タイトルスライド        (layout: title)
2. 目次                  (layout: toc)
3. 背景・課題            (layout: bullet)
4. ソリューション概要      (layout: two_column)
5. コスト比較            (layout: dual_table)
6. 技術詳細              (layout: table)
7. まとめ                (layout: bullet)
```

---

## Phase 2 — Writing presentation_spec.md

### File location convention

```
docs/<project_name>/presentation_spec.md   ← spec file (AI writes this)
docs/<project_name>/<output_name>.pptx     ← output (auto-generated)
```

### Spec file format

Two parts:

1. **YAML front matter** (between `---`): project-wide settings and template variables
2. **Slide sections**: one per slide, beginning with `## slide_NN | layout: LAYOUT_NAME`

```markdown
---
# === Reserved fields (control title slide appearance) ===
title: "プレゼンテーションタイトル"          # required — large title on slide 1
subtitle: "サブタイトルテキスト"             # optional — white line below title
tagline: "オレンジ強調ライン"               # optional — orange accent line
meta: "所属 / 氏名 / 技術スタック など"      # optional — gray info line
date: "2026年3月"                           # optional — date at bottom

# === Output ===
output: "output.pptx"                       # required — where to save

# === Project-specific variables (accessible via {{key}} in any slide) ===
# Add any fields here — product names, prices, model numbers, team names, etc.
product: "ProductName v2.0"
price: "¥9,570"
author: "島野拓実"
---

## slide_01 | layout: title
# (no params needed — reads from front matter automatically)

## slide_02 | layout: bullet
title: はじめに
section: Section 1  背景
bullets:
  - "{{product}} の主な特徴"       # ← {{key}} → replaced with front matter value
  - "価格: {{price}}"
```

### Template variables: `{{key}}`

Any `{{key}}` in slide content is replaced at render time with the front matter value for `key`.

**When to use**: Values that appear on multiple slides (product names, model numbers, prices, author names).  
**Single source of truth**: Change the value once in front matter → all slides update automatically.

```markdown
---
product: M5Stack Basic V2.7     # ← change here only
---

## slide_04 | layout: bullet
bullets:
  - "{{product}} の特徴"         # ← auto-replaced at render time
## slide_07 | layout: table
title: "{{product}} ピン配置"    # ← auto-replaced at render time
```

---

## Phase 3 — Running the Converter

```powershell
# Windows (use the Python that has pptx + yaml installed):
& "C:\Users\島野拓実\AppData\Local\Python\bin\python.exe" tools/spec_to_pptx.py docs/<project>/presentation_spec.md

# Or if python is in PATH:
python tools/spec_to_pptx.py docs/<project>/presentation_spec.md
```

**Required packages**: `python-pptx`, `PyYAML`  
Install if missing: `pip install python-pptx pyyaml`

### AI responsibilities in Phase 3

1. Run the command in terminal
2. Verify output shows correct slide count and no errors
3. Fix any Python errors (do NOT ask user to fix Python)
4. Report success and file path

---

## Phase 4 — Review Loop

After PPTX is generated, the user reviews it. Common feedback and how to handle:

| Feedback | AI action |
|----------|-----------|
| "〇〇が重なっている" | Check `safe_y_after_table()` usage; verify `row_height` matches actual data |
| "テキストが小さい" | Adjust `size=Pt(N)` for that rendered element |
| "名前・型番が違う" | Update the front matter variable → it propagates everywhere via `{{key}}` |
| "スライドを追加したい" | Add new `## slide_NN` to spec.md, re-run |
| "レイアウトを変えたい" | Change `layout:` value in that section |
| "スライドの順序を変えたい" | Renumber `## slide_NN` headers, re-run |

---

## Layout Reference

### `layout: title`

Draws from **front matter** fields. No params needed.

| Front matter key | Appears as |
|-----------------|------------|
| `title` | Large white title (36pt) |
| `subtitle` | White subtitle line (18pt) |
| `tagline` | Orange accent line (15pt) |
| `meta` | Gray info line — author/org/stack (14pt) |
| `date` | White date/version at bottom (13pt) |

All can be overridden per-slide in params (uncommon, but possible).

```yaml
## slide_01 | layout: title
# Normally no params needed.
# Override example (rare):
# tagline: "特定スライド用のキャッチコピー"
```

---

### `layout: toc`

```yaml
## slide_02 | layout: toc
title: 目次          # optional, default: "目次"
items:
  - "1. Section name"
  - "2. Section name"
```

---

### `layout: section_divider`

```yaml
## slide_03 | layout: section_divider
section_num: "Section 1"
section_title: セクションタイトル
description: "サブ説明テキスト（オプション）"
```

---

### `layout: bullet`

```yaml
## slide_04 | layout: bullet
title: スライドタイトル
section: "Section 1  セクション名"
bullets:
  - "箇条書き1"
  - "箇条書き2"
highlight: "強調ボックスのテキスト（オプション）"   # navy box
sub_text: "補足テキスト（オプション）"               # below highlight
```

**Limit**: Up to ~7 bullets fit without overflow. For more content, split into two slides.

---

### `layout: table`

```yaml
## slide_05 | layout: table
title: テーブルスライドタイトル
section: "Section X  セクション名"
subtitle: "ヘッダーバー直下のサブ見出し（オプション）"
columns: [列１, 列２, 列３, 列４]
col_widths: [2.0, 2.5, 4.0, 4.1]   # inches — optional; omit for auto equal-width split
rows:
  - [値1, 値2, 値3, 値4]
  - [値1, 値2, 値3, 値4]
row_height: 0.45     # optional, default: 0.40
warning: "⚠ 警告ボックスのテキスト（オプション）"
warning_code: "code snippet text"   # optional — monospace line inside warning
note: "グレー補足ボックスのテキスト（オプション）"
```

**IMPORTANT**: Warning and note boxes are auto-positioned below the table using `safe_y_after_table()`.  
Never specify y-coordinates manually.

---

### `layout: dual_table`

Two tables side-by-side with optional highlight box and checklist.  
**Fully configurable** — labels and highlight rows are set in the spec, not hardcoded.

```yaml
## slide_06 | layout: dual_table
title: スライドタイトル
section: "Section X  セクション名"
left_label: "左テーブルの見出し"            # arbitrary text
left_rows:
  - [ヘッダー1, ヘッダー2, ヘッダー3]       # ← first row = header
  - [データ, データ, データ]
left_col_widths: [2.9, 1.5, 1.4]           # optional — auto-split if omitted
left_highlight_last: true                   # optional — highlight last row (e.g. total)

right_label: "右テーブルの見出し"           # arbitrary text
right_rows:
  - [ヘッダー1, ヘッダー2, ヘッダー3]       # ← first row = header
  - [データ, データ, データ]
right_col_widths: [2.8, 2.18, 1.5]         # optional — auto-split if omitted
right_highlight_row: 1                      # optional — 0=header, 1=first data row, etc.

highlight: "サマリーボックス見出し（オプション）"        # navy box
sub_highlight: "サブテキスト（オプション）"              # orange text inside box
checklist:                                               # optional gray box
  - "✓ 項目A  ✓ 項目B  ✓ 項目C"
  - "✓ 項目D  ✓ 項目E  ✓ 項目F"
```

Use cases: cost comparison, before/after comparison, option A vs B, self vs competitors, etc.

---

### `layout: two_column`

```yaml
## slide_07 | layout: two_column
title: スライドタイトル
section: "Section X  セクション名"
left_title: 左カラム見出し
left_items:
  - "項目1"
  - "項目2"
right_title: 右カラム見出し
right_items:
  - "項目1"
  - "項目2"
note: "下部補足テキスト（オプション）"
```

---

## Coordinate System Reference

```
(0, 0) ──────────────────────── (13.33", 0)
  │  Header bar  h=1.0"          │
  │                              │
  │  Content area                │  CX=0.35"  CY=1.1"
  │  y: 1.1" → 6.85" (safe)     │  CW=12.63" CH=5.75"
  │                              │
  │  Footer bar  y=6.9"          │
(0, 7.5") ──────────────── (13.33", 7.5")
```

$$\text{table\_end\_y} = \text{start\_y} + \text{num\_rows} \times \text{row\_height}$$

> **Rule**: Never hardcode y-coordinates below a table. The converter calls `safe_y_after_table()` automatically.

---

## AI Behavior Checklist

Before starting any new PPTX, AI must:

- [ ] Read this file completely
- [ ] Ask all Phase 1 questions in a **single message**
- [ ] Propose slide structure and wait for approval before writing spec.md
- [ ] Use only documented layouts and parameters
- [ ] Put repeated values (names, prices, model numbers) in front matter as `{{key}}` variables
- [ ] Verify `col_widths` sums to ~12.6" for full-width tables
- [ ] Run the converter and report result with slide count
- [ ] Never ask user to edit Python files

---

## Common Mistakes to Avoid

| Mistake | Prevention |
|---------|-----------|
| Hardcoding a name that appears on multiple slides | Put it in front matter → use `{{key}}` in slides |
| Table overlapping with boxes below | Converter uses `safe_y_after_table()` automatically — never override |
| Content overflowing footer | `CONTENT_BOTTOM = 6.85"` — reduce rows or font size if content is too tall |
| YAML parse error | Wrap values with `:`, `¥`, `-` at start, or numbers in double quotes |
| PowerShell git commit with ¥ in message | Use single quotes or ASCII-only commit messages |
| Using `cost_comparison` layout | Removed — use `dual_table` instead |

---

## File Inventory

| File | Owner | Purpose |
|------|-------|---------|
| `tools/PPTX_CREATION_WORKFLOW.md` | AI reference | This file — master workflow |
| `tools/spec_to_pptx.py` | AI (do not edit manually) | Spec parser + PPTX renderer |
| `docs/<project>/presentation_spec.md` | Human + AI co-authored | Slide content spec |
| `docs/<project>/<output>.pptx` | Auto-generated | Final output |

---

*Last updated: 2026-03-02*

