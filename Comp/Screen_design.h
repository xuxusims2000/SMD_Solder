

#include "lvgl.h"


void btn_cb(lv_event_t * e);
void set_angle(void * obj, int32_t v);
void example_lvgl_demo_ui(lv_display_t *disp);
void btn_event_cb(lv_event_t * e);
void lvgl_main_screen_1(lv_disp_t *disp);


void show_screen1(lv_event_t *e);
void show_screen2(lv_event_t *e);
void create_screen1(void);
void create_screen2(void);


void lvgl_main_screen(lv_display_t *disp);