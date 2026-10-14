// cw_s_calendar.c — 大学校历:月视图 + 关键日期。
#include "cw_app.h"
#include "cw_ui.h"

#include <stdio.h>

static lv_obj_t *s_scr;
static int s_year;
static int s_month; // 1..12
static lv_obj_t *s_title;
static lv_obj_t *s_grid;
static lv_obj_t *s_events;

static const char *month_name(int m) {
    static const char *EN[12] = {"January", "February", "March",     "April",
                                 "May",     "June",     "July",      "August",
                                 "September", "October", "November", "December"};
    static const char *ZH[12] = {"1 月", "2 月", "3 月",  "4 月",  "5 月",  "6 月",
                                 "7 月", "8 月", "9 月", "10 月", "11 月", "12 月"};
    if (m < 1 || m > 12) return "";
    return (cw_get_lang() == CW_LANG_ZH) ? ZH[m - 1] : EN[m - 1];
}

static uint32_t kind_color(cw_event_kind_t k) {
    switch (k) {
        case CW_EVENT_HOLIDAY:
            return CW_C_AMBER;
        case CW_EVENT_EXAM:
            return CW_C_RED;
        default:
            return CW_C_ACCENT;
    }
}

static void rebuild(void) {
    char t[32];
    // 设计稿:中文写"2026 年 10 月",英文写"October 2026"
    if (cw_get_lang() == CW_LANG_ZH) {
        snprintf(t, sizeof(t), "%d 年 %s", s_year, month_name(s_month));
    } else {
        snprintf(t, sizeof(t), "%s %d", month_name(s_month), s_year);
    }
    lv_label_set_text(s_title, t);
    lv_obj_clean(s_grid);
    lv_obj_clean(s_events);

    static const char *DOW_EN[7] = {"M", "T", "W", "T", "F", "S", "S"};
    static const char *DOW_ZH[7] = {"一", "二", "三", "四", "五", "六", "日"};
    for (int i = 0; i < 7; i++) {
        lv_obj_t *d = lv_label_create(s_grid);
        lv_label_set_text(d, cw_get_lang() == CW_LANG_ZH ? DOW_ZH[i] : DOW_EN[i]);
        lv_obj_set_style_text_font(d, cw_font_body(), 0);
        lv_obj_set_style_text_color(d, lv_color_hex(CW_C_MUTED), 0);
        lv_obj_set_style_text_align(d, LV_TEXT_ALIGN_CENTER, 0);
        // LVGL grid 不会自动排位,每个子对象都必须显式指定单元格
        lv_obj_set_grid_cell(d, LV_GRID_ALIGN_STRETCH, i, 1, LV_GRID_ALIGN_CENTER, 0,
                             1);
    }
    int first = cw_weekday_of_date(s_year, s_month, 1);
    int days = cw_days_in_month(s_year, s_month);
    const cw_event_t *evs[CW_MAX_EVENTS];
    int nev = cw_events_for_month(s_year, s_month, evs, CW_MAX_EVENTS);
    // 演示“今天”:2026-10-13
    for (int i = 0; i < first; i++) {
        lv_obj_t *d = lv_label_create(s_grid);
        lv_label_set_text(d, "");
        lv_obj_set_grid_cell(d, LV_GRID_ALIGN_STRETCH, i % 7, 1, LV_GRID_ALIGN_CENTER,
                             1 + i / 7, 1);
    }
    for (int day = 1; day <= days; day++) {
        int slot = first + day - 1;
        lv_obj_t *cell = lv_obj_create(s_grid);
        lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_STRETCH, slot % 7, 1,
                             LV_GRID_ALIGN_STRETCH, 1 + slot / 7, 1);
        lv_obj_set_style_bg_opa(cell, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(cell, 0, 0);
        lv_obj_set_style_pad_all(cell, 0, 0);
        lv_obj_set_style_radius(cell, 6, 0);
        bool is_today = (s_year == 2026 && s_month == 10 && day == 13);
        bool has_ev = false;
        uint32_t evc = CW_C_ACCENT;
        for (int e = 0; e < nev; e++) {
            if (evs[e]->day == day) {
                has_ev = true;
                evc = kind_color(evs[e]->kind);
            }
        }
        if (is_today) {
            lv_obj_set_style_bg_color(cell, lv_color_hex(CW_C_ACCENT), 0);
            lv_obj_set_style_radius(cell, 6, 0);
            lv_obj_set_style_shadow_width(cell, 6, 0);
            lv_obj_set_style_shadow_color(cell, lv_color_hex(CW_C_ACCENT), 0);
            lv_obj_set_style_shadow_opa(cell, LV_OPA_50, 0);
        }
        char b[12];
        snprintf(b, sizeof(b), "%d", day);
        lv_obj_t *l = lv_label_create(cell);
        lv_label_set_text(l, b);
        lv_obj_set_style_text_font(l, cw_font_body(), 0);
        lv_obj_set_style_text_color(
            l, lv_color_hex(is_today ? 0xFFFFFF : CW_C_TEXT), 0);
        lv_obj_align(l, LV_ALIGN_CENTER, 0, 0);
        if (has_ev && !is_today) {
            lv_obj_t *dot = lv_obj_create(cell);
            lv_obj_set_size(dot, 4, 4);
            lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_color(dot, lv_color_hex(evc), 0);
            lv_obj_set_style_border_width(dot, 0, 0);
            lv_obj_align(dot, LV_ALIGN_TOP_RIGHT, -1, 1);
        }
    }
    // 244px 的内容预算(见 cw_ui.h)只放得下三行,事件压成单行、按类型着色
    int show = nev > 3 ? 2 : nev;
    for (int e = 0; e < show; e++) {
        const cw_event_t *ev = evs[e];
        lv_obj_t *row = cw_row(s_events);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_gap(row, 5, 0);
        lv_obj_t *dot = lv_obj_create(row);
        lv_obj_set_size(dot, 6, 6);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, lv_color_hex(kind_color(ev->kind)), 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        char b[96];
        snprintf(b, sizeof(b), "%d/%d %s", ev->month, ev->day, cw_event_title(ev));
        lv_obj_t *l = cw_label(row, b);
        lv_obj_set_style_text_color(l, lv_color_hex(kind_color(ev->kind)), 0);
        lv_obj_set_flex_grow(l, 1);
        lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    }
    if (nev > show) {
        char more[32];
        snprintf(more, sizeof(more), "+%d", nev - show);
        cw_label_small(s_events, more);
    }
}

static void cal_enter(void *ctx) {
    (void)ctx;
    s_scr = cw_screen_create(cw_str(CW_S_CALENDAR));
    lv_obj_t *body = cw_screen_content(s_scr);
    s_title = cw_label(body, "");
    s_grid = lv_obj_create(body);
    lv_obj_set_size(s_grid, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(s_grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_grid, 0, 0);
    lv_obj_set_style_pad_all(s_grid, 0, 0);
    lv_obj_set_layout(s_grid, LV_LAYOUT_GRID);
    static const int32_t col[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
                                  LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
                                  LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    // 表头行给 18:中文子集字体行高 17,16 会把星期字裁掉一像素
    static const int32_t row[] = {18, 20, 20, 20, 20, 20, 20, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(s_grid, col, row);
    lv_obj_set_style_pad_gap(s_grid, 2, 0);
    lv_obj_set_scrollbar_mode(s_grid, LV_SCROLLBAR_MODE_OFF);
    s_events = lv_obj_create(body);
    lv_obj_set_size(s_events, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(s_events, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_events, 0, 0);
    lv_obj_set_style_pad_all(s_events, 0, 0);
    lv_obj_set_flex_flow(s_events, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(s_events, 3, 0);
    lv_obj_set_scrollbar_mode(s_events, LV_SCROLLBAR_MODE_OFF);
    s_year = 2026;
    s_month = 10;
    rebuild();
    cw_hint_bar(s_scr, "UP/DN: Month", "Hold OK: Back");
}

static void cal_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (ev != BSP_BTN_CLICK) return;
    if (btn == BSP_BTN_UP) {
        if (--s_month < 1) {
            s_month = 12;
            s_year--;
        }
        rebuild();
    } else if (btn == BSP_BTN_DOWN) {
        if (++s_month > 12) {
            s_month = 1;
            s_year++;
        }
        rebuild();
    }
}

const cw_screen_def_t cw_scr_calendar_def = {
    .enter = cal_enter,
    .exit = NULL,
    .key = cal_key,
    .tick = NULL,
    .name = "calendar",
};
