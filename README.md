<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# ClassWhiz

Course-assistant firmware for the FoloToy AI Passport: ESP32-C3, 240 × 320 RGB565
display, three buttons, no touch, no PSRAM. Screens: today/home, apps grid,
schedule (day list + week grid), teachers, calendar, student ID, tasks, world
clock, pomodoro, voice memo, settings. English and Simplified Chinese UI.

## What this repository contains

The application layer only:

| Path | Contents |
| --- | --- |
| `main/` | `main.c` entry point, `cw_app.*` screen stack and key routing, `cw_ui.*` widget kit, `cw_data.*`/`cw_store.*` demo data and NVS-backed settings, `cw_s_*.c` screens, `fonts/cw_font_14.c` + `fonts/cw_font_16.c` generated CJK subsets |
| `assets/fonts/symbols.txt` | The codepoint inventory the generated fonts are built from |
| `sdkconfig.defaults` | Target, flash size, partition table, console, LVGL, NimBLE settings |
| `tests/test_cw_data.c`, `tests/test_cw_pomodoro.c`, `tests/test_cw_strings.py` | Host tests for the parts that do not need ESP-IDF or LVGL |
| `tools/extract_font_inventory.py` | Regenerates `assets/fonts/symbols.txt` from the string literals in `main/` |

## What it deliberately does not contain

No board support package, no root `CMakeLists.txt`, no `partitions.csv`, no
`docs/`, no `.github/`. **This repository cannot be built on its own** — it is an
application that is overlaid onto an AI Passport tree, which supplies the BSP,
the project scaffolding, and `main/key_guard.h` (the header-only deep-sleep key
guard that `main.c` includes).

The split also inherited carrier-level files that do not work against this slice:
`AGENTS.md`, `tools/validate.sh`, `tools/check_repo.py`, and the
`tests/test_demo_*`/`test_key_guard.c`/`test_ui_pixel_math.c`/`test_verify_firmware.py`
host tests all reference the carrier's demo shell, BSP, docs, and CI layout. Run
them from the carrier tree, not here.

## Building

Requirements: ESP-IDF 5.5.3, and a carrier tree from
[`FoloToy/ai-passport`](https://gitee.com/FoloToy/ai-passport) providing
`components/bsp` (display, audio, button, battery, LVGL port, and the managed
component manifest), `main/key_guard.h`, the root `CMakeLists.txt`, and
`partitions.csv` (NVS + PHY + one factory app over the remaining 8 MB).

Overlay without touching the carrier checkout:

```bash
git clone https://gitee.com/FoloToy/ai-passport.git carrier
rsync -a carrier/ build-tree/
rm -rf build-tree/main
cp -R /path/to/ClassWhiz/main build-tree/main
cp carrier/main/key_guard.h build-tree/main/          # ClassWhiz does not ship it
cp /path/to/ClassWhiz/sdkconfig.defaults build-tree/
cd build-tree
idf.py set-target esp32c3
idf.py build
```

`main/CMakeLists.txt` compiles only the sources it lists, so any other `.c` files
left in the carrier's `main/` are ignored. Measured with the current tree: image
845 KB, DRAM 46 % used (LVGL pool `CONFIG_LV_MEM_SIZE_KILOBYTES=64`).

## UI invariants

- Content area height is **274 px** without a title bar and **244 px** with one
  (320 − 22 status bar − 30 title bar − 24 hint bar). Without touch the content
  area cannot scroll, so anything over budget is clipped: cap item counts or
  compact rows instead. The per-screen arithmetic is in each `cw_s_*.c` header
  comment and in `main/cw_ui.h`.
- Widget heights used by that arithmetic: body font line 17 px, title font line
  20 px, `cw_tag` 23 px, `cw_set_row` 34 px, body gap 8 px.
- Keys: `OK` click confirms, long `OK` goes back (except on screens that capture
  it, which must say so in their hint bar), long `DN` goes home, `UP`/`DN` move.
  Double click is only meaningful on the schedule view toggle; `cw_app.c`
  normalizes it to a click everywhere else.
- Any new Chinese string must go through the font pipeline in
  [`assets/README.md`](assets/README.md), or it renders as a box.

## Tests

```bash
cc -std=c11 -Wall -Wextra -Werror -Imain tests/test_cw_data.c main/cw_data.c -o /tmp/t && /tmp/t
cc -std=c11 -Wall -Wextra -Werror -Imain tests/test_cw_pomodoro.c main/cw_pomodoro.c -o /tmp/t && /tmp/t
python3 tests/test_cw_strings.py
```

`test_cw_strings.py` fails if a `cw_str` ID is missing from either language table
or if a non-ASCII codepoint in `main/` is absent from `assets/fonts/symbols.txt`.
