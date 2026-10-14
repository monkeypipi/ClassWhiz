// cw_s_teachers.c — 教师列表 / 教师详情。
#include "cw_app.h"
#include "cw_ui.h"

#include <stdio.h>
#include <string.h>

static lv_obj_t *s_scr;
static int s_sel;
static lv_obj_t *s_rows[4];
static lv_obj_t *s_tags[4];
static lv_obj_t *s_subs[4];

static const char *day_name(int d) {
    static const char *EN[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    static const char *ZH[7] = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};
    return (cw_get_lang() == CW_LANG_ZH) ? ZH[d] : EN[d];
}

// "Prof. Carter" -> "Carter";一行放不下带头衔的全名(只有 14px 字体可用)
static const char *short_name(const char *n) {
    const char *sp = strrchr(n, ' ');
    return sp ? sp + 1 : n;
}

// 下一次 office hour(相对今天);返回天数偏移
static bool next_oh(int teacher_idx, int today, int *out_day, int *out_idx) {
    const cw_teacher_t *t = cw_teacher(teacher_idx);
    for (int d = 0; d < 7; d++) {
        int day = (today + d) % 7;
        for (int h = 0; h < t->hour_count; h++) {
            if (t->hours[h].day == day) {
                if (out_day) *out_day = day;
                if (out_idx) *out_idx = h;
                return true;
            }
        }
    }
    return false;
}

static lv_obj_t *avatar(lv_obj_t *parent, int size, const char *name, uint32_t color) {
    lv_obj_t *av = lv_obj_create(parent);
    lv_obj_set_size(av, size, size);
    lv_obj_set_style_radius(av, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(av, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(av, 0, 0);
    lv_obj_set_style_pad_all(av, 0, 0);
    lv_obj_set_scrollbar_mode(av, LV_SCROLLBAR_MODE_OFF);
    char ini[4];
    snprintf(ini, sizeof(ini), "%c%c", name[0], short_name(name)[0]);
    lv_obj_t *il = lv_label_create(av);
    lv_label_set_text(il, ini);
    lv_obj_set_style_text_font(il, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(il, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(il);
    return av;
}

// 设计稿第二行是课程代码("CPSC 110 · CPSC 210"),不是系名 —— 系名 132px 塞不进
// 只剩 70px 的中栏。
static void course_list(int teacher_idx, char *out, size_t n) {
    const cw_teacher_t *t = cw_teacher(teacher_idx);
    out[0] = '\0';
    size_t used = 0;
    for (int i = 0; i < t->course_count && used + 1 < n; i++) {
        const cw_course_t *c = cw_course(t->courses[i]);
        if (!c) continue;
        int w = snprintf(out + used, n - used, "%s%s", used ? " · " : "", c->code);
        if (w < 0) break;
        used += (size_t)w;
    }
}

static void refresh(void) {
    int today, dummy;
    cw_app_now_day_min(cw_app_mono_ms(), &today, &dummy);
    for (int i = 0; i < cw_teacher_count(); i++) {
        lv_obj_set_style_border_color(s_rows[i],
                                      lv_color_hex(i == s_sel ? CW_C_ACCENT : CW_C_CARD_LINE),
                                      0);
        lv_obj_set_style_border_width(s_rows[i], i == s_sel ? 2 : 1, 0);
        int day = 0, hi = 0;
        char sub[64] = "";
        course_list(i, sub, sizeof(sub));
        if (next_oh(i, today, &day, &hi)) {
            const cw_teacher_t *t = cw_teacher(i);
            char b[16], full[96];
            cw_format_time(t->hours[hi].start_min, true, b, sizeof(b));
            lv_label_set_text(s_tags[i], day_name(day));
            // 角标只放星期、副行放时间:两者都塞进角标要 92.8px,中栏就只剩 67px,
            // "Prof. Carter" 会被截成 "Prof. C…"。
            snprintf(full, sizeof(full), "%s · %s", b, sub);
            lv_label_set_text(s_subs[i], full);
        } else {
            lv_label_set_text(s_tags[i], "");
            lv_label_set_text(s_subs[i], sub);
        }
    }
}

static void teachers_enter(void *ctx) {
    (void)ctx;
    s_scr = cw_screen_create(cw_str(CW_S_TEACHERS));
    lv_obj_t *body = cw_screen_content(s_scr);
    for (int i = 0; i < cw_teacher_count(); i++) {
        const cw_teacher_t *t = cw_teacher(i);
        lv_obj_t *row = cw_card(body);
        lv_obj_set_style_pad_all(row, 6, 0);
        lv_obj_t *r = cw_row(row);
        lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_gap(r, 8, 0);
        avatar(r, 30, t->name, i == 0 ? CW_C_ACCENT : CW_C_PURPLE);
        lv_obj_t *nm = cw_label(r, t->name);
        lv_obj_set_flex_grow(nm, 1);
        lv_obj_set_width(nm, 0);
        lv_label_set_long_mode(nm, LV_LABEL_LONG_DOT);
        s_tags[i] = cw_tag(r, "", CW_C_ACCENT);
        lv_obj_t *sub = cw_label_small(row, "");
        lv_obj_set_width(sub, LV_PCT(100));
        lv_label_set_long_mode(sub, LV_LABEL_LONG_DOT);
        s_subs[i] = sub;
        s_rows[i] = row;
    }
    // 本周总览:244px 预算(见 cw_ui.h)只够三条单行条目(时间区间 + 姓氏)
    lv_obj_t *oh = cw_card(body);
    lv_obj_set_style_pad_all(oh, 6, 0);
    cw_label(oh, cw_str(CW_S_OFFICE_HOURS_WEEK));
    cw_oh_entry_t entries[8];
    int n = cw_office_hours_week(entries, 8);
    if (n > 3) n = 3;
    for (int i = 0; i < n; i++) {
        const cw_teacher_t *t = cw_teacher(entries[i].teacher);
        char b[64], bs[8], be[8];
        cw_format_time(entries[i].start_min, true, bs, sizeof(bs));
        cw_format_time(entries[i].end_min, true, be, sizeof(be));
        snprintf(b, sizeof(b), "%s %s-%s %s", day_name(entries[i].day), bs, be,
                 short_name(t->name));
        lv_obj_t *ol = cw_label_small(oh, b);
        lv_obj_set_width(ol, LV_PCT(100));
        lv_label_set_long_mode(ol, LV_LABEL_LONG_DOT);
    }
    s_sel = 0;
    refresh();
    cw_hint_bar(s_scr, "UP/DN: Move", "OK: Profile");
}

static void teachers_exit(void) {
}

static int s_detail_idx;

static void detail_enter(void *ctx) {
    int idx = ctx ? *(int *)ctx : s_detail_idx;
    s_detail_idx = idx;
    const cw_teacher_t *t = cw_teacher(idx);
    s_scr = cw_screen_create(cw_str(CW_S_TEACHER));
    lv_obj_t *body = cw_screen_content(s_scr);
    lv_obj_set_style_pad_gap(body, 6, 0);

    lv_obj_t *card = cw_card(body);
    lv_obj_set_style_pad_all(card, 6, 0);
    lv_obj_t *r = cw_row(card);
    lv_obj_set_style_pad_gap(r, 8, 0);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    avatar(r, 34, t->name, CW_C_ACCENT);
    lv_obj_t *mid = cw_col(r);
    lv_obj_set_style_pad_gap(mid, 1, 0);
    lv_obj_set_flex_grow(mid, 1);
    cw_label(mid, t->name);
    cw_label_small(mid, cw_str(t->dept));

    // 办公室 / 邮箱压成两行键值对,否则三张卡超出内容预算
    lv_obj_t *c2 = cw_card(body);
    lv_obj_set_style_pad_all(c2, 6, 0);
    lv_obj_t *off = cw_row(c2);
    cw_label_small(off, cw_str(CW_S_OFFICE));
    cw_label(off, t->office);
    lv_obj_t *emr = cw_row(c2);
    cw_label_small(emr, cw_str(CW_S_EMAIL));
    lv_obj_t *em = cw_label(emr, t->email);
    lv_obj_set_style_text_color(em, lv_color_hex(0x7FB0FF), 0);

    lv_obj_t *c3 = cw_card(body);
    lv_obj_set_style_pad_all(c3, 6, 0);
    cw_label_small(c3, cw_str(CW_S_OFFICE_HOURS));
    int today, dummy;
    cw_app_now_day_min(cw_app_mono_ms(), &today, &dummy);
    int nd = 0, ni = 0;
    next_oh(idx, today, &nd, &ni);
    for (int h = 0; h < t->hour_count; h++) {
        char b[32], bs[8], be[8];
        cw_format_time(t->hours[h].start_min, true, bs, sizeof(bs));
        cw_format_time(t->hours[h].end_min, true, be, sizeof(be));
        snprintf(b, sizeof(b), "%s %s-%s", day_name(t->hours[h].day), bs, be);
        lv_obj_t *row2 = cw_row(c3);
        cw_label(row2, b);
        if (t->hours[h].day == nd && h == ni) {
            lv_obj_t *tag = cw_tag(row2, cw_str(CW_S_NEXT), CW_C_GREEN);
            cw_anim_pulse(tag);
        }
    }
    cw_hint_bar(s_scr, "Hold OK: Back", NULL);
}

static const cw_screen_def_t s_teacher_detail_def = {
    .enter = detail_enter,
    .exit = NULL,
    .key = NULL,
    .tick = NULL,
    .name = "teacher-detail",
};

static void teachers_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (ev != BSP_BTN_CLICK) return;
    int n = cw_teacher_count();
    if (btn == BSP_BTN_UP) {
        s_sel = (s_sel + n - 1) % n;
        refresh();
    } else if (btn == BSP_BTN_DOWN) {
        s_sel = (s_sel + 1) % n;
        refresh();
    } else if (btn == BSP_BTN_OK) {
        s_detail_idx = s_sel;
        cw_app_push(&s_teacher_detail_def, &s_detail_idx);
    }
}

const cw_screen_def_t cw_scr_teachers_def = {
    .enter = teachers_enter,
    .exit = teachers_exit,
    .key = teachers_key,
    .tick = NULL,
    .name = "teachers",
};
