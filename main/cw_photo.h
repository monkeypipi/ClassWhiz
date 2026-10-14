// cw_photo.h — 学生证照片存储管理。
#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 照片文件路径(SPIFFS)
#define CW_PHOTO_PATH "/spiffs/student.jpg"
// 照片最大尺寸(96x96 JPEG 约 8KB,留余量)
#define CW_PHOTO_MAX_SIZE (32 * 1024)

// 初始化 SPIFFS
bool cw_photo_init(void);

// 保存照片数据(JPEG)
// 返回 true 表示成功
bool cw_photo_save(const uint8_t *data, size_t len);

// 照片是否存在
bool cw_photo_exists(void);

// 删除照片
bool cw_photo_delete(void);

// 获取照片路径(供 LVGL 显示)
const char *cw_photo_path(void);

#ifdef __cplusplus
}
#endif
