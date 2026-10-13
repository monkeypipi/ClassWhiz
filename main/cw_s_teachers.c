// cw_s_teachers.c — 教师列表 / 教师详情。
#include "cw_app.h"
#include "cw_ui.h"

#include <stdio.h>

static lv_obj_t *s_scr;
static int s_sel;
static lv_obj_t *s_rows[4];

static const char *day_name(int d) {
    static const char *EN[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    static const char *ZH[7] = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};
    return (cw_get_lang() == CW_LANG_ZH) ? ZH[d] : EN[d];
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

static void refresh(void) {
    int today, dummy;
    cw_app_now_day_min(cw_app_mono_ms(), &today, &dummy);
    for (int i = 0; i < cw_teacher_count(); i++) {
        lv_obj_set_style_border_color(s_rows[i],
                                      lv_color_hex(i == s_sel ? CW_C_ACCENT : CW_C_CARD_LINE),
                                      0);
        lv_obj_set_style_border_width(s_rows[i], i == s_sel ? 2 : 1, 0);
        lv_obj_t *tag = (lv_obj_t *)lv_obj_get_user_data(s_rows[i]);
        int day = 0, hi = 0;
        if (next_oh(i, today, &day, &hi)) {
            const cw_teacher_t *t = cw_teacher(i);
            char b[24], e[8];
            cw_format_time(t->hours[hi].start_min, true, b, sizeof(b));
            (void)e;
            char txt[32];
            snprintf(txt, sizeof(txt), "%s %s", day_name(day), b);
            lv_label_set_text(tag, txt);
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
        lv_obj_t *r = lv_obj_create(row);
        lv_obj_set_width(r, LV_PCT(100));
        lv_obj_set_style_bg_opa(r, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(r, 0, 0);
        lv_obj_set_style_pad_all(r, 0, 0);
        lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_gap(r, 8, 0);
        lv_obj_t *av = lv_obj_create(r);
        lv_obj_set_size(av, 30, 30);
        lv_obj_set_style_radius(av, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(av, lv_color_hex(i == 0 ? CW_C_ACCENT : CW_C_PURPLE),
                                  0);
        lv_obj_set_style_border_width(av, 0, 0);
        char ini[8];
        snprintf(ini, sizeof(ini), "%c%c", t->name[0], t->name[6]);
        lv_obj_t *il = lv_label_create(av);
        lv_label_set_text(il, ini);
        lv_obj_set_style_text_font(il, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(il, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(il);
        lv_obj_t *mid = lv_obj_create(r);
    lv_obj_set_flex_flow(mid, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(mid, 1, 0);
        lv_obj_set_flex_grow(mid, 1);
        lv_obj_set_style_bg_opa(mid, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(mid, 0, 0);
        lv_obj_set_style_pad_all(mid, 0, 0);
        cw_label(mid, t->name);
        cw_label_small(mid, cw_str(t->dept));
        lv_obj_t *tag = cw_tag(r, "", CW_C_ACCENT);
        lv_obj_set_user_data(row, tag);
        s_rows[i] = row;
    }
    // 本周总览
    lv_obj_t *oh = cw_card(body);
    cw_label(oh, cw_str(CW_S_OFFICE_HOURS_WEEK));
    cw_oh_entry_t entries[8];
    int n = cw_office_hours_week(entries, 8);
    for (int i = 0; i < n; i++) {
        const cw_teacher_t *t = cw_teacher(entries[i].teacher);
        char b[64], bs[8], be[8];
        cw_format_time(entries[i].start_min, true, bs, sizeof(bs));
        cw_format_time(entries[i].end_min, true, be, sizeof(be));
        snprintf(b, sizeof(b), "%s %s-%s · %s", day_name(entries[i].day), bs, be,
                 t->name);
        cw_label_small(oh, b);
    }
    s_sel = 0;
    refresh();
    cw_hint_bar(s_scr, "UP/DN: Move", "OK: Profile");
    cw_screen_set_scroll(s_scr, true);
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
    lv_obj_t *card = cw_card(body);
    lv_obj_t *r = lv_obj_create(card);
    lv_obj_set_width(r, LV_PCT(100));
    lv_obj_set_style_bg_opa(r, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(r, 0, 0);
    lv_obj_set_style_pad_all(r, 0, 0);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(r, 8, 0);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_t *av = lv_obj_create(r);
    lv_obj_set_size(av, 40, 40);
    lv_obj_set_style_radius(av, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(av, lv_color_hex(CW_C_ACCENT), 0);
    lv_obj_set_style_border_width(av, 0, 0);
    char ini[8];
    snprintf(ini, sizeof(ini), "%c%c", t->name[0], t->name[6]);
    lv_obj_t *il = lv_label_create(av);
    lv_label_set_text(il, ini);
    lv_obj_set_style_text_font(il, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(il, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(il);
    lv_obj_t *mid = lv_obj_create(r);
    lv_obj_set_flex_flow(mid, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(mid, 1, 0);
    lv_obj_set_flex_grow(mid, 1);
    lv_obj_set_style_bg_opa(mid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mid, 0, 0);
    lv_obj_set_style_pad_all(mid, 0, 0);
    lv_obj_t *nm = cw_label(mid, t->name);
    lv_obj_set_style_text_font(nm, cw_font_title(), 0);
    cw_label_small(mid, cw_str(t->dept));

    lv_obj_t *c2 = cw_card(body);
    cw_label_small(c2, cw_str(CW_S_OFFICE));
    cw_label(c2, t->office);
    cw_label_small(c2, cw_str(CW_S_EMAIL));
    lv_obj_t *em = cw_label(c2, t->email);
    lv_obj_set_style_text_color(em, lv_color_hex(0x7FB0FF), 0);

    lv_obj_t *c3 = cw_card(body);
    cw_label_small(c3, cw_str(CW_S_OFFICE_HOURS));
    int today, dummy;
    cw_app_now_day_min(cw_app_mono_ms(), &today, &dummy);
    int nd = 0, ni = 0;
    next_oh(idx, today, &nd, &ni);
    for (int h = 0; h < t->hour_count; h++) {
        char b[48], bs[8], be[8];
        cw_format_time(t->hours[h].start_min, true, bs, sizeof(bs));
        cw_format_time(t->hours[h].end_min, true, be, sizeof(be));
        snprintf(b, sizeof(b), "%s  %s-%s", day_name(t->hours[h].day), bs, be);
        lv_obj_t *row2 = lv_obj_create(c3);
        lv_obj_set_width(row2, LV_PCT(100));
        lv_obj_set_style_bg_opa(row2, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row2, 0, 0);
        lv_obj_set_style_pad_all(row2, 0, 0);
        lv_obj_set_flex_flow(row2, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row2, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
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
