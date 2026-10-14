# ClassWhiz 应用构建:复用 BSP,自有 UI(不含 demo 测试菜单与 ui_pixel 外壳)。
idf_component_register(
    SRCS "main.c"
         "cw_app.c"
         "cw_data.c"
         "cw_pomodoro.c"
         "cw_store.c"
         "cw_config.c"
         "cw_ui.c"
         "cw_s_home.c"
         "cw_s_schedule.c"
         "cw_s_teachers.c"
         "cw_s_calendar.c"
         "cw_s_student.c"
         "cw_s_tasks.c"
         "cw_s_clock.c"
         "cw_s_pomodoro.c"
         "cw_s_voice.c"
         "cw_s_settings.c"
         # 中文字体子集(由 assets/fonts/symbols.txt 生成到 main/fonts 参与编译,见 assets/README.md)
         "fonts/cw_font_14.c"
         "fonts/cw_font_16.c"
    INCLUDE_DIRS "."
    REQUIRES bsp bt esp_event esp_hw_support esp_netif esp_timer esp_wifi nvs_flash json
)
