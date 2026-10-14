// cw_s_student.c — 电子学生证。
#include "cw_app.h"
#include "cw_ui.h"
#include "cw_photo.h"
#include "cw_config.h"

#include <stdio.h>
#include <stdlib.h>

// 照片 LVGL 描述符(静态,避免重复分配)
static lv_img_dsc_t s_photo_dsc;
static uint8_t *s_photo_buf = NULL;
static bool s_photo_loaded = false;

static bool load_student_photo(void) {
    if (s_photo_loaded) return s_photo_buf != NULL;
    s_photo_loaded = true;

    if (!cw_photo_exists()) return false;

    FILE *f = fopen(cw_photo_path(), "rb");
    if (!f) return false;

    s_photo_buf = malloc(CW_PHOTO_SIZE);
    if (!s_photo_buf) {
        fclose(f);
        return false;
    }

    size_t n = fread(s_photo_buf, 1, CW_PHOTO_SIZE, f);
    fclose(f);

    if (n != CW_PHOTO_SIZE) {
        free(s_photo_buf);
        s_photo_buf = NULL;
        return false;
    }

    s_photo_dsc.header.always_zero = 0;
    s_photo_dsc.header.w = CW_PHOTO_W;
    s_photo_dsc.header.h = CW_PHOTO_H;
    s_photo_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    s_photo_dsc.data_size = CW_PHOTO_SIZE;
    s_photo_dsc.data = s_photo_buf;

    return true;
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
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(card, 6, 0);

    // 卡内可用宽 194px(220 - 左右 pad 12 - 边框 1),14px 字体下 "UNIVERSITY · STUDENT
    // CARD" 要 208px 会被裁掉,所以英文缩成 STUDENT ID;中文按设计稿原样。
    lv_obj_t *t = cw_label_small(card, cw_get_lang() == CW_LANG_ZH
                                           ? "大学 · 学生证"
                                           : "UNIVERSITY · STUDENT ID");
    lv_obj_set_style_text_color(t, lv_color_hex(0x9DC2FF), 0);

    // 卡内可用宽 194px:"ZHANG Hang" 一项就要 95.8px,头像 44 + 二维码 52 + 两个
    // 10px 间距会撑到 212px 把二维码挤出去,所以缩到头像 36 / 二维码 48 / 间距 6。
    lv_obj_t *row = cw_row(card);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row, 6, 0);

    // 头像:有照片显示照片,无照片显示首字母
    lv_obj_t *av;
    if (load_student_photo()) {
        av = lv_img_create(row);
        lv_img_set_src(av, &s_photo_dsc);
        lv_obj_set_size(av, 48, 48);
        lv_obj_set_style_radius(av, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_clip_corner(av, true, 0);
    } else {
        av = lv_obj_create(row);
        lv_obj_set_size(av, 36, 36);
        lv_obj_set_style_radius(av, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(av, lv_color_hex(CW_C_ACCENT), 0);
        lv_obj_set_style_border_width(av, 0, 0);
        lv_obj_t *il = lv_label_create(av);
        lv_label_set_text(il, "HZ");
        lv_obj_set_style_text_font(il, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(il, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(il);
    }

    lv_obj_t *mid = cw_col(row);
    lv_obj_set_style_pad_gap(mid, 1, 0);
    lv_obj_set_flex_grow(mid, 1);
    lv_obj_t *nm = cw_label(mid, st->name);
    lv_obj_set_style_text_font(nm, cw_font_title(), 0);
    lv_obj_t *no = cw_label(mid, st->student_no);
    lv_obj_set_style_text_color(no, lv_color_hex(0x9DC2FF), 0);

    // 二维码占位(演示):棋盘格
    lv_obj_t *qr = lv_obj_create(row);
    lv_obj_set_size(qr, 48, 48);
    lv_obj_set_style_bg_color(qr, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(qr, 6, 0);
    lv_obj_set_style_border_width(qr, 0, 0);
    lv_obj_set_style_pad_all(qr, 4, 0);
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

    // 专业一行 219.9px 超出卡内 194px,允许折行(卡片高度还有 30px 余量)
    lv_obj_t *pg = cw_label(card,
                            cw_get_lang() == CW_LANG_ZH ? st->program_zh : st->program_en);
    lv_obj_set_style_text_color(pg, lv_color_hex(0xC6D0E4), 0);
    lv_obj_set_width(pg, LV_PCT(100));
    lv_label_set_long_mode(pg, LV_LABEL_LONG_WRAP);
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
                                                     : "Demo ID · QR = student no.");
    cw_hint_bar(scr, "Hold OK: Back", NULL);
}

const cw_screen_def_t cw_scr_student_def = {
    .enter = student_enter,
    .exit = NULL,
    .key = NULL,
    .tick = NULL,
    .name = "student",
};
