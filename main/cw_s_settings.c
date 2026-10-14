// cw_s_settings.c — 设置:语言 / 时间格式 / 温度单位 / Wi-Fi(v2) / 重置 / 关于。
#include "cw_app.h"
#include "cw_store.h"
#include "cw_ui.h"

#include <stdio.h>
#include <string.h>

static lv_obj_t *s_scr;
static int s_sel;
static bool s_confirm_reset;

typedef enum {
    ROW_LANG = 0,
    ROW_TIME,
    ROW_TEMP,
    ROW_WIFI,
    ROW_RESET,
    ROW_ABOUT,
    ROW_COUNT,
} row_t;

static void build(void);

static void settings_enter(void *ctx) {
    (void)ctx;
    s_scr = cw_screen_create(cw_str(CW_S_SETTINGS));
    s_sel = 0;
    s_confirm_reset = false;
    build();
    cw_hint_bar(s_scr, "UP/DN: Move", "OK: Change");
}

static lv_obj_t *add_seg_row(lv_obj_t *body, const char *name, const char *o1,
                             const char *o2, bool first_sel, int row_idx) {
    lv_obj_t *row = cw_set_row(body, name);
    lv_obj_set_style_bg_color(
        row, lv_color_hex(row_idx == s_sel ? 0x1B2A52 : 0x000000), 0);
    lv_obj_set_style_bg_opa(row, row_idx == s_sel ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(row, 8, 0);
    lv_obj_t *seg = cw_seg(row);
    cw_seg_add(seg, o1, first_sel);
    cw_seg_add(seg, o2, !first_sel);
    return row;
}

static void build(void) {
    lv_obj_t *body = cw_screen_content(s_scr);
    lv_obj_clean(body);
    cw_settings_t *s = cw_app_settings();

    add_seg_row(body, cw_str(CW_S_LANGUAGE), "EN", "中文", s->lang == CW_LANG_EN,
                ROW_LANG);
    add_seg_row(body, cw_str(CW_S_TIME_FORMAT), "12h", "24h", !s->time_24h, ROW_TIME);
    add_seg_row(body, cw_str(CW_S_TEMP_UNIT), "°C", "°F", s->temp_celsius, ROW_TEMP);

    lv_obj_t *wifi = cw_set_row(body, cw_str(CW_S_WIFI));
    lv_obj_set_style_bg_opa(wifi, s_sel == ROW_WIFI ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_color(wifi, lv_color_hex(0x1B2A52), 0);
    lv_obj_set_style_radius(wifi, 8, 0);
    cw_tag(wifi, "v2", CW_C_AMBER);

    lv_obj_t *reset = cw_set_row(
        body, s_confirm_reset ? cw_str(CW_S_CONFIRM_RESET) : cw_str(CW_S_RESET_DATA));
    lv_obj_set_style_bg_opa(reset, s_sel == ROW_RESET ? LV_OPA_COVER : LV_OPA_TRANSP,
                            0);
    lv_obj_set_style_bg_color(reset, lv_color_hex(0x1B2A52), 0);
    lv_obj_set_style_radius(reset, 8, 0);
    if (s_confirm_reset) {
        cw_tag(reset, "!", CW_C_RED);
    }

    lv_obj_t *about = cw_set_row(body, cw_str(CW_S_ABOUT));
    lv_obj_set_style_bg_opa(about, s_sel == ROW_ABOUT ? LV_OPA_COVER : LV_OPA_TRANSP,
                            0);
    lv_obj_set_style_bg_color(about, lv_color_hex(0x1B2A52), 0);
    lv_obj_set_style_radius(about, 8, 0);
    cw_label_small(about, "ClassWhiz v1.0");
}

static void do_reset(void) {
    cw_store_reset();
    cw_settings_defaults(cw_app_settings());
    cw_set_lang(cw_app_settings()->lang);
    memset(cw_app_task_done(), 0, CW_STORE_MAX_TASKS);
    cw_pomo_init(cw_app_pomo());
    s_confirm_reset = false;
}

static void settings_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (ev != BSP_BTN_CLICK) return;
    cw_settings_t *s = cw_app_settings();
    if (btn == BSP_BTN_UP) {
        s_sel = (s_sel + ROW_COUNT - 1) % ROW_COUNT;
        s_confirm_reset = false;
        build();
    } else if (btn == BSP_BTN_DOWN) {
        s_sel = (s_sel + 1) % ROW_COUNT;
        s_confirm_reset = false;
        build();
    } else if (btn == BSP_BTN_OK) {
        switch (s_sel) {
            case ROW_LANG:
                s->lang = (s->lang == CW_LANG_EN) ? CW_LANG_ZH : CW_LANG_EN;
                cw_set_lang(s->lang);
                cw_app_mark_dirty();
                s_sel = ROW_LANG;
                cw_app_refresh_top();
                break;
            case ROW_TIME:
                s->time_24h = !s->time_24h;
                cw_app_mark_dirty();
                build();
                break;
            case ROW_TEMP:
                s->temp_celsius = !s->temp_celsius;
                cw_app_mark_dirty();
                build();
                break;
            case ROW_WIFI:
                break; // v2
            case ROW_RESET:
                if (s_confirm_reset) {
                    do_reset();
                    s_sel = ROW_RESET;
                    cw_app_refresh_top();
                } else {
                    s_confirm_reset = true;
                    build();
                }
                break;
            case ROW_ABOUT:
                break;
        }
    }
}

const cw_screen_def_t cw_scr_settings_def = {
    .enter = settings_enter,
    .exit = NULL,
    .key = settings_key,
    .tick = NULL,
    .name = "settings",
};
