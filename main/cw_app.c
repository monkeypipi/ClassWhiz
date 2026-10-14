// cw_app.c — 应用核心实现。
#include "cw_app.h"

#include <string.h>

#include "bsp_battery.h"
#include "bsp_display.h"
#include "cw_store.h"
#include "cw_ui.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "cw_app";

// 演示时钟起点:演示周周二 14:46(分钟,自周一 00:00)
#define DEMO_START_ABS_MIN (1 * 1440 + 14 * 60 + 46)

#define NAV_DEPTH 8

typedef struct {
    const cw_screen_def_t *def;
    void *ctx;
    lv_obj_t *scr;
} nav_entry_t;

static nav_entry_t s_stack[NAV_DEPTH];
static int s_depth;
static lv_obj_t *s_cur_scr;

static cw_settings_t s_settings;
static bool s_task_done[CW_STORE_MAX_TASKS];
static cw_pomo_t s_pomo;
static bool s_dirty;
static uint64_t s_last_save_ms;

bool cw_screen_captures_ok_long = false;

uint64_t cw_app_mono_ms(void) {
    return (uint64_t)esp_timer_get_time() / 1000;
}

int cw_app_now_min(uint64_t now_ms, int *day_offset) {
    int64_t abs_min = (int64_t)DEMO_START_ABS_MIN + (int64_t)(now_ms / 60000);
    if (day_offset) *day_offset = (int)(abs_min / 1440);
    return (int)(abs_min % 1440);
}

void cw_app_now_day_min(uint64_t now_ms, int *day, int *minute) {
    int off = 0;
    int m = cw_app_now_min(now_ms, &off);
    if (day) *day = ((off % 7) + 7) % 7;
    if (minute) *minute = m;
}

cw_settings_t *cw_app_settings(void) {
    return &s_settings;
}

bool *cw_app_task_done(void) {
    return s_task_done;
}

cw_pomo_t *cw_app_pomo(void) {
    return &s_pomo;
}

void cw_app_mark_dirty(void) {
    s_dirty = true;
}

static void teardown_top(void) {
    if (s_depth <= 0) return;
    nav_entry_t *e = &s_stack[s_depth - 1];
    if (e->def->exit) e->def->exit();
    if (e->scr) {
        cw_screen_teardown(e->scr);
        lv_obj_delete(e->scr);
        e->scr = NULL;
    }
    s_depth--;
}

void cw_app_push(const cw_screen_def_t *def, void *ctx) {
    if (!def || s_depth >= NAV_DEPTH) return;
    // 进入新屏幕前,旧屏幕对象由新屏幕 enter 时替换;这里先记栈
    nav_entry_t *e = &s_stack[s_depth++];
    e->def = def;
    e->ctx = ctx;
    e->scr = NULL;
    ESP_LOGI(TAG, "enter %s (depth %d)", def->name, s_depth);
    if (def->enter) def->enter(ctx);
    // enter 应创建并加载屏幕;记录当前屏幕
    s_cur_scr = lv_screen_active();
    e->scr = s_cur_scr;
}

void cw_app_pop(void) {
    if (s_depth <= 1) return; // 保留首页
    teardown_top();
    nav_entry_t *e = &s_stack[s_depth - 1];
    s_cur_scr = e->scr;
    if (s_cur_scr) {
        lv_screen_load(s_cur_scr);
    } else if (e->def->enter) {
        // 旧屏幕对象已不在,重建
        e->def->enter(e->ctx);
        s_cur_scr = lv_screen_active();
        e->scr = s_cur_scr;
    }
    ESP_LOGI(TAG, "back to %s", e->def->name);
}

void cw_app_home(void) {
    while (s_depth > 1) {
        teardown_top();
    }
    if (s_depth == 1) {
        nav_entry_t *e = &s_stack[0];
        if (e->scr) {
            lv_screen_load(e->scr);
            s_cur_scr = e->scr;
        }
    }
    // 回首页会一次性拆掉多层屏幕,不记日志的话串口里看不出这是 home 还是 pop
    ESP_LOGI(TAG, "home (depth %d)", s_depth);
}

void cw_app_refresh_top(void) {
    if (s_depth <= 0) return;
    nav_entry_t *e = &s_stack[s_depth - 1];
    if (e->def->exit) e->def->exit();
    if (e->scr) {
        cw_screen_teardown(e->scr);
        lv_obj_delete(e->scr);
        e->scr = NULL;
    }
    ESP_LOGI(TAG, "refresh %s", e->def->name);
    if (e->def->enter) e->def->enter(e->ctx);
    s_cur_scr = lv_screen_active();
    e->scr = s_cur_scr;
}

bool cw_app_can_pop(void) {
    return s_depth > 1;
}

void cw_app_input(bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (s_depth <= 0) return;
    // 全局:OK 长按返回(除非当前屏幕截获,如语音录音);DOWN 长按回首页
    if (ev == BSP_BTN_LONG) {
        if (btn == BSP_BTN_OK && !cw_screen_captures_ok_long) {
            cw_app_pop();
            return;
        }
        if (btn == BSP_BTN_DOWN) {
            cw_app_home();
            return;
        }
    }
    if (ev == BSP_BTN_PRESS) return; // RELEASE 交给屏幕(如语音停止录音)
    const cw_screen_def_t *def = s_stack[s_depth - 1].def;
    // 连按间隔小于按键库双击窗口时只会产出 DOUBLE;除显式使用双击的课程表页
    // (切换视图)外,一律按单击派发,避免连按被当成"按键失灵"。
    if (ev == BSP_BTN_DOUBLE && def != &cw_scr_schedule_def) ev = BSP_BTN_CLICK;
    if (def->key) def->key(btn, ev);
}

void cw_app_tick(uint64_t now_ms) {
    if (s_depth <= 0) return;
    // 状态栏刷新
    if (s_cur_scr) {
        int minute = 0;
        cw_app_now_day_min(now_ms, NULL, &minute);
        char tbuf[16];
        cw_format_time(minute, s_settings.time_24h, tbuf, sizeof(tbuf));
        int soc = bsp_battery_soc();
        cw_ui_status_tick(s_cur_scr, tbuf, soc);
    }
    const cw_screen_def_t *def = s_stack[s_depth - 1].def;
    if (def->tick) def->tick(now_ms);
    // 脏数据落盘(最多 5 秒一次)
    if (s_dirty && now_ms - s_last_save_ms > 5000) {
        s_dirty = false;
        s_last_save_ms = now_ms;
        if (!cw_store_save(&s_settings, s_task_done, &s_pomo)) {
            ESP_LOGW(TAG, "persist failed");
        }
    }
}

void cw_app_start(void) {
    cw_ui_init_fonts();
    cw_settings_defaults(&s_settings);
    memset(s_task_done, 0, sizeof(s_task_done));
    cw_pomo_init(&s_pomo);
    if (cw_store_init()) {
        if (cw_store_load(&s_settings, s_task_done, &s_pomo)) {
            ESP_LOGI(TAG, "restored persisted state");
        }
    } else {
        ESP_LOGW(TAG, "store init failed, using defaults");
    }
    cw_set_lang(s_settings.lang);
    s_depth = 0;
    s_cur_scr = NULL;
    cw_app_push(&cw_scr_today_def, NULL);
}
