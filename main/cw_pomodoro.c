// cw_pomodoro.c — 番茄钟纯状态机实现。
#include "cw_pomodoro.h"

#include <stddef.h>

void cw_pomo_init(cw_pomo_t *p) {
    if (p == NULL) return;
    p->focus_min = 25;
    p->break_min = 5;
    p->state = CW_POMO_IDLE;
    p->remaining_sec = 25 * 60;
    p->completed = 0;
    p->deadline_ms = 0;
}

void cw_pomo_set_preset(cw_pomo_t *p, uint8_t focus_min, uint8_t break_min) {
    if (p == NULL) return;
    if (focus_min == 0 || break_min == 0) return;
    // 仅空闲时允许切换预设,避免中途改时长导致计时混乱
    if (p->state != CW_POMO_IDLE) return;
    p->focus_min = focus_min;
    p->break_min = break_min;
    p->remaining_sec = (uint32_t)focus_min * 60;
}

bool cw_pomo_start(cw_pomo_t *p, uint64_t now_ms) {
    if (p == NULL) return false;
    if (p->state != CW_POMO_IDLE) return false;
    p->state = CW_POMO_FOCUS_RUNNING;
    p->remaining_sec = (uint32_t)p->focus_min * 60;
    p->deadline_ms = now_ms + (uint64_t)p->remaining_sec * 1000;
    return true;
}

bool cw_pomo_pause(cw_pomo_t *p, uint64_t now_ms) {
    if (p == NULL) return false;
    if (p->state != CW_POMO_FOCUS_RUNNING && p->state != CW_POMO_BREAK_RUNNING) {
        return false;
    }
    p->remaining_sec = cw_pomo_remaining(p, now_ms);
    p->state = (p->state == CW_POMO_FOCUS_RUNNING)
                   ? CW_POMO_FOCUS_PAUSED
                   : CW_POMO_BREAK_PAUSED;
    return true;
}

bool cw_pomo_resume(cw_pomo_t *p, uint64_t now_ms) {
    if (p == NULL) return false;
    if (p->state != CW_POMO_FOCUS_PAUSED && p->state != CW_POMO_BREAK_PAUSED) {
        return false;
    }
    p->state = (p->state == CW_POMO_FOCUS_PAUSED)
                   ? CW_POMO_FOCUS_RUNNING
                   : CW_POMO_BREAK_RUNNING;
    p->deadline_ms = now_ms + (uint64_t)p->remaining_sec * 1000;
    return true;
}

bool cw_pomo_reset(cw_pomo_t *p) {
    if (p == NULL) return false;
    cw_pomo_state_t keep_completed_state = p->state;
    (void)keep_completed_state;
    p->state = CW_POMO_IDLE;
    p->remaining_sec = (uint32_t)p->focus_min * 60;
    p->deadline_ms = 0;
    return true;
}

cw_pomo_event_t cw_pomo_tick(cw_pomo_t *p, uint64_t now_ms) {
    if (p == NULL) return CW_POMO_EV_NONE;
    if (p->state != CW_POMO_FOCUS_RUNNING && p->state != CW_POMO_BREAK_RUNNING) {
        return CW_POMO_EV_NONE;
    }
    if (now_ms < p->deadline_ms) return CW_POMO_EV_NONE;
    if (p->state == CW_POMO_FOCUS_RUNNING) {
        p->completed++;
        p->state = CW_POMO_BREAK_RUNNING;
        p->remaining_sec = (uint32_t)p->break_min * 60;
        p->deadline_ms = now_ms + (uint64_t)p->remaining_sec * 1000;
        return CW_POMO_EV_FOCUS_DONE;
    }
    p->state = CW_POMO_IDLE;
    p->remaining_sec = (uint32_t)p->focus_min * 60;
    p->deadline_ms = 0;
    return CW_POMO_EV_BREAK_DONE;
}

uint32_t cw_pomo_remaining(const cw_pomo_t *p, uint64_t now_ms) {
    if (p == NULL) return 0;
    if (p->state == CW_POMO_FOCUS_RUNNING || p->state == CW_POMO_BREAK_RUNNING) {
        if (now_ms >= p->deadline_ms) return 0;
        return (uint32_t)((p->deadline_ms - now_ms + 999) / 1000);
    }
    return p->remaining_sec;
}

uint32_t cw_pomo_phase_total(const cw_pomo_t *p) {
    if (p == NULL) return 1;
    switch (p->state) {
        case CW_POMO_BREAK_RUNNING:
        case CW_POMO_BREAK_PAUSED:
            return (uint32_t)p->break_min * 60;
        default:
            return (uint32_t)p->focus_min * 60;
    }
}
