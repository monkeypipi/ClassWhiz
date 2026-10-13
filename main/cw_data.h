// cw_data.h — ClassWhiz 纯数据层:演示数据集 + 无 ESP-IDF/LVGL 依赖的查询逻辑。
// 本文件可在主机上编译测试;涉及时钟推进的上层粘合在 cw_app.c。
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 星期:周一=0 … 周日=6
#define CW_DAY_MON 0
#define CW_DAY_TUE 1
#define CW_DAY_WED 2
#define CW_DAY_THU 3
#define CW_DAY_FRI 4
#define CW_DAY_SAT 5
#define CW_DAY_SUN 6

#define CW_PERIOD_COUNT 7
#define CW_MAX_SLOTS_PER_DAY 4
#define CW_MAX_TASKS 8
#define CW_MAX_EVENTS 12
#define CW_MAX_CITIES 4
#define CW_MAX_TEACHERS 4
#define CW_MAX_COURSES 6

// 演示周“今天”:2026-10-13,周二,14:46
#define CW_DEMO_TODAY_DAY CW_DAY_TUE
#define CW_DEMO_TODAY_MIN (14 * 60 + 46)

// 语言
typedef enum {
    CW_LANG_EN = 0,
    CW_LANG_ZH = 1,
} cw_lang_t;

void cw_set_lang(cw_lang_t lang);
cw_lang_t cw_get_lang(void);

// 本地化字符串 ID(英文默认,中文配对;中文字符串集中在 cw_data.c 的表中,
// 供 tools/extract_font_inventory.py 提取字形清单)。
typedef enum {
    CW_S_TODAY = 0,
    CW_S_APPS,
    CW_S_SCHEDULE,
    CW_S_TEACHERS,
    CW_S_TEACHER,
    CW_S_CALENDAR,
    CW_S_STUDENT_ID,
    CW_S_TASKS,
    CW_S_WORLD_CLOCK,
    CW_S_POMODORO,
    CW_S_VOICE,
    CW_S_SETTINGS,
    CW_S_LIST,
    CW_S_GRID,
    CW_S_NEXT_CLASS,
    CW_S_TODAY_CLASSES,
    CW_S_DONE,
    CW_S_NEXT,
    CW_S_UPCOMING,
    CW_S_OFFICE_HOURS,
    CW_S_OFFICE_HOURS_WEEK,
    CW_S_OFFICE,
    CW_S_EMAIL,
    CW_S_DEPT_CS,
    CW_S_DEPT_MATH,
    CW_S_OVERDUE,
    CW_S_DUE_TODAY,
    CW_S_THIS_WEEK,
    CW_S_EXAM,
    CW_S_HOMEWORK,
    CW_S_OPEN,
    CW_S_LANGUAGE,
    CW_S_TIME_FORMAT,
    CW_S_TEMP_UNIT,
    CW_S_WIFI,
    CW_S_RESET_DATA,
    CW_S_ABOUT,
    CW_S_HOLD_OK_RECORD,
    CW_S_ONDEVICE_MEMO,
    CW_S_GOOD_AFTERNOON,
    CW_S_DEMO_DATA,
    CW_S_NO_CLASSES,
    CW_S_MIN_ABBR,
    CW_S_WX_PARTLY_CLOUDY,
    CW_S_WX_CLEAR,
    CW_S_WX_RAIN,
    CW_S_COUNT
} cw_str_id_t;

const char *cw_str(cw_str_id_t id);

// 节次时间表(分钟,自 00:00)
int cw_period_start_min(int period); // period: 1..7
int cw_period_end_min(int period);

// 课程
typedef struct {
    uint8_t day;     // 0..6
    uint8_t period;  // 1..7
} cw_slot_t;

typedef struct {
    const char *code;      // "MATH 100"
    const char *title_en;
    const char *title_zh;
    const char *room;
    uint8_t teacher;       // 索引
    uint32_t color;        // 0xRRGGBB
    cw_slot_t slots[CW_MAX_SLOTS_PER_DAY];
    uint8_t slot_count;
} cw_course_t;

int cw_course_count(void);
const cw_course_t *cw_course(int idx);

// 某天课程(按节次排序)
typedef struct {
    const cw_course_t *course;
    uint8_t period;
} cw_class_t;

int cw_classes_for_day(int day, cw_class_t *out, int max_out);

typedef enum {
    CW_CLASS_DONE = 0,
    CW_CLASS_NOW,
    CW_CLASS_NEXT,
    CW_CLASS_UPCOMING,
} cw_class_state_t;

cw_class_state_t cw_class_state(int day, int period, int today_day, int now_min);

// 下一节课:在今后 8 天内搜索;返回 false 表示无课
bool cw_next_class(int today_day, int now_min, int *out_day, cw_class_t *out_class);

// 教师
typedef struct {
    uint8_t day;        // 0..6
    uint16_t start_min;
    uint16_t end_min;
} cw_office_hour_t;

typedef struct {
    const char *name;
    cw_str_id_t dept;   // 系名用字符串表(需本地化)
    const char *email;
    const char *office;
    const uint8_t *courses; // 课程索引数组
    uint8_t course_count;
    cw_office_hour_t hours[3];
    uint8_t hour_count;
} cw_teacher_t;

int cw_teacher_count(void);
const cw_teacher_t *cw_teacher(int idx);

// 本周 office hour 总览(按 day,start 排序)
typedef struct {
    uint8_t teacher;
    uint8_t day;
    uint16_t start_min;
    uint16_t end_min;
} cw_oh_entry_t;

int cw_office_hours_week(cw_oh_entry_t *out, int max_out);

// 校历事件
typedef enum {
    CW_EVENT_HOLIDAY = 0,
    CW_EVENT_EXAM,
    CW_EVENT_INFO,
} cw_event_kind_t;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    const char *title_en;
    const char *title_zh;
    const char *detail_en; // 可为 NULL
    cw_event_kind_t kind;
} cw_event_t;

int cw_event_count(void);
const cw_event_t *cw_event(int idx);
// 某月事件(按日期排序)
int cw_events_for_month(int year, int month, const cw_event_t **out, int max_out);

// 日期工具(纯逻辑,供校历网格)
int cw_days_in_month(int year, int month);
int cw_weekday_of_date(int year, int month, int day); // 0=周一..6=周日
const char *cw_event_title(const cw_event_t *e);

// 任务
typedef enum {
    CW_TASK_HOMEWORK = 0,
    CW_TASK_EXAM,
} cw_task_kind_t;

typedef struct {
    const char *title;
    const char *course;
    int due_day_offset; // 相对演示周周一的天数偏移;12=10月12日…以周一10月12日为0
    uint16_t due_min;   // 当天分钟;考试类可用 19:00
    const char *place;  // 可为 NULL
    cw_task_kind_t kind;
} cw_task_t;

int cw_task_count(void);
const cw_task_t *cw_task(int idx);

typedef enum {
    CW_TASK_OVERDUE = 0,
    CW_TASK_DUE_TODAY,
    CW_TASK_THIS_WEEK,
} cw_task_group_t;

// 任务分组:today_day_offset 为“今天”相对周一的偏移(演示=1)
cw_task_group_t cw_task_group(const cw_task_t *t, int today_day_offset);

// 城市与演示天气
typedef struct {
    const char *name;
    int utc_offset;      // 小时
    bool is_home;
    int temp_c;
    cw_str_id_t weather; // 天气状况字符串 ID
    int temp_hi_c;
    int temp_lo_c;
} cw_city_t;

int cw_city_count(void);
const cw_city_t *cw_city(int idx);

// 学生证(演示身份)
typedef struct {
    const char *name;
    const char *student_no;
    const char *program_en;
    const char *program_zh;
    const char *valid_thru;
} cw_student_t;

const cw_student_t *cw_student(void);

// 设置
typedef struct {
    cw_lang_t lang;
    bool time_24h;
    bool temp_celsius;
} cw_settings_t;

void cw_settings_defaults(cw_settings_t *s);

// 时间格式化(纯逻辑):buf 输出 "14:46" 或 "2:46 PM"
void cw_format_time(int minutes, bool h24, char *buf, size_t len);
void cw_format_dur(int minutes, char *buf, size_t len); // "42 MIN" / "1H 05MIN"

#ifdef __cplusplus
}
#endif
