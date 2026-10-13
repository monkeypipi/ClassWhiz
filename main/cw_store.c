// cw_store.c — NVS 持久化实现。
#include "cw_store.h"

#include <stddef.h>
#include <string.h>

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "cw_store";
static const char *NVS_NS = "classwhiz";
static const char *NVS_KEY = "state";

uint32_t cw_crc32(const void *data, size_t len) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t i = 0; i < len; i++) {
        crc ^= bytes[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ (0xEDB88320U & (uint32_t)-(int32_t)(crc & 1));
        }
    }
    return ~crc;
}

void cw_store_blob_pack(cw_store_blob_t *blob, const cw_settings_t *settings,
                        const bool *task_done, const cw_pomo_t *pomo) {
    memset(blob, 0, sizeof(*blob));
    blob->magic = CW_STORE_MAGIC;
    blob->version = CW_STORE_VERSION;
    blob->size = sizeof(*blob);
    if (settings) {
        blob->lang = (uint8_t)settings->lang;
        blob->time_24h = settings->time_24h ? 1 : 0;
        blob->temp_celsius = settings->temp_celsius ? 1 : 0;
    }
    if (task_done) {
        for (int i = 0; i < CW_STORE_MAX_TASKS; i++) {
            blob->task_done[i] = task_done[i] ? 1 : 0;
        }
    }
    if (pomo) {
        blob->pomo_completed = pomo->completed;
        blob->pomo_focus_min = pomo->focus_min;
        blob->pomo_break_min = pomo->break_min;
    }
    blob->crc32 = cw_crc32(blob, offsetof(cw_store_blob_t, crc32));
}

bool cw_store_blob_verify(const cw_store_blob_t *blob) {
    if (blob == NULL) return false;
    if (blob->magic != CW_STORE_MAGIC) return false;
    if (blob->version != CW_STORE_VERSION) return false;
    if (blob->size != sizeof(cw_store_blob_t)) return false;
    uint32_t crc = cw_crc32(blob, offsetof(cw_store_blob_t, crc32));
    return crc == blob->crc32;
}

bool cw_store_init(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS 初始化需要擦除: %s", esp_err_to_name(err));
        err = nvs_flash_erase();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "NVS 擦除失败: %s", esp_err_to_name(err));
            return false;
        }
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS 初始化失败: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool cw_store_load(cw_settings_t *settings, bool *task_done, cw_pomo_t *pomo) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return false;
    cw_store_blob_t blob;
    size_t len = sizeof(blob);
    esp_err_t err = nvs_get_blob(h, NVS_KEY, &blob, &len);
    nvs_close(h);
    if (err != ESP_OK || len != sizeof(blob)) return false;
    if (!cw_store_blob_verify(&blob)) {
        ESP_LOGW(TAG, "存储校验失败,使用默认值");
        return false;
    }
    if (settings) {
        settings->lang = (blob.lang == CW_LANG_ZH) ? CW_LANG_ZH : CW_LANG_EN;
        settings->time_24h = blob.time_24h != 0;
        settings->temp_celsius = blob.temp_celsius != 0;
    }
    if (task_done) {
        int n = cw_task_count();
        for (int i = 0; i < CW_STORE_MAX_TASKS; i++) {
            task_done[i] = (i < n) ? (blob.task_done[i] != 0) : false;
        }
    }
    if (pomo) {
        pomo->completed = blob.pomo_completed;
        if (blob.pomo_focus_min >= 5 && blob.pomo_break_min >= 1) {
            pomo->focus_min = blob.pomo_focus_min;
            pomo->break_min = blob.pomo_break_min;
        }
    }
    return true;
}

bool cw_store_save(const cw_settings_t *settings, const bool *task_done,
                   const cw_pomo_t *pomo) {
    cw_store_blob_t blob;
    cw_store_blob_pack(&blob, settings, task_done, pomo);
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return false;
    esp_err_t err = nvs_set_blob(h, NVS_KEY, &blob, sizeof(blob));
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "存储写入失败: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool cw_store_reset(void) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return false;
    esp_err_t err = nvs_erase_key(h, NVS_KEY);
    if (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND) err = nvs_commit(h);
    nvs_close(h);
    return err == ESP_OK;
}
