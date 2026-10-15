// cw_app.h — ClassWhiz 应用核心:屏幕导航栈、演示时钟、输入分发。
// 所有 LVGL 访问经 bsp_lvgl_lock;按键回调只入队,重活在输入任务中执行。
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "bsp_button.h"
#include "cw_data.h"
#include "cw_pomodoro.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// 屏幕接口:enter/exit 在持有 LVGL 锁的输入任务中调用
typedef struct {
    void (*enter)(void *ctx);
    void (*exit)(void);
    void (*key)(bsp_btn_t btn, bsp_btn_ev_t ev);
    void (*tick)(uint64_t now_ms); // 1 秒 tick,可为 NULL
    const char *name;
} cw_screen_def_t;

// 应用启动(持有 LVGL 锁时由 main 调用)
void cw_app_start(void);
// 输入事件(由输入任务在持锁状态下调用)
void cw_app_input(bsp_btn_t btn, bsp_btn_ev_t ev);
// 1 秒 tick(由输入任务在持锁状态下调用)
void cw_app_tick(uint64_t now_ms);

// 导航
void cw_app_push(const cw_screen_def_t *def, void *ctx);
void cw_app_pop(void);
void cw_app_home(void);
// 原地重建当前屏幕(设置变更后刷新)
void cw_app_refresh_top(void);
bool cw_app_can_pop(void);
// 为 true 时,OK 长按交由屏幕处理(不触发全局返回);用于语音录音等
extern bool cw_screen_captures_ok_long;

// 演示时钟:以编译期演示时刻为起点,随单调时钟推进
// 返回自演示周周一 00:00 起的分钟数;day_offset 输出相对周一的天数偏移
int cw_app_now_min(uint64_t now_ms, int *day_offset);
// 当天分钟数(0..1439)与星期(0=周一..6=周日)
void cw_app_now_day_min(uint64_t now_ms, int *day, int *minute);
// 单调毫秒(esp_timer)
uint64_t cw_app_mono_ms(void);

// 全局状态(供各屏幕读写)
cw_settings_t *cw_app_settings(void);
void cw_app_mark_dirty(void);
bool *cw_app_task_done(void); // [CW_STORE_MAX_TASKS]
cw_pomo_t *cw_app_pomo(void);
void cw_app_mark_dirty(void); // 标记需持久化(tick 中落盘)

// 各屏幕定义(在各自 cw_s_*.c 中实现)
extern const cw_screen_def_t cw_scr_today_def;
extern const cw_screen_def_t cw_scr_apps_def;
extern const cw_screen_def_t cw_scr_schedule_def;
extern const cw_screen_def_t cw_scr_teachers_def;
extern const cw_screen_def_t cw_scr_calendar_def;
extern const cw_screen_def_t cw_scr_student_def;
extern const cw_screen_def_t cw_scr_tasks_def;
extern const cw_screen_def_t cw_scr_clock_def;
extern const cw_screen_def_t cw_scr_pomodoro_def;
extern const cw_screen_def_t cw_scr_voice_def;
extern const cw_screen_def_t cw_scr_settings_def;

#ifdef __cplusplus
}
#endif
