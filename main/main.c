// main/main.c —— ClassWhiz 课程助手应用入口。
//
// 按键语义(全局统一):
//   UP/DOWN 短按  各页面内移动选中项
//   OK 短按       确认/进入
//   OK 长按       返回上一级(语音页录音时除外,交由该页处理)
//   DOWN 长按     回到首页
//   OK 双击       课程表页切换列表/周视图
#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "cw_app.h"
#include "key_guard.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "classwhiz";

#define INPUT_QUEUE_DEPTH 8

typedef enum {
    INPUT_KEY = 0,
    INPUT_TICK,
} input_type_t;

typedef struct {
    input_type_t type;
    bsp_btn_t btn;
    bsp_btn_ev_t event;
} input_event_t;

static QueueHandle_t s_input_queue;
static TaskHandle_t s_input_task;
static volatile bool s_input_ready;

// Waking from deep sleep by key reboots the chip while that key is still held:
// drop key actions until that press comes up; the deadline is only a safety net.
#define KEY_GUARD_MAX_US (10LL * 1000 * 1000)
static key_guard_t s_key_guard;

// button callbacks run on the shared esp_timer task; enqueue only and return immediately.
static void on_key(bsp_btn_t btn, bsp_btn_ev_t ev, void *user) {
    (void)user;
    const bool was_armed = s_key_guard.armed;
    if (key_guard_consume(&s_key_guard, ev == BSP_BTN_RELEASE, esp_timer_get_time())) {
        if (was_armed && !s_key_guard.armed) {
            ESP_LOGI(TAG, "唤醒键已松手,按键恢复响应");
        }
        return;
    }
    if (!s_input_ready || !s_input_queue) return;
    const input_event_t input = {.type = INPUT_KEY, .btn = btn, .event = ev};
    (void)xQueueSend(s_input_queue, &input, 0);
}

static void process_input(const input_event_t *input) {
    if (!bsp_lvgl_lock(500)) return;
    if (input->type == INPUT_KEY) {
        cw_app_input(input->btn, input->event);
    } else {
        cw_app_tick(cw_app_mono_ms());
    }
    bsp_lvgl_unlock();
}

static void input_task(void *arg) {
    (void)arg;
    input_event_t input;
    for (;;) {
        // 1 秒超时即产生一次 tick,驱动时钟/倒计时/番茄钟
        if (xQueueReceive(s_input_queue, &input, pdMS_TO_TICKS(1000)) == pdTRUE) {
            process_input(&input);
        } else {
            const input_event_t tick = {.type = INPUT_TICK};
            process_input(&tick);
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "ClassWhiz 启动");
    const uint32_t wake_causes = esp_sleep_get_wakeup_causes();
    if (wake_causes & ~(1u << ESP_SLEEP_WAKEUP_UNDEFINED)) {
        ESP_LOGI(TAG, "休眠唤醒原因位图: 0x%02x", (unsigned)wake_causes);
    }
    if (wake_causes & (1u << ESP_SLEEP_WAKEUP_GPIO)) {
        key_guard_arm(&s_key_guard, esp_timer_get_time(), KEY_GUARD_MAX_US);
        ESP_LOGI(TAG, "按键唤醒:先屏蔽按键动作,直到那颗键松手");
    }

    bsp_i2c_init();
    bsp_i2c_scan();

    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "显示/LVGL 初始化失败,应用无法继续。"
                      "检查 SPI 接线(MOSI=%d SCLK=%d CS=%d DC=%d BL=%d)",
                 BSP_LCD_MOSI, BSP_LCD_SCLK, BSP_LCD_CS, BSP_LCD_DC, BSP_LCD_BL);
        return;
    }
    bsp_display_backlight(100);

    s_input_queue = xQueueCreate(INPUT_QUEUE_DEPTH, sizeof(input_event_t));
    if (!s_input_queue) {
        ESP_LOGE(TAG, "按键事件队列创建失败");
        return;
    }
    bool btn_ok = (bsp_button_init(on_key, NULL) == ESP_OK);
    if (!btn_ok) {
        ESP_LOGE(TAG, "按键初始化失败");
    } else if (s_key_guard.armed) {
        const bool released = bsp_button_read_mv() >= BSP_BTN_MV_RELEASED_MIN;
        key_guard_sync_released(&s_key_guard, released);
    }
    if (xTaskCreate(input_task, "cw_input", 4096, NULL, 5, &s_input_task) != pdPASS) {
        ESP_LOGE(TAG, "输入任务创建失败");
        return;
    }

    bool audio_ok = (bsp_audio_init() == ESP_OK);
    bool bat_ok = (bsp_battery_init() == ESP_OK);
    ESP_LOGI(TAG, "外设: Button=%d Audio=%d Battery=%d", btn_ok, audio_ok, bat_ok);

    if (bsp_lvgl_lock(1000)) {
        cw_app_start();
        bsp_lvgl_unlock();
        s_input_ready = true;
    }

    ESP_LOGI(TAG, "就绪");
}
