<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# ClassWhiz

FoloToy AI Passport 上的课程助手固件：ESP32-C3、240 × 320 RGB565 屏、三个按键、
无触摸、无 PSRAM。页面包括今日/首页、应用九宫格、课表（日列表 + 周视图）、教师、
日历、学生证、作业、世界时钟、番茄钟、语音备忘、设置，中英文双语界面。

## 本仓库包含什么

只有应用层：

| 路径 | 内容 |
| --- | --- |
| `main/` | `main.c` 入口、`cw_app.*` 屏幕栈与按键分发、`cw_ui.*` 控件套件、`cw_data.*`/`cw_store.*` 演示数据与 NVS 设置、`cw_s_*.c` 各屏、`fonts/cw_font_14.c` 与 `fonts/cw_font_16.c` 生成的中文子集 |
| `assets/fonts/symbols.txt` | 生成字体所用的码位清单 |
| `sdkconfig.defaults` | 目标芯片、Flash 容量、分区表、控制台、LVGL、NimBLE 配置 |
| `tests/test_cw_data.c`、`tests/test_cw_pomodoro.c`、`tests/test_cw_strings.py` | 不依赖 ESP-IDF/LVGL 部分的主机测试 |
| `tools/extract_font_inventory.py` | 从 `main/` 的字符串字面量重新生成 `assets/fonts/symbols.txt` |

## 有意不包含什么

没有板级支持包、没有根 `CMakeLists.txt`、没有 `partitions.csv`、没有 `docs/`、
没有 `.github/`。**本仓库无法单独构建** —— 它是一个叠加到 AI Passport 载体树上的
应用；BSP、工程骨架，以及 `main.c` 需要的 `main/key_guard.h`（深睡按键守卫，纯头文件）
都由载体树提供。

拆分时还带进来一批载体级文件，它们对不上这个切片：`AGENTS.md`、
`tools/validate.sh`、`tools/check_repo.py`，以及
`tests/test_demo_*`、`test_key_guard.c`、`test_ui_pixel_math.c`、
`test_verify_firmware.py` 这些主机测试，引用的都是载体的 demo 外壳、BSP、文档与 CI
布局。请在载体树里跑，不要在这里跑。

## 构建

前置条件：ESP-IDF 5.5.3，以及一棵来自
[`FoloToy/ai-passport`](https://gitee.com/FoloToy/ai-passport) 的载体树，它提供
`components/bsp`（显示、音频、按键、电池、LVGL port，以及托管组件清单）、
`main/key_guard.h`、根 `CMakeLists.txt` 与 `partitions.csv`（NVS + PHY + 占满剩余
8 MB 的单个 factory app）。

叠加时不要弄脏载体的工作副本：

```bash
git clone https://gitee.com/FoloToy/ai-passport.git carrier
rsync -a carrier/ build-tree/
rm -rf build-tree/main
cp -R /path/to/ClassWhiz/main build-tree/main
cp carrier/main/key_guard.h build-tree/main/          # ClassWhiz 不带这个文件
cp /path/to/ClassWhiz/sdkconfig.defaults build-tree/
cd build-tree
idf.py set-target esp32c3
idf.py build
```

`main/CMakeLists.txt` 只编译它列出的源文件，所以载体 `main/` 里残留的其他 `.c`
不会被带进来。当前代码树的实测结果：镜像 845 KB，DRAM 占用 46 %（LVGL 内存池
`CONFIG_LV_MEM_SIZE_KILOBYTES=64`）。

## 界面约束

- 内容区高度：无标题栏 **274 px**，有标题栏 **244 px**（320 − 22 状态栏 −
  30 标题栏 − 24 提示条）。没有触摸，内容区滚不动，超出预算就会被裁掉 ——
  应当限制条目数量或压缩行高。各屏的算法写在对应 `cw_s_*.c` 的文件头注释与
  `main/cw_ui.h` 里。
- 上述算法用到的控件高度：正文字体行高 17 px，标题字体行高 20 px，`cw_tag`
  23 px，`cw_set_row` 34 px，正文容器间距 8 px。
- 按键：`OK` 单击确认，`OK` 长按返回（截获该行为的页面必须在提示条里写清退出方式），
  `DN` 长按回首页，`UP`/`DN` 移动。双击只在课表的视图切换上有意义，其余页面由
  `cw_app.c` 归一化为单击。
- 任何新增中文字符串都必须走 [`assets/README.md`](assets/README.md) 里的字体流程，
  否则会显示成方块。

## 测试

```bash
cc -std=c11 -Wall -Wextra -Werror -Imain tests/test_cw_data.c main/cw_data.c -o /tmp/t && /tmp/t
cc -std=c11 -Wall -Wextra -Werror -Imain tests/test_cw_pomodoro.c main/cw_pomodoro.c -o /tmp/t && /tmp/t
python3 tests/test_cw_strings.py
```

`test_cw_strings.py` 会在某个 `cw_str` ID 缺少任一语言文本，或 `main/` 里出现
`assets/fonts/symbols.txt` 之外的非 ASCII 码位时失败。
