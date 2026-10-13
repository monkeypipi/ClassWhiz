// cw_s_clock.c — 世界时钟 + 天气(演示数据)。
#include "cw_app.h"
#include "cw_ui.h"

#include <stdio.h>

static lv_obj_t *s_scr;
static lv_obj_t *s_clocks[CW_MAX_CITIES];
static lv_obj_t *s_times[CW_MAX_CITIES];

static int wx_kind(int city_idx) {
    // 0=多云 1=晴 2=雨
    static const int KINDS[3] = {0, 1, 2};
    return KINDS[city_idx % 3];
}

static void clock_enter(void *ctx) {
    (void)ctx;
    s_scr = cw_screen_create(cw_str(CW_S_WORLD_CLOCK));
    lv_obj_t *body = cw_screen_content(s_scr);

    for (int i = 0; i < cw_city_count(); i++) {
        const cw_city_t *c = cw_city(i);
        lv_obj_t *card = cw_card(body);
        if (c->is_home) {
            lv_obj_set_style_border_color(card, lv_color_hex(CW_C_ACCENT), 0);
        }
        lv_obj_t *row = lv_obj_create(card);
        lv_obj_set_width(row, LV_PCT(100));
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        lv_obj_t *left = lv_obj_create(row);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(left, 1, 0);
        lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(left, 0, 0);
        lv_obj_set_style_pad_all(left, 0, 0);
        lv_obj_t *nmrow = lv_obj_create(left);
        lv_obj_set_style_bg_opa(nmrow, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(nmrow, 0, 0);
        lv_obj_set_style_pad_all(nmrow, 0, 0);
        lv_obj_set_flex_flow(nmrow, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_gap(nmrow, 6, 0);
        lv_obj_set_flex_align(nmrow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        cw_label(nmrow, c->name);
        if (c->is_home) cw_tag(nmrow, "HOME", CW_C_GREEN);
        char wx[64];
        int temp = cw_app_settings()->temp_celsius
                       ? c->temp_c
                       : (c->temp_c * 9 / 5 + 32);
        snprintf(wx, sizeof(wx), "%s %d°", cw_str(c->weather), temp);
        cw_label_small(left, wx);

        lv_obj_t *right = lv_obj_create(row);
        lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(right, 0, 0);
        lv_obj_set_style_pad_all(right, 0, 0);
        lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_gap(right, 6, 0);
        cw_weather_icon(right, wx_kind(i), 36);
        s_times[i] = lv_label_create(right);
        lv_obj_set_style_text_font(s_times[i], &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(s_times[i], lv_color_hex(CW_C_TEXT), 0);
        s_clocks[i] = cw_analog_clock(right, 44,
                                      i == 0 ? CW_C_ACCENT : (i == 1 ? CW_C_PURPLE : CW_C_AMBER));
    }
    cw_label_small(body, cw_str(CW_S_DEMO_DATA));
    cw_hint_bar(s_scr, "Hold OK: Back", NULL);
    cw_screen_set_scroll(s_scr, true);
}

static void clock_tick(uint64_t now_ms) {
    int minute = 0;
    cw_app_now_day_min(now_ms, NULL, &minute);
    int second = (int)((now_ms / 1000) % 60);
    // 基准:温哥华(UTC-7)
    for (int i = 0; i < cw_city_count(); i++) {
        const cw_city_t *c = cw_city(i);
        int cm = minute + (c->utc_offset + 7) * 60;
        cm = ((cm % 1440) + 1440) % 1440;
        char b[16];
        cw_format_time(cm, cw_app_settings()->time_24h, b, sizeof(b));
        lv_label_set_text(s_times[i], b);
        cw_clock_tick(s_clocks[i], cm / 60, cm % 60, second);
    }
}

const cw_screen_def_t cw_scr_clock_def = {
    .enter = clock_enter,
    .exit = NULL,
    .key = NULL,
    .tick = clock_tick,
    .name = "worldclock",
};
