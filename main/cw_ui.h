// cw_ui.h — ClassWhiz UI 主题与控件助手(LVGL 9.5)。
// 所有界面共用:颜色、字体(CJK 子集 + Montserrat 回退)、状态栏、卡片、标签。
#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// 主题色(与设计稿 v3 对应)
#define CW_C_BG        0x0B101C
#define CW_C_CARD      0x182238
#define CW_C_CARD_LINE 0x24304D
#define CW_C_TEXT      0xEEF2FA
#define CW_C_MUTED     0x8A94A8
#define CW_C_ACCENT    0x4C8DFF
#define CW_C_GREEN     0x34C77B
#define CW_C_AMBER     0xFFB020
#define CW_C_RED       0xFF5D5D
#define CW_C_PURPLE    0x8B5CF6
#define CW_C_TEAL       0x22B8A8
#define CW_C_PINK       0xFF8CC3

#define CW_W 240
#define CW_H 320

// 生成字体(assets/fonts/cw_font_14.c 等由 lv_font_conv 产生)
LV_FONT_DECLARE(cw_font_14);
LV_FONT_DECLARE(cw_font_16);

// 初始化字体描述符(含 Montserrat 回退链),在 LVGL 初始化后调用一次。
void cw_ui_init_fonts(void);
// 带回退的 14/16 号字体(正文/标题)
const lv_font_t *cw_font_body(void);
const lv_font_t *cw_font_title(void);

// 创建 240x320 屏幕并加载;返回内容容器(已留出状态栏与标题区)。
// title 为 NULL 时不画标题栏。
lv_obj_t *cw_screen_create(const char *title);
// 屏幕内容容器(状态栏/标题/提示条之外的区域)
lv_obj_t *cw_screen_content(lv_obj_t *scr);
// 状态栏:时间 + 电量,需在 tick 中调用更新
void cw_ui_status_tick(lv_obj_t *scr, const char *time_str, int bat_pct);
// 卡片容器
lv_obj_t *cw_card(lv_obj_t *parent);
// 正文标签(自动选 CJK 字体)
lv_obj_t *cw_label(lv_obj_t *parent, const char *text);
lv_obj_t *cw_label_small(lv_obj_t *parent, const char *text); // muted 小字
// 彩色胶囊标签
lv_obj_t *cw_tag(lv_obj_t *parent, const char *text, uint32_t color);
// 底部提示条
void cw_hint_bar(lv_obj_t *scr, const char *left, const char *right);
// 渐变主按钮
lv_obj_t *cw_button(lv_obj_t *parent, const char *text);
// 设置页行
lv_obj_t *cw_set_row(lv_obj_t *parent, const char *name);
// 分段选择器;返回容器,选项用 cw_seg_add
lv_obj_t *cw_seg(lv_obj_t *parent);
lv_obj_t *cw_seg_add(lv_obj_t *seg, const char *text, bool selected);
// 模拟时钟(表盘 + 时/分/秒针);tick 时调 cw_clock_tick 更新
lv_obj_t *cw_analog_clock(lv_obj_t *parent, int size, uint32_t accent);
void cw_clock_tick(lv_obj_t *clock, int hour24, int minute, int second);
// 天气图标:kind 0=多云 1=晴 2=雨;返回容器(带漂移动画)
lv_obj_t *cw_weather_icon(lv_obj_t *parent, int kind, int size);
// 呼吸透明度动画(用于角标/提醒)
void cw_anim_pulse(lv_obj_t *obj);
// 绝对 x 位移动画回调(流光扫过等)
void anim_abs_x_cb(void *var, int32_t v);
// 删除屏幕前停掉其全部动画与定时器
void cw_screen_teardown(lv_obj_t *scr);
// 内容区滚动开关(内容可能溢出的页面启用)
void cw_screen_set_scroll(lv_obj_t *scr, bool en);

#ifdef __cplusplus
}
#endif
