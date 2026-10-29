#!/usr/bin/env python3
"""Copies the generated factory presets (src/PresetsGen.h) into docs/mockup/purrone-mockup.html so the mockup lists the same
banks and sounds as the plugin. Usage: tools/export_mockup_presets.py   (run after tools/gen_presets.py)"""
import re, json, os
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
gen = open(os.path.join(ROOT, "src", "PresetsGen.h")).read()
items = []
for m in re.finditer(r'\{\s*"([^"]+)",\s*"([^"]+)",\s*\{(.*?)\},\s*"([^"]*)"\s*\},', gen, re.S):
    vals = {k: float(v) for k, v in re.findall(r'\{"(\w+)",\s*(-?[\d.]+)f\}', m.group(3))}
    items.append([m.group(1), m.group(2), vals, m.group(4)])
def num(x): return int(x) if x == int(x) else round(x, 4)
rows = ["[%s,%s,{%s},%s]" % (json.dumps(n), json.dumps(c), ",".join("%s:%s" % (k, num(v)) for k, v in vals.items()), json.dumps(sp)) for n, c, vals, sp in items]
block = "/* GEN-PRESETS-BEGIN: written by tools/export_mockup_presets.py, do not edit */\n  var GEN = [\n" + ",\n".join(rows) + "\n  ];\n  GEN.forEach(function (g) { FACTORY.push({ name: g[0], cat: g[1], p: g[2], spec: g[3] }); });\n  /* GEN-PRESETS-END */"
p = os.path.join(ROOT, "docs", "mockup", "purrone-mockup.html"); h = open(p).read()
if "GEN-PRESETS-BEGIN" in h:
    h = re.sub(r"/\* GEN-PRESETS-BEGIN.*?GEN-PRESETS-END \*/", lambda _: block, h, flags=re.S)
else:
    a = "  ];\n  var USER = [], cur = 0, dirty = false;"
    assert a in h; h = h.replace(a, "  ];\n  " + block + "\n  var USER = [], cur = 0, dirty = false;", 1)
open(p, "w").write(h); print(len(items), "presets copied")
