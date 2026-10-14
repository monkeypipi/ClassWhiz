// cw_photo.c — 学生证照片存储实现。
#include "cw_photo.h"

#include <string.h>

#include "esp_log.h"
#include "esp_spiffs.h"

static const char *TAG = "cw_photo";
static bool s_mounted = false;

bool cw_photo_init(void) {
    if (s_mounted) return true;

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 4,
        .format_if_mount_failed = true,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(ret));
        return false;
    }

    size_t total, used;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS mounted: total=%d used=%d", (int)total, (int)used);
    }

    s_mounted = true;
    return true;
}

bool cw_photo_save(const uint8_t *data, size_t len) {
    if (!s_mounted || !data || len != CW_PHOTO_SIZE) {
        ESP_LOGE(TAG, "Invalid photo size: %d (expected %d)", (int)len, CW_PHOTO_SIZE);
        return false;
    }

    FILE *f = fopen(CW_PHOTO_PATH, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open photo file for writing");
        return false;
    }

    size_t written = fwrite(data, 1, len, f);
    fclose(f);

    if (written != len) {
        ESP_LOGE(TAG, "Photo write incomplete");
        return false;
    }

    ESP_LOGI(TAG, "Photo saved: %d bytes", (int)len);
    return true;
}

bool cw_photo_exists(void) {
    if (!s_mounted) return false;
    FILE *f = fopen(CW_PHOTO_PATH, "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

bool cw_photo_delete(void) {
    if (!s_mounted) return false;
    return unlink(CW_PHOTO_PATH) == 0;
}

const char *cw_photo_path(void) {
    return CW_PHOTO_PATH;
}
