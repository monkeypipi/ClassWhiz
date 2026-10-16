// tests/test_cw_data.c — ClassWhiz 数据层主机测试。
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "cw_data.h"
#include "cw_config.h"

#define CHECK(cond) do { \
    if (!(cond)) { \
        printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        return 1; \
    } \
} while (0)

static int test_schedule(void) {
    cw_class_t classes[CW_MAX_SLOTS_PER_DAY];
    int n = cw_classes_for_day(CW_DAY_TUE, classes, CW_MAX_SLOTS_PER_DAY);
    CHECK(n == 3);
    CHECK(classes[0].start_min == 540);
    CHECK(strcmp(classes[0].course->code, "MATH 100") == 0);
    CHECK(classes[1].start_min == 660);
    CHECK(classes[2].start_min == 930);
    CHECK(strcmp(classes[2].course->code, "CPSC 210") == 0);
    n = cw_classes_for_day(CW_DAY_SUN, classes, CW_MAX_SLOTS_PER_DAY);
    CHECK(n == 0);
    n = cw_classes_for_day(CW_DAY_SAT, classes, 1);
    CHECK(n == 0); // 周六无课,上限1也不影响
    return 0;
}

static int test_next_class(void) {
    int day;
    cw_class_t cls;
    // 周二 14:46 -> 下一节是周二 P6 CPSC 210
    CHECK(cw_next_class(CW_DAY_TUE, CW_DEMO_TODAY_MIN, &day, &cls));
    CHECK(day == CW_DAY_TUE);
    CHECK(cls.start_min == 930);
    // 周二 17:00 之后 -> 周三 P3
    CHECK(cw_next_class(CW_DAY_TUE, 17 * 60, &day, &cls));
    CHECK(day == CW_DAY_WED && cls.start_min == 660);
    // 周日晚上 -> 周一 P1
    CHECK(cw_next_class(CW_DAY_SUN, 23 * 60, &day, &cls));
    CHECK(day == CW_DAY_MON && cls.start_min == 540);
    return 0;
}

static int test_class_state(void) {
    // 周二 14:46:P1/P3 已上完,P6 为 NEXT
    CHECK(cw_class_state(CW_DAY_TUE, 540, 590, CW_DAY_TUE, CW_DEMO_TODAY_MIN) == CW_CLASS_DONE);
    CHECK(cw_class_state(CW_DAY_TUE, 930, 1010, CW_DAY_TUE, CW_DEMO_TODAY_MIN) == CW_CLASS_NEXT);
    // 上课中:周二 15:40 P6
    CHECK(cw_class_state(CW_DAY_TUE, 930, 1010, CW_DAY_TUE, 15 * 60 + 40) == CW_CLASS_NOW);
    // 非今天
    CHECK(cw_class_state(CW_DAY_WED, 660, 740, CW_DAY_TUE, CW_DEMO_TODAY_MIN) == CW_CLASS_UPCOMING);
    return 0;
}

static int test_office_hours(void) {
    cw_oh_entry_t entries[8];
    int n = cw_office_hours_week(entries, 8);
    CHECK(n == 3);
    CHECK(entries[0].day == CW_DAY_WED); // 周三 14:00
    CHECK(entries[1].day == CW_DAY_THU); // 周四 10:00
    CHECK(entries[2].day == CW_DAY_FRI); // 周五 10:00
    return 0;
}

static int test_calendar(void) {
    CHECK(cw_days_in_month(2026, 10) == 31);
    CHECK(cw_days_in_month(2026, 2) == 28);
    CHECK(cw_days_in_month(2024, 2) == 29);
    CHECK(cw_weekday_of_date(2026, 10, 1) == 3);  // 周四
    CHECK(cw_weekday_of_date(2026, 10, 13) == 1); // 周二
    const cw_event_t *ev[8];
    int n = cw_events_for_month(2026, 10, ev, 8);
    CHECK(n == 2);
    CHECK(ev[0]->day == 12 && ev[1]->day == 16);
    return 0;
}

static int test_tasks(void) {
    // today offset=1(周二);task0 due offset 0 -> overdue
    CHECK(cw_task_group(cw_task(0), 1) == CW_TASK_OVERDUE);
    CHECK(cw_task_group(cw_task(1), 1) == CW_TASK_DUE_TODAY);
    CHECK(cw_task_group(cw_task(2), 1) == CW_TASK_THIS_WEEK);
    CHECK(cw_task_count() == 3);
    return 0;
}

static int test_strings(void) {
    cw_set_lang(CW_LANG_EN);
    CHECK(strcmp(cw_str(CW_S_SCHEDULE), "Schedule") == 0);
    cw_set_lang(CW_LANG_ZH);
    CHECK(strcmp(cw_str(CW_S_SCHEDULE), "课程表") == 0);
    CHECK(strcmp(cw_str(CW_S_WX_RAIN), "雨") == 0);
    // 越界保护
    CHECK(strcmp(cw_str((cw_str_id_t)9999), "?") == 0);
    cw_set_lang(CW_LANG_EN);
    // 教师系名本地化
    const cw_teacher_t *t = cw_teacher(0);
    cw_set_lang(CW_LANG_ZH);
    CHECK(strcmp(cw_str(t->dept), "计算机科学") == 0);
    cw_set_lang(CW_LANG_EN);
    return 0;
}

static int test_format(void) {
    char buf[32];
    cw_format_time(886, true, buf, sizeof(buf));
    CHECK(strcmp(buf, "14:46") == 0);
    cw_format_time(886, false, buf, sizeof(buf));
    CHECK(strcmp(buf, "2:46 PM") == 0);
    cw_format_time(0, true, buf, sizeof(buf));
    CHECK(strcmp(buf, "00:00") == 0);
    cw_set_lang(CW_LANG_EN);
    cw_format_dur(42, buf, sizeof(buf));
    CHECK(strcmp(buf, "42 MIN") == 0);
    cw_format_dur(65, buf, sizeof(buf));
    CHECK(strcmp(buf, "1H 05MIN") == 0);
    cw_set_lang(CW_LANG_ZH);
    cw_format_dur(65, buf, sizeof(buf));
    CHECK(strcmp(buf, "1小时05分") == 0);
    cw_set_lang(CW_LANG_EN);
    return 0;
}

int main(void) {
    // 初始化动态配置(加载默认数据并同步视图)
    cw_config_init();
    cw_data_sync_views();
    int fails = 0;
    fails += test_schedule();
    fails += test_next_class();
    fails += test_class_state();
    fails += test_office_hours();
    fails += test_calendar();
    fails += test_tasks();
    fails += test_strings();
    fails += test_format();
    if (fails == 0) printf("test_cw_data: PASS\n");
    return fails ? 1 : 0;
}
