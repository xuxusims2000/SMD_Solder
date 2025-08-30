



#include "esp_lcd_ili9341.h"
#include "esp_lcd_touch_xpt2046.h"
#include "lvgl.h"


static const char *TAG = "example";

// Using SPI2 in the example
#define LCD_HOST  SPI2_HOST

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (20 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_SCLK           14
#define EXAMPLE_PIN_NUM_MOSI           13
#define EXAMPLE_PIN_NUM_MISO           12
#define EXAMPLE_PIN_NUM_LCD_DC         2
#define EXAMPLE_PIN_NUM_LCD_RST        4
#define EXAMPLE_PIN_NUM_LCD_CS         15
#define EXAMPLE_PIN_NUM_BK_LIGHT       2
#define EXAMPLE_PIN_NUM_TOUCH_CS       21

#define EXAMPLE_LCD_H_RES              240
#define EXAMPLE_LCD_V_RES              320

// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

#define EXAMPLE_LVGL_DRAW_BUF_LINES    20 // number of display lines in each draw buffer
#define EXAMPLE_LVGL_TICK_PERIOD_MS    2
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS 500
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS 1000 / CONFIG_FREERTOS_HZ
#define EXAMPLE_LVGL_TASK_STACK_SIZE   (4 * 1024)
#define EXAMPLE_LVGL_TASK_PRIORITY     2
#define CONFIG_EXAMPLE_LCD_MIRROR_Y 1

void example_lvgl_demo_ui(lv_disp_t *disp);
void lvgl_main_screen(lv_disp_t *disp);

bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);
void example_lvgl_port_update_callback(lv_display_t *disp);
void example_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
void example_lvgl_touch_cb(lv_indev_t *indev, lv_indev_data_t *data);
void example_increase_lvgl_tick(void *arg);
void example_lvgl_port_task(void *arg);

void Test_main_screen(void);











