// cw_ui.c — ClassWhiz UI 主题与控件助手实现。
#include "cw_ui.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 带 Montserrat 回退的字体描述符(应用生命周期内有效)
static lv_font_t s_font_body;
static lv_font_t s_font_title;
static bool s_fonts_ready;

void cw_ui_init_fonts(void) {
    if (s_fonts_ready) return;
    s_font_body = cw_font_14;
    s_font_body.fallback = &lv_font_montserrat_14;
    s_font_title = cw_font_16;
    s_font_title.fallback = &lv_font_montserrat_16;
    s_fonts_ready = true;
}

const lv_font_t *cw_font_body(void) {
    return s_fonts_ready ? &s_font_body : &lv_font_montserrat_14;
}

const lv_font_t *cw_font_title(void) {
    return s_fonts_ready ? &s_font_title : &lv_font_montserrat_16;
}

typedef struct {
    lv_obj_t *time_label;
    lv_obj_t *bat_label;
    lv_obj_t *content;
    lv_obj_t *hint;
} screen_data_t;

static void screen_delete_cb(lv_event_t *e) {
    screen_data_t *d = (screen_data_t *)lv_event_get_user_data(e);
    if (d) {
        // 动画与定时器随对象删除自动清理;仅释放私有数据
        lv_free(d);
    }
}

lv_obj_t *cw_screen_create(const char *title) {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_size(scr, CW_W, CW_H);
    lv_obj_set_style_bg_color(scr, lv_color_hex(CW_C_BG), 0);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0x0B101C), 0);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_style_border_width(scr, 0, 0);

    screen_data_t *d = (screen_data_t *)lv_malloc(sizeof(screen_data_t));
    memset(d, 0, sizeof(*d));

    // 状态栏
    lv_obj_t *sb = lv_obj_create(scr);
    lv_obj_set_size(sb, CW_W, 22);
    lv_obj_set_style_bg_opa(sb, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sb, 0, 0);
    lv_obj_set_style_pad_all(sb, 0, 0);
    d->time_label = lv_label_create(sb);
    lv_label_set_text(d->time_label, "--:--");
    lv_obj_set_style_text_font(d->time_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(d->time_label, lv_color_hex(CW_C_MUTED), 0);
    lv_obj_align(d->time_label, LV_ALIGN_LEFT_MID, 10, 0);
    d->bat_label = lv_label_create(sb);
    lv_label_set_text(d->bat_label, "--%");
    lv_obj_set_style_text_font(d->bat_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(d->bat_label, lv_color_hex(CW_C_MUTED), 0);
    lv_obj_align(d->bat_label, LV_ALIGN_RIGHT_MID, -10, 0);

    int y = 22;
    if (title) {
        lv_obj_t *t = lv_label_create(scr);
        lv_label_set_text(t, title);
        lv_obj_set_style_text_font(t, cw_font_title(), 0);
        lv_obj_set_style_text_color(t, lv_color_hex(CW_C_TEXT), 0);
        lv_obj_set_pos(t, 10, y);
        y += 30;
    }

    d->content = lv_obj_create(scr);
    lv_obj_set_pos(d->content, 0, y);
    lv_obj_set_size(d->content, CW_W, CW_H - y - 24);
    lv_obj_set_style_bg_opa(d->content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(d->content, 0, 0);
    lv_obj_set_style_pad_all(d->content, 0, 0);
    lv_obj_set_style_pad_hor(d->content, 10, 0);
    lv_obj_set_flex_flow(d->content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(d->content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(d->content, 8, 0);
    lv_obj_set_scrollbar_mode(d->content, LV_SCROLLBAR_MODE_OFF);

    d->hint = lv_obj_create(scr);
    lv_obj_set_size(d->hint, CW_W, 24);
    lv_obj_align(d->hint, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(d->hint, lv_color_hex(0x0A0F1C), 0);
    lv_obj_set_style_bg_opa(d->hint, LV_OPA_90, 0);
    lv_obj_set_style_border_width(d->hint, 0, 0);
    lv_obj_set_style_pad_all(d->hint, 0, 0);

    lv_obj_add_event_cb(scr, screen_delete_cb, LV_EVENT_DELETE, d);
    lv_obj_set_user_data(scr, d);
    lv_screen_load(scr);
    return scr;
}

static screen_data_t *get_data(lv_obj_t *scr) {
    return (screen_data_t *)lv_obj_get_user_data(scr);
}

void cw_ui_status_tick(lv_obj_t *scr, const char *time_str, int bat_pct) {
    screen_data_t *d = get_data(scr);
    if (!d) return;
    lv_label_set_text(d->time_label, time_str);
    if (bat_pct < 0) {
        lv_label_set_text(d->bat_label, "--%");
    } else {
        char buf[12];
        snprintf(buf, sizeof(buf), "%d%%", bat_pct);
        lv_label_set_text(d->bat_label, buf);
    }
}

lv_obj_t *cw_screen_content(lv_obj_t *scr) {
    screen_data_t *d = get_data(scr);
    return d ? d->content : NULL;
}

lv_obj_t *cw_card(lv_obj_t *parent) {
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_width(c, LV_PCT(100));
    lv_obj_set_height(c, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(c, lv_color_hex(CW_C_CARD), 0);
    lv_obj_set_style_bg_opa(c, LV_OPA_90, 0);
    lv_obj_set_style_border_color(c, lv_color_hex(CW_C_CARD_LINE), 0);
    lv_obj_set_style_border_width(c, 1, 0);
    lv_obj_set_style_radius(c, 10, 0);
    lv_obj_set_style_pad_all(c, 8, 0);
    lv_obj_set_style_shadow_width(c, 6, 0);
    lv_obj_set_style_shadow_color(c, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(c, LV_OPA_30, 0);
    lv_obj_set_scrollbar_mode(c, LV_SCROLLBAR_MODE_OFF);
    // 卡片内容垂直流式,避免多子元素重叠
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(c, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(c, 4, 0);
    return c;
}

static lv_obj_t *plain_box(lv_obj_t *parent) {
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
    return o;
}

lv_obj_t *cw_row(lv_obj_t *parent) {
    lv_obj_t *r = plain_box(parent);
    lv_obj_set_size(r, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    return r;
}

lv_obj_t *cw_col(lv_obj_t *parent) {
    lv_obj_t *c = plain_box(parent);
    lv_obj_set_size(c, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(c, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(c, 2, 0);
    return c;
}

lv_obj_t *cw_label(lv_obj_t *parent, const char *text) {
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text ? text : "");
    lv_obj_set_style_text_font(l, cw_font_body(), 0);
    lv_obj_set_style_text_color(l, lv_color_hex(CW_C_TEXT), 0);
    return l;
}

lv_obj_t *cw_label_small(lv_obj_t *parent, const char *text) {
    lv_obj_t *l = cw_label(parent, text);
    lv_obj_set_style_text_color(l, lv_color_hex(CW_C_MUTED), 0);
    return l;
}

lv_obj_t *cw_tag(lv_obj_t *parent, const char *text, uint32_t color) {
    lv_obj_t *t = lv_label_create(parent);
    lv_label_set_text(t, text ? text : "");
    lv_obj_set_style_text_font(t, cw_font_body(), 0); // 标签可能含中文
    lv_obj_set_style_text_color(t, lv_color_hex(color), 0);
    lv_obj_set_style_bg_color(t, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(t, LV_OPA_20, 0);
    lv_obj_set_style_border_color(t, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(t, 1, 0);
    lv_obj_set_style_border_opa(t, LV_OPA_40, 0);
    lv_obj_set_style_radius(t, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_hor(t, 7, 0);
    lv_obj_set_style_pad_ver(t, 2, 0);
    return t;
}

void cw_hint_bar(lv_obj_t *scr, const char *left, const char *right) {
    screen_data_t *d = get_data(scr);
    if (!d) return;
    lv_obj_clean(d->hint);
    if (left) {
        lv_obj_t *l = lv_label_create(d->hint);
        lv_label_set_text(l, left);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(l, lv_color_hex(CW_C_MUTED), 0);
        lv_obj_align(l, LV_ALIGN_LEFT_MID, 10, 0);
    }
    if (right) {
        lv_obj_t *r = lv_label_create(d->hint);
        lv_label_set_text(r, right);
        lv_obj_set_style_text_font(r, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(r, lv_color_hex(CW_C_MUTED), 0);
        lv_obj_align(r, LV_ALIGN_RIGHT_MID, -10, 0);
    }
}

lv_obj_t *cw_button(lv_obj_t *parent, const char *text) {
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_style_bg_color(b, lv_color_hex(CW_C_ACCENT), 0);
    lv_obj_set_style_bg_grad_color(b, lv_color_hex(0x6AA5FF), 0);
    lv_obj_set_style_bg_grad_dir(b, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_width(b, 8, 0);
    lv_obj_set_style_shadow_color(b, lv_color_hex(CW_C_ACCENT), 0);
    lv_obj_set_style_shadow_opa(b, LV_OPA_40, 0);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, text ? text : "");
    lv_obj_set_style_text_font(l, cw_font_body(), 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(l);
    return b;
}

lv_obj_t *cw_set_row(lv_obj_t *parent, const char *name) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, 34);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_t *l = cw_label(row, name);
    (void)l;
    return row;
}

lv_obj_t *cw_seg(lv_obj_t *parent) {
    lv_obj_t *s = lv_obj_create(parent);
    lv_obj_set_size(s, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(s, lv_color_hex(0x141D33), 0);
    lv_obj_set_style_radius(s, 8, 0);
    lv_obj_set_style_border_width(s, 0, 0);
    lv_obj_set_style_pad_all(s, 2, 0);
    lv_obj_set_flex_flow(s, LV_FLEX_FLOW_ROW);
    lv_obj_set_scrollbar_mode(s, LV_SCROLLBAR_MODE_OFF);
    return s;
}

lv_obj_t *cw_seg_add(lv_obj_t *seg, const char *text, bool selected) {
    lv_obj_t *o = lv_label_create(seg);
    lv_label_set_text(o, text ? text : "");
    lv_obj_set_style_text_font(o, cw_font_body(), 0);
    lv_obj_set_style_text_color(o, lv_color_hex(selected ? 0xFFFFFF : CW_C_MUTED), 0);
    if (selected) {
        lv_obj_set_style_bg_color(o, lv_color_hex(CW_C_ACCENT), 0);
        lv_obj_set_style_bg_grad_color(o, lv_color_hex(0x6AA5FF), 0);
        lv_obj_set_style_bg_grad_dir(o, LV_GRAD_DIR_HOR, 0);
    }
    lv_obj_set_style_bg_opa(o, selected ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(o, 6, 0);
    lv_obj_set_style_pad_hor(o, 10, 0);
    lv_obj_set_style_pad_ver(o, 4, 0);
    return o;
}

// ---- 模拟时钟 ----
typedef struct {
    lv_obj_t *hh;
    lv_obj_t *mh;
    lv_obj_t *sh;
    int cx;
    int cy;
    lv_point_precise_t *pts; // 12 刻度*2 + 3 指针*2,随对象释放
} clock_data_t;

static void clock_del_cb(lv_event_t *e) {
    clock_data_t *cd = (clock_data_t *)lv_event_get_user_data(e);
    if (cd) {
        lv_free(cd->pts);
        lv_free(cd);
    }
}

// line 对象的点数组随对象释放
static void line_pts_del_cb(lv_event_t *e) {
    lv_point_precise_t *pp = (lv_point_precise_t *)lv_event_get_user_data(e);
    lv_free(pp);
}

static lv_obj_t *clock_line(lv_obj_t *parent, int size) {
    lv_obj_t *l = lv_line_create(parent);
    lv_point_precise_t *pp = (lv_point_precise_t *)lv_malloc(sizeof(*pp) * 2);
    lv_obj_add_event_cb(l, line_pts_del_cb, LV_EVENT_DELETE, pp);
    lv_obj_set_size(l, size, size);
    lv_obj_align(l, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_user_data(l, pp);
    return l;
}

static void line_set(lv_obj_t *l, int x0, int y0, int x1, int y1) {
    lv_point_precise_t *pp = (lv_point_precise_t *)lv_obj_get_user_data(l);
    pp[0].x = x0;
    pp[0].y = y0;
    pp[1].x = x1;
    pp[1].y = y1;
    lv_line_set_points(l, pp, 2);
}

lv_obj_t *cw_analog_clock(lv_obj_t *parent, int size, uint32_t accent) {
    int r = size / 2;
    lv_obj_t *face = lv_obj_create(parent);
    lv_obj_set_size(face, size, size);
    lv_obj_set_style_bg_color(face, lv_color_hex(0x0D1526), 0);
    lv_obj_set_style_border_color(face, lv_color_hex(accent), 0);
    lv_obj_set_style_border_width(face, 2, 0);
    lv_obj_set_style_radius(face, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(face, 0, 0);
    lv_obj_set_scrollbar_mode(face, LV_SCROLLBAR_MODE_OFF);

    // 刻度
    for (int i = 0; i < 12; i++) {
        double a = i * M_PI / 6.0;
        bool maj = (i % 3 == 0);
        int r1 = maj ? r - 7 : r - 5;
        int r2 = r - 3;
        lv_obj_t *tick = clock_line(face, size);
        line_set(tick, (int)(r + r1 * sin(a)), (int)(r - r1 * cos(a)),
                 (int)(r + r2 * sin(a)), (int)(r - r2 * cos(a)));
        lv_obj_set_style_line_width(tick, maj ? 2 : 1, 0);
        lv_obj_set_style_line_color(tick, lv_color_hex(maj ? 0x7FB0FF : 0x3A4A63), 0);
    }

    clock_data_t *cd = (clock_data_t *)lv_malloc(sizeof(*cd));
    cd->cx = r;
    cd->cy = r;
    cd->pts = NULL;

    // 指针
    cd->hh = clock_line(face, size);
    cd->mh = clock_line(face, size);
    cd->sh = clock_line(face, size);
    lv_obj_t *hands[3] = {cd->hh, cd->mh, cd->sh};
    int lens[3] = {r - 12, r - 7, r - 5};
    int widths[3] = {3, 2, 1};
    uint32_t cols[3] = {0xFFFFFF, 0x7FB0FF, CW_C_RED};
    for (int i = 0; i < 3; i++) {
        line_set(hands[i], r, r, r, r - lens[i]);
        lv_obj_set_style_line_width(hands[i], widths[i], 0);
        lv_obj_set_style_line_color(hands[i], lv_color_hex(cols[i]), 0);
        lv_obj_set_style_line_rounded(hands[i], true, 0);
    }
    lv_obj_t *dot = lv_obj_create(face);
    lv_obj_set_size(dot, 5, 5);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(CW_C_RED), 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_center(dot);

    lv_obj_add_event_cb(face, clock_del_cb, LV_EVENT_DELETE, cd);
    lv_obj_set_user_data(face, cd);
    return face;
}

void cw_clock_tick(lv_obj_t *clock, int hour24, int minute, int second) {
    clock_data_t *cd = (clock_data_t *)lv_obj_get_user_data(clock);
    if (!cd) return;
    int size;
    {
        int w = lv_obj_get_width(clock);
        size = w > 0 ? w : 40;
    }
    int r = size / 2;
    double ha = ((hour24 % 12) + minute / 60.0) / 12.0 * 2 * M_PI;
    double ma = (minute + second / 60.0) / 60.0 * 2 * M_PI;
    double sa = second / 60.0 * 2 * M_PI;
    struct {
        lv_obj_t *h;
        double a;
        int len;
        int tail;
    } hands[3] = {
        {cd->hh, ha, r - 12, 0},
        {cd->mh, ma, r - 7, 0},
        {cd->sh, sa, r - 5, 4},
    };
    for (int i = 0; i < 3; i++) {
        int x0 = (int)(r - hands[i].tail * sin(hands[i].a));
        int y0 = (int)(r + hands[i].tail * cos(hands[i].a));
        int x1 = (int)(r + hands[i].len * sin(hands[i].a));
        int y1 = (int)(r - hands[i].len * cos(hands[i].a));
        line_set(hands[i].h, x0, y0, x1, y1);
    }
}

// ---- 天气图标 ----
static void anim_align_x_cb(void *var, int32_t v) {
    // 保持居中对齐,仅左右偏移(直接 set_x 会丢掉对齐)
    lv_obj_align((lv_obj_t *)var, LV_ALIGN_CENTER, (int16_t)v, 0);
}

void anim_abs_x_cb(void *var, int32_t v) {
    lv_obj_set_x((lv_obj_t *)var, (int16_t)v);
}

static void anim_y_opa_cb(void *var, int32_t v) {
    // v: 0..1000,前半下移,后半淡出 — 简单起见只做 y 位移循环
    lv_obj_t *o = (lv_obj_t *)var;
    lv_obj_set_y(o, (int16_t)(v / 100));
    lv_obj_set_style_opa(o, (v < 700) ? LV_OPA_COVER : (lv_opa_t)(LV_OPA_COVER * (1000 - v) / 300), 0);
}

lv_obj_t *cw_weather_icon(lv_obj_t *parent, int kind, int size) {
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_size(box, size, size);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_set_scrollbar_mode(box, LV_SCROLLBAR_MODE_OFF);
    int c = size / 2;

    if (kind == 1) { // 晴:太阳 + 光芒
        lv_obj_t *sun = lv_obj_create(box);
        int sr = size / 5;
        lv_obj_set_size(sun, sr * 2, sr * 2);
        lv_obj_set_style_radius(sun, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(sun, lv_color_hex(0xFFD54F), 0);
        lv_obj_set_style_border_width(sun, 0, 0);
        lv_obj_set_style_shadow_width(sun, 10, 0);
        lv_obj_set_style_shadow_color(sun, lv_color_hex(0xFFD54F), 0);
        lv_obj_set_style_shadow_opa(sun, LV_OPA_50, 0);
        lv_obj_align(sun, LV_ALIGN_CENTER, 0, 0);
        for (int i = 0; i < 8; i++) {
            double a = i * M_PI / 4.0;
            lv_obj_t *ray = clock_line(box, size);
            int r1 = sr + 3, r2 = sr + 7;
            line_set(ray, (int)(c + r1 * cos(a)), (int)(c + r1 * sin(a)),
                     (int)(c + r2 * cos(a)), (int)(c + r2 * sin(a)));
            lv_obj_set_style_line_width(ray, 2, 0);
            lv_obj_set_style_line_color(ray, lv_color_hex(0xFFD54F), 0);
        }
    } else {
        // 云:三个圆
        lv_obj_t *cloud = lv_obj_create(box);
        lv_obj_set_size(cloud, size, size);
        lv_obj_align(cloud, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_opa(cloud, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(cloud, 0, 0);
        lv_obj_set_style_pad_all(cloud, 0, 0);
        int cr[3] = {size / 5, size / 4, size / 6};
        int cx[3] = {-size / 6, size / 8, size / 4};
        for (int i = 0; i < 3; i++) {
            lv_obj_t *puff = lv_obj_create(cloud);
            lv_obj_set_size(puff, cr[i] * 2, cr[i] * 2);
            lv_obj_set_style_radius(puff, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_color(puff, lv_color_hex(0x8FA8C8), 0);
            lv_obj_set_style_border_width(puff, 0, 0);
            lv_obj_align(puff, LV_ALIGN_CENTER, cx[i], size / 8);
        }
        // 云漂移动画
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, cloud);
        lv_anim_set_exec_cb(&a, anim_align_x_cb);
        lv_anim_set_values(&a, -3, 3);
        lv_anim_set_duration(&a, 2600);
        lv_anim_set_reverse_duration(&a, 2600);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&a);

        if (kind == 2) { // 雨:三滴(绝对坐标,y 动画下落)
            for (int i = 0; i < 3; i++) {
                lv_obj_t *drop = lv_line_create(box);
                lv_point_precise_t *pp =
                    (lv_point_precise_t *)lv_malloc(sizeof(*pp) * 2);
                lv_obj_add_event_cb(drop, line_pts_del_cb, LV_EVENT_DELETE, pp);
                int dx = c - size / 5 + i * size / 5;
                pp[0].x = dx;
                pp[0].y = c + size / 5;
                pp[1].x = dx - 2;
                pp[1].y = c + size / 5 + 6;
                lv_line_set_points(drop, pp, 2);
                lv_obj_set_style_line_width(drop, 2, 0);
                lv_obj_set_style_line_color(drop, lv_color_hex(0x4C8DFF), 0);
                lv_obj_set_size(drop, size, size);
                lv_obj_set_pos(drop, 0, 0);
                lv_anim_t ra;
                lv_anim_init(&ra);
                lv_anim_set_var(&ra, drop);
                lv_anim_set_exec_cb(&ra, anim_y_opa_cb);
                lv_anim_set_values(&ra, 0, 1000);
                lv_anim_set_duration(&ra, 1400);
                lv_anim_set_repeat_count(&ra, LV_ANIM_REPEAT_INFINITE);
                lv_anim_set_delay(&ra, i * 450);
                lv_anim_start(&ra);
            }
        } else if (kind == 0) { // 多云:小太阳一角
            lv_obj_t *sun = lv_obj_create(box);
            int sr = size / 7;
            lv_obj_set_size(sun, sr * 2, sr * 2);
            lv_obj_set_style_radius(sun, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_color(sun, lv_color_hex(0xFFD54F), 0);
            lv_obj_set_style_border_width(sun, 0, 0);
            lv_obj_align(sun, LV_ALIGN_TOP_LEFT, size / 8, size / 10);
        }
    }
    return box;
}

// ---- 应用图标 ----
// 设计稿的图标是 SVG 线稿;本机没有位图资源,用 LVGL 基本件按 26x26 设计坐标画,
// 再等比缩放到 size。坐标一律走 sc(),改 size 不用改图。
static int sc(int v, int size) {
    return v * size / 26;
}

static lv_obj_t *ic_rect(lv_obj_t *box, int size, int x, int y, int w, int h, int r,
                         bool fill) {
    lv_obj_t *o = lv_obj_create(box);
    lv_obj_set_pos(o, sc(x, size), sc(y, size));
    lv_obj_set_size(o, sc(w, size), sc(h, size));
    lv_obj_set_style_radius(o, r >= 99 ? LV_RADIUS_CIRCLE : sc(r, size), 0);
    lv_obj_set_style_border_width(o, 2, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_color(o, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(o, fill ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
    return o;
}

static lv_obj_t *ic_line(lv_obj_t *box, int size, int x0, int y0, int x1, int y1) {
    lv_obj_t *l = clock_line(box, size);
    line_set(l, sc(x0, size), sc(y0, size), sc(x1, size), sc(y1, size));
    lv_obj_set_style_line_width(l, 2, 0);
    lv_obj_set_style_line_color(l, lv_color_hex(0xFFFFFF), 0);
    return l;
}

static lv_obj_t *ic_arc(lv_obj_t *box, int size, int x, int y, int w, int h, uint16_t a0,
                        uint16_t a1) {
    lv_obj_t *a = lv_arc_create(box);
    lv_obj_set_pos(a, sc(x, size), sc(y, size));
    lv_obj_set_size(a, sc(w, size), sc(h, size));
    lv_arc_set_bg_angles(a, a0, a1);
    lv_obj_set_style_arc_width(a, 2, 0);
    lv_obj_set_style_arc_color(a, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_arc_opa(a, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(a, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(a, 0, 0);
    lv_obj_set_style_bg_opa(a, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_width(a, 0, LV_PART_KNOB);
    lv_obj_set_scrollbar_mode(a, LV_SCROLLBAR_MODE_OFF);
    return a;
}

lv_obj_t *cw_app_icon(lv_obj_t *parent, int kind, int size) {
    lv_obj_t *b = lv_obj_create(parent);
    lv_obj_set_size(b, size, size);
    lv_obj_set_style_bg_opa(b, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(b, 0, 0);
    lv_obj_set_style_pad_all(b, 0, 0);
    lv_obj_set_scrollbar_mode(b, LV_SCROLLBAR_MODE_OFF);
    switch (kind) {
        case 0: // 课表:日历 + 两条课表线
            ic_rect(b, size, 2, 4, 22, 20, 4, false);
            ic_line(b, size, 2, 10, 24, 10);
            ic_line(b, size, 8, 1, 8, 6);
            ic_line(b, size, 18, 1, 18, 6);
            ic_line(b, size, 7, 15, 15, 15);
            ic_line(b, size, 7, 19, 19, 19);
            break;
        case 1: // 教师:头 + 肩
            ic_rect(b, size, 8, 2, 10, 10, 99, false);
            ic_arc(b, size, 4, 13, 18, 14, 180, 360);
            break;
        case 2: // 日历:日历 + 实心日期块
            ic_rect(b, size, 2, 4, 22, 20, 4, false);
            ic_line(b, size, 2, 10, 24, 10);
            ic_line(b, size, 8, 1, 8, 6);
            ic_line(b, size, 18, 1, 18, 6);
            ic_rect(b, size, 7, 14, 7, 6, 1, true);
            break;
        case 3: // 学生证:卡片 + 头像 + 两行信息
            ic_rect(b, size, 2, 5, 22, 16, 3, false);
            ic_rect(b, size, 5, 9, 6, 6, 99, true);
            ic_line(b, size, 14, 11, 21, 11);
            ic_line(b, size, 14, 15, 20, 15);
            break;
        case 4: // 作业:方框 + 勾
            ic_rect(b, size, 3, 3, 20, 20, 4, false);
            ic_line(b, size, 8, 13, 12, 17);
            ic_line(b, size, 12, 17, 19, 8);
            break;
        case 5: // 世界时钟:表盘 + 指针
            ic_rect(b, size, 3, 3, 20, 20, 99, false);
            ic_line(b, size, 13, 13, 13, 7);
            ic_line(b, size, 13, 13, 18, 15);
            break;
        case 6: // 番茄:圆身 + 顶柄 + 指针
            ic_rect(b, size, 3, 6, 20, 20, 99, false);
            ic_line(b, size, 13, 2, 13, 6);
            ic_line(b, size, 10, 2, 16, 2);
            ic_line(b, size, 13, 16, 13, 10);
            break;
        case 7: // 麦克风:胶囊 + 托架 + 立杆
            ic_rect(b, size, 9, 2, 8, 12, 4, false);
            ic_arc(b, size, 6, 8, 14, 12, 0, 180);
            ic_line(b, size, 13, 20, 13, 23);
            ic_line(b, size, 9, 23, 17, 23);
            break;
        default: // 设置:圆芯 + 八齿
            ic_rect(b, size, 7, 7, 12, 12, 99, false);
            for (int i = 0; i < 8; i++) {
                double a = i * M_PI / 4.0;
                ic_line(b, size, 13 + (int)(6.5 * cos(a)), 13 + (int)(6.5 * sin(a)),
                        13 + (int)(10 * cos(a)), 13 + (int)(10 * sin(a)));
            }
            break;
    }
    return b;
}

// ---- 呼吸动画 ----
static void anim_opa_cb(void *var, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
}

void cw_anim_pulse(lv_obj_t *obj) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, anim_opa_cb);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_40);
    lv_anim_set_duration(&a, 900);
    lv_anim_set_reverse_duration(&a, 900);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
}

void cw_screen_teardown(lv_obj_t *scr) {
    if (!scr) return;
    // 递归停掉屏幕及全部子对象的动画(对象删除时动画不会自动清理),
    // 定时器由各屏幕自行管理并在 exit 中删除(本应用未使用 lv_timer)。
    lv_anim_delete(scr, NULL);
    uint32_t n = lv_obj_get_child_count(scr);
    for (uint32_t i = 0; i < n; i++) {
        cw_screen_teardown(lv_obj_get_child(scr, i));
    }
}
