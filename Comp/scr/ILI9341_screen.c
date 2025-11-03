

#include <stdio.h>
#include <unistd.h>
#include <sys/lock.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"




#include "esp_lcd_ili9341.h"
#include "esp_lcd_touch_xpt2046.h"
#include "lvgl.h"
#include "lv_conf_template.h"

#include "ILI9341_screen.h"
#include "Screen_design.h"

static void lv_port_task(void *arg);

static esp_lcd_panel_handle_t g_panel_handle = NULL;
static esp_lcd_touch_handle_t g_tp_handle = NULL;
#define LVGL_DRAW_LINES 10
#define LVGL_BUF_SIZE (320 * 240 / 100) // Define the size using the new width (320)
#define MY_DISP_HOR_RES 240
#define MY_DISP_VER_RES 320

static lv_disp_draw_buf_t g_draw_buf;
static lv_color_t g_buf1[MY_DISP_HOR_RES * 10];
static lv_color_t g_buf2[MY_DISP_HOR_RES * 10];

static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    const int offsetx1 = area->x1;
    const int offsetx2 = area->x2;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;
    // Copy the LVGL buffer to the display using the panel's write function
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
    lv_disp_flush_ready(drv); // Tell LVGL the flushing is done
}



static void example_lvgl_touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    // The touch controller handle is passed via user_data
    esp_lcd_touch_handle_t tp = (esp_lcd_touch_handle_t)drv->user_data;
    
    // Local variables to hold data
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t strength = 0;
    uint8_t point_num = 0;
    
    // Call the function with ALL six required arguments
    esp_lcd_touch_get_coordinates(tp, &x, &y, &strength, &point_num, 1);

    if (point_num > 0) {
        // Touch is pressed
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        // Touch is released
        data->state = LV_INDEV_STATE_RELEASED;
    }
}





void init_ili9341_screen(void)
{
    printf("Initializing ILI9341 screen...\n");

    spi_bus_config_t buscfg = {
            .sclk_io_num = EXAMPLE_PIN_NUM_SCLK,
            .mosi_io_num = EXAMPLE_PIN_NUM_MOSI,
            .miso_io_num = EXAMPLE_PIN_NUM_MISO,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = EXAMPLE_LCD_H_RES * 80 * sizeof(uint16_t),
        };
        ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
    
        esp_lcd_panel_io_handle_t io_handle = NULL;
        esp_lcd_panel_io_spi_config_t io_config = {
            .dc_gpio_num = EXAMPLE_PIN_NUM_LCD_DC,
            .cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS,
            .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
            .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
            .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
            .spi_mode = 0,
            .trans_queue_depth = 10,
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

        ESP_LOGI("DisplayManager_Request", "Creating LCD panel driver");

        
        esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
            .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
            .bits_per_pixel = 16,
        };
        // "Install ILI9341 panel driver");
        ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &g_panel_handle));    

        // Initialize panel
        ESP_ERROR_CHECK(esp_lcd_panel_reset(g_panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_init(g_panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(g_panel_handle, true, false));
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(g_panel_handle, true));

            // ---------- TOUCH SETUP ----------
        ESP_LOGI("DisplayManager_Request", "Initializing touch controller XPT2046");

        // Create SPI IO for touch (shares the same SPI bus)
        esp_lcd_panel_io_handle_t tp_io_handle = NULL;
        esp_lcd_panel_io_spi_config_t tp_io_config =
            ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(EXAMPLE_PIN_NUM_TOUCH_CS);

        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &tp_io_config, &tp_io_handle));

         esp_lcd_touch_config_t tp_cfg = {
            .x_max = 4096,//EXAMPLE_LCD_H_RES,
            .y_max = 4095,//EXAMPLE_LCD_V_RES,
            .rst_gpio_num = -1,
            .int_gpio_num = -1,
            .flags = {
                .swap_xy = 1,
                .mirror_x = 0,
                .mirror_y = CONFIG_EXAMPLE_LCD_MIRROR_Y,
            },
        };

        
        ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &g_tp_handle));

        // --- LVGL INPUT DEVICE REGISTRATION ---
        static lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER; 
        indev_drv.read_cb = example_lvgl_touch_read_cb; // Your custom read function
        indev_drv.user_data = g_tp_handle; // **CRITICAL: Pass the handle here!**
        lv_indev_drv_register(&indev_drv);

}




// Define a variable to track the button's color state
static bool button_is_red = false;

// Event handler function for the button click
static void button_event_handler(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e); // Get the button object that triggered the event

    // Change button color
    if (button_is_red) {
        lv_obj_set_style_bg_color(btn, lv_color_make(0x00, 0x3a, 0x6e), LV_STATE_DEFAULT); // Dark Blue
    } else {
        lv_obj_set_style_bg_color(btn, lv_color_make(0xFF, 0x00, 0x00), LV_STATE_DEFAULT); // Red
    }
    button_is_red = !button_is_red; // Toggle the state

    ESP_LOGI("LVGL", "Button Clicked! New color state: %s", button_is_red ? "Red" : "Blue");
}

void create_button_screen(void)
{
    // 1. Create a base screen
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_make(0x33, 0x33, 0x33), LV_STATE_DEFAULT); // Dark Grey Background
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE); // Make the screen non-scrollable

    // 2. Create the Button
    lv_obj_t *btn = lv_btn_create(scr);                     // Create a button as a child of the screen
    lv_obj_set_size(btn, 150, 70);                           // Set button size
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -40);              // Align button to center-top

    // Set initial button color (Dark Blue)
    lv_obj_set_style_bg_color(btn, lv_color_make(0x00, 0x3a, 0x6e), LV_STATE_DEFAULT);

    // 3. Create a label on the button
    lv_obj_t *label = lv_label_create(btn);                 // Create a label as a child of the button
    lv_label_set_text(label, "Change Color");               // Set label text
    lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT); // White text
    lv_obj_center(label);                                   // Center label on the button

    // 4. Attach the event handler to the button
    lv_obj_add_event_cb(btn, button_event_handler, LV_EVENT_CLICKED, NULL); // NULL for user_data

    // 5. Create a status label below the button
    lv_obj_t *status_label = lv_label_create(scr);
    lv_label_set_text(status_label, "Press the button!");
    lv_obj_set_style_text_color(status_label, lv_color_make(0xFF, 0xFF, 0x00), LV_STATE_DEFAULT); // Yellow text
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 40); // Align below button

    // 6. Load this screen
    lv_disp_load_scr(scr);
}

void create_ui(void)
{
    
    // 1. Create a Screen
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_make(0x00, 0x3a, 0x6e), LV_STATE_DEFAULT); 

    // 2. Create a Label
    lv_obj_t *label = lv_label_create(scr); 
    lv_label_set_text(label, "LVGL v8.x is Working!"); 
    
    // Position and Style
    lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0); 
    
    // 3. Load the Screen
    lv_disp_load_scr(scr);
}



void  Test_main_screen(void)
{
    printf("Starting main screen test...\n");

    init_ili9341_screen();
    
     gpio_config_t bk_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT
            };
        ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

         // Optionally, turn the backlight off during setup
        gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, 0);

    // Initialize LVGL and display
    lv_init();

    lv_disp_draw_buf_init(&g_draw_buf, g_buf1, g_buf2, MY_DISP_HOR_RES*10) ;  /*Initialize the display buffer.*/

   static lv_disp_drv_t disp_drv;        /*Descriptor of a display driver*/
   lv_disp_drv_init(&disp_drv);          /*Basic initialization*/
   disp_drv.flush_cb = example_lvgl_flush_cb; //my_disp_flush;    /*Set your driver function*/
   disp_drv.draw_buf = &g_draw_buf;        /*Assign the buffer to the display*/
   disp_drv.hor_res = 240;   /*Set the horizontal resolution of the display*/
   disp_drv.ver_res = 320;   /*Set the vertical resolution of the display*/// TO 320x240 (Landscape):




      // *** FIX: Use the global static handler to link the display driver ***
    disp_drv.user_data = g_panel_handle;

   lv_disp_drv_register(&disp_drv);      /*Finally register the driver*/



// Optional: Set the rotation if configured in panel_config
// lv_disp_set_rotation(disp, LV_DISP_ROT_90);

   static lv_indev_drv_t indev_drv;           /*Descriptor of a input device driver*/
    lv_indev_drv_init(&indev_drv);             /*Basic initialization*/
    indev_drv.type = LV_INDEV_TYPE_POINTER;    /*Touch pad is a pointer-like device*/
    indev_drv.read_cb = example_lvgl_touch_read_cb; //my_touchpad_read;      /*Set your driver function*/

// *** FIX: Use the global static handler to link the touch driver ***
    indev_drv.user_data = g_tp_handle;

    lv_indev_drv_register(&indev_drv);         /*Finally register the driver*/

    //create_ui();

    create_button_screen();

     // Turn on the backlight after setup
    // 4. Start the LVGL Task
    //    This is what starts the engine that draws the UI you just created.
    xTaskCreatePinnedToCore(lv_port_task, "LVGL", 8192 * 2, NULL, 5, NULL, 1);

    printf("Main screen test completed.\n");
}

// Mutex variable
static SemaphoreHandle_t lvgl_mux = NULL;

// Helper function to acquire the lock
static bool lvgl_lock(uint32_t timeout_ms)
{
    // Try to acquire the mutex, waiting for the specified timeout
    const TickType_t timeout_ticks = (timeout_ms == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

// Helper function to release the lock
static void lvgl_unlock(void)
{
    xSemaphoreGiveRecursive(lvgl_mux);
}

// Define this task outside your init function
static void lv_port_task(void *arg)
{
    ESP_LOGI("LVGL", "Starting LVGL task");
    while (1) {
        // The task needs to take the lock before accessing LVGL APIs
        // Assuming a recursive mutex is used as per common examples:
        // if (pdTRUE == lvgl_lock(portMAX_DELAY)) {
        //     lv_timer_handler();
        //     lvgl_unlock();
        // }
        // For a single-task setup, a simple handler is enough:
        //lvgl_lock(portMAX_DELAY);
        lv_timer_handler();
        //lvgl_unlock();

        // Delay to prevent the task from hogging the CPU
        vTaskDelay(pdMS_TO_TICKS(5)); 
    }
}
