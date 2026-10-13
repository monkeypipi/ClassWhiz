// cw_store.h — ClassWhiz 持久化:设置、任务完成态、番茄计数、语音备忘录索引。
// CRC 校验的 blob 存 NVS;写操作由调用方在任务上下文发起(禁在按键回调中直接调用)。
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cw_data.h"
#include "cw_pomodoro.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CW_STORE_MAX_TASKS 8

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint8_t lang;
    uint8_t time_24h;
    uint8_t temp_celsius;
    uint8_t reserved;
    uint8_t task_done[CW_STORE_MAX_TASKS];
    uint32_t pomo_completed;
    uint8_t pomo_focus_min;
    uint8_t pomo_break_min;
    uint16_t reserved2;
    uint32_t crc32;
} cw_store_blob_t;

#define CW_STORE_MAGIC 0x4357495AU // "CWI\x5a"
#define CW_STORE_VERSION 1

uint32_t cw_crc32(const void *data, size_t len);
void cw_store_blob_pack(cw_store_blob_t *blob, const cw_settings_t *settings,
                        const bool *task_done, const cw_pomo_t *pomo);
bool cw_store_blob_verify(const cw_store_blob_t *blob);

// NVS 生命周期(需在 nvs_flash_init 之后调用)
bool cw_store_init(void);
// 读取:成功返回 true(无记录时填默认值并返回 false)
bool cw_store_load(cw_settings_t *settings, bool *task_done, cw_pomo_t *pomo);
// 写入(同步,约数毫秒;在工作任务/LVGL 上下文中调用)
bool cw_store_save(const cw_settings_t *settings, const bool *task_done,
                   const cw_pomo_t *pomo);
// 恢复出厂演示数据
bool cw_store_reset(void);

#ifdef __cplusplus
}
#endif
