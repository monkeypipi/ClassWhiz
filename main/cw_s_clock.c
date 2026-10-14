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
        int temp = cw_app_settings()->temp_celsius
                       ? c->temp_c
                       : (c->temp_c * 9 / 5 + 32);

        if (c->is_home) {
            // 设计稿:本城是大卡。两行才放得下 名字 + HOME + 时区 + 天气 + 图标 + 大钟,
            // 挤在一行会把 182px 的可用宽度撑爆(见 cw_ui.h 的预算说明)。
            lv_obj_t *top = cw_row(card);
            lv_obj_set_flex_align(top, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                                  LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_gap(top, 6, 0);
            cw_label(top, c->name);
            cw_tag(top, "HOME", CW_C_GREEN);
            char off[8];
            snprintf(off, sizeof(off), "UTC%+d", c->utc_offset);
            cw_label_small(top, off);

            lv_obj_t *bot = cw_row(card);
            lv_obj_set_flex_align(bot, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                                  LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_gap(bot, 6, 0);
            lv_obj_t *left = cw_col(bot);
            lv_obj_set_style_pad_gap(left, 1, 0);
            // 宽度先归零再 grow:里面的天气标签是 PCT(100),父级若按内容取宽会算不出来
            lv_obj_set_width(left, 0);
            lv_obj_set_flex_grow(left, 1);
            lv_obj_t *wxl = cw_label_small(left, cw_str(c->weather));
            lv_obj_set_width(wxl, LV_PCT(100));
            lv_label_set_long_mode(wxl, LV_LABEL_LONG_WRAP);
            char tb[8];
            snprintf(tb, sizeof(tb), "%d°", temp);
            lv_obj_t *tl = lv_label_create(left);
            lv_label_set_text(tl, tb);
            lv_obj_set_style_text_font(tl, &lv_font_montserrat_20, 0);
            lv_obj_set_style_text_color(tl, lv_color_hex(CW_C_TEXT), 0);
            cw_weather_icon(bot, wx_kind(i), 40);
            s_clocks[i] = cw_analog_clock(bot, 46, CW_C_ACCENT);
            s_times[i] = NULL;
            continue;
        }

        // 其余城市是小行:小钟 + 名字/天气 + 数字时间
        lv_obj_t *row = cw_row(card);
        lv_obj_set_style_pad_gap(row, 6, 0);
        s_clocks[i] = cw_analog_clock(row, 34, i == 1 ? CW_C_PURPLE : CW_C_AMBER);
        lv_obj_t *left = cw_col(row);
        lv_obj_set_style_pad_gap(left, 1, 0);
        lv_obj_set_width(left, 0);
        lv_obj_set_flex_grow(left, 1);
        cw_label(left, c->name);
        char wx[64];
        snprintf(wx, sizeof(wx), "%+d %s %d°", c->utc_offset, cw_str(c->weather), temp);
        lv_obj_t *wxl = cw_label_small(left, wx);
        lv_obj_set_width(wxl, LV_PCT(100));
        lv_label_set_long_mode(wxl, LV_LABEL_LONG_DOT);
        s_times[i] = lv_label_create(row);
        lv_label_set_text(s_times[i], "--:--");
        lv_obj_set_style_text_font(s_times[i], &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(s_times[i], lv_color_hex(CW_C_TEXT), 0);
    }
    cw_label_small(body, cw_str(CW_S_DEMO_DATA));
    cw_hint_bar(s_scr, "Hold OK: Back", NULL);
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
        if (s_times[i]) lv_label_set_text(s_times[i], b);
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
