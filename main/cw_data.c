// cw_data.c — ClassWhiz 演示数据集与纯查询逻辑。
// 中文字符串集中在此文件,供 tools/extract_font_inventory.py 提取。
#include "cw_data.h"

#include <stdio.h>
#include <string.h>

static cw_lang_t s_lang = CW_LANG_EN;

void cw_set_lang(cw_lang_t lang) {
    s_lang = (lang == CW_LANG_ZH) ? CW_LANG_ZH : CW_LANG_EN;
}

cw_lang_t cw_get_lang(void) {
    return s_lang;
}

// 字符串表:英文默认,中文配对
static const char *const S_EN[CW_S_COUNT] = {
    [CW_S_TODAY] = "Today",
    [CW_S_APPS] = "Apps",
    [CW_S_SCHEDULE] = "Schedule",
    [CW_S_TEACHERS] = "Teachers",
    [CW_S_TEACHER] = "Teacher",
    [CW_S_CALENDAR] = "Academic Calendar",
    [CW_S_STUDENT_ID] = "Student ID",
    [CW_S_TASKS] = "Tasks",
    [CW_S_WORLD_CLOCK] = "World Clock",
    [CW_S_POMODORO] = "Pomodoro",
    [CW_S_VOICE] = "Muse Voice",
    [CW_S_SETTINGS] = "Settings",
    [CW_S_LIST] = "List",
    [CW_S_GRID] = "Grid",
    [CW_S_NEXT_CLASS] = "Next class",
    [CW_S_TODAY_CLASSES] = "Today's classes",
    [CW_S_DONE] = "Done",
    [CW_S_NEXT] = "Next",
    [CW_S_UPCOMING] = "Upcoming",
    [CW_S_OFFICE_HOURS] = "Office hours",
    [CW_S_OFFICE_HOURS_WEEK] = "Office hours this week",
    [CW_S_OFFICE] = "Office",
    [CW_S_EMAIL] = "Email",
    [CW_S_DEPT_CS] = "Computer Science",
    [CW_S_DEPT_MATH] = "Mathematics",
    [CW_S_OVERDUE] = "Overdue",
    [CW_S_DUE_TODAY] = "Due today",
    [CW_S_THIS_WEEK] = "This week",
    [CW_S_EXAM] = "Exam",
    [CW_S_HOMEWORK] = "Homework",
    [CW_S_OPEN] = "open",
    [CW_S_LANGUAGE] = "Language",
    [CW_S_TIME_FORMAT] = "Time format",
    [CW_S_TEMP_UNIT] = "Temperature",
    [CW_S_WIFI] = "Wi-Fi",
    [CW_S_RESET_DATA] = "Reset demo data",
    [CW_S_CONFIRM_RESET] = "Confirm reset?",
    [CW_S_START] = "Start",
    [CW_S_PAUSE] = "Pause",
    [CW_S_ABOUT] = "About",
    [CW_S_HOLD_OK_RECORD] = "Hold OK to record",
    [CW_S_ONDEVICE_MEMO] = "On-device memos",
    [CW_S_GOOD_AFTERNOON] = "Good afternoon",
    [CW_S_DEMO_DATA] = "Demo data",
    [CW_S_NO_CLASSES] = "No classes",
    [CW_S_ALL_DONE] = "All done!",
    [CW_S_MIN_ABBR] = "min",
    [CW_S_WX_PARTLY_CLOUDY] = "Partly cloudy",
    [CW_S_WX_CLEAR] = "Clear",
    [CW_S_WX_RAIN] = "Rain",
};

static const char *const S_ZH[CW_S_COUNT] = {
    [CW_S_TODAY] = "今日概览",
    [CW_S_APPS] = "应用",
    [CW_S_SCHEDULE] = "课程表",
    [CW_S_TEACHERS] = "教师",
    [CW_S_TEACHER] = "教师详情",
    [CW_S_CALENDAR] = "大学校历",
    [CW_S_STUDENT_ID] = "学生证",
    [CW_S_TASKS] = "作业考试",
    [CW_S_WORLD_CLOCK] = "世界时钟",
    [CW_S_POMODORO] = "番茄钟",
    [CW_S_VOICE] = "语音",
    [CW_S_SETTINGS] = "设置",
    [CW_S_LIST] = "列表",
    [CW_S_GRID] = "周视图",
    [CW_S_NEXT_CLASS] = "下节课程",
    [CW_S_TODAY_CLASSES] = "今日课程",
    [CW_S_DONE] = "已上完",
    [CW_S_NEXT] = "下节",
    [CW_S_UPCOMING] = "稍后",
    [CW_S_OFFICE_HOURS] = "答疑时间",
    [CW_S_OFFICE_HOURS_WEEK] = "本周答疑",
    [CW_S_OFFICE] = "办公室",
    [CW_S_EMAIL] = "邮箱",
    [CW_S_DEPT_CS] = "计算机科学",
    [CW_S_DEPT_MATH] = "数学",
    [CW_S_OVERDUE] = "已逾期",
    [CW_S_DUE_TODAY] = "今日截止",
    [CW_S_THIS_WEEK] = "本周",
    [CW_S_EXAM] = "考试",
    [CW_S_HOMEWORK] = "作业",
    [CW_S_OPEN] = "未完成",
    [CW_S_LANGUAGE] = "语言",
    [CW_S_TIME_FORMAT] = "时间格式",
    [CW_S_TEMP_UNIT] = "温度单位",
    [CW_S_WIFI] = "无线网络",
    [CW_S_RESET_DATA] = "重置演示数据",
    [CW_S_CONFIRM_RESET] = "确认重置?",
    [CW_S_START] = "开始",
    [CW_S_PAUSE] = "暂停",
    [CW_S_ABOUT] = "关于",
    [CW_S_HOLD_OK_RECORD] = "按住 OK 录音",
    [CW_S_ONDEVICE_MEMO] = "本机备忘",
    [CW_S_GOOD_AFTERNOON] = "下午好",
    [CW_S_DEMO_DATA] = "演示数据",
    [CW_S_NO_CLASSES] = "不上课",
    [CW_S_ALL_DONE] = "全部完成!",
    [CW_S_MIN_ABBR] = "分钟",
    [CW_S_WX_PARTLY_CLOUDY] = "多云",
    [CW_S_WX_CLEAR] = "晴",
    [CW_S_WX_RAIN] = "雨",
};

const char *cw_str(cw_str_id_t id) {
    if ((int)id < 0 || id >= CW_S_COUNT) return "?";
    return (s_lang == CW_LANG_ZH) ? S_ZH[id] : S_EN[id];
}

// 节次:1:09:00-09:50 2:10:00-10:50 3:11:00-12:20 4:13:00-13:50
//      5:14:00-14:50 6:15:30-16:50 7:17:00-17:50
static const int PERIOD_START[CW_PERIOD_COUNT] = {
    9 * 60, 10 * 60, 11 * 60, 13 * 60, 14 * 60, 15 * 60 + 30, 17 * 60,
};
static const int PERIOD_END[CW_PERIOD_COUNT] = {
    9 * 60 + 50, 10 * 60 + 50, 12 * 60 + 20, 13 * 60 + 50,
    14 * 60 + 50, 16 * 60 + 50, 17 * 60 + 50,
};

int cw_period_start_min(int period) {
    if (period < 1 || period > CW_PERIOD_COUNT) return -1;
    return PERIOD_START[period - 1];
}

int cw_period_end_min(int period) {
    if (period < 1 || period > CW_PERIOD_COUNT) return -1;
    return PERIOD_END[period - 1];
}

// 课程数据
static const uint8_t COURSES_MATH100[] = {0};
static const uint8_t COURSES_CARTER[] = {1, 2};

static const cw_course_t COURSES[] = {
    {
        .code = "MATH 100", .title_en = "Differential Calculus", .title_zh = "微分学",
        .room = "LSK 200", .teacher = 1, .color = 0x8B5CF6,
        .slots = {{CW_DAY_MON, 1}, {CW_DAY_TUE, 1}, {CW_DAY_THU, 1}}, .slot_count = 3,
    },
    {
        .code = "CPSC 110", .title_en = "Computation, Programs", .title_zh = "计算与程序",
        .room = "DMP 110", .teacher = 0, .color = 0x4C8DFF,
        .slots = {{CW_DAY_MON, 3}, {CW_DAY_TUE, 3}, {CW_DAY_WED, 3}, {CW_DAY_FRI, 3}},
        .slot_count = 4,
    },
    {
        .code = "CPSC 210", .title_en = "Software Construction", .title_zh = "软件构造",
        .room = "ICCS X836", .teacher = 0, .color = 0x22B8A8,
        .slots = {{CW_DAY_TUE, 6}, {CW_DAY_WED, 6}}, .slot_count = 2,
    },
};

int cw_course_count(void) {
    return (int)(sizeof(COURSES) / sizeof(COURSES[0]));
}

const cw_course_t *cw_course(int idx) {
    if (idx < 0 || idx >= cw_course_count()) return NULL;
    return &COURSES[idx];
}

int cw_classes_for_day(int day, cw_class_t *out, int max_out) {
    int n = 0;
    if (day < 0 || day > 6 || out == NULL || max_out <= 0) return 0;
    for (int c = 0; c < cw_course_count() && n < max_out; c++) {
        const cw_course_t *course = &COURSES[c];
        for (int s = 0; s < course->slot_count && n < max_out; s++) {
            if (course->slots[s].day == day) {
                // 按节次插入排序(每天课少,直接插入)
                int pos = n;
                while (pos > 0 && out[pos - 1].period > course->slots[s].period) {
                    out[pos] = out[pos - 1];
                    pos--;
                }
                out[pos].course = course;
                out[pos].period = course->slots[s].period;
                n++;
            }
        }
    }
    return n;
}

cw_class_state_t cw_class_state(int day, int period, int today_day, int now_min) {
    if (day != today_day) return CW_CLASS_UPCOMING;
    int start = cw_period_start_min(period);
    int end = cw_period_end_min(period);
    if (now_min >= end) return CW_CLASS_DONE;
    if (now_min >= start) return CW_CLASS_NOW;
    // 今天尚未开始的第一节约课标 NEXT,其余 UPCOMING
    cw_class_t classes[CW_MAX_SLOTS_PER_DAY];
    int n = cw_classes_for_day(day, classes, CW_MAX_SLOTS_PER_DAY);
    for (int i = 0; i < n; i++) {
        if (cw_period_start_min(classes[i].period) > now_min) {
            return (classes[i].period == period) ? CW_CLASS_NEXT : CW_CLASS_UPCOMING;
        }
    }
    return CW_CLASS_UPCOMING;
}

bool cw_next_class(int today_day, int now_min, int *out_day, cw_class_t *out_class) {
    cw_class_t classes[CW_MAX_SLOTS_PER_DAY];
    for (int d = 0; d < 8; d++) {
        int day = (today_day + d) % 7;
        int n = cw_classes_for_day(day, classes, CW_MAX_SLOTS_PER_DAY);
        for (int i = 0; i < n; i++) {
            int start = cw_period_start_min(classes[i].period);
            if (d == 0 && start <= now_min) continue;
            if (out_day) *out_day = day;
            if (out_class) *out_class = classes[i];
            return true;
        }
    }
    return false;
}

// 教师数据
static const cw_teacher_t TEACHERS[] = {
    {
        .name = "Prof. Carter", .dept = CW_S_DEPT_CS,
        .email = "carter@cs.ubc.ca", .office = "ICCS 239",
        .courses = COURSES_CARTER, .course_count = 2,
        .hours = {{CW_DAY_WED, 14 * 60, 15 * 60}, {CW_DAY_FRI, 10 * 60, 11 * 60}},
        .hour_count = 2,
    },
    {
        .name = "Dr. Liu", .dept = CW_S_DEPT_MATH,
        .email = "liu@math.ubc.ca", .office = "MATH 204",
        .courses = COURSES_MATH100, .course_count = 1,
        .hours = {{CW_DAY_THU, 10 * 60, 11 * 60}},
        .hour_count = 1,
    },
};

int cw_teacher_count(void) {
    return (int)(sizeof(TEACHERS) / sizeof(TEACHERS[0]));
}

const cw_teacher_t *cw_teacher(int idx) {
    if (idx < 0 || idx >= cw_teacher_count()) return NULL;
    return &TEACHERS[idx];
}

int cw_office_hours_week(cw_oh_entry_t *out, int max_out) {
    int n = 0;
    if (out == NULL || max_out <= 0) return 0;
    for (int t = 0; t < cw_teacher_count() && n < max_out; t++) {
        const cw_teacher_t *teacher = &TEACHERS[t];
        for (int h = 0; h < teacher->hour_count && n < max_out; h++) {
            int pos = n;
            uint32_t key =
                ((uint32_t)teacher->hours[h].day << 16) | teacher->hours[h].start_min;
            while (pos > 0) {
                uint32_t prev = ((uint32_t)out[pos - 1].day << 16) | out[pos - 1].start_min;
                if (prev <= key) break;
                out[pos] = out[pos - 1];
                pos--;
            }
            out[pos].teacher = (uint8_t)t;
            out[pos].day = teacher->hours[h].day;
            out[pos].start_min = teacher->hours[h].start_min;
            out[pos].end_min = teacher->hours[h].end_min;
            n++;
        }
    }
    return n;
}

// 校历事件
static const cw_event_t EVENTS[] = {
    {2026, 10, 12, "Thanksgiving Day", "感恩节", "No classes", CW_EVENT_HOLIDAY},
    {2026, 10, 16, "CPSC 110 Midterm", "CPSC 110 期中考试", "19:00 DMP 110", CW_EVENT_EXAM},
    {2026, 11, 11, "Remembrance Day", "国殇纪念日", "No classes", CW_EVENT_HOLIDAY},
    {2026, 12, 8, "Last day of classes", "结课日", NULL, CW_EVENT_INFO},
    {2026, 12, 14, "Final examinations begin", "期末考试周开始", NULL, CW_EVENT_EXAM},
};

int cw_event_count(void) {
    return (int)(sizeof(EVENTS) / sizeof(EVENTS[0]));
}

const cw_event_t *cw_event(int idx) {
    if (idx < 0 || idx >= cw_event_count()) return NULL;
    return &EVENTS[idx];
}

int cw_events_for_month(int year, int month, const cw_event_t **out, int max_out) {
    int n = 0;
    if (out == NULL || max_out <= 0) return 0;
    for (int i = 0; i < cw_event_count() && n < max_out; i++) {
        if (EVENTS[i].year == year && EVENTS[i].month == month) {
            int pos = n;
            while (pos > 0 && out[pos - 1]->day > EVENTS[i].day) {
                out[pos] = out[pos - 1];
                pos--;
            }
            out[pos] = &EVENTS[i];
            n++;
        }
    }
    return n;
}

const char *cw_event_title(const cw_event_t *e) {
    if (e == NULL) return "";
    return (s_lang == CW_LANG_ZH) ? e->title_zh : e->title_en;
}

int cw_days_in_month(int year, int month) {
    static const int DAYS[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return 0;
    int d = DAYS[month - 1];
    if (month == 2) {
        bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        if (leap) d = 29;
    }
    return d;
}

// Zeller 变体,返回 0=周一..6=周日
int cw_weekday_of_date(int year, int month, int day) {
    int y = year, m = month;
    if (m < 3) {
        m += 12;
        y -= 1;
    }
    int k = y % 100;
    int j = y / 100;
    int h = (day + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7; // 0=周六
    return (h + 5) % 7; // 转为 0=周一
}

// 任务数据(due_day_offset 相对 2026-10-12 周一)
static const cw_task_t TASKS[] = {
    {"MATH 100 Problem Set 4", "MATH 100", 0, 23 * 60 + 59, NULL, CW_TASK_HOMEWORK},
    {"CPSC 210 Lab 5", "CPSC 210", 1, 23 * 60 + 59, NULL, CW_TASK_HOMEWORK},
    {"CPSC 110 Midterm", "CPSC 110", 4, 19 * 60, "DMP 110", CW_TASK_EXAM},
};

int cw_task_count(void) {
    return (int)(sizeof(TASKS) / sizeof(TASKS[0]));
}

const cw_task_t *cw_task(int idx) {
    if (idx < 0 || idx >= cw_task_count()) return NULL;
    return &TASKS[idx];
}

cw_task_group_t cw_task_group(const cw_task_t *t, int today_day_offset) {
    if (t == NULL) return CW_TASK_THIS_WEEK;
    if (t->due_day_offset < today_day_offset) return CW_TASK_OVERDUE;
    if (t->due_day_offset == today_day_offset) return CW_TASK_DUE_TODAY;
    return CW_TASK_THIS_WEEK;
}

// 城市与演示天气
static const cw_city_t CITIES[] = {
    {"Vancouver", -7, true, 14, CW_S_WX_PARTLY_CLOUDY, 15, 9},
    {"Tokyo", 9, false, 21, CW_S_WX_CLEAR, 22, 17},
    {"London", 1, false, 11, CW_S_WX_RAIN, 12, 7},
};

int cw_city_count(void) {
    return (int)(sizeof(CITIES) / sizeof(CITIES[0]));
}

const cw_city_t *cw_city(int idx) {
    if (idx < 0 || idx >= cw_city_count()) return NULL;
    return &CITIES[idx];
}

static const cw_student_t STUDENT = {
    .name = "ZHANG Hang",
    .student_no = "1234 5678",
    .program_en = "B.Sc. Computer Science · Year 2",
    .program_zh = "理学学士 · 计算机科学 · 二年级",
    .valid_thru = "04/2027",
};

const cw_student_t *cw_student(void) {
    return &STUDENT;
}

void cw_settings_defaults(cw_settings_t *s) {
    if (s == NULL) return;
    s->lang = CW_LANG_EN;
    s->time_24h = true;
    s->temp_celsius = true;
}

void cw_format_time(int minutes, bool h24, char *buf, size_t len) {
    if (buf == NULL || len == 0) return;
    int m = ((minutes % 1440) + 1440) % 1440;
    int h = m / 60;
    int mm = m % 60;
    if (h24) {
        snprintf(buf, len, "%02d:%02d", h, mm);
    } else {
        const char *ap = (h < 12) ? "AM" : "PM";
        int h12 = h % 12;
        if (h12 == 0) h12 = 12;
        snprintf(buf, len, "%d:%02d %s", h12, mm, ap);
    }
}

void cw_format_dur(int minutes, char *buf, size_t len) {
    if (buf == NULL || len == 0) return;
    if (minutes < 0) minutes = 0;
    if (s_lang == CW_LANG_ZH) {
        if (minutes < 60) {
            snprintf(buf, len, "%d分钟", minutes);
        } else {
            snprintf(buf, len, "%d小时%02d分", minutes / 60, minutes % 60);
        }
    } else {
        if (minutes < 60) {
            snprintf(buf, len, "%d MIN", minutes);
        } else {
            snprintf(buf, len, "%dH %02dMIN", minutes / 60, minutes % 60);
        }
    }
}
