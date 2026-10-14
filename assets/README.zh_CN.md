<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 资源目录（Assets）

本目录集中存放可复用的资源（字库、图片、音乐等），按资源类型分子目录管理。每个资源放在其类型对应的子目录，并记录放置路径、命名方式、集成方式与来源/许可。二进制资源（字体、图片、音频）不属于纯 markdown 文档，请勿与文档混放。涉及版权/授权的资源需注明来源与许可。

## 字库（fonts）

这里唯一需要手工维护的是 `fonts/symbols.txt`，它列出界面用到的全部 CJK 与符号码位。
生成的 LVGL 字库源码放在 `main/fonts/`，因为 `main/CMakeLists.txt` 从那里编译它们。
它们是生成物，但仍然入库，这样新克隆的仓库不装 Node.js 也能直接构建。

- 命名要能反映字族、字重、字级与格式。
- 记录来源、许可、字符范围、转换命令与目标放置路径。
- 添加字库前评估 Flash 与内部 RAM 影响；ESP32-C3 无 PSRAM。
- 不提交许可不允许分发的字库。

| 文件 | 字符范围与格式 | 用途与来源 |
| --- | --- | --- |
| [`fonts/symbols.txt`](fonts/symbols.txt) | 204 个码位，单行 UTF-8，无结尾换行 | 字库清单，格式即 `lv_font_conv --symbols` 需要的字面形式。`python3 tools/extract_font_inventory.py` 从 `main/*.c` 与 `main/*.h` 的字符串字面量重新生成它；`tests/test_cw_strings.py` 会在 `main/` 出现清单外的非 ASCII 码位时失败。 |
| [`../main/fonts/cw_font_14.c`](../main/fonts/cw_font_14.c) | Noto Sans SC 子集，14 px，bpp 4，未压缩 LVGL C 源码（约 27 KB Flash，178 KB 生成文本） | ClassWhiz 正文字体，由 `cw_font_body()` 返回，回退字体为 `lv_font_montserrat_14`。覆盖 ASCII 0x20–0x7E、U+00B0、U+00B7 及 `fonts/symbols.txt` 中的码位。行高 17、基线 4 —— `main/cw_ui.h` 中各屏布局预算就是按这两个数字算的。来源：[Noto Sans SC](https://github.com/google/fonts/tree/main/ofl/notosanssc)（SIL Open Font License 1.1）。用 `lv_font_conv` 1.5.3 转换；完整命令行记录在生成文件的头部注释里。 |
| [`../main/fonts/cw_font_16.c`](../main/fonts/cw_font_16.c) | Noto Sans SC 子集，16 px，bpp 4，未压缩 LVGL C 源码（约 33 KB Flash，212 KB 生成文本） | ClassWhiz 标题字体，由 `cw_font_title()` 返回，回退字体为 `lv_font_montserrat_16`。行高 20、基线 5。覆盖范围、来源与工具同上。只用于屏幕标题栏；按设计稿，列表卡片标题用正文字体。 |

改动 `main/` 中任何界面字符串后，需要重新生成两个字库：先运行
`python3 tools/extract_font_inventory.py`，再按生成文件头部注释里的 `lv_font_conv`
命令（保持其 `main/fonts/...` 输出路径）对 Noto Sans SC Regular 的 TTF/OTF 重新转换。

## 图片（images）

可复用的源图与生成的显示资产放在 `images/`。

| 文件 | 尺寸与格式 | 用途与来源 |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160，JPEG | 嵌入中英文项目 README 的产品主图，突出 AI Passport 产品形象与开放、人人可创作的理念。 |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724，PNG RGBA | 保留为可选技术参考图，不再用于首页主视觉。于 2026-09-17 使用内置图像生成工具为本仓库生成；已根据文档中的硬件能力契约核对图中的六项标签与参数。 |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336，PNG RGBA | 从仓库原始 `images/logo.png` 中精确裁切并去除背景的黑色字标；用于中英文项目 README 的浅色主题。 |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336，PNG RGBA | 提取字标的白色版本；README 使用 `<picture>` 在 GitHub 深色主题下显示。 |

- 使用描述性命名，并记录尺寸、像素格式、转换步骤与目标路径。
- 优先采用适合 240 × 320 RGB565 显示的格式，并纳入 Flash 与内部 RAM 考量。
- 许可允许时保留可编辑源文件，并记录来源与许可。
- 图片中不得包含设备二维码秘密、凭证或个人数据。

## 音乐与音效（music）

可复用的音乐与音效源码放在 `music/`。

- 记录来源、许可、采样率、位深、声道、转换命令与目标路径。
- 与当前 BSP 音频路径匹配时优先采用 16 kHz、16 位单声道 PCM。
- 嵌入音频前评估 Flash 与内部 RAM 成本；长录音应流式或分块。
- 无再分发许可不提交媒体文件。
