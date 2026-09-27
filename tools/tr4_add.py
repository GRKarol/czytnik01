#!/usr/bin/env python3
"""Append TrKey4 strings: enum member in Translations.h + CSV row.

Usage: python tools/tr4_add.py entries.txt
Each line: Key|pl|en|es|fr|de|ro   (existing keys are updated in place).
Then run tools/gen_translations.py.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
H = ROOT / "firmware/src/app/Translations.h"
CSV = ROOT / "tools/translations.csv"

entries = []
for line in Path(sys.argv[1]).read_text(encoding="utf-8").splitlines():
    if not line.strip() or line.startswith("#"):
        continue
    parts = line.split("|")
    if len(parts) != 7:
        sys.exit(f"bad line: {line}")
    entries.append([p.strip() if i == 0 else p for i, p in enumerate(parts)])

h = H.read_text(encoding="utf-8")
m = re.search(r"(enum class TrKey4 : uint8_t \{)(.*?)(\};)", h, re.S)
existing = [l.split("//")[0].strip().rstrip(",") for l in m.group(2).splitlines()]
existing = [e for e in existing if e]
body = m.group(2)
for key, *_ in entries:
    if key not in existing:
        body = body.rstrip() + f"\n  {key},\n"
        existing.append(key)
h = h[: m.start(2)] + body + h[m.end(2):]
H.write_text(h, encoding="utf-8")

rows = CSV.read_text(encoding="utf-8").splitlines()
index = {r.split(";", 1)[0]: i for i, r in enumerate(rows)}
for key, pl, en, es, fr, de, ro in entries:
    row = ";".join([f"TrKey4.{key}", pl, en, es, fr, de, ro])
    if f"TrKey4.{key}" in index:
        rows[index[f"TrKey4.{key}"]] = row
    else:
        rows.append(row)
CSV.write_text("\n".join(rows) + "\n", encoding="utf-8")
print(f"{len(entries)} entries, TrKey4 has {len(existing)} keys")
