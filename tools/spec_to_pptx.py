#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
spec_to_pptx.py
---------------
Reads a presentation_spec.md file and generates a PowerPoint file.

Usage:
    python tools/spec_to_pptx.py docs/<project>/presentation_spec.md

Spec format: see tools/PPTX_CREATION_WORKFLOW.md for full documentation.

Design:
    - parse_spec()      : Parses YAML front matter + slide sections from .md
    - render_*()        : One renderer per layout type
    - safe_y_after_table(): Prevents overlap bugs by calculating table bottom y
    - main()            : Orchestrates parse → render → save
"""

import os
import re
import sys
import yaml
from pptx import Presentation
from pptx.util import Inches, Pt
from pptx.dml.color import RGBColor
from pptx.enum.text import PP_ALIGN

# ===========================================================================
# ▼ COLOR CONSTANTS
# ===========================================================================
C_NAVY     = RGBColor(0x0D, 0x47, 0x7F)
C_ORANGE   = RGBColor(0xFF, 0x6D, 0x00)
C_WHITE    = RGBColor(0xFF, 0xFF, 0xFF)
C_DARK_TXT = RGBColor(0x1A, 0x1A, 0x2E)
C_GREEN    = RGBColor(0x27, 0xAE, 0x60)
C_RED      = RGBColor(0xC0, 0x39, 0x2B)
C_GRAY_LT  = RGBColor(0xF4, 0xF4, 0xF4)
C_GRAY_MID = RGBColor(0x99, 0x99, 0x99)
C_RED_LT   = RGBColor(0xFF, 0xE0, 0xE0)
C_BLACK    = RGBColor(0x00, 0x00, 0x00)

# ===========================================================================
# ▼ FONT CONSTANTS
# ===========================================================================
FONT_JP   = "游ゴシック"
FONT_EN   = "Segoe UI"
FONT_CODE = "Consolas"

# ===========================================================================
# ▼ LAYOUT CONSTANTS  (16:9, 13.33" × 7.5")
# ===========================================================================
W              = Inches(13.33)
H              = Inches(7.5)
HDR_H          = Inches(1.0)
FTR_Y          = Inches(6.9)
FTR_H          = Inches(0.6)
CX             = Inches(0.35)
CY             = Inches(1.1)
CW             = Inches(12.63)
CH             = Inches(5.75)
CONTENT_BOTTOM = 6.85   # inches — last safe y before footer

RECT = 1  # MSO_AUTO_SHAPE_TYPE.RECTANGLE


# ===========================================================================
# ▼ COORDINATE HELPERS  — use these, never hardcode y after a table
# ===========================================================================

def calc_table_bottom(start_y: float, num_rows: int, row_h: float) -> float:
    """Return the y-coordinate (inches) where the table ends."""
    return start_y + num_rows * row_h


def safe_y_after_table(start_y: float, num_rows: int, row_h: float,
                        margin: float = 0.12) -> float:
    """Return a safe y (inches) for the first element below a table."""
    return calc_table_bottom(start_y, num_rows, row_h) + margin


def clamp_to_content(y: float, element_h: float) -> float:
    """Warn if element would overflow footer area."""
    if y + element_h > CONTENT_BOTTOM:
        print(f"  ⚠ WARNING: element at y={y:.2f}\" h={element_h:.2f}\" "
              f"exceeds content bottom ({CONTENT_BOTTOM}\"). May overlap footer.")
    return y


# ===========================================================================
# ▼ PPTX PRIMITIVE HELPERS
# ===========================================================================

def _blank_slide(prs):
    return prs.slides.add_slide(prs.slide_layouts[6])


def _set_bg(slide, color: RGBColor):
    bg = slide.background
    fill = bg.fill
    fill.solid()
    fill.fore_color.rgb = color


def add_rect(slide, x, y, w, h, fill_color,
             line_color=None, line_width=None):
    shape = slide.shapes.add_shape(RECT, x, y, w, h)
    shape.fill.solid()
    shape.fill.fore_color.rgb = fill_color
    if line_color:
        shape.line.color.rgb = line_color
        shape.line.width = Pt(line_width or 1.0)
    else:
        shape.line.fill.background()
    return shape


def add_textbox(slide, text, x, y, w, h,
                size=Pt(13), color=None, bold=False, italic=False,
                font=FONT_JP, align=PP_ALIGN.LEFT, wrap=True):
    if color is None:
        color = C_DARK_TXT
    tb = slide.shapes.add_textbox(x, y, w, h)
    tf = tb.text_frame
    tf.word_wrap = wrap
    p = tf.paragraphs[0]
    p.alignment = align
    run = p.add_run()
    run.text = text
    run.font.size = size
    run.font.color.rgb = color
    run.font.bold = bold
    run.font.italic = italic
    run.font.name = font
    return tb


def add_multiline_textbox(slide, lines: list, x, y, w, h,
                           size=Pt(13), color=None, bold=False,
                           font=FONT_JP, align=PP_ALIGN.LEFT):
    if color is None:
        color = C_DARK_TXT
    tb = slide.shapes.add_textbox(x, y, w, h)
    tf = tb.text_frame
    tf.word_wrap = True
    for i, line in enumerate(lines):
        p = tf.paragraphs[i] if i == 0 else tf.add_paragraph()
        p.alignment = align
        run = p.add_run()
        run.text = line
        run.font.size = size
        run.font.color.rgb = color
        run.font.bold = bold
        run.font.name = font
    return tb


def header_bar(slide, title: str, section: str = ""):
    add_rect(slide, Inches(0), Inches(0), W, HDR_H, C_NAVY)
    add_textbox(slide, title,
                Inches(0.35), Inches(0.08), Inches(10.5), Inches(0.55),
                size=Pt(24), color=C_WHITE, bold=True, font=FONT_JP)
    if section:
        add_textbox(slide, section,
                    Inches(0.35), Inches(0.63), Inches(10.5), Inches(0.3),
                    size=Pt(12), color=C_ORANGE, font=FONT_JP)


def footer_bar(slide, page_num: int, project_title: str = ""):
    add_rect(slide, Inches(0), FTR_Y, W, FTR_H, C_NAVY)
    add_textbox(slide, project_title,
                Inches(0.35), Inches(6.95), Inches(9.0), Inches(0.4),
                size=Pt(11), color=C_GRAY_MID, font=FONT_EN)
    add_textbox(slide, str(page_num),
                Inches(11.5), Inches(6.95), Inches(1.5), Inches(0.4),
                size=Pt(11), color=C_WHITE, font=FONT_EN, align=PP_ALIGN.RIGHT)


def make_table(slide, x, y, w, rows: list, col_widths: list,
               row_height=Inches(0.40), header_fill=C_NAVY,
               header_text_color=C_WHITE, alt_fill=C_GRAY_LT):
    num_rows = len(rows)
    num_cols = len(rows[0])
    total_h  = row_height * num_rows
    tbl = slide.shapes.add_table(num_rows, num_cols, x, y, w, total_h).table
    for ci, cw in enumerate(col_widths):
        tbl.columns[ci].width = cw
    for ri, row_data in enumerate(rows):
        tbl.rows[ri].height = row_height
        for ci, cell_text in enumerate(row_data):
            cell = tbl.cell(ri, ci)
            cell.text = str(cell_text)
            if ri == 0:
                cell.fill.solid()
                cell.fill.fore_color.rgb = header_fill
                r = cell.text_frame.paragraphs[0].runs[0]
                r.font.color.rgb = header_text_color
                r.font.bold = True
                r.font.name = FONT_JP
                r.font.size = Pt(12)
            else:
                if ri % 2 == 0:
                    cell.fill.solid()
                    cell.fill.fore_color.rgb = alt_fill
                else:
                    cell.fill.solid()
                    cell.fill.fore_color.rgb = C_WHITE
                r = cell.text_frame.paragraphs[0].runs[0]
                r.font.name = FONT_JP
                r.font.size = Pt(12)
    return tbl


def color_row(tbl, row_index, fill, text_color, bold=False):
    row = tbl.rows[row_index]
    for cell in row.cells:
        cell.fill.solid()
        cell.fill.fore_color.rgb = fill
        for para in cell.text_frame.paragraphs:
            for run in para.runs:
                run.font.color.rgb = text_color
                if bold:
                    run.font.bold = True


# ===========================================================================
# ▼ SPEC PARSER
# ===========================================================================

def parse_spec(filepath: str):
    """
    Parse a presentation_spec.md file.

    Returns:
        config (dict):  YAML front matter (project-wide settings)
        slides (list):  list of {'layout': str, 'params': dict}
    """
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    # --- Extract YAML front matter ---
    fm_match = re.match(r'^---\r?\n(.*?)\r?\n---\r?\n', content, re.DOTALL)
    if fm_match:
        config = yaml.safe_load(fm_match.group(1)) or {}
        body   = content[fm_match.end():]
    else:
        print("WARNING: No YAML front matter found (--- ... ---)")
        config = {}
        body   = content

    # --- Extract slide sections ---
    # Pattern: ## slide_NN | layout: LAYOUT_NAME
    # Captures everything until the next ## slide_ or end of string
    slide_header = re.compile(
        r'^## slide_\d+\s*\|\s*layout:\s*(\S+)\s*\n(.*?)(?=^## slide_|\Z)',
        re.MULTILINE | re.DOTALL
    )
    slides = []
    for match in slide_header.finditer(body):
        layout     = match.group(1).strip()
        yaml_block = match.group(2).strip()
        try:
            params = yaml.safe_load(yaml_block) if yaml_block else {}
        except yaml.YAMLError as e:
            print(f"WARNING: YAML parse error in slide (layout={layout}): {e}")
            params = {}
        slides.append({'layout': layout, 'params': params or {}})

    return config, slides


# ===========================================================================
# ▼ LAYOUT RENDERERS
# ===========================================================================

def render_title(prs, s, config: dict, params: dict, page_num: int):
    """
    layout: title
    Uses config fields: project, hardware_mcu, hardware_sensor,
                        tech_stack, version
    """
    _set_bg(s, C_NAVY)

    project   = config.get('project', 'Presentation Title')
    hw_mcu    = config.get('hardware_mcu', '')
    hw_sensor = config.get('hardware_sensor', '')
    tech      = config.get('tech_stack', '')
    version   = config.get('version', '')

    # Build subtitle from config — single source of truth
    # Priority: explicit 'subtitle' field > auto-build from hw_mcu × hw_sensor
    subtitle = config.get('subtitle', '')
    if not subtitle and hw_mcu and hw_sensor:
        subtitle = f"{hw_mcu} × {hw_sensor}"

    add_textbox(s, project,
                Inches(1.0), Inches(1.8), Inches(11.3), Inches(0.9),
                size=Pt(36), color=C_WHITE, bold=True, font=FONT_EN,
                align=PP_ALIGN.CENTER)

    if subtitle:
        add_textbox(s, subtitle,
                    Inches(1.0), Inches(2.9), Inches(11.3), Inches(0.55),
                    size=Pt(18), color=C_WHITE, font=FONT_JP,
                    align=PP_ALIGN.CENTER)

    add_rect(s, Inches(1.0), Inches(3.62), Inches(11.3), Inches(0.04), C_ORANGE)

    features = params.get('features', config.get('features', ''))
    add_textbox(s, features,
                Inches(1.0), Inches(3.8), Inches(11.3), Inches(0.45),
                size=Pt(15), color=C_ORANGE, font=FONT_JP,
                align=PP_ALIGN.CENTER)

    if tech:
        add_textbox(s, tech,
                    Inches(1.0), Inches(4.3), Inches(11.3), Inches(0.45),
                    size=Pt(14), color=C_GRAY_MID, font=FONT_EN,
                    align=PP_ALIGN.CENTER)

    if version:
        add_textbox(s, version,
                    Inches(1.0), Inches(5.2), Inches(11.3), Inches(0.4),
                    size=Pt(13), color=C_WHITE, font=FONT_JP,
                    align=PP_ALIGN.CENTER)


def render_toc(prs, s, config: dict, params: dict, page_num: int):
    """
    layout: toc
    params: title (optional), items (list of strings)
    """
    title = params.get('title', '目次')
    items = params.get('items', [])

    header_bar(s, title, "")
    footer_bar(s, page_num, config.get('project', ''))

    # Render items as numbered list in a navy left-bordered style
    for i, item in enumerate(items):
        y = 1.35 + i * 0.62
        # Left accent bar
        add_rect(s, Inches(0.5), Inches(y), Inches(0.06), Inches(0.42), C_ORANGE)
        add_textbox(s, item,
                    Inches(0.72), Inches(y), Inches(11.5), Inches(0.42),
                    size=Pt(18), color=C_DARK_TXT, font=FONT_JP)


def render_section_divider(prs, s, config: dict, params: dict, page_num: int):
    """
    layout: section_divider
    params: section_num, section_title, description (optional)
    """
    _set_bg(s, C_NAVY)

    section_num   = params.get('section_num', '')
    section_title = params.get('section_title', '')
    description   = params.get('description', '')

    add_textbox(s, section_num,
                Inches(1.0), Inches(2.2), Inches(11.3), Inches(0.6),
                size=Pt(22), color=C_ORANGE, bold=True, font=FONT_EN,
                align=PP_ALIGN.CENTER)

    add_textbox(s, section_title,
                Inches(1.0), Inches(2.9), Inches(11.3), Inches(0.8),
                size=Pt(36), color=C_WHITE, bold=True, font=FONT_JP,
                align=PP_ALIGN.CENTER)

    add_rect(s, Inches(3.5), Inches(3.85), Inches(6.3), Inches(0.04), C_ORANGE)

    if description:
        add_textbox(s, description,
                    Inches(1.0), Inches(4.05), Inches(11.3), Inches(0.5),
                    size=Pt(15), color=C_GRAY_MID, font=FONT_JP,
                    align=PP_ALIGN.CENTER)

    footer_bar(s, page_num, config.get('project', ''))


def render_bullet(prs, s, config: dict, params: dict, page_num: int):
    """
    layout: bullet
    params: title, section, bullets (list), highlight (optional), sub_text (optional)
    """
    title     = params.get('title', '')
    section   = params.get('section', '')
    bullets   = params.get('bullets', [])
    highlight = params.get('highlight', '')
    sub_text  = params.get('sub_text', '')

    header_bar(s, title, section)
    footer_bar(s, page_num, config.get('project', ''))

    # Bullet items
    bullet_start_y = 1.2
    for i, bullet in enumerate(bullets):
        y = bullet_start_y + i * 0.52
        add_rect(s, Inches(0.35), Inches(y + 0.12), Inches(0.06), Inches(0.25), C_ORANGE)
        add_textbox(s, bullet,
                    Inches(0.55), Inches(y), Inches(12.2), Inches(0.45),
                    size=Pt(14), color=C_DARK_TXT, font=FONT_JP)

    # Optional highlight box below bullets
    if highlight or sub_text:
        box_y = bullet_start_y + len(bullets) * 0.52 + 0.2
        box_y = clamp_to_content(box_y, 0.8)
        add_rect(s, CX, Inches(box_y), CW, Inches(0.55), C_NAVY)
        add_textbox(s, highlight,
                    Inches(0.5), Inches(box_y + 0.07), Inches(12.3), Inches(0.38),
                    size=Pt(16), color=C_WHITE, bold=True, font=FONT_JP,
                    align=PP_ALIGN.CENTER)
        if sub_text:
            sub_y = box_y + 0.62
            sub_y = clamp_to_content(sub_y, 0.4)
            add_textbox(s, sub_text,
                        Inches(0.5), Inches(sub_y), Inches(12.3), Inches(0.4),
                        size=Pt(13), color=C_DARK_TXT, font=FONT_JP,
                        align=PP_ALIGN.CENTER)


def render_table(prs, s, config: dict, params: dict, page_num: int):
    """
    layout: table
    params: title, section, columns, col_widths (list of floats),
            rows (list of lists), row_height (float, default 0.40),
            warning (str, optional), warning_code (str, optional),
            note (str, optional)
    """
    title        = params.get('title', '')
    section      = params.get('section', '')
    columns      = params.get('columns', [])
    col_widths_f = params.get('col_widths', [])
    data_rows    = params.get('rows', [])
    row_h        = float(params.get('row_height', 0.40))
    warning      = params.get('warning', '')
    warning_code = params.get('warning_code', '')
    note         = params.get('note', '')

    header_bar(s, title, section)
    footer_bar(s, page_num, config.get('project', ''))

    # Subtitle if present
    subtitle = params.get('subtitle', '')
    if subtitle:
        add_textbox(s, subtitle,
                    CX, Inches(1.1), CW, Inches(0.35),
                    size=Pt(16), color=C_NAVY, bold=True, font=FONT_JP)

    # Build full rows (header + data)
    all_rows = [columns] + [list(r) for r in data_rows]
    col_widths_in = [Inches(w) for w in col_widths_f]

    TABLE_START_Y = 1.55
    make_table(s, CX, Inches(TABLE_START_Y), CW, all_rows, col_widths_in,
               row_height=Inches(row_h))

    # Calculate safe y below table
    next_y = safe_y_after_table(TABLE_START_Y, len(all_rows), row_h, margin=0.12)

    # Optional warning box
    if warning:
        warn_h = 1.0 if warning_code else 0.52
        clamp_to_content(next_y, warn_h)
        add_rect(s, Inches(0.35), Inches(next_y), CW, Inches(warn_h),
                 C_RED_LT, C_RED, line_width=1.5)
        add_textbox(s, warning,
                    Inches(0.55), Inches(next_y + 0.06), Inches(12.2), Inches(0.38),
                    size=Pt(13), color=C_RED, bold=True, font=FONT_JP)
        if warning_code:
            add_textbox(s, warning_code,
                        Inches(0.55), Inches(next_y + 0.48), Inches(12.2), Inches(0.4),
                        size=Pt(12), color=C_DARK_TXT, font=FONT_CODE)
        next_y = next_y + warn_h + 0.12

    # Optional note box
    if note:
        note_h = 0.52
        clamp_to_content(next_y, note_h)
        add_rect(s, CX, Inches(next_y), CW, Inches(note_h), C_GRAY_LT)
        add_textbox(s, note,
                    Inches(0.55), Inches(next_y + 0.07), Inches(12.2), Inches(0.38),
                    size=Pt(12), color=C_DARK_TXT, font=FONT_JP)


def render_cost_comparison(prs, s, config: dict, params: dict, page_num: int):
    """
    layout: cost_comparison
    params: title, section, cost_rows, market_rows,
            highlight, sub_highlight, checklist (optional)

    Layout (y-axis, auto-calculated):
      y=1.10  Left/right table labels
      y=1.50  Two tables (left: cost breakdown, right: market comparison)
      y=safe  Navy highlight box
              Orange sub_highlight text
              Gray checklist box
    """
    title         = params.get('title', 'コストメリット')
    section       = params.get('section', '')
    cost_rows     = params.get('cost_rows', [])
    market_rows   = params.get('market_rows', [])
    highlight     = params.get('highlight', '')
    sub_highlight = params.get('sub_highlight', '')
    checklist     = params.get('checklist', [])

    header_bar(s, title, section)
    footer_bar(s, page_num, config.get('project', ''))

    # --- Left table (cost breakdown) ---
    add_textbox(s, "本ツール 品目別コスト",
                Inches(0.35), Inches(1.1), Inches(5.8), Inches(0.35),
                size=Pt(15), color=C_NAVY, bold=True, font=FONT_JP)

    LEFT_ROW_H  = 0.40
    TABLE_START = 1.50
    cost_col_w  = [Inches(2.9), Inches(1.5), Inches(1.4)]
    tbl_left = make_table(s, Inches(0.35), Inches(TABLE_START), Inches(5.8),
                          cost_rows, cost_col_w, row_height=Inches(LEFT_ROW_H))
    # Highlight total row (last data row)
    if len(cost_rows) > 1:
        color_row(tbl_left, len(cost_rows) - 1, C_ORANGE, C_WHITE, bold=True)

    # --- Right table (market comparison) ---
    add_textbox(s, "市販温度ロガーとの価格比較",
                Inches(6.5), Inches(1.1), Inches(6.48), Inches(0.35),
                size=Pt(15), color=C_NAVY, bold=True, font=FONT_JP)

    RIGHT_ROW_H = 0.40
    mkt_col_w   = [Inches(2.8), Inches(2.18), Inches(1.5)]
    tbl_right = make_table(s, Inches(6.5), Inches(TABLE_START), Inches(6.48),
                           market_rows, mkt_col_w, row_height=Inches(RIGHT_ROW_H))
    # Highlight "own tool" row (row 1)
    if len(market_rows) > 1:
        color_row(tbl_right, 1, C_NAVY, C_WHITE, bold=True)

    # --- Calculate safe y below the TALLER of the two tables ---
    left_end  = calc_table_bottom(TABLE_START, len(cost_rows),  LEFT_ROW_H)
    right_end = calc_table_bottom(TABLE_START, len(market_rows), RIGHT_ROW_H)
    box_y = max(left_end, right_end) + 0.12

    # --- Navy highlight box ---
    # Height budget: title line ~0.45" + sub line ~0.38" + padding = 1.0"
    if highlight:
        hl_h = 1.0 if not sub_highlight else 1.0
        clamp_to_content(box_y, hl_h)
        add_rect(s, Inches(0.35), Inches(box_y), Inches(12.6), Inches(hl_h), C_NAVY)
        add_textbox(s, highlight,
                    Inches(0.5), Inches(box_y + 0.07), Inches(12.3), Inches(0.45),
                    size=Pt(20), color=C_WHITE, bold=True, font=FONT_JP,
                    align=PP_ALIGN.CENTER)
        if sub_highlight:
            add_textbox(s, sub_highlight,
                        Inches(0.5), Inches(box_y + 0.56), Inches(12.3), Inches(0.38),
                        size=Pt(14), color=C_ORANGE, font=FONT_JP,
                        align=PP_ALIGN.CENTER)
        box_y = box_y + hl_h + 0.1

    # --- Gray checklist box ---
    if checklist:
        list_h = 0.2 + len(checklist) * 0.35
        clamp_to_content(box_y, list_h)
        add_rect(s, Inches(0.35), Inches(box_y), Inches(12.6), Inches(list_h), C_GRAY_LT)
        add_multiline_textbox(s, checklist,
                              Inches(0.55), Inches(box_y + 0.07),
                              Inches(12.2), Inches(list_h - 0.1),
                              size=Pt(12), color=C_DARK_TXT, font=FONT_JP,
                              align=PP_ALIGN.CENTER)


def render_two_column(prs, s, config: dict, params: dict, page_num: int):
    """
    layout: two_column
    params: title, section, left_title, left_items (list),
            right_title, right_items (list), note (optional)
    """
    title       = params.get('title', '')
    section     = params.get('section', '')
    left_title  = params.get('left_title', '')
    left_items  = params.get('left_items', [])
    right_title = params.get('right_title', '')
    right_items = params.get('right_items', [])
    note        = params.get('note', '')

    header_bar(s, title, section)
    footer_bar(s, page_num, config.get('project', ''))

    # Divider line down the center
    add_rect(s, Inches(6.62), Inches(1.15), Inches(0.04), Inches(5.55), C_GRAY_MID)

    def render_column(x_start, col_title, items):
        add_textbox(s, col_title,
                    Inches(x_start), Inches(1.2), Inches(5.9), Inches(0.38),
                    size=Pt(16), color=C_NAVY, bold=True, font=FONT_JP)
        for i, item in enumerate(items):
            y = 1.7 + i * 0.52
            add_rect(s, Inches(x_start), Inches(y + 0.12),
                     Inches(0.06), Inches(0.25), C_ORANGE)
            add_textbox(s, item,
                        Inches(x_start + 0.18), Inches(y), Inches(5.7), Inches(0.45),
                        size=Pt(13), color=C_DARK_TXT, font=FONT_JP)

    render_column(0.35, left_title, left_items)
    render_column(6.72, right_title, right_items)

    if note:
        # Place note below the longer column's last item
        max_items = max(len(left_items), len(right_items))
        note_y = 1.7 + max_items * 0.52 + 0.2
        note_y = clamp_to_content(note_y, 0.48)
        add_rect(s, CX, Inches(note_y), CW, Inches(0.48), C_GRAY_LT)
        add_textbox(s, note,
                    Inches(0.55), Inches(note_y + 0.07), Inches(12.2), Inches(0.35),
                    size=Pt(12), color=C_DARK_TXT, font=FONT_JP)


# ===========================================================================
# ▼ LAYOUT REGISTRY — maps layout name to renderer function
# ===========================================================================

LAYOUT_RENDERERS = {
    'title':            render_title,
    'toc':              render_toc,
    'section_divider':  render_section_divider,
    'bullet':           render_bullet,
    'table':            render_table,
    'cost_comparison':  render_cost_comparison,
    'two_column':       render_two_column,
}


# ===========================================================================
# ▼ MAIN
# ===========================================================================

def main():
    if len(sys.argv) < 2:
        print("Usage: python tools/spec_to_pptx.py path/to/presentation_spec.md")
        sys.exit(1)

    spec_path = sys.argv[1]
    if not os.path.exists(spec_path):
        print(f"ERROR: spec file not found: {spec_path}")
        sys.exit(1)

    print(f"📖 Reading spec: {spec_path}")
    config, slides = parse_spec(spec_path)

    if not slides:
        print("ERROR: No slides found in spec. Check ## slide_NN | layout: headers.")
        sys.exit(1)

    print(f"   Project    : {config.get('project', '(unnamed)')}")
    print(f"   Slide count: {len(slides)}")
    print()

    prs = Presentation()
    prs.slide_width  = W
    prs.slide_height = H

    for i, slide_def in enumerate(slides, 1):
        layout = slide_def['layout']
        params = slide_def['params']
        renderer = LAYOUT_RENDERERS.get(layout)

        if renderer is None:
            print(f"  [{i:02d}] ⚠ Unknown layout '{layout}' — skipping")
            continue

        s = _blank_slide(prs)
        renderer(prs, s, config, params, i)
        print(f"  [{i:02d}/{len(slides):02d}] layout={layout:<20} ... done")

    # --- Determine output path ---
    output_file = config.get('output', 'output_presentation.pptx')
    spec_dir    = os.path.dirname(os.path.abspath(spec_path))
    output_path = os.path.join(spec_dir, output_file)

    prs.save(output_path)
    print(f"\n✅ 保存完了: {output_path}")
    print(f"   スライド枚数: {len(slides)} 枚")


if __name__ == "__main__":
    main()
