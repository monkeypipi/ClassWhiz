// cw_s_schedule.c — 课程表:列表 / 周网格 / 课程详情。
#include "cw_app.h"
#include "cw_ui.h"

#include <stdio.h>
#include <string.h>

static lv_obj_t *s_scr;
static int s_day;   // 当前查看的星期
static int s_view;  // 0=列表 1=网格
static lv_obj_t *s_listbox;
static lv_obj_t *s_gridbox;
static lv_obj_t *s_daytabs[7];
static lv_obj_t *s_seg;

static const char *day_name(int d) {
    static const char *EN[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    static const char *ZH[7] = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};
    return (cw_get_lang() == CW_LANG_ZH) ? ZH[d] : EN[d];
}

static void build_list(void);
static void build_grid(void);

static void refresh_tabs(void) {
    for (int i = 0; i < 7; i++) {
        lv_obj_set_style_bg_color(s_daytabs[i],
                                 lv_color_hex(i == s_day ? CW_C_ACCENT : 0x141D33), 0);
        lv_obj_t *l = lv_obj_get_child(s_daytabs[i], 0);
        if (l) {
            lv_obj_set_style_text_color(
                l, lv_color_hex(i == s_day ? 0xFFFFFF : CW_C_MUTED), 0);
        }
    }
}

static void rebuild(void) {
    lv_obj_clean(s_listbox);
    lv_obj_clean(s_gridbox);
    lv_obj_clean(s_seg);
    cw_seg_add(s_seg, cw_str(CW_S_LIST), s_view == 0);
    cw_seg_add(s_seg, cw_str(CW_S_GRID), s_view == 1);
    if (s_view == 0) {
        lv_obj_add_flag(s_gridbox, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_listbox, LV_OBJ_FLAG_HIDDEN);
        build_list();
        // 高亮卡片滚入可见区
        int today, now_min;
        cw_app_now_day_min(cw_app_mono_ms(), &today, &now_min);
        cw_class_t classes[CW_MAX_SLOTS_PER_DAY];
        int n = cw_classes_for_day(s_day, classes, CW_MAX_SLOTS_PER_DAY);
        for (int i = 0; i < n; i++) {
            cw_class_state_t st =
                cw_class_state(s_day, classes[i].period, today, now_min);
            if (st == CW_CLASS_NOW || st == CW_CLASS_NEXT) {
                lv_obj_t *sel = lv_obj_get_child(s_listbox, (uint32_t)i);
                if (sel) lv_obj_scroll_to_view(sel, LV_ANIM_OFF);
                break;
            }
        }
    } else {
        lv_obj_add_flag(s_listbox, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_gridbox, LV_OBJ_FLAG_HIDDEN);
        build_grid();
    }
    refresh_tabs();
}

static void build_list(void) {
    int today, now_min;
    uint64_t ms = cw_app_mono_ms();
    cw_app_now_day_min(ms, &today, &now_min);
    cw_class_t classes[CW_MAX_SLOTS_PER_DAY];
    int n = cw_classes_for_day(s_day, classes, CW_MAX_SLOTS_PER_DAY);
    if (n == 0) {
        lv_obj_t *l = cw_label(s_listbox, cw_str(CW_S_NO_CLASSES));
        lv_obj_align(l, LV_ALIGN_TOP_MID, 0, 30);
        return;
    }
    for (int i = 0; i < n; i++) {
        const cw_class_t *c = &classes[i];
        cw_class_state_t st = cw_class_state(s_day, c->period, today, now_min);
        bool hl = (st == CW_CLASS_NOW || st == CW_CLASS_NEXT);
        lv_obj_t *card = cw_card(s_listbox);
        lv_obj_set_style_border_color(
            card, lv_color_hex(hl ? CW_C_ACCENT : CW_C_CARD_LINE), 0);
        lv_obj_set_style_border_width(card, hl ? 2 : 1, 0);
        char tb[8], te[8];
        cw_format_time(cw_period_start_min(c->period), true, tb, sizeof(tb));
        cw_format_time(cw_period_end_min(c->period), true, te, sizeof(te));
        lv_obj_t *row = lv_obj_create(card);
        lv_obj_set_width(row, LV_PCT(100));
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        char tstr[20];
        snprintf(tstr, sizeof(tstr), "%s-%s", tb, te);
        cw_label(row, tstr);
        if (st == CW_CLASS_DONE) {
            cw_tag(row, cw_str(CW_S_DONE), CW_C_GREEN);
        } else if (st == CW_CLASS_NOW || st == CW_CLASS_NEXT) {
            lv_obj_t *tag = cw_tag(row, cw_str(CW_S_NEXT), CW_C_AMBER);
            cw_anim_pulse(tag);
        }
        char title[64];
        snprintf(title, sizeof(title), "%s", c->course->code);
        lv_obj_t *tl = cw_label(card, title);
        lv_obj_set_style_text_font(tl, cw_font_title(), 0);
        char meta[96];
        snprintf(meta, sizeof(meta), "%s · %s · %s", c->course->room,
                 cw_teacher(c->course->teacher)->name,
                 cw_get_lang() == CW_LANG_ZH ? c->course->title_zh : c->course->title_en);
        cw_label_small(card, meta);
    }
}

static void build_grid(void) {
    // 顶栏:星期
    lv_obj_t *head = lv_obj_create(s_gridbox);
    lv_obj_set_width(head, LV_PCT(100));
    lv_obj_set_height(head, 20);
    lv_obj_set_style_bg_opa(head, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(head, 0, 0);
    lv_obj_set_style_pad_all(head, 0, 0);
    lv_obj_set_flex_flow(head, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(head, 2, 0);
    lv_obj_t *corner = lv_obj_create(head);
    lv_obj_set_size(corner, 28, 18);
    lv_obj_set_style_bg_opa(corner, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(corner, 0, 0);
    int today, now_min;
    cw_app_now_day_min(cw_app_mono_ms(), &today, &now_min);
    for (int d = 0; d < 7; d++) {
        lv_obj_t *h = lv_label_create(head);
        lv_label_set_text(h, day_name(d));
        lv_obj_set_style_text_font(h, cw_font_body(), 0);
        lv_obj_set_flex_grow(h, 1);
        lv_obj_set_style_text_align(h, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(h, lv_color_hex(d == today ? 0xFFFFFF : CW_C_MUTED),
                                    0);
        if (d == today) {
            lv_obj_set_style_bg_color(h, lv_color_hex(CW_C_ACCENT), 0);
            lv_obj_set_style_radius(h, 4, 0);
        }
    }
    // 网格体
    lv_obj_t *grid = lv_obj_create(s_gridbox);
    lv_obj_set_width(grid, LV_PCT(100));
    lv_obj_set_height(grid, 190);
    lv_obj_set_pos(grid, 0, 22);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(grid, 2, 0);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *gut = lv_obj_create(grid);
    lv_obj_set_size(gut, 28, 190);
    lv_obj_set_style_bg_opa(gut, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(gut, 0, 0);
    lv_obj_set_style_pad_all(gut, 0, 0);
    for (int p = 1; p <= 7; p++) {
        lv_obj_t *l = lv_label_create(gut);
        char b[8];
        snprintf(b, sizeof(b), "%d", p);
        lv_label_set_text(l, b);
        lv_obj_set_style_text_font(l, cw_font_body(), 0);
        lv_obj_set_style_text_color(l, lv_color_hex(CW_C_MUTED), 0);
        lv_obj_set_pos(l, 8, (p - 1) * 26 + 4);
    }
    for (int d = 0; d < 7; d++) {
        lv_obj_t *col = lv_obj_create(grid);
        lv_obj_set_flex_grow(col, 1);
        lv_obj_set_height(col, 190);
        lv_obj_set_style_bg_color(col, lv_color_hex(d == today ? 0x16233F : 0x0E1526),
                                  0);
        lv_obj_set_style_radius(col, 4, 0);
        lv_obj_set_style_border_width(col, 0, 0);
        lv_obj_set_style_pad_all(col, 0, 0);
        lv_obj_set_scrollbar_mode(col, LV_SCROLLBAR_MODE_OFF);
        // 行线
        for (int p = 1; p < 7; p++) {
            lv_obj_t *ln = lv_obj_create(col);
            lv_obj_set_size(ln, LV_PCT(100), 1);
            lv_obj_set_pos(ln, 0, p * 26);
            lv_obj_set_style_bg_color(ln, lv_color_hex(0x131C31), 0);
            lv_obj_set_style_border_width(ln, 0, 0);
            lv_obj_set_style_pad_all(ln, 0, 0);
        }
        cw_class_t classes[CW_MAX_SLOTS_PER_DAY];
        int n = cw_classes_for_day(d, classes, CW_MAX_SLOTS_PER_DAY);
        for (int i = 0; i < n; i++) {
            lv_obj_t *blk = lv_obj_create(col);
            lv_obj_set_size(blk, LV_PCT(96), 24);
            lv_obj_set_pos(blk, 1, (classes[i].period - 1) * 26 + 1);
            lv_obj_set_style_bg_color(blk, lv_color_hex(classes[i].course->color), 0);
            lv_obj_set_style_radius(blk, 4, 0);
            lv_obj_set_style_border_width(blk, 0, 0);
            lv_obj_set_style_pad_all(blk, 1, 0);
            lv_obj_set_scrollbar_mode(blk, LV_SCROLLBAR_MODE_OFF);
            char b[24];
            snprintf(b, sizeof(b), "%s", classes[i].course->code);
            // 紧凑:只显示代码前 4 字母
            lv_obj_t *bl = lv_label_create(blk);
            lv_label_set_text(bl, b);
            lv_obj_set_style_text_font(bl, cw_font_body(), 0);
            lv_obj_set_style_text_color(bl, lv_color_hex(0xFFFFFF), 0);
            lv_obj_center(bl);
        }
        // 现在时间线
        if (d == today && now_min >= cw_period_start_min(1) &&
            now_min <= cw_period_end_min(7)) {
            int y = 0;
            for (int p = 1; p <= 7; p++) {
                if (now_min >= cw_period_start_min(p) && now_min <= cw_period_end_min(p)) {
                    int s = cw_period_start_min(p), e = cw_period_end_min(p);
                    y = (p - 1) * 26 + (now_min - s) * 26 / (e - s);
                    break;
                }
                if (p < 7 && now_min > cw_period_end_min(p) &&
                    now_min < cw_period_start_min(p + 1)) {
                    y = p * 26 - 13;
                    break;
                }
            }
            lv_obj_t *nl = lv_obj_create(col);
            lv_obj_set_size(nl, LV_PCT(100), 2);
            lv_obj_set_pos(nl, 0, y);
            lv_obj_set_style_bg_color(nl, lv_color_hex(CW_C_RED), 0);
            lv_obj_set_style_border_width(nl, 0, 0);
            lv_obj_set_style_pad_all(nl, 0, 0);
            lv_obj_set_style_shadow_width(nl, 4, 0);
            lv_obj_set_style_shadow_color(nl, lv_color_hex(CW_C_RED), 0);
        }
    }
}

static void sched_enter(void *ctx) {
    (void)ctx;
    s_scr = cw_screen_create(cw_str(CW_S_SCHEDULE));
    lv_obj_t *body = cw_screen_content(s_scr);
    // 视图切换
    lv_obj_t *top = lv_obj_create(body);
    lv_obj_set_width(top, LV_PCT(100));
    lv_obj_set_height(top, 30);
    lv_obj_set_style_bg_opa(top, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(top, 0, 0);
    lv_obj_set_style_pad_all(top, 0, 0);
    lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_t *seg = cw_seg(top);
    s_seg = seg;
    cw_seg_add(seg, cw_str(CW_S_LIST), true);
    cw_seg_add(seg, cw_str(CW_S_GRID), false);
    // 星期切换
    lv_obj_t *tabs = lv_obj_create(body);
    lv_obj_set_width(tabs, LV_PCT(100));
    lv_obj_set_height(tabs, 28);
    lv_obj_set_style_bg_opa(tabs, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tabs, 0, 0);
    lv_obj_set_style_pad_all(tabs, 0, 0);
    lv_obj_set_flex_flow(tabs, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(tabs, 3, 0);
    for (int i = 0; i < 7; i++) {
        lv_obj_t *t = lv_obj_create(tabs);
        lv_obj_set_flex_grow(t, 1);
        lv_obj_set_height(t, 26);
        lv_obj_set_style_bg_color(t, lv_color_hex(0x141D33), 0);
        lv_obj_set_style_radius(t, 6, 0);
        lv_obj_set_style_border_width(t, 0, 0);
        lv_obj_set_style_pad_all(t, 0, 0);
        lv_obj_t *l = lv_label_create(t);
        lv_label_set_text(l, day_name(i));
        lv_obj_set_style_text_font(l, cw_font_body(), 0);
        lv_obj_center(l);
        s_daytabs[i] = t;
    }
    s_listbox = lv_obj_create(body);
    lv_obj_set_width(s_listbox, LV_PCT(100));
    lv_obj_set_height(s_listbox, 186);
    lv_obj_set_style_bg_opa(s_listbox, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_listbox, 0, 0);
    lv_obj_set_style_pad_all(s_listbox, 0, 0);
    lv_obj_set_flex_flow(s_listbox, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(s_listbox, 6, 0);
    lv_obj_set_scrollbar_mode(s_listbox, LV_SCROLLBAR_MODE_AUTO);

    s_gridbox = lv_obj_create(body);
    lv_obj_set_width(s_gridbox, LV_PCT(100));
    lv_obj_set_height(s_gridbox, 220);
    lv_obj_set_style_bg_opa(s_gridbox, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_gridbox, 0, 0);
    lv_obj_set_style_pad_all(s_gridbox, 0, 0);
    lv_obj_set_scrollbar_mode(s_gridbox, LV_SCROLLBAR_MODE_OFF);

    int today, dummy;
    cw_app_now_day_min(cw_app_mono_ms(), &today, &dummy);
    s_day = today;
    s_view = 0;
    rebuild();
    cw_hint_bar(s_scr, "UP/DN: Day", "OK: Detail 2x: View");
}

static void sched_exit(void) {
}

// 课程详情(作为子屏幕)
typedef struct {
    int day;
    int period;
} detail_ctx_t;

static detail_ctx_t s_detail_ctx;

static void detail_enter(void *ctx) {
    detail_ctx_t *dc = (detail_ctx_t *)ctx;
    if (dc) s_detail_ctx = *dc;
    s_scr = cw_screen_create(cw_str(CW_S_SCHEDULE));
    lv_obj_t *body = cw_screen_content(s_scr);
    cw_class_t classes[CW_MAX_SLOTS_PER_DAY];
    int n = cw_classes_for_day(s_detail_ctx.day, classes, CW_MAX_SLOTS_PER_DAY);
    const cw_course_t *course = NULL;
    for (int i = 0; i < n; i++) {
        if (classes[i].period == s_detail_ctx.period) {
            course = classes[i].course;
            break;
        }
    }
    if (!course) {
        cw_label(body, "?");
        return;
    }
    lv_obj_t *card = cw_card(body);
    lv_obj_set_style_border_color(card, lv_color_hex(course->color), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_t *code = cw_label(card, course->code);
    lv_obj_set_style_text_font(code, cw_font_title(), 0);
    cw_label(card, cw_get_lang() == CW_LANG_ZH ? course->title_zh : course->title_en);
    char tb[8], te[8];
    cw_format_time(cw_period_start_min(s_detail_ctx.period), true, tb, sizeof(tb));
    cw_format_time(cw_period_end_min(s_detail_ctx.period), true, te, sizeof(te));
    char when[64];
    snprintf(when, sizeof(when), "%s %s-%s", day_name(s_detail_ctx.day), tb, te);
    cw_label_small(card, when);
    const cw_teacher_t *t = cw_teacher(course->teacher);
    char tr[96];
    snprintf(tr, sizeof(tr), "%s · %s", course->room, t->name);
    cw_label(card, tr);
    // 该教师下一次 office hour
    cw_oh_entry_t ohs[8];
    int on = cw_office_hours_week(ohs, 8);
    for (int i = 0; i < on; i++) {
        if (ohs[i].teacher == course->teacher && ohs[i].day >= s_detail_ctx.day) {
            char oh[64];
            char os[8], oe[8];
            cw_format_time(ohs[i].start_min, true, os, sizeof(os));
            cw_format_time(ohs[i].end_min, true, oe, sizeof(oe));
            snprintf(oh, sizeof(oh), "%s: %s %s-%s", cw_str(CW_S_OFFICE_HOURS),
                     day_name(ohs[i].day), os, oe);
            cw_label_small(card, oh);
            break;
        }
    }
    cw_hint_bar(s_scr, "Hold OK: Back", NULL);
}

static void detail_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    (void)btn;
    (void)ev;
}

static const cw_screen_def_t s_detail_def = {
    .enter = detail_enter,
    .exit = NULL,
    .key = detail_key,
    .tick = NULL,
    .name = "class-detail",
};

static void sched_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (ev == BSP_BTN_DOUBLE) {
        // 双击任意键切换列表/网格(iot_button 双击不附带单击事件)
        s_view = 1 - s_view;
        rebuild();
        return;
    }
    if (ev != BSP_BTN_CLICK) return;
    if (btn == BSP_BTN_UP) {
        s_day = (s_day + 6) % 7;
        rebuild();
    } else if (btn == BSP_BTN_DOWN) {
        s_day = (s_day + 1) % 7;
        rebuild();
    } else if (btn == BSP_BTN_OK) {
        if (s_view == 1) {
            s_view = 0; // 网格下 OK 切回列表
            rebuild();
            return;
        }
        // 列表下 OK 打开高亮(下一节)课程详情
        int today, now_min;
        cw_app_now_day_min(cw_app_mono_ms(), &today, &now_min);
        cw_class_t classes[CW_MAX_SLOTS_PER_DAY];
        int n = cw_classes_for_day(s_day, classes, CW_MAX_SLOTS_PER_DAY);
        int pick = -1;
        for (int i = 0; i < n; i++) {
            cw_class_state_t st = cw_class_state(s_day, classes[i].period, today, now_min);
            if (st == CW_CLASS_NOW || st == CW_CLASS_NEXT) {
                pick = i;
                break;
            }
        }
        if (pick < 0 && n > 0) pick = 0;
        if (pick >= 0) {
            s_detail_ctx.day = s_day;
            s_detail_ctx.period = classes[pick].period;
            cw_app_push(&s_detail_def, &s_detail_ctx);
        }
    }
}

const cw_screen_def_t cw_scr_schedule_def = {
    .enter = sched_enter,
    .exit = sched_exit,
    .key = sched_key,
    .tick = NULL,
    .name = "schedule",
};
