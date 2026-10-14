// cw_s_voice.c — Muse 语音:本机语音备忘(按住 OK 录音,扬声器回放)。
// 录音/播放在工作任务中执行阻塞式 bsp_audio_read/write;按键回调只发标志。
#include "cw_app.h"
#include "cw_ui.h"

#include <stdio.h>
#include <string.h>

#include "bsp_audio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "cw_voice";

#define VOICE_RATE 8000
#define VOICE_SEC 5
#define VOICE_SAMPLES (VOICE_RATE * VOICE_SEC)
#define VOICE_SLOTS 2

static lv_obj_t *s_scr;
static int s_sel;
static lv_obj_t *s_rows[VOICE_SLOTS];
static lv_obj_t *s_status;
static lv_obj_t *s_bars[7];
static lv_obj_t *s_rings[2];

static int16_t *s_slots[VOICE_SLOTS];
static bool s_has[VOICE_SLOTS];
static volatile int s_mode; // 0=空闲 1=录音中 2=播放中
static volatile bool s_stop_req;
static int s_active_slot;
static TaskHandle_t s_worker;

static void anim_h_cb(void *var, int32_t v) {
    lv_obj_set_height((lv_obj_t *)var, (int16_t)v);
}

// C3 上 transform_scale 每帧要做整对象变换渲染,会卡死 LVGL 任务;扩散环只用透明度。
static void anim_ring_opa_cb(void *var, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)(60 - v * 60 / 1000), 0);
}

static void eq_start(void) {
    for (int i = 0; i < 7; i++) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, s_bars[i]);
        lv_anim_set_exec_cb(&a, anim_h_cb);
        lv_anim_set_values(&a, 6, 26);
        lv_anim_set_duration(&a, 500);
        lv_anim_set_reverse_duration(&a, 500);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_delay(&a, i * 120);
        lv_anim_start(&a);
    }
    for (int i = 0; i < 2; i++) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, s_rings[i]);
        lv_anim_set_exec_cb(&a, anim_ring_opa_cb);
        lv_anim_set_values(&a, 0, 1000);
        lv_anim_set_duration(&a, 1600);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_delay(&a, i * 800);
        lv_anim_start(&a);
    }
}

static void eq_stop(void) {
    for (int i = 0; i < 7; i++) {
        lv_anim_delete(s_bars[i], NULL);
        lv_obj_set_height(s_bars[i], 10);
    }
    for (int i = 0; i < 2; i++) {
        lv_anim_delete(s_rings[i], NULL);
        lv_obj_set_style_opa(s_rings[i], LV_OPA_60, 0);
    }
}

static void worker_task(void *arg) {
    (void)arg;
    if (s_mode == 1) {
        if (bsp_audio_set_format(VOICE_RATE, 16, 1) != ESP_OK) {
            ESP_LOGE(TAG, "record format failed");
            s_mode = 0;
            vTaskDelete(NULL);
            return;
        }
        int16_t *buf = s_slots[s_active_slot];
        size_t got = 0;
        const size_t chunk = 1024;
        while (got < VOICE_SAMPLES && !s_stop_req) {
            size_t n = chunk;
            if (got + n > VOICE_SAMPLES) n = VOICE_SAMPLES - got;
            if (bsp_audio_read(buf + got, n * sizeof(int16_t)) != ESP_OK) break;
            got += n;
        }
        memset(buf + got, 0, (VOICE_SAMPLES - got) * sizeof(int16_t));
        s_has[s_active_slot] = (got > VOICE_RATE / 2);
        ESP_LOGI(TAG, "recorded slot %d: %d samples", s_active_slot, (int)got);
    } else if (s_mode == 2) {
        if (bsp_audio_set_format(VOICE_RATE, 16, 1) != ESP_OK) {
            ESP_LOGE(TAG, "play format failed");
            s_mode = 0;
            vTaskDelete(NULL);
            return;
        }
        bsp_audio_set_volume(80);
        int16_t *buf = s_slots[s_active_slot];
        size_t played = 0;
        const size_t chunk = 1024;
        while (played < VOICE_SAMPLES && !s_stop_req) {
            size_t n = chunk;
            if (played + n > VOICE_SAMPLES) n = VOICE_SAMPLES - played;
            if (bsp_audio_write(buf + played, n * sizeof(int16_t)) != ESP_OK) break;
            played += n;
        }
        ESP_LOGI(TAG, "played slot %d", s_active_slot);
    }
    s_mode = 0;
    s_worker = NULL;
    vTaskDelete(NULL);
}

static bool ensure_slot(int slot) {
    if (s_slots[slot]) return true;
    size_t bytes = VOICE_SAMPLES * sizeof(int16_t);
    if (heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) < bytes + 16384) {
        ESP_LOGW(TAG, "not enough heap for voice buffer");
        return false;
    }
    s_slots[slot] = (int16_t *)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
    return s_slots[slot] != NULL;
}

static void start_record(void) {
    if (s_mode != 0) return;
    if (!ensure_slot(s_sel)) {
        lv_label_set_text(s_status, "No memory");
        return;
    }
    s_active_slot = s_sel;
    s_stop_req = false;
    s_mode = 1;
    char b[40];
    snprintf(b, sizeof(b), "REC %d/%ds", s_sel + 1, VOICE_SEC);
    lv_label_set_text(s_status, b);
    eq_start();
    xTaskCreate(worker_task, "cw_voice", 4096, NULL, 5, &s_worker);
}

static void start_play(void) {
    if (s_mode != 0 || !s_has[s_sel]) return;
    s_active_slot = s_sel;
    s_stop_req = false;
    s_mode = 2;
    char b[40];
    snprintf(b, sizeof(b), "PLAY %d", s_sel + 1);
    lv_label_set_text(s_status, b);
    xTaskCreate(worker_task, "cw_voice", 4096, NULL, 5, &s_worker);
}

static void stop_all(void) {
    if (s_mode == 0) return;
    s_stop_req = true; // 工作任务在下一个 chunk 边界退出,无需持锁等待
    eq_stop();
    lv_label_set_text(s_status, cw_str(CW_S_HOLD_OK_RECORD));
}

static void refresh_rows(void) {
    for (int i = 0; i < VOICE_SLOTS; i++) {
        lv_obj_set_style_border_color(
            s_rows[i], lv_color_hex(i == s_sel ? CW_C_ACCENT : CW_C_CARD_LINE), 0);
        lv_obj_set_style_border_width(s_rows[i], i == s_sel ? 2 : 1, 0);
        lv_obj_t *l = lv_obj_get_child(s_rows[i], 0);
        char b[48];
        if (s_has[i]) {
            snprintf(b, sizeof(b), "Memo %d · %ds", i + 1, VOICE_SEC);
        } else {
            snprintf(b, sizeof(b), "Memo %d · --", i + 1);
        }
        if (s_mode != 0 && s_active_slot == i) {
            snprintf(b, sizeof(b), "Memo %d · %s", i + 1,
                     s_mode == 1 ? "REC" : "PLAY");
        }
        lv_label_set_text(l, b);
    }
}

static void voice_enter(void *ctx) {
    (void)ctx;
    cw_screen_captures_ok_long = true;
    s_scr = cw_screen_create(cw_str(CW_S_VOICE));
    lv_obj_t *body = cw_screen_content(s_scr);

    // 麦克风 + 波纹
    lv_obj_t *micwrap = lv_obj_create(body);
    lv_obj_set_size(micwrap, 72, 72);
    lv_obj_set_style_bg_opa(micwrap, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(micwrap, 0, 0);
    lv_obj_set_style_pad_all(micwrap, 0, 0);
    lv_obj_set_scrollbar_mode(micwrap, LV_SCROLLBAR_MODE_OFF);
    for (int i = 0; i < 2; i++) {
        lv_obj_t *ring = lv_obj_create(micwrap);
        lv_obj_set_size(ring, 56, 56);
        lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(ring, lv_color_hex(CW_C_PINK), 0);
        lv_obj_set_style_border_width(ring, 2, 0);
        lv_obj_set_style_opa(ring, LV_OPA_60, 0);
        lv_obj_center(ring);
        s_rings[i] = ring;
    }
    lv_obj_t *mic = lv_obj_create(micwrap);
    lv_obj_set_size(mic, 48, 48);
    lv_obj_set_style_radius(mic, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(mic, lv_color_hex(0x3A2A52), 0);
    lv_obj_set_style_border_color(mic, lv_color_hex(CW_C_PINK), 0);
    lv_obj_set_style_border_width(mic, 2, 0);
    lv_obj_center(mic);
    lv_obj_t *ml = cw_label(mic, "MIC");
    lv_obj_center(ml);

    // EQ 条
    lv_obj_t *bars = lv_obj_create(body);
    lv_obj_set_size(bars, 120, 28);
    lv_obj_set_style_bg_opa(bars, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bars, 0, 0);
    lv_obj_set_style_pad_all(bars, 0, 0);
    lv_obj_set_flex_flow(bars, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bars, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(bars, 5, 0);
    lv_obj_set_scrollbar_mode(bars, LV_SCROLLBAR_MODE_OFF);
    for (int i = 0; i < 7; i++) {
        lv_obj_t *bar = lv_obj_create(bars);
        lv_obj_set_size(bar, 5, 10);
        lv_obj_set_style_bg_color(bar, lv_color_hex(CW_C_PINK), 0);
        lv_obj_set_style_bg_grad_color(bar, lv_color_hex(CW_C_PURPLE), 0);
        lv_obj_set_style_bg_grad_dir(bar, LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_radius(bar, 2, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        s_bars[i] = bar;
    }

    s_status = cw_label(body, cw_str(CW_S_HOLD_OK_RECORD));
    cw_label_small(body, cw_str(CW_S_ONDEVICE_MEMO));

    for (int i = 0; i < VOICE_SLOTS; i++) {
        lv_obj_t *row = cw_card(body);
        lv_obj_set_width(row, LV_PCT(100));
        lv_obj_set_height(row, 32);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_START);
        cw_label(row, "");
        s_rows[i] = row;
    }
    refresh_rows();
    // 本页截获了 OK 长按(录音),全局的"长按 OK 返回"在这里失效;
    // 唯一的退出是全局的"长按 DN 回首页",必须写进提示条,否则用户会以为卡死了。
    cw_hint_bar(s_scr, "Hold OK: Rec", "Hold DN: Home");
}

static void voice_exit(void) {
    // 非阻塞停止:置位后工作任务自行退出(约 <100ms),tick 会刷新状态
    s_stop_req = true;
    eq_stop();
    cw_screen_captures_ok_long = false;
}

static void voice_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (ev == BSP_BTN_LONG && btn == BSP_BTN_OK) {
        start_record();
        refresh_rows();
        return;
    }
    if (ev == BSP_BTN_RELEASE && btn == BSP_BTN_OK) {
        if (s_mode == 1) {
            stop_all();
            refresh_rows();
        }
        return;
    }
    if (ev != BSP_BTN_CLICK) return;
    if (btn == BSP_BTN_UP) {
        s_sel = (s_sel + VOICE_SLOTS - 1) % VOICE_SLOTS;
        refresh_rows();
    } else if (btn == BSP_BTN_DOWN) {
        s_sel = (s_sel + 1) % VOICE_SLOTS;
        refresh_rows();
    } else if (btn == BSP_BTN_OK) {
        if (s_mode == 2) {
            stop_all();
        } else {
            start_play();
        }
        refresh_rows();
    }
}

static void voice_tick(uint64_t now_ms) {
    (void)now_ms;
    // 录音/播放结束后刷新状态(工作任务置 s_mode=0)
    static int last_mode = -1;
    if ((int)s_mode != last_mode) {
        last_mode = (int)s_mode;
        if (s_mode == 0) {
            eq_stop();
            lv_label_set_text(s_status, cw_str(CW_S_HOLD_OK_RECORD));
        }
        refresh_rows();
    }
}

const cw_screen_def_t cw_scr_voice_def = {
    .enter = voice_enter,
    .exit = voice_exit,
    .key = voice_key,
    .tick = voice_tick,
    .name = "voice",
};
