#!/usr/bin/env python3
"""Builds the short PDF manual of PurrOne from docs/mockup/purrone-mockup.html (control names, ranges, info-bubble texts,
factory presets) plus docs/manual.txt (intro and tips). Usage: python3 tools/make_manual.py <mockup.html> <docs/manual.txt> <out.pdf>
Needs: pip install reportlab"""
import re, sys
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, KeepTogether

html_path, txt_path, out = sys.argv[1:4]
src = open(html_path, encoding="utf-8").read()
name = "PurrOne"
S = r'"((?:[^"\\]|\\.)*)"'
N = r'(-?[\d.]+)'
sections = {"secMod": "Modulation", "secA": "Osc A", "secB": "Osc B", "secMix": "Mixer", "secFilter": "Filter and drive", "secFenv": "Filter envelope",
            "secAenv": "Amp envelope", "secLfo": "LFO", "secKeys": "Glide and mode", "secEq": "EQ", "secVerb": "Space", "secOut": "Master"}
panels = {k: [] for k in sections}
for m in re.finditer(r'\bK\("(\w+)",\s*"(\w+)",\s*"([^"]+)",\s*' + N + r',\s*' + N + r',\s*' + N + r'(.*?)\);', src):
    id_, sec, label, lo, hi, de, rest = m.groups()
    tip = re.search(r'tip:\s*' + S, rest)
    if sec in panels: panels[sec].append((label, "%s to %s, default %s" % (lo, hi, de), tip.group(1) if tip else ""))
for m in re.finditer(r'\bT\("(\w+)",\s*"(\w+)",\s*"([^"]+)",\s*(true|false)(.*?)\);', src):
    id_, sec, label, de, rest = m.groups()
    tip = re.search(r'tip:\s*' + S, rest)
    if sec in panels: panels[sec].append((label, "switch, default " + de, tip.group(1) if tip else ""))
for m in re.finditer(r'\bG\("(\w+)",\s*"(\w+)",\s*"([^"]+)",\s*\[([^\]]*)\],\s*(\d+)(?:,\s*' + S + r')?\);', src):
    id_, sec, label, opts, de, tip = m.groups()
    items = [o.strip().strip('"') for o in opts.split(",")]
    if sec in panels: panels[sec].append((label, "choice: " + ", ".join(items) + " (default " + items[int(de)] + ")", tip or ""))
presets = re.findall(r'\bP\("([^"]+)",\s*"(\w+)"', src)

sec = {}; cur = None
for line in open(txt_path, encoding="utf-8").read().splitlines():
    if line.startswith("# "): cur = line[2:].strip(); sec[cur] = []
    elif cur is not None: sec[cur].append(line)
para = lambda k: [p.strip() for p in "\n".join(sec.get(k, [])).split("\n\n") if p.strip()]
bullets = lambda k: [l[2:].strip() for l in sec.get(k, []) if l.startswith("- ")]

BG, PANEL = colors.HexColor("#161638"), colors.HexColor("#2c2c62")
AMBER, LILAC, PINK, INK, DIM = colors.HexColor("#ffa600"), colors.HexColor("#e1cbf4"), colors.HexColor("#ff7bb1"), colors.HexColor("#dcdaf5"), colors.HexColor("#9b98c8")
st = lambda n, **k: ParagraphStyle(n, fontName=k.pop("font", "Helvetica"), **k)
H1 = st("h1", fontSize=24, leading=28, textColor=AMBER, font="Helvetica-Bold")
H2 = st("h2", fontSize=14, leading=18, textColor=LILAC, font="Helvetica-Bold", spaceBefore=10, spaceAfter=4)
B = st("b", fontSize=10, leading=14, textColor=INK)
SM = st("s", fontSize=8.5, leading=11.5, textColor=DIM)
CELL = st("c", fontSize=9, leading=12, textColor=INK)
CELLB = st("cb", fontSize=9, leading=12, textColor=PINK, font="Helvetica-Bold")
esc = lambda t: t.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")

story = [Paragraph(name, H1), Paragraph("Short manual . free and open source (AGPL-3.0-or-later) . github.com/SpeakingColors/PurrOne", SM), Spacer(1, 6)]
for p in para("Intro"): story += [Paragraph(esc(p), B), Spacer(1, 4)]
story.append(Paragraph("Quick start", H2))
for t in bullets("Quick start"): story.append(Paragraph("&bull; " + esc(t), B))
story.append(Paragraph("Controls", H2))
for key, title in sections.items():
    rows = [[Paragraph(esc(a), CELLB), Paragraph(esc(b), CELL), Paragraph(esc(c), CELL)] for a, b, c in panels[key]]
    if not rows: continue
    t = Table(rows, colWidths=[28 * mm, 52 * mm, 90 * mm])
    t.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"), ("BACKGROUND", (0, 0), (-1, -1), PANEL), ("LINEBELOW", (0, 0), (-1, -1), 0.5, BG),
                           ("TOPPADDING", (0, 0), (-1, -1), 5), ("BOTTOMPADDING", (0, 0), (-1, -1), 5), ("LEFTPADDING", (0, 0), (-1, -1), 6)]))
    story.append(KeepTogether([Paragraph(esc(title), st("ph", fontSize=11.5, leading=15, textColor=LILAC, font="Helvetica-Bold", spaceBefore=8, spaceAfter=4)), t]))
tips = bullets("Recommendations")
if tips:
    story.append(Paragraph("Recommendations", H2))
    for t in tips: story.append(Paragraph("&bull; " + esc(t), B))
if presets:
    story.append(Paragraph("Factory presets", H2))
    story.append(Paragraph(esc(", ".join(p[0] for p in presets)), B))
story.append(Paragraph("Install, remove and your saved presets", H2))
for t in bullets("Install"): story.append(Paragraph("&bull; " + esc(t), B))

def footer(c, d):
    c.saveState(); c.setFillColor(BG); c.rect(0, 0, A4[0], A4[1], stroke=0, fill=1)
    c.setFillColor(PANEL); c.rect(0, A4[1] - 5 * mm, A4[0], 5 * mm, stroke=0, fill=1)
    c.setFillColor(AMBER); c.rect(0, A4[1] - 5.8 * mm, A4[0], 0.8 * mm, stroke=0, fill=1)
    c.setFont("Helvetica", 8); c.setFillColor(DIM)
    c.drawString(20 * mm, 10 * mm, name + " manual"); c.drawRightString(190 * mm, 10 * mm, "page %d" % d.page); c.restoreState()
SimpleDocTemplate(out, pagesize=A4, leftMargin=20 * mm, rightMargin=20 * mm, topMargin=16 * mm, bottomMargin=18 * mm, title=name + " manual", author="Studio 87").build(story, onFirstPage=footer, onLaterPages=footer)
print("wrote", out, "controls", sum(len(v) for v in panels.values()), "presets", len(presets))
