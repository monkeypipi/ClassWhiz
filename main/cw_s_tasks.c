// cw_s_tasks.c — 作业 / 考试提醒。
#include "cw_app.h"
#include "cw_ui.h"

#include <stdio.h>
#include <string.h>

// 有标题的屏幕只有 244px 内容预算(见 cw_ui.h)。卡片是两行(标题 + 到期时间),
// 不带标签 52px、带 EXAM 标签 58px(标签本身 23px 高),分组标题 17px,间距 4px。
// 三组各一张卡的最坏情况 = 3×17 + 58 + 52 + 52 + 5×4 = 234px,刚好放下;
// 但再多一条任务就要追加 "+N"(17px),会顶到 255px 被裁掉 —— 所以还有剩余任务时
// 只放两张卡,把位置让给 "+N"。没有触摸,滚动条滚不动,溢出等于信息丢失。
#define TASKS_MAX_SHOWN 3

static lv_obj_t *s_scr;
static lv_obj_t *s_cards[TASKS_MAX_SHOWN];
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

static void tasks_apply_selection(void) {
    for (int i = 0; i < s_shown; i++) {
        lv_obj_set_style_bg_color(s_cards[i],
                                  lv_color_hex(i == s_sel ? 0x243557 : CW_C_CARD), 0);
    }
}

static void tasks_build(void) {
    lv_obj_t *body = cw_screen_content(s_scr);
    lv_obj_clean(body);
    memset(s_cards, 0, sizeof(s_cards));
    lv_obj_set_style_pad_gap(body, 4, 0);
    s_shown = 0;

    bool *done = cw_app_task_done();
    int total = 0;
    int group_n[CW_TASK_THIS_WEEK + 1] = {0};
    for (int i = 0; i < cw_task_count(); i++) {
        if (done[i]) continue;
        total++;
        group_n[cw_task_group(cw_task(i), 1)]++;
    }
    if (total == 0) {
        cw_label(body, cw_str(CW_S_ALL_DONE));
        cw_hint_bar(s_scr, NULL, "Hold OK: Back");
        return;
    }

    // 按分组排序:逾期 / 今日 / 本周
    int cap = total > TASKS_MAX_SHOWN ? TASKS_MAX_SHOWN - 1 : TASKS_MAX_SHOWN;
    for (int g = 0; g <= CW_TASK_THIS_WEEK && s_shown < cap; g++) {
        bool head = false;
        for (int i = 0; i < cw_task_count() && s_shown < cap; i++) {
            if (done[i]) continue;
            if (cw_task_group(cw_task(i), 1) != g) continue;
            uint32_t gc = group_color((cw_task_group_t)g);
            if (!head) {
                char gh[40];
                snprintf(gh, sizeof(gh), "%s · %d", group_name((cw_task_group_t)g),
                         group_n[g]);
                lv_obj_t *hl = cw_label(body, gh);
                lv_obj_set_style_text_color(hl, lv_color_hex(gc), 0);
                head = true;
            }
            const cw_task_t *t = cw_task(i);
            lv_obj_t *card = cw_card(body);
            lv_obj_set_style_pad_all(card, 5, 0);
            lv_obj_set_style_pad_gap(card, 2, 0);
            // 设计稿的签名元素:左侧 3px 分组色条
            lv_obj_set_style_border_side(card, LV_BORDER_SIDE_LEFT, 0);
            lv_obj_set_style_border_width(card, 3, 0);
            lv_obj_set_style_border_color(card, lv_color_hex(gc), 0);

            // 第一行只有标题(+考试标签)。截止时间留在第二行,否则它和标签会吃掉
            // 158px,204px 的卡内宽度只剩 46px 给标题,"MATH 100 Problem Set 4"
            // 会被截成 "MATH…"。
            lv_obj_t *row = cw_row(card);
            lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                                  LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_gap(row, 6, 0);

            lv_obj_t *tl = cw_label(row, t->title);
            lv_obj_set_flex_grow(tl, 1);
            lv_obj_set_width(tl, 0);
            lv_label_set_long_mode(tl, LV_LABEL_LONG_DOT);
            if (t->kind == CW_TASK_EXAM) cw_tag(row, cw_str(CW_S_EXAM), CW_C_RED);

            lv_obj_t *meta = cw_row(card);
            lv_obj_set_flex_align(meta, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                                  LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_gap(meta, 6, 0);

            lv_obj_t *box = lv_obj_create(meta);
            lv_obj_set_size(box, 11, 11);
            lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(box, 1, 0);
            lv_obj_set_style_border_color(box, lv_color_hex(gc), 0);
            lv_obj_set_style_radius(box, 3, 0);
            lv_obj_set_style_pad_all(box, 0, 0);

            char db[8], due[40];
            cw_format_time(t->due_min, true, db, sizeof(db));
            if (t->place) {
                snprintf(due, sizeof(due), "10/%d %s · %s", 12 + t->due_day_offset, db,
                         t->place);
            } else {
                snprintf(due, sizeof(due), "10/%d %s", 12 + t->due_day_offset, db);
            }
            lv_obj_t *dl = cw_label_small(meta, due);
            lv_obj_set_flex_grow(dl, 1);
            lv_obj_set_width(dl, 0);
            lv_label_set_long_mode(dl, LV_LABEL_LONG_DOT);
            if (g == CW_TASK_OVERDUE) cw_anim_pulse(box);

            s_order[s_shown] = i;
            s_cards[s_shown] = card;
            s_shown++;
        }
    }
    if (total > s_shown) {
        char more[32];
        snprintf(more, sizeof(more), "+%d", total - s_shown);
        cw_label_small(body, more);
    }
    if (s_sel >= s_shown) s_sel = 0;
    tasks_apply_selection();
    cw_hint_bar(s_scr, "OK: Toggle done", "Hold OK: Back");
}

static void tasks_enter(void *ctx) {
    (void)ctx;
    s_scr = cw_screen_create(cw_str(CW_S_TASKS));
    s_sel = 0;
    tasks_build();
}

static void tasks_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (ev != BSP_BTN_CLICK) return;
    if (s_shown == 0) return;
    if (btn == BSP_BTN_UP) {
        s_sel = (s_sel + s_shown - 1) % s_shown;
        tasks_apply_selection();
    } else if (btn == BSP_BTN_DOWN) {
        s_sel = (s_sel + 1) % s_shown;
        tasks_apply_selection();
    } else if (btn == BSP_BTN_OK) {
        cw_app_task_done()[s_order[s_sel]] = true;
        cw_app_mark_dirty();
        if (s_sel >= s_shown - 1) s_sel = 0;
        tasks_build();
    }
}

const cw_screen_def_t cw_scr_tasks_def = {
    .enter = tasks_enter,
    .exit = NULL,
    .key = tasks_key,
    .tick = NULL,
    .name = "tasks",
};
