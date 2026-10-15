#!/usr/bin/env python3
"""Host test: every cw_str ID has non-empty EN+ZH text, and every non-ASCII
codepoint used in main/ UI strings is covered by assets/fonts/symbols.txt
(the lv_font_conv inventory)."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

STR_RE = re.compile(r'"(?:[^"\\]|\\.)*"')


def main() -> int:
    failures = 0

    # 1. 字符串表:EN/ZH 配对且非空
    data_c = (ROOT / "main" / "cw_data.c").read_text(encoding="utf-8")
    data_h = (ROOT / "main" / "cw_data.h").read_text(encoding="utf-8")
    ids = re.findall(r"\[CW_S_([A-Z0-9_]+)\]", data_c)
    en_ids = set(re.findall(r"\[CW_S_([A-Z0-9_]+)\] = \"", data_c.split("S_EN[")[1]))
    # 简单统计:两个表应覆盖相同 ID 集合
    s_en = data_c.split("static const char *const S_EN")[1].split("};")[0]
    s_zh = data_c.split("static const char *const S_ZH")[1].split("};")[0]
    en = set(re.findall(r"\[CW_S_([A-Z0-9_]+)\] = \"", s_en))
    zh = set(re.findall(r"\[CW_S_([A-Z0-9_]+)\] = \"", s_zh))
    enum_body = data_h.split("} cw_str_id_t;")[0].split("typedef enum")[-1]
    enum_ids = set(re.findall(r"\bCW_S_([A-Z0-9_]+)\b", enum_body))
    enum_ids.discard("COUNT")
    if en != enum_ids:
        print(f"FAIL: EN table mismatch: {sorted(enum_ids - en)}")
        failures += 1
    if zh != enum_ids:
        print(f"FAIL: ZH table mismatch: {sorted(enum_ids - zh)}")
        failures += 1
    for table, name in ((s_en, "EN"), (s_zh, "ZH")):
        for mid, text in re.findall(r"\[CW_S_([A-Z0-9_]+)\] = \"((?:[^\"\\]|\\.)*)\"", table):
            if not text:
                print(f"FAIL: empty {name} string for CW_S_{mid}")
                failures += 1
    # 英文表不应含 CJK(中英分离检查)
    cjk = re.compile(r"[\u4e00-\u9fff]")
    for mid, text in re.findall(r"\[CW_S_([A-Z0-9_]+)\] = \"((?:[^\"\\]|\\.)*)\"", s_en):
        if cjk.search(text):
            print(f"FAIL: EN string CW_S_{mid} contains CJK: {text}")
            failures += 1

    # 2. 字形覆盖:main/ 字符串字面量中的非 ASCII 码位必须在 symbols.txt 中
    symbols = set((ROOT / "assets" / "fonts" / "symbols.txt").read_text(encoding="utf-8"))
    used: set[str] = set()
    for path in sorted((ROOT / "main").glob("*.c")) + sorted((ROOT / "main").glob("*.h")):
        # 门户 HTML 在手机浏览器里渲染,不经过 LVGL,不参与设备字形覆盖检查
        if path.name == "cw_portal_page.h":
            continue
        for m in STR_RE.finditer(path.read_text(encoding="utf-8")):
            for ch in m.group(0):
                if ord(ch) > 0x7E and not ch.isspace():
                    used.add(ch)
    missing = sorted(used - symbols - {'"', "\\"})
    # 转义序列已解码?STR_RE 保留原文;\" 会产生 '"' 字符,已排除
    missing = [c for c in missing if c not in ('"',)]
    if missing:
        print(f"FAIL: {len(missing)} codepoints not in symbols.txt: "
              + " ".join(f"U+{ord(c):04X}" for c in missing[:10]))
        failures += 1

    if failures == 0:
        print(f"test_cw_strings: PASS ({len(enum_ids)} ids, {len(used)} non-ASCII codepoints covered)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
