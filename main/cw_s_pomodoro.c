// cw_s_pomodoro.c — 番茄钟。
#include "cw_app.h"
#include "cw_ui.h"

#include <stdio.h>

static lv_obj_t *s_scr;
static lv_obj_t *s_arc;
static lv_obj_t *s_time_label;
static lv_obj_t *s_phase_label;
static lv_obj_t *s_dots_label;
static lv_obj_t *s_btn;
static int s_preset; // 0=25/5, 1=50/10

// 按钮文字随状态切换,三处调用共用一个助手,避免中英文案走散
static void pomo_set_btn(bool running) {
    lv_obj_t *l = lv_obj_get_child(s_btn, 0);
    lv_label_set_text(l, cw_str(running ? CW_S_PAUSE : CW_S_START));
}

static void pomo_refresh(uint64_t now_ms) {
    cw_pomo_t *p = cw_app_pomo();
    uint32_t rem = cw_pomo_remaining(p, now_ms);
    uint32_t total = cw_pomo_phase_total(p);
    char b[16];
    snprintf(b, sizeof(b), "%02d:%02d", (int)(rem / 60), (int)(rem % 60));
    lv_label_set_text(s_time_label, b);
    int pct = total ? (int)((uint64_t)rem * 100 / total) : 0;
    lv_arc_set_value(s_arc, pct);
    const char *phase = "";
    switch (p->state) {
        case CW_POMO_FOCUS_RUNNING:
            phase = "FOCUS";
            break;
        case CW_POMO_FOCUS_PAUSED:
            phase = "PAUSED";
            break;
        case CW_POMO_BREAK_RUNNING:
            phase = "BREAK";
            break;
        case CW_POMO_BREAK_PAUSED:
            phase = "BREAK PAUSED";
            break;
        default:
            phase = "READY";
            break;
    }
    lv_label_set_text(s_phase_label, phase);
    char dots[16];
    int n = 0;
    for (uint32_t i = 0; i < 4 && i < p->completed; i++) dots[n++] = '*';
    dots[n] = '\0';
    lv_label_set_text(s_dots_label, dots);
}

static void pomo_enter(void *ctx) {
    (void)ctx;
    s_scr = cw_screen_create(cw_str(CW_S_POMODORO));
    lv_obj_t *body = cw_screen_content(s_scr);
    cw_pomo_t *p0 = cw_app_pomo();
    s_preset = (p0->focus_min == 50) ? 1 : 0;

    s_arc = lv_arc_create(body);
    lv_obj_set_size(s_arc, 130, 130);
    lv_arc_set_bg_angles(s_arc, 0, 360);
    lv_arc_set_rotation(s_arc, 270);
    lv_obj_set_style_arc_width(s_arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_arc, lv_color_hex(0x1B2540), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_arc, lv_color_hex(CW_C_ACCENT), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(s_arc, true, LV_PART_INDICATOR);
    lv_obj_remove_flag(s_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_arc_set_value(s_arc, 100);

    // 时间与阶段放在环内,否则 244px 的预算塞不下(见 cw_ui.h)
    s_time_label = lv_label_create(s_arc);
    lv_label_set_text(s_time_label, "25:00");
    lv_obj_set_style_text_font(s_time_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_time_label, lv_color_hex(CW_C_TEXT), 0);
    lv_obj_align(s_time_label, LV_ALIGN_CENTER, 0, -8);
    s_phase_label = lv_label_create(s_arc);
    lv_obj_set_style_text_font(s_phase_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_phase_label, lv_color_hex(0x7FB0FF), 0);
    lv_obj_align(s_phase_label, LV_ALIGN_CENTER, 0, 14);

    // 预设
    lv_obj_t *seg = cw_seg(body);
    cw_seg_add(seg, "25/5", s_preset == 0);
    cw_seg_add(seg, "50/10", s_preset == 1);

    lv_obj_t *btn = cw_button(body, cw_str(CW_S_START));
    lv_obj_set_size(btn, 120, 36);
    s_btn = btn;

    s_dots_label = lv_label_create(body);
    lv_obj_set_style_text_font(s_dots_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_dots_label, lv_color_hex(CW_C_ACCENT), 0);

    pomo_refresh(cw_app_mono_ms());
    cw_hint_bar(s_scr, "OK: Start/Pause", "UP/DN: Preset");
}

static void pomo_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (ev != BSP_BTN_CLICK) return;
    cw_pomo_t *p = cw_app_pomo();
    uint64_t ms = cw_app_mono_ms();
    if (btn == BSP_BTN_UP || btn == BSP_BTN_DOWN) {
        if (p->state == CW_POMO_IDLE) {
            s_preset = 1 - s_preset;
            cw_pomo_set_preset(p, s_preset == 0 ? 25 : 50, s_preset == 0 ? 5 : 10);
            cw_app_mark_dirty();
            cw_app_refresh_top(); // 刷新预设显示
        }
    } else if (btn == BSP_BTN_OK) {
        bool changed = false;
        if (p->state == CW_POMO_IDLE) {
            changed = cw_pomo_start(p, ms);
        } else if (p->state == CW_POMO_FOCUS_RUNNING ||
                   p->state == CW_POMO_BREAK_RUNNING) {
            changed = cw_pomo_pause(p, ms);
        } else {
            changed = cw_pomo_resume(p, ms);
        }
        if (changed) {
            bool running = (p->state == CW_POMO_FOCUS_RUNNING ||
                            p->state == CW_POMO_BREAK_RUNNING);
            pomo_set_btn(running);
            if (running) cw_anim_pulse(s_btn);
            cw_app_mark_dirty();
        }
        pomo_refresh(ms);
    }
}

static void pomo_tick(uint64_t now_ms) {
    cw_pomo_t *p = cw_app_pomo();
    cw_pomo_event_t ev = cw_pomo_tick(p, now_ms);
    if (ev != CW_POMO_EV_NONE) {
        cw_app_mark_dirty();
        pomo_set_btn(false);
    }
    pomo_refresh(now_ms);
}

const cw_screen_def_t cw_scr_pomodoro_def = {
    .enter = pomo_enter,
    .exit = NULL,
    .key = pomo_key,
    .tick = pomo_tick,
    .name = "pomodoro",
};
