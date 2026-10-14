<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Assets

This directory stores reusable fonts, images, music, and sound effects, organized by asset type.

Keep each asset in the matching subdirectory and document its destination, naming, integration method, and source/license. Do not mix binary assets with Markdown documentation.

## Fonts

`fonts/symbols.txt` is the only hand-maintained file here; it lists every CJK and
symbol codepoint the UI needs. The generated LVGL sources live in `main/fonts/`,
because that is where `main/CMakeLists.txt` compiles them from. They are committed
even though they are generated, so a fresh clone builds without Node.js.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

| File | Character range and format | Use and source |
| --- | --- | --- |
| [`fonts/symbols.txt`](fonts/symbols.txt) | 204 codepoints on one UTF-8 line, no trailing newline | Font inventory in the literal form `lv_font_conv --symbols` expects. `python3 tools/extract_font_inventory.py` regenerates it from the string literals in `main/*.c` and `main/*.h`; `tests/test_cw_strings.py` fails if any non-ASCII codepoint in `main/` is missing from it. |
| [`../main/fonts/cw_font_14.c`](../main/fonts/cw_font_14.c) | Noto Sans SC subset, 14 px, bpp 4, uncompressed LVGL C source (~27 KB Flash, 178 KB of generated text) | ClassWhiz body font, returned by `cw_font_body()` with `lv_font_montserrat_14` as fallback. Covers ASCII 0x20–0x7E, U+00B0, U+00B7 and the codepoints in `fonts/symbols.txt`. Line height 17, baseline 4 — the per-screen layout budgets documented in `main/cw_ui.h` assume these numbers. Source: [Noto Sans SC](https://github.com/google/fonts/tree/main/ofl/notosanssc) (SIL Open Font License 1.1). Converted with `lv_font_conv` 1.5.3; the exact command line is recorded in the header comment of the generated file. |
| [`../main/fonts/cw_font_16.c`](../main/fonts/cw_font_16.c) | Noto Sans SC subset, 16 px, bpp 4, uncompressed LVGL C source (~33 KB Flash, 212 KB of generated text) | ClassWhiz title font, returned by `cw_font_title()` with `lv_font_montserrat_16` as fallback. Line height 20, baseline 5. Same coverage, source, and tool as above. Used only for the screen title bar; list-card titles use the body font per the design mockup. |

After changing any UI string in `main/`, regenerate both fonts: run
`python3 tools/extract_font_inventory.py`, then re-run the `lv_font_conv` command
from the header comment of each generated file (keeping its `main/fonts/...`
output path) against a Noto Sans SC Regular TTF/OTF.

## Images

Store reusable source images and generated display assets in `images/`.

| File | Dimensions and format | Use and source |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160, JPEG | Product hero image embedded in both project README files to foreground AI Passport and its open, maker-oriented identity. |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724, PNG RGBA | Optional technical infographic retained as a reference asset; it is no longer used as the homepage hero. Generated for this repository with the built-in image generation tool on 2026-09-17; the six labels and values were checked against the documented hardware contract. |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336, PNG RGBA | Transparent black wordmark extracted from the repository's original `images/logo.png`; embedded in both project README files for light backgrounds. |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336, PNG RGBA | White version of the extracted wordmark, used by the README `<picture>` element when GitHub is in dark mode. |

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

## Music and sound effects

Store reusable music and sound-effect sources in `music/`.

- Document the source, license, sample rate, bit depth, channels, conversion command, and destination.
- Prefer 16 kHz, 16-bit mono PCM when it matches the current BSP audio path.
- Check Flash and internal-RAM cost before embedding audio; stream or chunk long recordings.
- Do not commit media without redistribution permission.
