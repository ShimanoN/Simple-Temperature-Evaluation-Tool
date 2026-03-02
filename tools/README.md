# PPTX Automation

> **Markdown spec → PowerPoint 自動生成**
>
> 仕様書（Markdown）を書くだけで、PowerPoint を自動生成。  
> 複数プロジェクト対応。テンプレートで座標計算バグを防止。

---

## Quick Start

### Installation

```bash
# Required packages
pip install python-pptx pyyaml
```

### Your First PPTX

```bash
# Create a spec file
cat > presentation_spec.md << 'EOF'
---
title: "My Presentation"
output: my_presentation.pptx
---

## slide_01 | layout: title

## slide_02 | layout: toc
items:
  - "1. Section One"
  - "2. Section Two"

## slide_03 | layout: bullet
title: First Content Slide
section: Section 1
bullets:
  - "Point A"
  - "Point B"
EOF

# Generate PPTX
python spec_to_pptx.py presentation_spec.md

# Output: my_presentation.pptx ✅
```

---

## How to Create a Presentation (4 Phases)

### **Phase 1: Plan Slide Structure**

You decide:
- What is the presentation for? (job interview, technical report, etc.)
- Who is the audience?
- How many slides roughly?
- What are the main topics?

### **Phase 2: Write Spec**

Human responsibility: author `presentation_spec.md` with YAML front matter + slide sections.

```markdown
---
title: "Project Title"                 # required — shown on title slide
subtitle: "Subtitle text"              # optional
tagline: "Key value proposition"       # optional — orange accent
meta: "Author / Org / Tech stack"      # optional — gray info line
date: "2026 March"                      # optional
output: output.pptx

# Project-specific variables (accessible via {{key}} in any slide)
product: "ProductName"
price: "\u00a59,570"
---

## slide_01 | layout: title

## slide_02 | layout: bullet
title: My content
bullets:
  - "{{product}} の主な特徴"
  - "価格: {{price}}"
```

**Single source of truth**: Put values like product names or prices in front matter.  
Use `{{key}}` in slide content — they are replaced automatically at render time.

### **Phase 3: Generate**

```bash
python tools/spec_to_pptx.py docs/<project>/presentation_spec.md
```

Output location is specified in front matter `output:` field.

### **Phase 4: Review & Iterate**

If changes needed:
- Edit `presentation_spec.md`
- Re-run the command
- PPTX is regenerated instantly

---

## Spec Format Reference

### YAML Front Matter (Project Config)

```yaml
---
# === Reserved fields (control title slide appearance) ===
title: "Presentation Title"             # required
subtitle: "Subtitle"                    # optional — white line below title
tagline: "Orange accent line"           # optional
meta: "Author / Org / Tech stack"       # optional — gray info line
date: "2026 March"                      # optional
output: "output_presentation.pptx"      # required

# === Project-specific variables ===
# Any field defined here is accessible via {{key}} in any slide.
product: "M5Stack Basic V2.7"
price: "\u00a59,570"
author: "Your Name"
---
```

All values defined here are accessible to all slides.  
If a product name changes, update it **once** in front matter — all slides update via `{{product}}`.

### Template Variables: `{{key}}`

Any `{{key}}` in slide content is replaced at render time with the front matter value.

```markdown
---
product: M5Stack Basic V2.7    # ← define once
---

## slide_04 | layout: bullet
title: "{{product}} の特徴"            # ← auto-replaced
bullets:
  - "{{product}} + MAX31855 構成"

## slide_07 | layout: table
title: "{{product}} ピン配置"          # ← auto-replaced
```

### Slide Section Format

```markdown
## slide_NN | layout: LAYOUT_NAME
param1: value1
param2: value2
```

Each slide begins with `## slide_NN | layout: XXX`.  
Everything after the header until the next slide is treated as YAML.

---

## Built-in Layouts (7 Types)

### 1. `layout: title`

Builds the title slide from front matter fields. No params needed.

| Front matter key | Appears as |
|-----------------|------------|
| `title` | Large white title (required) |
| `subtitle` | White subtitle line |
| `tagline` | Orange accent line |
| `meta` | Gray info line (author / org / stack) |
| `date` | White date/version at bottom |

All fields can be overridden per-slide in params.

```markdown
## slide_01 | layout: title
# No params needed — reads from front matter automatically
```

---

### 2. `layout: toc`

Table of contents.

```markdown
## slide_02 | layout: toc
title: 目次                              # optional
items:
  - "1. Introduction"
  - "2. Hardware Design"
  - "3. Software Architecture"
```

---

### 3. `layout: section_divider`

Section break with large title.

```markdown
## slide_03 | layout: section_divider
section_num: "Section 1"
section_title: Hardware Design
description: "M5Stack + MAX31855 Configuration"   # optional
```

---

### 4. `layout: bullet`

General-purpose slide with title, bullet list, and optional highlight box.

```markdown
## slide_04 | layout: bullet
title: Project Overview
section: Section 1  Project Background    # header bar label
bullets:
  - "Key point A"
  - "Key point B"
  - "Key point C"
highlight: "Main message to emphasize"    # optional navy box
sub_text: "Supporting detail text"        # optional text below highlight
```

---

### 5. `layout: table`

Data table with optional warning and note boxes.

```markdown
## slide_05 | layout: table
title: Pin Assignment
section: Section 2  Hardware
subtitle: "Optional sub-heading shown below header bar"  # optional
columns: [GPIO, Signal, Target, Note]
col_widths: [2.0, 2.5, 4.0, 4.1]          # optional — omit for auto equal-width split
rows:
  - [GPIO 18, SCK, MAX31855 CLK, Shared]
  - [GPIO 19, MISO, MAX31855 DO, Shared]
row_height: 0.40                           # optional, default 0.40
warning: "⚠ SD CS pin must be explicitly set"   # optional red warning
warning_code: "SD.begin(TFCARD_CS_PIN);"       # code snippet in warning
note: "EEPROM: Built-in ESP32 flash (no external wiring)"  # optional gray box
```

**Important**: The converter automatically calculates safe y-coordinates for warning and note boxes.  
Never manually adjust positions.

---

### 6. `layout: dual_table`

Two tables side-by-side with optional highlight box and checklist.  
**Fully configurable** — all labels and highlight rows are set in the spec.

```markdown
## slide_06 | layout: dual_table
title: Cost Comparison
section: Section 1  Project Overview
left_label: "Left table heading"           # arbitrary text
left_rows:
  - [Header1, Header2, Header3]             # ← first row = header
  - [Data, Data, Data]
left_col_widths: [2.9, 1.5, 1.4]          # optional — auto-split if omitted
left_highlight_last: true                   # optional — highlight total row
right_label: "Right table heading"         # arbitrary text
right_rows:
  - [Header1, Header2, Header3]
  - [Data, Data, Data]
right_col_widths: [2.8, 2.18, 1.5]        # optional — auto-split if omitted
right_highlight_row: 1                     # optional — 1=first data row, 0=header
highlight: "Summary box heading"           # optional — navy box
sub_highlight: "Sub text"                  # optional — orange inside box
checklist:                                  # optional — gray box
  - "✓ Feature A  ✓ Feature B  ✓ Feature C"
```

Use cases: cost ÷ price comparison, before/after, option A vs B, self vs competitors.

---

### 7. `layout: two_column`

Two-column layout for side-by-side comparison.

```markdown
## slide_07 | layout: two_column
title: Hardware Overview
section: Section 2  Hardware
left_title: Components
left_items:
  - "M5Stack Basic V2.7"
  - "K-type thermocouple"
  - "MicroSD card"
right_title: Interfaces
right_items:
  - "SPI: LCD + MAX31855 shared"
  - "USB-C 5V power"
  - "EEPROM: internal flash"
note: "No external wiring required for EEPROM"   # optional gray footer box
```

---

## Coordinate System & Overlap Prevention

**Slide dimensions**: 16:9, 13.33" × 7.5"

```
y=0.0"   ┌─────────────────────┐
         │   Header bar (1.0") │
y=1.1"   ├─────────────────────┤
         │                     │
         │  Content area       │  ← 5.75" tall
         │  (safe: up to 6.85")│
         │                     │
y=6.9"   ├─────────────────────┤
         │   Footer bar (0.6") │
y=7.5"   └─────────────────────┘
```

**Key safeguard**: When using tables, the converter automatically calculates `table_end_y` and places warning/note boxes **below** with a 0.12" margin.

**Never override y-coordinates manually.**

---

## Troubleshooting

### "Element exceeds content bottom" warning

**Cause**: A slide element would overlap the footer.

**Fix**:
- Reduce table row height (0.40" → 0.38")
- Reduce font size
- Move content to a new slide

### PyYAML parse error in slide

**Cause**: YAML syntax error in a slide section.

```markdown
## slide_03 | layout: bullet
title: "My: Title With Colon"   # ← Needs quotes
bullets:
  - "Item with ¥ symbol"        # ← This is OK
```

**Fix**: Wrap values in double quotes if they contain colons, special chars, or numbers.

### "Layout X not found"

**Cause**: Typo in layout name.

```markdown
## slide_03 | layout: bullit    # ← Wrong! Should be "bullet"
```

**Fix**: Check layout name spelling. See "Built-in Layouts" section.

### Value changed in front matter but PPTX shows old value

**Cause**: The value is hardcoded in the slide section instead of using `{{key}}` substitution.

```markdown
---
product: "New Product Name"   # ← Change here
---

## slide_04 | layout: bullet
bullets:
  - "{{product}} の特徴"        # ← Auto-updated via {{key}}
  # NOT: "Old Product Name の特徴"  # ← Hardcoded — won't update!
```

**Fix**: Move the value to front matter, reference it with `{{key}}` in slides.

### Table overlaps warning/note box below

**Cause**: Manual y-coordinate override, or miscalculated `row_height`.

**Fix**: Let the converter auto-calculate. Verify:
- `row_height` matches actual table rows
- No manual `y=` values in warning/note sections

---

## Example Project

See `examples/simple_temperature_tool/presentation_spec.md` for a complete 9-slide example using all 7 layouts.

```bash
python tools/spec_to_pptx.py examples/simple_temperature_tool/presentation_spec.md
```

Output: `examples/simple_temperature_tool/simple_temperature_tool.pptx`

---

## For Developers: Advanced Customization

### Adding a New Layout

Edit `spec_to_pptx.py`:

1. Write a new `render_YOUR_LAYOUT()` function
2. Register in `LAYOUT_RENDERERS` dict
3. Document in README

See existing `render_bullet()`, `render_table()`, etc. for patterns.

### Color Scheme

Edit color constants at the top of `spec_to_pptx.py`:

```python
C_NAVY     = RGBColor(0x0D, 0x47, 0x7F)
C_ORANGE   = RGBColor(0xFF, 0x6D, 0x00)
C_WHITE    = RGBColor(0xFF, 0xFF, 0xFF)
# ... etc
```

### Font Family

Default: 游ゴシック (Japanese), Segoe UI (English), Consolas (code).

Change in `spec_to_pptx.py`:

```python
FONT_JP   = "Yu Gothic"      # or any installed font
FONT_EN   = "Calibri"
FONT_CODE = "Courier New"
```

---

## Requirements

- **Python 3.7+**
- **python-pptx** ≥ 0.6.21
- **PyYAML** ≥ 5.1

Install:

```bash
pip install python-pptx pyyaml
```

---

## License

MIT License (or specify your license)

---

## Version

**v1.0.0** — 2026-03-02

---

**Issues? Questions?** See `PPTX_CREATION_WORKFLOW.md` for detailed reference.
