// cw_s_tasks.c — 作业 / 考试提醒。
#include "cw_app.h"
#include "cw_ui.h"

#include <stdio.h>

static lv_obj_t *s_scr;
static int s_sel;
static int s_order[CW_MAX_TASKS]; // 显示顺序:任务索引
static int s_shown;

static const char *group_name(cw_task_group_t g) {
    switch (g) {
        case CW_TASK_OVERDUE:
            return cw_str(CW_S_OVERDUE);
        case CW_TASK_DUE_TODAY:
            return cw_str(CW_S_DUE_TODAY);
        default:
            return cw_str(CW_S_THIS_WEEK);
    }
}

static uint32_t group_color(cw_task_group_t g) {
    switch (g) {
        case CW_TASK_OVERDUE:
            return CW_C_RED;
        case CW_TASK_DUE_TODAY:
            return CW_C_AMBER;
        default:
            return CW_C_ACCENT;
    }
}

static void tasks_enter(void *ctx) {
    (void)ctx;
    s_scr = cw_screen_create(cw_str(CW_S_TASKS));
    lv_obj_t *body = cw_screen_content(s_scr);

    // 按分组排序:逾期 / 今日 / 本周
    s_shown = 0;
    bool *done = cw_app_task_done();
    for (int g = 0; g <= CW_TASK_THIS_WEEK; g++) {
        bool head = false;
        for (int i = 0; i < cw_task_count(); i++) {
            if (done[i]) continue;
            if (cw_task_group(cw_task(i), 1) != g) continue;
            if (!head) {
                char h[48];
                snprintf(h, sizeof(h), "%s", group_name((cw_task_group_t)g));
                lv_obj_t *hl = cw_label(body, h);
                lv_obj_set_style_text_color(hl, lv_color_hex(group_color((cw_task_group_t)g)),
                                            0);
                head = true;
            }
            const cw_task_t *t = cw_task(i);
            lv_obj_t *card = cw_card(body);
            lv_obj_set_style_border_color(card, lv_color_hex(group_color((cw_task_group_t)g)),
                                          0);
            lv_obj_set_style_border_width(card, s_shown == s_sel ? 2 : 1, 0);
            lv_obj_t *row = lv_obj_create(card);
            lv_obj_set_width(row, LV_PCT(100));
            lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(row, 0, 0);
            lv_obj_set_style_pad_all(row, 0, 0);
            lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                                  LV_FLEX_ALIGN_CENTER);
            char title[80];
            snprintf(title, sizeof(title), "[ ] %s", t->title);
            cw_label(row, title);
            if (t->kind == CW_TASK_EXAM) {
                cw_tag(row, cw_str(CW_S_EXAM), CW_C_RED);
            }
            char meta[80];
            char db[8];
            cw_format_time(t->due_min, true, db, sizeof(db));
            if (t->place) {
                snprintf(meta, sizeof(meta), "10/%d %s · %s", 12 + t->due_day_offset, db,
                         t->place);
            } else {
                snprintf(meta, sizeof(meta), "10/%d %s", 12 + t->due_day_offset, db);
            }
            cw_label_small(card, meta);
            if (g == CW_TASK_OVERDUE) cw_anim_pulse(card);
            s_order[s_shown++] = i;
        }
    }
    if (s_shown == 0) {
        cw_label(body, cw_get_lang() == CW_LANG_ZH ? "全部完成!" : "All done!");
    }
    if (s_sel >= s_shown) s_sel = 0;
    cw_hint_bar(s_scr, "OK: Toggle done", "Hold OK: Back");
    cw_screen_set_scroll(s_scr, true);
}

static void tasks_rebuild_selection(void) {
    // 简化:重建整个屏幕(任务数少)
    lv_obj_t *body = cw_screen_content(s_scr);
    lv_obj_clean(body);
    // 复用 enter 逻辑但保留 s_sel
    int keep = s_sel;
    // 手动重建(避免递归 push)
    s_shown = 0;
    bool *done = cw_app_task_done();
    for (int g = 0; g <= CW_TASK_THIS_WEEK; g++) {
        bool head = false;
        for (int i = 0; i < cw_task_count(); i++) {
            if (done[i]) continue;
            if (cw_task_group(cw_task(i), 1) != g) continue;
            if (!head) {
                lv_obj_t *hl = cw_label(body, group_name((cw_task_group_t)g));
                lv_obj_set_style_text_color(hl, lv_color_hex(group_color((cw_task_group_t)g)),
                                            0);
                head = true;
            }
            const cw_task_t *t = cw_task(i);
            lv_obj_t *card = cw_card(body);
            lv_obj_set_style_border_color(card, lv_color_hex(group_color((cw_task_group_t)g)),
                                          0);
            lv_obj_set_style_border_width(card, s_shown == keep ? 2 : 1, 0);
            lv_obj_t *row = lv_obj_create(card);
            lv_obj_set_width(row, LV_PCT(100));
            lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(row, 0, 0);
            lv_obj_set_style_pad_all(row, 0, 0);
            lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                                  LV_FLEX_ALIGN_CENTER);
            char title[80];
            snprintf(title, sizeof(title), "[ ] %s", t->title);
            cw_label(row, title);
            if (t->kind == CW_TASK_EXAM) {
                cw_tag(row, cw_str(CW_S_EXAM), CW_C_RED);
            }
            char meta[80];
            char db[8];
            cw_format_time(t->due_min, true, db, sizeof(db));
            snprintf(meta, sizeof(meta), "10/%d %s", 12 + t->due_day_offset, db);
            cw_label_small(card, meta);
            if (g == CW_TASK_OVERDUE) cw_anim_pulse(card);
            s_order[s_shown++] = i;
        }
    }
    if (s_shown == 0) {
        cw_label(body, cw_get_lang() == CW_LANG_ZH ? "全部完成!" : "All done!");
    }
}

static void tasks_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (ev != BSP_BTN_CLICK) return;
    if (btn == BSP_BTN_UP) {
        if (s_shown > 0) {
            s_sel = (s_sel + s_shown - 1) % s_shown;
            tasks_rebuild_selection();
        }
    } else if (btn == BSP_BTN_DOWN) {
        if (s_shown > 0) {
            s_sel = (s_sel + 1) % s_shown;
            tasks_rebuild_selection();
        }
    } else if (btn == BSP_BTN_OK) {
        if (s_shown > 0 && s_sel < s_shown) {
            bool *done = cw_app_task_done();
            done[s_order[s_sel]] = true;
            cw_app_mark_dirty();
            if (s_sel >= s_shown - 1) s_sel = 0;
            tasks_rebuild_selection();
        }
    }
}

const cw_screen_def_t cw_scr_tasks_def = {
    .enter = tasks_enter,
    .exit = NULL,
    .key = tasks_key,
    .tick = NULL,
    .name = "tasks",
};
