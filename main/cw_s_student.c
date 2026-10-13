// cw_s_student.c — 电子学生证。
#include "cw_app.h"
#include "cw_ui.h"

static void anim_x_cb(void *var, int32_t v) {
    lv_obj_set_x((lv_obj_t *)var, (int16_t)v);
}

static void student_enter(void *ctx) {
    (void)ctx;
    lv_obj_t *scr = cw_screen_create(cw_str(CW_S_STUDENT_ID));
    lv_obj_t *body = cw_screen_content(scr);
    const cw_student_t *st = cw_student();

    lv_obj_t *card = lv_obj_create(body);
    lv_obj_set_width(card, LV_PCT(100));
    lv_obj_set_height(card, 190);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1E3A6E), 0);
    lv_obj_set_style_bg_grad_color(card, lv_color_hex(0x0E1A38), 0);
    lv_obj_set_style_bg_grad_dir(card, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(CW_C_ACCENT), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_opa(card, LV_OPA_50, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_set_style_shadow_width(card, 12, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(CW_C_ACCENT), 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_30, 0);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *t = cw_label_small(card, "UNIVERSITY · STUDENT CARD");
    lv_obj_set_style_text_color(t, lv_color_hex(0x9DC2FF), 0);

    lv_obj_t *row = lv_obj_create(card);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row, 10, 0);

    lv_obj_t *av = lv_obj_create(row);
    lv_obj_set_size(av, 44, 44);
    lv_obj_set_style_radius(av, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(av, lv_color_hex(CW_C_ACCENT), 0);
    lv_obj_set_style_border_width(av, 0, 0);
    lv_obj_t *il = lv_label_create(av);
    lv_label_set_text(il, "HZ");
    lv_obj_set_style_text_font(il, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(il, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(il);

    lv_obj_t *mid = lv_obj_create(row);
    lv_obj_set_flex_flow(mid, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(mid, 1, 0);
    lv_obj_set_flex_grow(mid, 1);
    lv_obj_set_style_bg_opa(mid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mid, 0, 0);
    lv_obj_set_style_pad_all(mid, 0, 0);
    lv_obj_t *nm = cw_label(mid, st->name);
    lv_obj_set_style_text_font(nm, cw_font_title(), 0);
    lv_obj_t *no = cw_label(mid, st->student_no);
    lv_obj_set_style_text_color(no, lv_color_hex(0x9DC2FF), 0);

    // 二维码占位(演示):棋盘格
    lv_obj_t *qr = lv_obj_create(row);
    lv_obj_set_size(qr, 52, 52);
    lv_obj_set_style_bg_color(qr, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(qr, 6, 0);
    lv_obj_set_style_border_width(qr, 0, 0);
    lv_obj_set_style_pad_all(qr, 5, 0);
    lv_obj_set_layout(qr, LV_LAYOUT_GRID);
    static const int32_t col[] = {7, 7, 7, 7, 7, LV_GRID_TEMPLATE_LAST};
    static const int32_t r[] = {7, 7, 7, 7, 7, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(qr, col, r);
    lv_obj_set_style_pad_gap(qr, 1, 0);
    lv_obj_set_scrollbar_mode(qr, LV_SCROLLBAR_MODE_OFF);
    for (int i = 0; i < 25; i++) {
        lv_obj_t *px = lv_obj_create(qr);
        lv_obj_set_grid_cell(px, LV_GRID_ALIGN_STRETCH, i % 5, 1,
                             LV_GRID_ALIGN_STRETCH, i / 5, 1);
        bool dark = ((i * 7 + 3) % 5) < 3;
        lv_obj_set_style_bg_color(px, lv_color_hex(dark ? 0x0D1424 : 0xFFFFFF), 0);
        lv_obj_set_style_border_width(px, 0, 0);
        lv_obj_set_style_pad_all(px, 0, 0);
        lv_obj_set_style_radius(px, 0, 0);
    }

    lv_obj_t *pg = cw_label(card,
                            cw_get_lang() == CW_LANG_ZH ? st->program_zh : st->program_en);
    lv_obj_set_style_text_color(pg, lv_color_hex(0xC6D0E4), 0);
    char vt[48];
    snprintf(vt, sizeof(vt), "%s %s",
             cw_get_lang() == CW_LANG_ZH ? "有效期至" : "Valid thru", st->valid_thru);
    cw_label_small(card, vt);

    // 流光(浮动,不参与卡片流式布局)
    lv_obj_t *shine = lv_obj_create(card);
    lv_obj_add_flag(shine, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_size(shine, 30, 190);
    lv_obj_set_style_bg_color(shine, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(shine, LV_OPA_10, 0);
    lv_obj_set_style_border_width(shine, 0, 0);
    lv_obj_set_style_pad_all(shine, 0, 0);
    lv_obj_set_pos(shine, -40, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, shine);
    lv_anim_set_exec_cb(&a, anim_abs_x_cb);
    lv_anim_set_values(&a, -40, 240);
    lv_anim_set_duration(&a, 3800);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_delay(&a, 1200);
    lv_anim_start(&a);

    cw_label_small(body, cw_get_lang() == CW_LANG_ZH ? "演示身份 · 二维码编码学号"
                                                    : "Demo identity · QR = student no.");
    cw_hint_bar(scr, "Hold OK: Back", NULL);
}

const cw_screen_def_t cw_scr_student_def = {
    .enter = student_enter,
    .exit = NULL,
    .key = NULL,
    .tick = NULL,
    .name = "student",
};
