// tests/test_cw_pomodoro.c — 番茄钟状态机主机测试。
#include <stdio.h>

#include "cw_pomodoro.h"

#define CHECK(cond) do { \
    if (!(cond)) { \
        printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        return 1; \
    } \
} while (0)

static int test_lifecycle(void) {
    cw_pomo_t p;
    cw_pomo_init(&p);
    CHECK(p.state == CW_POMO_IDLE);
    CHECK(p.remaining_sec == 1500);
    // 开始
    CHECK(cw_pomo_start(&p, 1000));
    CHECK(p.state == CW_POMO_FOCUS_RUNNING);
    CHECK(!cw_pomo_start(&p, 1000)); // 运行中不能重复开始
    // 未到期无事件
    CHECK(cw_pomo_tick(&p, 1000 + 60 * 1000) == CW_POMO_EV_NONE);
    CHECK(cw_pomo_remaining(&p, 1000 + 60 * 1000) == 1440);
    // 暂停/继续
    CHECK(cw_pomo_pause(&p, 1000 + 60 * 1000));
    CHECK(p.state == CW_POMO_FOCUS_PAUSED);
    CHECK(p.remaining_sec == 1440);
    CHECK(cw_pomo_tick(&p, 1000 + 3600 * 1000) == CW_POMO_EV_NONE); // 暂停时不推进
    CHECK(cw_pomo_resume(&p, 2000 + 60 * 1000));
    CHECK(p.state == CW_POMO_FOCUS_RUNNING);
    // 专注完成 -> 进入休息
    CHECK(cw_pomo_tick(&p, 2000 + 60 * 1000 + 1440 * 1000) == CW_POMO_EV_FOCUS_DONE);
    CHECK(p.state == CW_POMO_BREAK_RUNNING);
    CHECK(p.completed == 1);
    CHECK(cw_pomo_phase_total(&p) == 300);
    // 休息完成 -> 回空闲
    CHECK(cw_pomo_tick(&p, 2000 + 60 * 1000 + 1440 * 1000 + 300 * 1000)
          == CW_POMO_EV_BREAK_DONE);
    CHECK(p.state == CW_POMO_IDLE);
    CHECK(p.remaining_sec == 1500);
    return 0;
}

static int test_preset_and_reset(void) {
    cw_pomo_t p;
    cw_pomo_init(&p);
    cw_pomo_set_preset(&p, 50, 10);
    CHECK(p.focus_min == 50 && p.break_min == 10);
    CHECK(p.remaining_sec == 3000);
    CHECK(cw_pomo_start(&p, 0));
    cw_pomo_set_preset(&p, 25, 5); // 运行中切换无效
    CHECK(p.focus_min == 50);
    CHECK(cw_pomo_reset(&p));
    CHECK(p.state == CW_POMO_IDLE);
    CHECK(p.remaining_sec == 3000);
    cw_pomo_set_preset(&p, 25, 5); // 空闲时可切换
    CHECK(p.focus_min == 25);
    // 非法预设被拒绝
    cw_pomo_set_preset(&p, 0, 5);
    CHECK(p.focus_min == 25);
    return 0;
}

static int test_edges(void) {
    cw_pomo_t p;
    cw_pomo_init(&p);
    CHECK(!cw_pomo_pause(&p, 0));  // 空闲不能暂停
    CHECK(!cw_pomo_resume(&p, 0)); // 空闲不能继续
    CHECK(cw_pomo_remaining(&p, 0) == 1500);
    CHECK(cw_pomo_tick(&p, 999999) == CW_POMO_EV_NONE);
    return 0;
}

int main(void) {
    int fails = 0;
    fails += test_lifecycle();
    fails += test_preset_and_reset();
    fails += test_edges();
    if (fails == 0) printf("test_cw_pomodoro: PASS\n");
    return fails ? 1 : 0;
}
