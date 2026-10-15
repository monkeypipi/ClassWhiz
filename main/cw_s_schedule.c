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
static lv_obj_t *s_tabs;
static lv_obj_t *s_daytabs[7];
static lv_obj_t *s_seg;

static const char *day_name(int d) {
    static const char *EN[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    static const char *ZH[7] = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};
    return (cw_get_lang() == CW_LANG_ZH) ? ZH[d] : EN[d];
}

// 七等分的窄格子(每格不到 30px)只放得下单字母:"Mon" 在 14px 下要 31.8px。
static const char *day_short(int d) {
    static const char *EN[7] = {"M", "T", "W", "T", "F", "S", "S"};
    static const char *ZH[7] = {"一", "二", "三", "四", "五", "六", "日"};
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
        lv_obj_clear_flag(s_tabs, LV_OBJ_FLAG_HIDDEN);
        build_list();
        // 高亮卡片滚入可见区
        int today, now_min;
        cw_app_now_day_min(cw_app_mono_ms(), &today, &now_min);
        cw_class_t classes[CW_MAX_SLOTS_PER_DAY];
        int n = cw_classes_for_day(s_day, classes, CW_MAX_SLOTS_PER_DAY);
        for (int i = 0; i < n; i++) {
            cw_class_state_t st =
                cw_class_state(s_day, classes[i].start_min, classes[i].end_min, today, now_min);
            if (st == CW_CLASS_NOW || st == CW_CLASS_NEXT) {
                lv_obj_t *sel = lv_obj_get_child(s_listbox, (uint32_t)i);
                if (sel) lv_obj_scroll_to_view(sel, LV_ANIM_OFF);
                break;
            }
        }
    } else {
        lv_obj_add_flag(s_listbox, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_tabs, LV_OBJ_FLAG_HIDDEN);
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
    // 没有触摸,列表滚不动:两行卡 54px,三张加间距 174px 刚好塞进 174px 的列表框;
    // 真出现四节课时改单行卡(31px,四张 142px),宁可标题打点也不能裁掉整张卡。
    bool compact = n > 3;
    for (int i = 0; i < n; i++) {
        const cw_class_t *c = &classes[i];
        cw_class_state_t st = cw_class_state(s_day, c->start_min, c->end_min, today, now_min);
        bool hl = (st == CW_CLASS_NOW || st == CW_CLASS_NEXT);
        lv_obj_t *card = cw_card(s_listbox);
        // 一天最多三节课,listbox 只有 174px(见 cw_ui.h 的预算说明):
        // 卡片压到两行(时间 + 课程),教室与教师放到详情页。
        lv_obj_set_style_pad_all(card, compact ? 4 : 6, 0);
        lv_obj_set_style_pad_gap(card, 2, 0);
        // 设计稿的签名元素:左侧 3px 课程色条,NOW/NEXT 用琥珀色
        lv_obj_set_style_border_side(card, LV_BORDER_SIDE_LEFT, 0);
        lv_obj_set_style_border_width(card, 3, 0);
        lv_obj_set_style_border_color(
            card, lv_color_hex(hl ? CW_C_AMBER : c->course->color), 0);
        if (hl) {
            lv_obj_set_style_bg_color(card, lv_color_hex(0x2A2417), 0);
            lv_obj_set_style_bg_grad_color(card, lv_color_hex(CW_C_CARD), 0);
            lv_obj_set_style_bg_grad_dir(card, LV_GRAD_DIR_HOR, 0);
        }
        char tb[8], te[8];
        cw_format_time(c->start_min, true, tb, sizeof(tb));
        cw_format_time(c->end_min, true, te, sizeof(te));
        char tstr[20];
        snprintf(tstr, sizeof(tstr), "%s-%s", tb, te);
        char title[64];
        snprintf(title, sizeof(title), "%s %s", c->course->code,
                 cw_get_lang() == CW_LANG_ZH ? c->course->title_zh
                                             : c->course->title_en);
        lv_obj_t *row = cw_row(card);
        lv_obj_t *lead = cw_row(row);
        lv_obj_set_width(lead, LV_SIZE_CONTENT);
        lv_obj_set_flex_align(lead, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_gap(lead, 5, 0);
        lv_obj_t *dot = lv_obj_create(lead);
        lv_obj_set_size(dot, 7, 7);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, lv_color_hex(c->course->color), 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        cw_label(lead, tstr);
        lv_obj_t *tl = NULL;
        if (compact) {
            tl = cw_label(row, title);
            lv_obj_set_flex_grow(tl, 1);
            lv_obj_set_width(tl, 0);
            lv_label_set_long_mode(tl, LV_LABEL_LONG_DOT);
        }
        if (st == CW_CLASS_DONE) {
            cw_tag(row, cw_str(CW_S_DONE), CW_C_GREEN);
        } else if (st == CW_CLASS_NOW || st == CW_CLASS_NEXT) {
            lv_obj_t *tag = cw_tag(row, cw_str(CW_S_NEXT), CW_C_AMBER);
            cw_anim_pulse(tag);
        }
        if (!compact) {
            tl = cw_label(card, title);
            lv_obj_set_width(tl, LV_PCT(100));
            lv_label_set_long_mode(tl, LV_LABEL_LONG_DOT);
        }
    }
}

static void build_grid(void) {
    // 顶栏:星期
    lv_obj_t *head = lv_obj_create(s_gridbox);
    lv_obj_set_width(head, LV_PCT(100));
    lv_obj_set_height(head, 18);
    lv_obj_set_style_bg_opa(head, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(head, 0, 0);
    lv_obj_set_style_pad_all(head, 0, 0);
    lv_obj_set_flex_flow(head, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(head, 2, 0);
    lv_obj_t *corner = lv_obj_create(head);
    lv_obj_set_size(corner, 12, 18);
    lv_obj_set_style_bg_opa(corner, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(corner, 0, 0);
    int today, now_min;
    cw_app_now_day_min(cw_app_mono_ms(), &today, &now_min);
    for (int d = 0; d < 7; d++) {
        lv_obj_t *h = lv_label_create(head);
        lv_label_set_text(h, day_short(d));
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
    lv_obj_set_height(grid, 186);
    lv_obj_set_pos(grid, 0, 20);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(grid, 1, 0);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF);

    // 七天挤在 220px 里,每列只有 28px:左侧节次槽压到 12px、列间距压到 1px,
    // 色块里才放得下 8px 的课程代码(14px 的 "MATH" 要 37px,必然被裁)。
    lv_obj_t *gut = lv_obj_create(grid);
    lv_obj_set_size(gut, 12, 186);
    lv_obj_set_style_bg_opa(gut, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(gut, 0, 0);
    lv_obj_set_style_pad_all(gut, 0, 0);
    for (int p = 1; p <= 7; p++) {
        lv_obj_t *l = lv_label_create(gut);
        char b[8];
        snprintf(b, sizeof(b), "%d", p);
        lv_label_set_text(l, b);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_8, 0);
        lv_obj_set_style_text_color(l, lv_color_hex(CW_C_MUTED), 0);
        lv_obj_set_pos(l, 2, (p - 1) * 26 + 8);
    }
    for (int d = 0; d < 7; d++) {
        lv_obj_t *col = lv_obj_create(grid);
        lv_obj_set_flex_grow(col, 1);
        lv_obj_set_height(col, 186);
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
            { int y = (classes[i].start_min - 480) * 26 / 60;
              int h = (classes[i].end_min - classes[i].start_min) * 26 / 60;
              if (h < 20) h = 20;
              lv_obj_set_size(blk, LV_PCT(100), h);
              lv_obj_set_pos(blk, 0, y + 1); }
            lv_obj_set_style_bg_color(blk, lv_color_hex(classes[i].course->color), 0);
            lv_obj_set_style_radius(blk, 4, 0);
            lv_obj_set_style_border_width(blk, 0, 0);
            lv_obj_set_style_pad_all(blk, 1, 0);
            lv_obj_set_flex_flow(blk, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(blk, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                                  LV_FLEX_ALIGN_CENTER);
            lv_obj_set_scrollbar_mode(blk, LV_SCROLLBAR_MODE_OFF);
            // 设计稿的色块是两行:院系前缀 + 课程号("MATH" / "100")。
            // 列宽只有 28px,8px 字体下 "MATH" 要 24.7px,已经贴边,教室放不下。
            // 这两行只能放 ASCII:montserrat_8 没有汉字和 "·"。
            const char *code = classes[i].course->code;
            const char *sp = strchr(code, ' ');
            char b[16];
            snprintf(b, sizeof(b), "%.*s", sp ? (int)(sp - code) : (int)strlen(code),
                     code);
            lv_obj_t *bl = lv_label_create(blk);
            lv_label_set_text(bl, b);
            lv_obj_set_style_text_font(bl, &lv_font_montserrat_8, 0);
            lv_obj_set_style_text_color(bl, lv_color_hex(0xFFFFFF), 0);
            snprintf(b, sizeof(b), "%s", sp ? sp + 1 : "");
            lv_obj_t *bn = lv_label_create(blk);
            lv_label_set_text(bn, b);
            lv_obj_set_style_text_font(bn, &lv_font_montserrat_8, 0);
            lv_obj_set_style_text_color(bn, lv_color_hex(0xE6EDFF), 0);
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
    // 列表视图正好吃满 244px 预算(30 视图切换 + 28 星期 + 174 列表),间距压到 6
    lv_obj_set_style_pad_gap(body, 6, 0);
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
    // 星期切换(仅列表视图需要,网格视图本身就画了七天)
    s_tabs = lv_obj_create(body);
    lv_obj_t *tabs = s_tabs;
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
        lv_label_set_text(l, day_short(i));
        lv_obj_set_style_text_font(l, cw_font_body(), 0);
        lv_obj_center(l);
        s_daytabs[i] = t;
    }
    s_listbox = lv_obj_create(body);
    lv_obj_set_width(s_listbox, LV_PCT(100));
    lv_obj_set_height(s_listbox, 174);
    lv_obj_set_style_bg_opa(s_listbox, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_listbox, 0, 0);
    lv_obj_set_style_pad_all(s_listbox, 0, 0);
    lv_obj_set_flex_flow(s_listbox, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(s_listbox, 6, 0);
    lv_obj_set_scrollbar_mode(s_listbox, LV_SCROLLBAR_MODE_AUTO);

    s_gridbox = lv_obj_create(body);
    lv_obj_set_width(s_gridbox, LV_PCT(100));
    lv_obj_set_height(s_gridbox, 206);
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
    int start_min;
    int end_min;
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
        if (classes[i].start_min == s_detail_ctx.start_min) {
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
    cw_format_time(s_detail_ctx.start_min, true, tb, sizeof(tb));
    cw_format_time(s_detail_ctx.end_min, true, te, sizeof(te));
    char when[64];
    snprintf(when, sizeof(when), "%s %s-%s", day_name(s_detail_ctx.day), tb, te);
    cw_label_small(card, when);
    const cw_teacher_t *t = cw_teacher(course->teacher);
    char tr[96];
    snprintf(tr, sizeof(tr), "%s, %s", course->room, t->name);
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
            cw_class_state_t st = cw_class_state(s_day, classes[i].start_min, classes[i].end_min, today, now_min);
            if (st == CW_CLASS_NOW || st == CW_CLASS_NEXT) {
                pick = i;
                break;
            }
        }
        if (pick < 0 && n > 0) pick = 0;
        if (pick >= 0) {
            s_detail_ctx.day = s_day;
            s_detail_ctx.start_min = classes[pick].start_min;
            s_detail_ctx.end_min = classes[pick].end_min;
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
