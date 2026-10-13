// cw_s_home.c — Today(首页)与 Apps(应用抽屉)。
#include "cw_app.h"
#include "cw_ui.h"

#include <stdio.h>
#include <string.h>

// ---------- Today ----------
static lv_obj_t *s_scr;
static lv_obj_t *s_clock;
static lv_obj_t *s_time_label;
static lv_obj_t *s_next_card;
static lv_obj_t *s_next_title;
static lv_obj_t *s_next_meta;
static lv_obj_t *s_next_tag;
static lv_obj_t *s_count_label;

static void today_enter(void *ctx) {
    (void)ctx;
    s_scr = cw_screen_create(NULL); // 自定义标题区
    lv_obj_t *body = cw_screen_content(s_scr);

    // Hero:大时钟 + 模拟钟
    lv_obj_t *hero = lv_obj_create(body);
    lv_obj_set_width(hero, LV_PCT(100));
    lv_obj_set_height(hero, 92);
    lv_obj_set_style_bg_color(hero, lv_color_hex(0x1B2A52), 0);
    lv_obj_set_style_bg_grad_color(hero, lv_color_hex(0x0E1A38), 0);
    lv_obj_set_style_bg_grad_dir(hero, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_color(hero, lv_color_hex(0x2C3F6E), 0);
    lv_obj_set_style_border_width(hero, 1, 0);
    lv_obj_set_style_radius(hero, 12, 0);
    lv_obj_set_style_pad_all(hero, 8, 0);
    lv_obj_set_flex_flow(hero, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hero, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(hero, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *left = lv_obj_create(hero);
    lv_obj_set_size(left, 148, 76);
    lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left, 0, 0);
    lv_obj_set_style_pad_all(left, 0, 0);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(left, 1, 0);
    lv_obj_set_scrollbar_mode(left, LV_SCROLLBAR_MODE_OFF);
    s_time_label = lv_label_create(left);
    lv_obj_set_style_text_font(s_time_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_time_label, lv_color_hex(CW_C_TEXT), 0);
    lv_label_set_text(s_time_label, "--:--");
    // 用更大字号模拟:放大 1.6 倍(256=100%)
    lv_obj_set_style_transform_scale(s_time_label, 410, 0);
    lv_obj_t *date = cw_label_small(
        left, cw_get_lang() == CW_LANG_ZH ? "10月13日 周二" : "TUE, OCT 13");
    lv_obj_set_style_text_color(date, lv_color_hex(0xBCD0F5), 0);
    lv_obj_t *greet = cw_label_small(left, cw_str(CW_S_GOOD_AFTERNOON));
    (void)greet;

    s_clock = cw_analog_clock(hero, 64, CW_C_ACCENT);

    // 下节课卡片
    s_next_card = cw_card(body);
    lv_obj_set_style_border_color(s_next_card, lv_color_hex(CW_C_ACCENT), 0);
    lv_obj_t *row = lv_obj_create(s_next_card);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    s_next_tag = cw_tag(row, "", CW_C_AMBER);
    cw_anim_pulse(s_next_tag);
    lv_obj_t *ntime = cw_label_small(row, "");
    lv_obj_set_user_data(s_next_card, ntime);
    s_next_title = cw_label(s_next_card, "");
    lv_obj_set_style_text_font(s_next_title, cw_font_title(), 0);
    s_next_meta = cw_label_small(s_next_card, "");

    // 今日课程数
    s_count_label = cw_label_small(body, "");

    // 天气卡片
    lv_obj_t *wx = cw_card(body);
    lv_obj_t *wxrow = lv_obj_create(wx);
    lv_obj_set_width(wxrow, LV_PCT(100));
    lv_obj_set_style_bg_opa(wxrow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wxrow, 0, 0);
    lv_obj_set_style_pad_all(wxrow, 0, 0);
    lv_obj_set_flex_flow(wxrow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wxrow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_t *wxleft = lv_obj_create(wxrow);
    lv_obj_set_style_bg_opa(wxleft, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wxleft, 0, 0);
    lv_obj_set_style_pad_all(wxleft, 0, 0);
    lv_obj_set_flex_flow(wxleft, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(wxleft, 2, 0);
    cw_label(wxleft, "Vancouver");
    lv_obj_t *temp = lv_label_create(wxleft);
    lv_label_set_text(temp, "14°");
    lv_obj_set_style_text_font(temp, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(temp, lv_color_hex(CW_C_TEXT), 0);
    cw_label_small(wxleft, cw_str(CW_S_WX_PARTLY_CLOUDY));
    cw_weather_icon(wxrow, 0, 52);

    cw_hint_bar(s_scr, "OK: Apps", "Hold OK: Back");
    cw_screen_set_scroll(s_scr, true);
}

static void today_exit(void) {
}

static void today_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (ev == BSP_BTN_CLICK && btn == BSP_BTN_OK) {
        cw_app_push(&cw_scr_apps_def, NULL);
    }
}

static void today_tick(uint64_t now_ms) {
    int day, minute;
    cw_app_now_day_min(now_ms, &day, &minute);
    int second = (int)((now_ms / 1000) % 60);
    char tbuf[16];
    cw_format_time(minute, cw_app_settings()->time_24h, tbuf, sizeof(tbuf));
    lv_label_set_text(s_time_label, tbuf);
    cw_clock_tick(s_clock, minute / 60, minute % 60, second);

    int nday = 0;
    cw_class_t cls;
    if (cw_next_class(day, minute, &nday, &cls)) {
        int start = cw_period_start_min(cls.period);
        int mins_left = (nday - day) * 1440 + start - minute;
        char dur[24];
        cw_format_dur(mins_left, dur, sizeof(dur));
        char tag[40];
        if (cw_get_lang() == CW_LANG_ZH) {
            snprintf(tag, sizeof(tag), "%s后开始", dur);
        } else {
            snprintf(tag, sizeof(tag), "IN %s", dur);
        }
        lv_label_set_text(s_next_tag, tag);
        char tb[16];
        char te[16];
        cw_format_time(start, true, tb, sizeof(tb));
        cw_format_time(cw_period_end_min(cls.period), true, te, sizeof(te));
        char ntime[40];
        snprintf(ntime, sizeof(ntime), "%s-%s", tb, te);
        lv_obj_t *ntime_l = (lv_obj_t *)lv_obj_get_user_data(s_next_card);
        lv_label_set_text(ntime_l, ntime);
        char title[64];
        snprintf(title, sizeof(title), "%s", cls.course->code);
        lv_label_set_text(s_next_title, title);
        char meta[80];
        snprintf(meta, sizeof(meta), "%s · %s",
                 cls.course->room, cw_teacher(cls.course->teacher)->name);
        lv_label_set_text(s_next_meta, meta);
    }
    cw_class_t classes[CW_MAX_SLOTS_PER_DAY];
    int n = cw_classes_for_day(day, classes, CW_MAX_SLOTS_PER_DAY);
    char cnt[48];
    if (cw_get_lang() == CW_LANG_ZH) {
        snprintf(cnt, sizeof(cnt), "今日课程 · %d", n);
    } else {
        snprintf(cnt, sizeof(cnt), "Today's classes · %d", n);
    }
    lv_label_set_text(s_count_label, cnt);
}

const cw_screen_def_t cw_scr_today_def = {
    .enter = today_enter,
    .exit = today_exit,
    .key = today_key,
    .tick = today_tick,
    .name = "today",
};

// ---------- Apps ----------
typedef struct {
    const cw_screen_def_t *def;
    cw_str_id_t name;
    uint32_t g1;
    uint32_t g2;
} app_entry_t;

static const app_entry_t APPS[9] = {
    {&cw_scr_schedule_def, CW_S_SCHEDULE, 0x4C8DFF, 0x2F6FE0},
    {&cw_scr_teachers_def, CW_S_TEACHERS, 0x8B5CF6, 0x6D3FF0},
    {&cw_scr_calendar_def, CW_S_CALENDAR, 0xFFB020, 0xF08C00},
    {&cw_scr_student_def, CW_S_STUDENT_ID, 0x22B8A8, 0x0E8A7D},
    {&cw_scr_tasks_def, CW_S_TASKS, 0xFF8A5C, 0xF06423},
    {&cw_scr_clock_def, CW_S_WORLD_CLOCK, 0x4CE0C3, 0x1DA88F},
    {&cw_scr_pomodoro_def, CW_S_POMODORO, 0x34C77B, 0x1D9A58},
    {&cw_scr_voice_def, CW_S_VOICE, 0xC35CFF, 0x9333E8},
    {&cw_scr_settings_def, CW_S_SETTINGS, 0x5B6B82, 0x3C4759},
};

static lv_obj_t *s_apps_scr;
static lv_obj_t *s_tiles[9];
static int s_app_sel;

static void apps_refresh(void) {
    for (int i = 0; i < 9; i++) {
        lv_obj_set_style_border_width(s_tiles[i], i == s_app_sel ? 2 : 0, 0);
        lv_obj_set_style_border_color(s_tiles[i], lv_color_hex(0xFFFFFF), 0);
    }
}

static void apps_enter(void *ctx) {
    (void)ctx;
    s_apps_scr = cw_screen_create(cw_str(CW_S_APPS));
    lv_obj_t *body = cw_screen_content(s_apps_scr);
    lv_obj_t *grid = lv_obj_create(body);
    lv_obj_set_size(grid, LV_PCT(100), 250);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    static const int32_t col[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
                                  LV_GRID_TEMPLATE_LAST};
    static const int32_t row[] = {68, 68, 68, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid, col, row);
    lv_obj_set_style_pad_gap(grid, 8, 0);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF);

    for (int i = 0; i < 9; i++) {
        lv_obj_t *t = lv_obj_create(grid);
        lv_obj_set_grid_cell(t, LV_GRID_ALIGN_STRETCH, i % 3, 1,
                             LV_GRID_ALIGN_STRETCH, i / 3, 1);
        lv_obj_set_style_bg_color(t, lv_color_hex(APPS[i].g1), 0);
        lv_obj_set_style_bg_grad_color(t, lv_color_hex(APPS[i].g2), 0);
        lv_obj_set_style_bg_grad_dir(t, LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_radius(t, 12, 0);
        lv_obj_set_style_border_width(t, 0, 0);
        lv_obj_set_style_pad_all(t, 4, 0);
        lv_obj_set_style_shadow_width(t, 6, 0);
        lv_obj_set_style_shadow_color(t, lv_color_hex(APPS[i].g1), 0);
        lv_obj_set_style_shadow_opa(t, LV_OPA_30, 0);
        // 首字母徽标(取首个完整 UTF-8 字符,兼容中文)
        const char *nm = cw_str(APPS[i].name);
        char ini[8] = {0};
        {
            const unsigned char *p = (const unsigned char *)nm;
            int clen = 1;
            if (*p >= 0xF0) clen = 4;
            else if (*p >= 0xE0) clen = 3;
            else if (*p >= 0xC0) clen = 2;
            if (clen > 7) clen = 7;
            memcpy(ini, nm, (size_t)clen);
            ini[clen] = '\0';
        }
        lv_obj_t *badge = lv_label_create(t);
        lv_label_set_text(badge, ini);
        lv_obj_set_style_text_font(badge, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(badge, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_opa(badge, LV_OPA_70, 0);
        lv_obj_align(badge, LV_ALIGN_TOP_MID, 0, 2);
        lv_obj_t *l = lv_label_create(t);
        lv_label_set_text(l, nm);
        lv_obj_set_style_text_font(l, cw_font_body(), 0);
        lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), 0);
        lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(l, LV_PCT(100));
        lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(l, LV_ALIGN_BOTTOM_MID, 0, -4);
        s_tiles[i] = t;
    }
    s_app_sel = 0;
    apps_refresh();
    cw_hint_bar(s_apps_scr, "UP/DN: Move", "OK: Open");
}

static void apps_exit(void) {
}

static void apps_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (ev != BSP_BTN_CLICK && ev != BSP_BTN_DOUBLE) return;
    if (btn == BSP_BTN_UP) {
        s_app_sel = (s_app_sel + 8) % 9;
        apps_refresh();
    } else if (btn == BSP_BTN_DOWN) {
        s_app_sel = (s_app_sel + 1) % 9;
        apps_refresh();
    } else if (btn == BSP_BTN_OK && ev == BSP_BTN_CLICK) {
        cw_app_push(APPS[s_app_sel].def, NULL);
    }
}

const cw_screen_def_t cw_scr_apps_def = {
    .enter = apps_enter,
    .exit = apps_exit,
    .key = apps_key,
    .tick = NULL,
    .name = "apps",
};
