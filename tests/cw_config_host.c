// cw_config_host.c — 主机测试用的 cw_config 桩实现。
// 提供默认数据,不依赖 ESP-IDF NVS。
#include "cw_config.h"
#include <string.h>

static cw_course_mut_t s_courses[CW_MAX_COURSES];
static int s_course_count = 0;
static cw_teacher_mut_t s_teachers[CW_MAX_TEACHERS];
static int s_teacher_count = 0;
static cw_task_mut_t s_tasks[CW_MAX_TASKS];
static int s_task_count = 0;
static cw_event_mut_t s_events[CW_MAX_EVENTS];
static int s_event_count = 0;
static cw_student_mut_t s_student;
static cw_weather_cfg_t s_weather;

static void load_defaults(void) {
    s_course_count = 3;
    memset(s_courses, 0, sizeof(s_courses));
    strncpy(s_courses[0].code, "MATH 100", 15);
    strncpy(s_courses[0].title_en, "Differential Calculus", 63);
    strncpy(s_courses[0].title_zh, "微分学", 63);
    strncpy(s_courses[0].room, "LSK 200", 31);
    s_courses[0].teacher = 1;
    s_courses[0].color = 0x8B5CF6;
    s_courses[0].slots[0] = (cw_slot_t){CW_DAY_MON, 1};
    s_courses[0].slots[1] = (cw_slot_t){CW_DAY_TUE, 1};
    s_courses[0].slots[2] = (cw_slot_t){CW_DAY_THU, 1};
    s_courses[0].slot_count = 3;

    strncpy(s_courses[1].code, "CPSC 110", 15);
    strncpy(s_courses[1].title_en, "Computation, Programs", 63);
    strncpy(s_courses[1].title_zh, "计算与程序", 63);
    strncpy(s_courses[1].room, "DMP 110", 31);
    s_courses[1].teacher = 0;
    s_courses[1].color = 0x4C8DFF;
    s_courses[1].slots[0] = (cw_slot_t){CW_DAY_MON, 3};
    s_courses[1].slots[1] = (cw_slot_t){CW_DAY_TUE, 3};
    s_courses[1].slots[2] = (cw_slot_t){CW_DAY_WED, 3};
    s_courses[1].slots[3] = (cw_slot_t){CW_DAY_FRI, 3};
    s_courses[1].slot_count = 4;

    strncpy(s_courses[2].code, "CPSC 210", 15);
    strncpy(s_courses[2].title_en, "Software Construction", 63);
    strncpy(s_courses[2].title_zh, "软件构造", 63);
    strncpy(s_courses[2].room, "ICCS X836", 31);
    s_courses[2].teacher = 0;
    s_courses[2].color = 0x22B8A8;
    s_courses[2].slots[0] = (cw_slot_t){CW_DAY_TUE, 6};
    s_courses[2].slots[1] = (cw_slot_t){CW_DAY_WED, 6};
    s_courses[2].slot_count = 2;

    s_teacher_count = 2;
    memset(s_teachers, 0, sizeof(s_teachers));
    strncpy(s_teachers[0].name, "Prof. Carter", 47);
    s_teachers[0].dept = CW_S_DEPT_CS;
    strncpy(s_teachers[0].email, "carter@cs.ubc.ca", 63);
    strncpy(s_teachers[0].office, "ICCS 239", 31);
    s_teachers[0].courses[0] = 1; s_teachers[0].courses[1] = 2;
    s_teachers[0].course_count = 2;
    s_teachers[0].hours[0] = (cw_office_hour_t){CW_DAY_WED, 14*60, 15*60};
    s_teachers[0].hours[1] = (cw_office_hour_t){CW_DAY_FRI, 10*60, 11*60};
    s_teachers[0].hour_count = 2;

    strncpy(s_teachers[1].name, "Dr. Liu", 47);
    s_teachers[1].dept = CW_S_DEPT_MATH;
    strncpy(s_teachers[1].email, "liu@math.ubc.ca", 63);
    strncpy(s_teachers[1].office, "MATH 204", 31);
    s_teachers[1].courses[0] = 0;
    s_teachers[1].course_count = 1;
    s_teachers[1].hours[0] = (cw_office_hour_t){CW_DAY_THU, 10*60, 11*60};
    s_teachers[1].hour_count = 1;

    s_task_count = 0;
    s_event_count = 0;
    memset(s_tasks, 0, sizeof(s_tasks));
    memset(s_events, 0, sizeof(s_events));

    // 任务默认(3个)
    s_task_count = 3;
    strncpy(s_tasks[0].title, "MATH 100 Problem Set 4", 63);
    strncpy(s_tasks[0].course, "MATH 100", 15);
    s_tasks[0].due_day_offset = 0;
    s_tasks[0].due_min = 23 * 60 + 59;
    s_tasks[0].kind = 0; // CW_TASK_HOMEWORK
    strncpy(s_tasks[1].title, "CPSC 210 Lab 5", 63);
    strncpy(s_tasks[1].course, "CPSC 210", 15);
    s_tasks[1].due_day_offset = 1;
    s_tasks[1].due_min = 23 * 60 + 59;
    s_tasks[1].kind = 0;
    strncpy(s_tasks[2].title, "CPSC 110 Midterm", 63);
    strncpy(s_tasks[2].course, "CPSC 110", 15);
    s_tasks[2].due_day_offset = 4;
    s_tasks[2].due_min = 19 * 60;
    strncpy(s_tasks[2].place, "DMP 110", 31);
    s_tasks[2].kind = 1; // CW_TASK_EXAM

    // 校历默认
    s_event_count = 5;
    s_events[0].year = 2026; s_events[0].month = 10; s_events[0].day = 12;
    strncpy(s_events[0].title_en, "Thanksgiving Day", 63);
    strncpy(s_events[0].title_zh, "感恩节", 63);
    strncpy(s_events[0].detail_en, "No classes", 127);
    s_events[0].kind = 0;
    s_events[1].year = 2026; s_events[1].month = 10; s_events[1].day = 16;
    strncpy(s_events[1].title_en, "CPSC 110 Midterm", 63);
    strncpy(s_events[1].title_zh, "CPSC 110 期中考试", 63);
    strncpy(s_events[1].detail_en, "19:00 DMP 110", 127);
    s_events[1].kind = 1;
    s_events[2].year = 2026; s_events[2].month = 11; s_events[2].day = 11;
    strncpy(s_events[2].title_en, "Remembrance Day", 63);
    strncpy(s_events[2].title_zh, "国殇纪念日", 63);
    strncpy(s_events[2].detail_en, "No classes", 127);
    s_events[2].kind = 0;
    s_events[3].year = 2026; s_events[3].month = 12; s_events[3].day = 8;
    strncpy(s_events[3].title_en, "Last day of classes", 63);
    strncpy(s_events[3].title_zh, "结课日", 63);
    s_events[3].kind = 2;
    s_events[4].year = 2026; s_events[4].month = 12; s_events[4].day = 14;
    strncpy(s_events[4].title_en, "Final examinations begin", 63);
    strncpy(s_events[4].title_zh, "期末考试周开始", 63);
    s_events[4].kind = 1;

    memset(&s_student, 0, sizeof(s_student));
    memset(&s_weather, 0, sizeof(s_weather));
}

bool cw_config_init(void) {
    load_defaults();
    return false;
}

bool cw_config_save(void) { return true; }
void cw_config_reset_defaults(void) { load_defaults(); }

cw_course_mut_t *cw_config_courses_mut(int *count) {
    if (count) *count = s_course_count;
    return s_courses;
}
cw_teacher_mut_t *cw_config_teachers_mut(int *count) {
    if (count) *count = s_teacher_count;
    return s_teachers;
}
cw_task_mut_t *cw_config_tasks_mut(int *count) {
    if (count) *count = s_task_count;
    return s_tasks;
}
cw_event_mut_t *cw_config_events_mut(int *count) {
    if (count) *count = s_event_count;
    return s_events;
}
cw_student_mut_t *cw_config_student_mut(void) { return &s_student; }
cw_weather_cfg_t *cw_config_weather_mut(void) { return &s_weather; }

bool cw_config_to_json(char *buf, size_t buf_len) {
    (void)buf; (void)buf_len;
    return false;
}
bool cw_config_from_json(const char *json) {
    (void)json;
    return false;
}
