

#include "lvgl.h"

 lv_obj_t * btn;
 lv_display_rotation_t rotation = LV_DISP_ROTATION_0;

 void btn_cb(lv_event_t * e)
{
    lv_display_t *disp = lv_event_get_user_data(e);
    rotation++;
    if (rotation > LV_DISP_ROTATION_270) {
        rotation = LV_DISP_ROTATION_0;
    }
    lv_disp_set_rotation(disp, rotation);
}
void set_angle(void * obj, int32_t v)
{
    lv_arc_set_value(obj, v);
}

void example_lvgl_demo_ui(lv_display_t *disp)
{
    lv_obj_t *scr = lv_display_get_screen_active(disp);

    btn = lv_button_create(scr);
    lv_obj_t * lbl = lv_label_create(btn);
    lv_label_set_text_static(lbl, LV_SYMBOL_REFRESH" ROTATE");
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 30, -30);
    /*Button event*/
    lv_obj_add_event_cb(btn, btn_cb, LV_EVENT_CLICKED, disp);

    /*Create an Arc*/
    lv_obj_t * arc = lv_arc_create(scr);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);   /*Be sure the knob is not displayed*/
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);  /*To not allow adjusting by click*/
    lv_obj_center(arc);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, arc);
    lv_anim_set_exec_cb(&a, set_angle);
    lv_anim_set_duration(&a, 1000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);    /*Just for the demo*/
    lv_anim_set_repeat_delay(&a, 500);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_start(&a);
}

//-----------------------------------------------------------------------------------------------------------


 void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target_obj(e);
    if(code == LV_EVENT_CLICKED) {
        static uint8_t cnt = 0;
        cnt++;

        /*Get the first child of the button which is the label and change its text*/
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Button: %d", cnt);
    }
}

 void lvgl_main_screen_1(lv_disp_t *disp)
{
 /**
 * Create a button with a label and react on click event.
 */
    lv_obj_t * btn = lv_button_create(lv_screen_active());     /*Add a button the current screen*/
    lv_obj_set_pos(btn, 50 , 150 );              /*Set its position*/
    lv_obj_set_size(btn, 120, 50);                          /*Set its size*/
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);           /*Assign a callback to the button*/

    lv_obj_t * label = lv_label_create(btn);          /*Add a label to the button*/
    lv_label_set_text(label, "Tocame");                     /*Set the labels text*/
    lv_obj_center(label);

}

//-----------------------------------------------------------------------------------------------------------

// Declare screen and button objects globally
lv_obj_t *screen1;
lv_obj_t *screen2;

void show_screen1(lv_event_t *e) {
    lv_scr_load(screen1);
}

void show_screen2(lv_event_t *e) {
    lv_scr_load(screen2);
}

void create_screen1(void) {
    screen1 = lv_obj_create(NULL); // Create a blank screen
    lv_obj_set_style_bg_color(screen1, lv_color_black(), LV_PART_MAIN);

    lv_obj_t *btn = lv_btn_create(screen1);
    lv_obj_center(btn);  // Center the button
    lv_obj_add_event_cb(btn, show_screen2, LV_EVENT_CLICKED, NULL); // Attach callback

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Go to Screen 2");
    lv_obj_center(label);
}

void create_screen2(void) {
    screen2 = lv_obj_create(NULL);

    lv_obj_t *btn = lv_btn_create(screen2);
    lv_obj_center(btn);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF0000), LV_PART_MAIN); // Red button
    lv_obj_add_event_cb(btn, show_screen1, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Back to Screen 1");
    lv_obj_center(label);
}

void lvgl_main_screen(void) {
    create_screen1();
    create_screen2();
    lv_scr_load(screen1);  // Load the first screen initially
}