#!/usr/bin/env python3
"""Extract the CJK/ASCII codepoint inventory needed by ClassWhiz UI strings.

Scans main/*.c and main/*.h for non-ASCII characters and emits a
lv_font_conv-compatible --symbols range file plus a human-readable report.
Run from the repository root:

    python3 tools/extract_font_inventory.py

Outputs:
    assets/fonts/inventory.txt  — ranges like "U+4E00-U+9FFF" (not used directly)
    assets/fonts/symbols.txt    — literal characters for lv_font_conv --symbols
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "main"
OUT_DIR = ROOT / "assets" / "fonts"


def is_cjk(ch: str) -> bool:
    o = ord(ch)
    return (
        0x2E80 <= o <= 0x2EFF      # CJK Radicals Supplement
        or 0x3000 <= o <= 0x303F   # CJK Symbols and Punctuation
        or 0x3040 <= o <= 0x309F   # Hiragana (not used, kept for completeness)
        or 0x31F0 <= o <= 0x31FF   # Katakana Phonetic Extensions
        or 0x3400 <= o <= 0x4DBF   # CJK Ext A
        or 0x4E00 <= o <= 0x9FFF   # CJK Unified Ideographs
        or 0xF900 <= o <= 0xFAFF   # CJK Compatibility Ideographs
        or 0xFF00 <= o <= 0xFFEF   # Halfwidth and Fullwidth Forms
    )


def main() -> int:
    chars: set[str] = set()
    str_re = re.compile(r'"(?:[^"\\]|\\.)*"')
    # ASCII printable is always included by lv_font_conv --symbols ranges below
    for path in sorted(MAIN.glob("*.c")) + sorted(MAIN.glob("*.h")):
        text = path.read_text(encoding="utf-8")
        # 只扫描字符串字面量,排除注释
        for m in str_re.finditer(text):
            for ch in m.group(0):
                if ord(ch) > 0x7E and not ch.isspace() and is_cjk(ch):
                    chars.add(ch)
                elif 0xA0 <= ord(ch) <= 0x2FFF and not ch.isspace():
                    # other punctuation/symbols actually used (e.g. · — °)
                    chars.add(ch)

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    symbols_path = OUT_DIR / "symbols.txt"
    # lv_font_conv --symbols takes literal characters; keep ASCII range separate
    symbols_path.write_text("".join(sorted(chars)), encoding="utf-8")

    print(f"CJK/symbol codepoints used in main/: {len(chars)}")
    for ch in sorted(chars):
        print(f"  U+{ord(ch):04X} {ch}")
    print(f"wrote {symbols_path}")
    if not chars:
        print("WARNING: no non-ASCII characters found; check the scan", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
