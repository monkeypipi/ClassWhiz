// cw_config.h — ClassWhiz 动态配置管理。
//
// 将课程/教师/任务/校历从编译时 static const 改为运行时可修改、NVS 持久化。
// Web 配置门户通过 JSON 更新这些数据。
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cw_data.h"

#ifdef __cplusplus
extern "C" {
#endif

// 可变课程存储(字符串用定长数组,便于 NVS 序列化)
typedef struct {
    char code[16];
    char title_en[64];
    char title_zh[64];
    char room[32];
    uint8_t teacher;
    uint32_t color;
    cw_slot_t slots[CW_MAX_SLOTS_PER_DAY];
    uint8_t slot_count;
} cw_course_mut_t;

// 可变教师存储
typedef struct {
    char name[48];
    cw_str_id_t dept;
    char email[64];
    char office[32];
    uint8_t courses[CW_MAX_COURSES];
    uint8_t course_count;
    cw_office_hour_t hours[3];
    uint8_t hour_count;
} cw_teacher_mut_t;

// 可变任务存储
typedef struct {
    char title_en[64];
    char title_zh[64];
    uint8_t course;      // 课程索引, 0xFF 表示无
    uint8_t kind;        // 0=作业 1=考试 2=项目
    uint16_t due_year;
    uint8_t due_month;
    uint8_t due_day;
    uint16_t due_min;    // 当天分钟数
} cw_task_mut_t;

// 可变校历事件存储
typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t kind;        // cw_event_kind_t
    char title_en[64];
    char title_zh[64];
} cw_event_mut_t;

// 学生证信息(可变)
typedef struct {
    char name_en[64];
    char name_zh[64];
    char student_id[32];
    char university_en[64];
    char university_zh[64];
    char major_en[64];
    char major_zh[64];
    // 照片存 SPIFFS,此处仅标记是否有照片
    bool has_photo;
} cw_student_mut_t;

// 天气城市配置
typedef struct {
    char cities[CW_MAX_CITIES][48]; // 城市名(英文,用于 API 查询)
    char city_labels[CW_MAX_CITIES][48]; // 显示名(可中文)
    uint8_t city_count;
} cw_weather_cfg_t;

// 初始化:从 NVS 加载,失败则用默认数据
// 返回 true 表示从 NVS 恢复, false 表示使用默认值
bool cw_config_init(void);

// 保存当前配置到 NVS
bool cw_config_save(void);

// 恢复出厂默认
void cw_config_reset_defaults(void);

// 获取可变存储(供 Web API 更新)
cw_course_mut_t *cw_config_courses_mut(int *count);
cw_teacher_mut_t *cw_config_teachers_mut(int *count);
cw_task_mut_t *cw_config_tasks_mut(int *count);
cw_event_mut_t *cw_config_events_mut(int *count);
cw_student_mut_t *cw_config_student_mut(void);
cw_weather_cfg_t *cw_config_weather_mut(void);

// JSON 序列化/反序列化(供 Web API)
bool cw_config_to_json(char *buf, size_t buf_len);
bool cw_config_from_json(const char *json);

#ifdef __cplusplus
}
#endif
