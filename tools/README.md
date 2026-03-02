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
project: My Presentation
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
project: Project Name
hardware_mcu: M5Stack Basic V2.7
hardware_sensor: MAX31855
version: v1.0.0
output: output.pptx
---

## slide_01 | layout: title

## slide_02 | layout: bullet
title: My content
bullets:
  - Item A
  - Item B
```

**Single source of truth**: Define values (hardware names, costs, etc.) **only in YAML front matter**.  
They automatically propagate to all slides. No hardcoding in slide sections.

### **Phase 3: Generate**

```bash
python spec_to_pptx.py docs/project/presentation_spec.md
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
project: "Project Title"
subtitle: "Subtitle (hardware names, version, etc.)"
hardware_mcu: "M5Stack Basic V2.7"        # single source of truth
hardware_sensor: "MAX31855"
tech_stack: "ESP32 / C++ (Arduino)"
version: "v1.0.0 | 2026 March"
output: "output_presentation.pptx"        # where to save
---
```

All values defined here are accessible to all slides.  
If hardware name changes, update ONLY the `hardware_mcu` field.

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

Generates a title slide from front matter.

**No params needed** — uses `project`, `hardware_mcu`, `hardware_sensor`, `tech_stack`, `version` from front matter.

```markdown
## slide_01 | layout: title
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
columns: [GPIO, Signal, Target, Note]
col_widths: [2.0, 2.5, 4.0, 4.1]          # must sum to ~12.6 inches
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

### 6. `layout: cost_comparison`

Dual-table layout: component cost (left) + market comparison (right) + highlight + checklist.

```markdown
## slide_06 | layout: cost_comparison
title: Cost Advantage
section: Section 1  Project Overview
cost_rows:
  - [Component, Source, Price]
  - [M5Stack Basic V2.7, Amazon, "¥7,990"]
  - [MAX31855, Akizuki Electronics, "¥980"]
  - [Total, —, "¥9,570"]
market_rows:
  - [Category, Product, Price Range]
  - [DIY Tool, M5Stack + MAX31855, "¥9,570"]
  - [Standard, USB Data Logger, "¥30,000~"]
  - [Professional, Industrial Logger, "¥100,000~"]
highlight: "1/3.1 the price of commercial products"
sub_highlight: "¥9,570 vs ¥30,000~ → saves ¥20,000+"
checklist:
  - "✓ SD auto-log  ✓ LCD real-time  ✓ Alarm function"
  - "✓ Standalone  ✓ EEPROM config  ✓ Statistics"
```

Left and right tables can have different row counts.  
Highlight box and checklist automatically position below the **taller table**.

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

### Hardware name changed but PPTX still has old value

**Cause**: Hardcoded in slide section instead of using front matter.

```markdown
---
hardware_mcu: M5Stack Basic V2.7   # ← Change here
---

## slide_01 | layout: title
# No need to edit below — it reads from front matter automatically
```

**Fix**: Define in YAML front matter. Reference via front matter, not hardcoded text.

### Table overlaps warning/note box below

**Cause**: Manual y-coordinate override, or miscalculated `row_height`.

**Fix**: Let the converter auto-calculate. Verify:
- `row_height` matches actual table rows
- No manual `y=` values in warning/note sections

---

## Example Project

See `examples/simple_temperature_tool/presentation_spec.md` for a complete 9-slide example using all 7 layouts.

```bash
python spec_to_pptx.py examples/simple_temperature_tool/presentation_spec.md
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
