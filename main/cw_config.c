// cw_config.c — 动态配置管理实现。
#include "cw_config.h"

#include <string.h>

#include "cJSON.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "cw_config";
static const char *NVS_NS = "classwhiz";
static const char *NVS_KEY_CFG = "dyn_cfg";  // JSON 配置

// ---------- 运行时存储 ----------
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

// ---------- 默认数据(从 cw_data.c 静态数据迁移) ----------
static void load_defaults(void) {
    // 课程默认
    s_course_count = 3;
    memset(s_courses, 0, sizeof(s_courses));
    // MATH 100
    strncpy(s_courses[0].code, "MATH 100", sizeof(s_courses[0].code) - 1);
    strncpy(s_courses[0].title_en, "Differential Calculus", sizeof(s_courses[0].title_en) - 1);
    strncpy(s_courses[0].title_zh, "微分学", sizeof(s_courses[0].title_zh) - 1);
    strncpy(s_courses[0].room, "LSK 200", sizeof(s_courses[0].room) - 1);
    s_courses[0].teacher = 1;
    s_courses[0].color = 0x8B5CF6;
    s_courses[0].slots[0] = (cw_slot_t){CW_DAY_MON, 1};
    s_courses[0].slots[1] = (cw_slot_t){CW_DAY_TUE, 1};
    s_courses[0].slots[2] = (cw_slot_t){CW_DAY_THU, 1};
    s_courses[0].slot_count = 3;
    // CPSC 110
    strncpy(s_courses[1].code, "CPSC 110", sizeof(s_courses[1].code) - 1);
    strncpy(s_courses[1].title_en, "Computation, Programs", sizeof(s_courses[1].title_en) - 1);
    strncpy(s_courses[1].title_zh, "计算与程序", sizeof(s_courses[1].title_zh) - 1);
    strncpy(s_courses[1].room, "DMP 110", sizeof(s_courses[1].room) - 1);
    s_courses[1].teacher = 0;
    s_courses[1].color = 0x4C8DFF;
    s_courses[1].slots[0] = (cw_slot_t){CW_DAY_MON, 3};
    s_courses[1].slots[1] = (cw_slot_t){CW_DAY_TUE, 3};
    s_courses[1].slots[2] = (cw_slot_t){CW_DAY_WED, 3};
    s_courses[1].slots[3] = (cw_slot_t){CW_DAY_FRI, 3};
    s_courses[1].slot_count = 4;
    // CPSC 210
    strncpy(s_courses[2].code, "CPSC 210", sizeof(s_courses[2].code) - 1);
    strncpy(s_courses[2].title_en, "Software Construction", sizeof(s_courses[2].title_en) - 1);
    strncpy(s_courses[2].title_zh, "软件构造", sizeof(s_courses[2].title_zh) - 1);
    strncpy(s_courses[2].room, "ICCS X836", sizeof(s_courses[2].room) - 1);
    s_courses[2].teacher = 0;
    s_courses[2].color = 0x22B8A8;
    s_courses[2].slots[0] = (cw_slot_t){CW_DAY_TUE, 6};
    s_courses[2].slots[1] = (cw_slot_t){CW_DAY_WED, 6};
    s_courses[2].slot_count = 2;

    // 教师默认
    s_teacher_count = 2;
    memset(s_teachers, 0, sizeof(s_teachers));
    strncpy(s_teachers[0].name, "Prof. Carter", sizeof(s_teachers[0].name) - 1);
    s_teachers[0].dept = CW_S_DEPT_CS;
    strncpy(s_teachers[0].email, "carter@cs.ubc.ca", sizeof(s_teachers[0].email) - 1);
    strncpy(s_teachers[0].office, "ICCS 239", sizeof(s_teachers[0].office) - 1);
    s_teachers[0].courses[0] = 1; s_teachers[0].courses[1] = 2;
    s_teachers[0].course_count = 2;
    s_teachers[0].hours[0] = (cw_office_hour_t){CW_DAY_WED, 14 * 60, 15 * 60};
    s_teachers[0].hours[1] = (cw_office_hour_t){CW_DAY_FRI, 10 * 60, 11 * 60};
    s_teachers[0].hour_count = 2;

    strncpy(s_teachers[1].name, "Dr. Smith", sizeof(s_teachers[1].name) - 1);
    s_teachers[1].dept = CW_S_DEPT_MATH;
    strncpy(s_teachers[1].email, "smith@math.ubc.ca", sizeof(s_teachers[1].email) - 1);
    strncpy(s_teachers[1].office, "LSK 300", sizeof(s_teachers[1].office) - 1);
    s_teachers[1].courses[0] = 0;
    s_teachers[1].course_count = 1;
    s_teachers[1].hours[0] = (cw_office_hour_t){CW_DAY_TUE, 13 * 60, 14 * 60};
    s_teachers[1].hour_count = 1;

    // 任务默认(空,由用户通过网页添加)
    s_task_count = 0;
    memset(s_tasks, 0, sizeof(s_tasks));

    // 校历默认(空)
    s_event_count = 0;
    memset(s_events, 0, sizeof(s_events));

    // 学生证默认
    memset(&s_student, 0, sizeof(s_student));
    strncpy(s_student.name_en, "Hang Zhang", sizeof(s_student.name_en) - 1);
    strncpy(s_student.name_zh, "张杭", sizeof(s_student.name_zh) - 1);
    strncpy(s_student.student_id, "12345678", sizeof(s_student.student_id) - 1);
    strncpy(s_student.university_en, "UBC", sizeof(s_student.university_en) - 1);
    strncpy(s_student.university_zh, "不列颠哥伦比亚大学", sizeof(s_student.university_zh) - 1);
    s_student.has_photo = false;

    // 天气默认:温哥华
    memset(&s_weather, 0, sizeof(s_weather));
    strncpy(s_weather.cities[0], "Vancouver", sizeof(s_weather.cities[0]) - 1);
    strncpy(s_weather.city_labels[0], "温哥华", sizeof(s_weather.city_labels[0]) - 1);
    s_weather.city_count = 1;
}

// ---------- NVS ----------

bool cw_config_init(void) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "NVS open failed, using defaults");
        load_defaults();
        return false;
    }

    size_t len = 0;
    err = nvs_get_str(h, NVS_KEY_CFG, NULL, &len);
    nvs_close(h);

    if (err != ESP_OK || len == 0) {
        ESP_LOGI(TAG, "No saved config, using defaults");
        load_defaults();
        return false;
    }

    char *json = malloc(len);
    if (!json) {
        load_defaults();
        return false;
    }

    err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err == ESP_OK) {
        nvs_get_str(h, NVS_KEY_CFG, json, &len);
        nvs_close(h);
    }

    bool ok = cw_config_from_json(json);
    free(json);
    if (!ok) {
        ESP_LOGW(TAG, "Config parse failed, using defaults");
        load_defaults();
        return false;
    }
    ESP_LOGI(TAG, "Config loaded from NVS");
    return true;
}

bool cw_config_save(void) {
    char buf[8192];
    if (!cw_config_to_json(buf, sizeof(buf))) {
        ESP_LOGE(TAG, "JSON serialize failed");
        return false;
    }

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return false;

    err = nvs_set_str(h, NVS_KEY_CFG, buf);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err == ESP_OK;
}

void cw_config_reset_defaults(void) {
    load_defaults();
    cw_config_save();
}

// ---------- 访问器 ----------

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

cw_student_mut_t *cw_config_student_mut(void) {
    return &s_student;
}

cw_weather_cfg_t *cw_config_weather_mut(void) {
    return &s_weather;
}

// ---------- JSON ----------

static cJSON *course_to_json(const cw_course_mut_t *c) {
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "code", c->code);
    cJSON_AddStringToObject(obj, "title_en", c->title_en);
    cJSON_AddStringToObject(obj, "title_zh", c->title_zh);
    cJSON_AddStringToObject(obj, "room", c->room);
    cJSON_AddNumberToObject(obj, "teacher", c->teacher);
    cJSON_AddNumberToObject(obj, "color", (double)c->color);
    cJSON *slots = cJSON_CreateArray();
    for (int i = 0; i < c->slot_count; i++) {
        cJSON *s = cJSON_CreateObject();
        cJSON_AddNumberToObject(s, "day", c->slots[i].day);
        cJSON_AddNumberToObject(s, "period", c->slots[i].period);
        cJSON_AddItemToArray(slots, s);
    }
    cJSON_AddItemToObject(obj, "slots", slots);
    return obj;
}

static cJSON *teacher_to_json(const cw_teacher_mut_t *t) {
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "name", t->name);
    cJSON_AddNumberToObject(obj, "dept", (double)t->dept);
    cJSON_AddStringToObject(obj, "email", t->email);
    cJSON_AddStringToObject(obj, "office", t->office);
    cJSON *courses = cJSON_CreateArray();
    for (int i = 0; i < t->course_count; i++) {
        cJSON_AddItemToArray(courses, cJSON_CreateNumber(t->courses[i]));
    }
    cJSON_AddItemToObject(obj, "courses", courses);
    cJSON *hours = cJSON_CreateArray();
    for (int i = 0; i < t->hour_count; i++) {
        cJSON *h = cJSON_CreateObject();
        cJSON_AddNumberToObject(h, "day", t->hours[i].day);
        cJSON_AddNumberToObject(h, "start", t->hours[i].start_min);
        cJSON_AddNumberToObject(h, "end", t->hours[i].end_min);
        cJSON_AddItemToArray(hours, h);
    }
    cJSON_AddItemToObject(obj, "hours", hours);
    return obj;
}

bool cw_config_to_json(char *buf, size_t buf_len) {
    if (!buf || buf_len == 0) return false;

    cJSON *root = cJSON_CreateObject();

    cJSON *courses = cJSON_CreateArray();
    for (int i = 0; i < s_course_count; i++) {
        cJSON_AddItemToArray(courses, course_to_json(&s_courses[i]));
    }
    cJSON_AddItemToObject(root, "courses", courses);

    cJSON *teachers = cJSON_CreateArray();
    for (int i = 0; i < s_teacher_count; i++) {
        cJSON_AddItemToArray(teachers, teacher_to_json(&s_teachers[i]));
    }
    cJSON_AddItemToObject(root, "teachers", teachers);

    // 学生证
    cJSON *student = cJSON_CreateObject();
    cJSON_AddStringToObject(student, "name_en", s_student.name_en);
    cJSON_AddStringToObject(student, "name_zh", s_student.name_zh);
    cJSON_AddStringToObject(student, "student_id", s_student.student_id);
    cJSON_AddStringToObject(student, "university_en", s_student.university_en);
    cJSON_AddStringToObject(student, "university_zh", s_student.university_zh);
    cJSON_AddBoolToObject(student, "has_photo", s_student.has_photo);
    cJSON_AddItemToObject(root, "student", student);

    // 天气
    cJSON *weather = cJSON_CreateObject();
    cJSON *cities = cJSON_CreateArray();
    for (int i = 0; i < s_weather.city_count; i++) {
        cJSON *c = cJSON_CreateObject();
        cJSON_AddStringToObject(c, "query", s_weather.cities[i]);
        cJSON_AddStringToObject(c, "label", s_weather.city_labels[i]);
        cJSON_AddItemToArray(cities, c);
    }
    cJSON_AddItemToObject(weather, "cities", cities);
    cJSON_AddItemToObject(root, "weather", weather);

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json_str) return false;

    size_t len = strlen(json_str);
    bool ok = (len < buf_len);
    if (ok) strcpy(buf, json_str);
    cJSON_free(json_str);
    return ok;
}

static void json_to_course(const cJSON *obj, cw_course_mut_t *c) {
    memset(c, 0, sizeof(*c));
    const cJSON *item;
    item = cJSON_GetObjectItem(obj, "code");
    if (cJSON_IsString(item)) strncpy(c->code, item->valuestring, sizeof(c->code) - 1);
    item = cJSON_GetObjectItem(obj, "title_en");
    if (cJSON_IsString(item)) strncpy(c->title_en, item->valuestring, sizeof(c->title_en) - 1);
    item = cJSON_GetObjectItem(obj, "title_zh");
    if (cJSON_IsString(item)) strncpy(c->title_zh, item->valuestring, sizeof(c->title_zh) - 1);
    item = cJSON_GetObjectItem(obj, "room");
    if (cJSON_IsString(item)) strncpy(c->room, item->valuestring, sizeof(c->room) - 1);
    item = cJSON_GetObjectItem(obj, "teacher");
    if (cJSON_IsNumber(item)) c->teacher = (uint8_t)item->valueint;
    item = cJSON_GetObjectItem(obj, "color");
    if (cJSON_IsNumber(item)) c->color = (uint32_t)item->valueint;
    item = cJSON_GetObjectItem(obj, "slots");
    if (cJSON_IsArray(item)) {
        int n = cJSON_GetArraySize(item);
        for (int i = 0; i < n && i < CW_MAX_SLOTS_PER_DAY; i++) {
            const cJSON *s = cJSON_GetArrayItem(item, i);
            const cJSON *day = cJSON_GetObjectItem(s, "day");
            const cJSON *period = cJSON_GetObjectItem(s, "period");
            if (cJSON_IsNumber(day) && cJSON_IsNumber(period)) {
                c->slots[c->slot_count].day = (uint8_t)day->valueint;
                c->slots[c->slot_count].period = (uint8_t)period->valueint;
                c->slot_count++;
            }
        }
    }
}

bool cw_config_from_json(const char *json) {
    if (!json) return false;
    cJSON *root = cJSON_Parse(json);
    if (!root) return false;

    // 课程
    const cJSON *courses = cJSON_GetObjectItem(root, "courses");
    if (cJSON_IsArray(courses)) {
        int n = cJSON_GetArraySize(courses);
        s_course_count = 0;
        for (int i = 0; i < n && i < CW_MAX_COURSES; i++) {
            json_to_course(cJSON_GetArrayItem(courses, i), &s_courses[s_course_count]);
            s_course_count++;
        }
    }

    // 学生证
    const cJSON *student = cJSON_GetObjectItem(root, "student");
    if (cJSON_IsObject(student)) {
        const cJSON *item;
        item = cJSON_GetObjectItem(student, "name_en");
        if (cJSON_IsString(item)) strncpy(s_student.name_en, item->valuestring, sizeof(s_student.name_en) - 1);
        item = cJSON_GetObjectItem(student, "name_zh");
        if (cJSON_IsString(item)) strncpy(s_student.name_zh, item->valuestring, sizeof(s_student.name_zh) - 1);
        item = cJSON_GetObjectItem(student, "student_id");
        if (cJSON_IsString(item)) strncpy(s_student.student_id, item->valuestring, sizeof(s_student.student_id) - 1);
    }

    // 天气
    const cJSON *weather = cJSON_GetObjectItem(root, "weather");
    if (cJSON_IsObject(weather)) {
        const cJSON *cities = cJSON_GetObjectItem(weather, "cities");
        if (cJSON_IsArray(cities)) {
            int n = cJSON_GetArraySize(cities);
            s_weather.city_count = 0;
            for (int i = 0; i < n && i < CW_MAX_CITIES; i++) {
                const cJSON *c = cJSON_GetArrayItem(cities, i);
                const cJSON *q = cJSON_GetObjectItem(c, "query");
                const cJSON *l = cJSON_GetObjectItem(c, "label");
                if (cJSON_IsString(q)) strncpy(s_weather.cities[s_weather.city_count], q->valuestring, 47);
                if (cJSON_IsString(l)) strncpy(s_weather.city_labels[s_weather.city_count], l->valuestring, 47);
                s_weather.city_count++;
            }
        }
    }

    cJSON_Delete(root);
    return true;
}
