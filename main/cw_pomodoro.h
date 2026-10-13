// cw_pomodoro.h — 番茄钟纯状态机,与 ESP-IDF/LVGL 无关,可主机测试。
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CW_POMO_IDLE = 0,
    CW_POMO_FOCUS_RUNNING,
    CW_POMO_FOCUS_PAUSED,
    CW_POMO_BREAK_RUNNING,
    CW_POMO_BREAK_PAUSED,
} cw_pomo_state_t;

typedef enum {
    CW_POMO_EV_NONE = 0,
    CW_POMO_EV_FOCUS_DONE,
    CW_POMO_EV_BREAK_DONE,
} cw_pomo_event_t;

typedef struct {
    uint8_t focus_min;   // 25 或 50
    uint8_t break_min;   // 5 或 10
    cw_pomo_state_t state;
    uint32_t remaining_sec;   // 当前阶段剩余秒
    uint32_t completed;       // 今日完成番茄数
    uint64_t deadline_ms;     // 运行时的到期时刻(单调时钟),暂停时无意义
} cw_pomo_t;

void cw_pomo_init(cw_pomo_t *p);
void cw_pomo_set_preset(cw_pomo_t *p, uint8_t focus_min, uint8_t break_min);

bool cw_pomo_start(cw_pomo_t *p, uint64_t now_ms);
bool cw_pomo_pause(cw_pomo_t *p, uint64_t now_ms);
bool cw_pomo_resume(cw_pomo_t *p, uint64_t now_ms);
bool cw_pomo_reset(cw_pomo_t *p);
// 到期推进:返回事件;调用方据此切阶段/记数
cw_pomo_event_t cw_pomo_tick(cw_pomo_t *p, uint64_t now_ms);
// 当前剩余秒(含运行中按 deadline 折算)
uint32_t cw_pomo_remaining(const cw_pomo_t *p, uint64_t now_ms);
// 当前阶段总秒数(画进度环用)
uint32_t cw_pomo_phase_total(const cw_pomo_t *p);

#ifdef __cplusplus
}
#endif
