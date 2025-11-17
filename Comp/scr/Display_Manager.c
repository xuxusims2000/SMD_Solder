#include "Display_Manager.h"

/*============================== Defines ==============================*/

#define DISPLAY_MANAGER_SIGNAL_REQUESTED   (1 << 0)
#define DISPLAY_MANAGER_SIGNAL_RELEASE     (1 << 1)
#define DISPLAY_MANAGER_SIGNAL_START       (1 << 2)
#define DISPLAY_MANAGER_SIGNAL_STOP        (1 << 3)

/*============================== Static Prototypes ==============================*/

static esp_err_t DisplayManager_Requesting(void);
static esp_err_t DisplayManager_Releasing(void);
static uint32_t DisplayManagerSignalWait(uint32_t signal, uint32_t timeout);

/*============================== Private Global Variables ==============================*/

static _lock_t lvgl_api_lock2;

typedef struct {
    DisplayManagerState              state;
    DisplayManager_Configuration_t   config;
    uint32_t                         signals;

    TaskHandle_t                     taskHandle;
    SemaphoreHandle_t                xSemaphore;

    lv_display_t*                    lvgl_display;
    TaskHandle_t                     lvgl_task_handle;
    bool                             lvgl_initialized;

    esp_lcd_panel_io_handle_t        panel_io_handle;
    esp_lcd_panel_handle_t           panel_handle;
    esp_lcd_panel_io_handle_t        touch_io_handle;
    esp_lcd_touch_handle_t           touch_handle;
    esp_timer_handle_t               lvgl_tick_timer;

    void*                            lvgl_buf1;
    void*                            lvgl_buf2;

    lv_obj_t*                        current_screen;

    float                           temperature;
} DisplayManager_t;

static DisplayManager_t display_manager = {
    .state = DISPLAY_MANAGER_UNDEFINED,
    .signals = 0,
    .taskHandle = NULL,
    .xSemaphore = NULL,
};

/*============================== Public API ==============================*/

void DisplayManager_Init(void)
{
    if ((display_manager.state == DISPLAY_MANAGER_UNDEFINED) && (display_manager.taskHandle == NULL)) {
        ESP_LOGI("Display_Manager", "Initializing...");

        // Configure backlight GPIO
        gpio_config_t bk_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT
        };
        ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
        gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, 0);

        // Create semaphore
        display_manager.xSemaphore = xSemaphoreCreateBinary();
        if (display_manager.xSemaphore == NULL) {
            ESP_LOGE("Display_Manager_Init", "Failed to create semaphore");
            return;
        }
        xSemaphoreGive(display_manager.xSemaphore);

        display_manager.state = DISPLAY_MANAGER_POWER_OFF;

        // Create display manager main task
        xTaskCreate(Display_Manager_Task,
                    "Display_Manager_Task",
                    4096 * 4,
                    NULL,
                    5,
                    &display_manager.taskHandle);
    }
}

esp_err_t DisplayManager_Request(DisplayManager_Configuration_t* config)
{
    if (xSemaphoreTake(display_manager.xSemaphore, 0) != pdTRUE) {
        ESP_LOGW("DisplayManager_Request", "Semaphore not available");
        return ESP_FAIL;
    }

    ESP_LOGI("DisplayManager_Request", "Semaphore taken, requesting display");

    memcpy(&display_manager.config, config, sizeof(DisplayManager_Configuration_t));
    display_manager.state = DISPLAY_MANAGER_REQUESTING;

    xTaskNotify(display_manager.taskHandle, DISPLAY_MANAGER_SIGNAL_REQUESTED, eSetBits);
    return ESP_OK;
}

esp_err_t DisplayManager_Start(void)
{
 
    
    if (display_manager.state != DISPLAY_MANAGER_REQUESTED) {
        ESP_LOGE("DisplayManager_Start", "Invalid state (%d)", display_manager.state);
        return ESP_FAIL;
    }

    xTaskNotify(display_manager.taskHandle, DISPLAY_MANAGER_SIGNAL_START, eSetBits);
    return ESP_OK;
}

void DisplayManager_Stop(void)
{
    if (display_manager.state == DISPLAY_MANAGER_MAIN_SCREEN) {
        xTaskNotify(display_manager.taskHandle, DISPLAY_MANAGER_SIGNAL_STOP, eSetBits);
        ESP_LOGI("DisplayManager_Stop", "Stopping display manager");
    } else {
        ESP_LOGE("DisplayManager_Stop", "Invalid state (%d)", display_manager.state);
    }
}

esp_err_t DisplayManager_Release(void)
{
    xTaskNotify(display_manager.taskHandle, DISPLAY_MANAGER_SIGNAL_RELEASE, eSetBits);
    xSemaphoreGive(display_manager.xSemaphore);
    return ESP_OK;
}

esp_err_t DisplayManager_SetTemperature(float temperature)
{
    esp_err_t result = ESP_FAIL;

    display_manager.temperature = temperature;

    static char temp_buffer[16];
    snprintf(temp_buffer, sizeof(temp_buffer), "%.2f °C", temperature);
    lv_label_set_text(ui_varTemp, temp_buffer);

    result = ESP_OK;

    return result;
}

/*============================== Main Task ==============================*/

void Display_Manager_Task(void *pvParameters)
{
    esp_err_t result;
    uint32_t signal;
    void (*OperationCompleteCallback)(DisplayManager_Result_t result);

    for (;;) {
        switch (display_manager.state) {

        case DISPLAY_MANAGER_POWER_OFF:
            ESP_LOGI("Display_Manager_Task", "STATE: POWER_OFF");
            DisplayManagerSignalWait(DISPLAY_MANAGER_SIGNAL_REQUESTED, portMAX_DELAY);
            break;

        case DISPLAY_MANAGER_REQUESTING:
            result = DisplayManager_Requesting();
            if (result == ESP_OK) {
                display_manager.state = DISPLAY_MANAGER_REQUESTED;
                ESP_LOGI("Display_Manager_Task", "STATE: REQUESTED");
                if (display_manager.config.callbacks.OperationCompleteCallback)
                    display_manager.config.callbacks.OperationCompleteCallback(DISPLAY_MANAGER_RESULT_REQUEST);
            } else {
                ESP_LOGE("Display_Manager_Task", "REQUEST ERROR -> POWER_OFF");
                display_manager.state = DISPLAY_MANAGER_POWER_OFF;
            }
            break;

        case DISPLAY_MANAGER_REQUESTED:
            signal = DisplayManagerSignalWait(DISPLAY_MANAGER_SIGNAL_START | DISPLAY_MANAGER_SIGNAL_RELEASE, portMAX_DELAY);
            if (signal & DISPLAY_MANAGER_SIGNAL_START) {
                display_manager.state = DISPLAY_MANAGER_MAIN_SCREEN;
                if (display_manager.config.callbacks.OperationCompleteCallback)
                    display_manager.config.callbacks.OperationCompleteCallback(DISPLAY_MANAGER_RESULT_START);
            } else if (signal & DISPLAY_MANAGER_SIGNAL_RELEASE) {
                display_manager.state = DISPLAY_MANAGER_RELEASING;
            }

            ESP_LOGI("Display_Manager_Task", "STATE: START");
            //_lock_acquire(&lvgl_api_lock2);
            ESP_LOGI("Display_Manager_Task", "Calling ui_init()...");
            lv_async_call((lv_async_cb_t)ui_init, NULL);
            ESP_LOGI("Display_Manager_Task", "ui_init() returned");
            //_lock_release(&lvgl_api_lock2);
            vTaskDelay(pdMS_TO_TICKS(500)); // feeds watchdog  //shold do a semafor

            lv_label_set_text(ui_varTemp, "15 °C");


            break;

        case DISPLAY_MANAGER_MAIN_SCREEN:

            //display_manager.current_screen = lv_scr_act();
        lv_obj_t * current_screen = lv_scr_act(); // Get the active screen just once

        if (current_screen == ui_Screen1) {
            ESP_LOGI("Display_Manager_Task", "STATE: Screen 1 (Home)");
        } else if (current_screen == ui_Screen2) {
            // Now you can specifically handle when Screen 2 is active
            ESP_LOGI("Display_Manager_Task", "STATE: Screen 2 (Settings)");
        } else {
            // This handles any other screen that might exist, or if the screen is NULL
            ESP_LOGW("Display_Manager_Task", "STATE: Unknown Screen active: %p", (void*)current_screen);
        }
             
            /* La idea es fer que en fucnio de la pantalla que estigui 
            actualitzi els valors que siguin necesaris
                La pregunta es faig un altre estat? com gestion aquests estats
                - podria fer que sactualitzes tot tot el rato ?*/

            signal = DisplayManagerSignalWait(DISPLAY_MANAGER_SIGNAL_STOP, portMAX_DELAY);
            if (signal & DISPLAY_MANAGER_SIGNAL_STOP) {
                display_manager.state = DISPLAY_MANAGER_REQUESTED;
                if (display_manager.config.callbacks.OperationCompleteCallback)
                    display_manager.config.callbacks.OperationCompleteCallback(DISPLAY_MANAGER_RESULT_STOP);
            }
            break;

        case DISPLAY_MANAGER_RELEASING:
            OperationCompleteCallback = display_manager.config.callbacks.OperationCompleteCallback;
            result = DisplayManager_Releasing();
            if (result == ESP_OK) {
                display_manager.state = DISPLAY_MANAGER_POWER_OFF;
                if (OperationCompleteCallback)
                    OperationCompleteCallback(DISPLAY_MANAGER_RESULT_RELEASE);
            } else {
                display_manager.state = DISPLAY_MANAGER_POWER_OFF;
            }
            break;

        default:
            display_manager.state = DISPLAY_MANAGER_POWER_OFF;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/*============================== Private Functions ==============================*/

static esp_err_t DisplayManager_Requesting(void)
{
    ESP_LOGI("DisplayManager_Requesting", "Initializing LCD & LVGL");

    // SPI init
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
    display_manager.panel_io_handle = io_handle;

    // Panel driver
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
    display_manager.panel_handle = panel_handle;

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);

    if (!display_manager.lvgl_initialized) {
        lv_init();
        display_manager.lvgl_initialized = true;
    }

    size_t buf_size = EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_DRAW_BUF_LINES * sizeof(lv_color16_t);
    display_manager.lvgl_buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    display_manager.lvgl_buf2 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    assert(display_manager.lvgl_buf1 && display_manager.lvgl_buf2);

    lv_display_t *disp = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    display_manager.lvgl_display = disp;
    lv_display_set_buffers(disp, display_manager.lvgl_buf1, display_manager.lvgl_buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_user_data(disp, panel_handle);
    lv_display_set_flush_cb(disp, example_lvgl_flush_cb);

    // LVGL tick
    const esp_timer_create_args_t tick_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &display_manager.lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(display_manager.lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    // Flush done callback
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = example_notify_lvgl_flush_ready,
    };
    ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, disp));

    // Touch controller
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_spi_config_t tp_io_config = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(EXAMPLE_PIN_NUM_TOUCH_CS);
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &tp_io_config, &tp_io_handle));
    display_manager.touch_io_handle = tp_io_handle;

    esp_lcd_touch_handle_t tp = NULL;
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = EXAMPLE_LCD_H_RES,
        .y_max = EXAMPLE_LCD_V_RES,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .flags = {.swap_xy = 0, .mirror_x = 0, .mirror_y = CONFIG_EXAMPLE_LCD_MIRROR_Y},
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &tp));
    display_manager.touch_handle = tp;

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(indev, disp);
    lv_indev_set_user_data(indev, tp);
    lv_indev_set_read_cb(indev, example_lvgl_touch_cb);

    // Start LVGL handling task
    if (display_manager.lvgl_task_handle == NULL) {
        xTaskCreate(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, &display_manager.lvgl_task_handle);
    } 

    return ESP_OK;
}

static esp_err_t DisplayManager_Releasing(void)
{
    ESP_LOGI("DisplayManager_Releasing", "Releasing display resources...");

    if (display_manager.lvgl_tick_timer) {
        esp_timer_stop(display_manager.lvgl_tick_timer);
        esp_timer_delete(display_manager.lvgl_tick_timer);
        display_manager.lvgl_tick_timer = NULL;
    }

    if (display_manager.lvgl_task_handle) {
        vTaskDelete(display_manager.lvgl_task_handle);
        display_manager.lvgl_task_handle = NULL;
    }

    if (display_manager.panel_handle) {
        esp_lcd_panel_del(display_manager.panel_handle);
        display_manager.panel_handle = NULL;
    }

    if (display_manager.panel_io_handle) {
        esp_lcd_panel_io_del(display_manager.panel_io_handle);
        display_manager.panel_io_handle = NULL;
    }

    spi_bus_free(LCD_HOST);
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, 0);

    free(display_manager.lvgl_buf1);
    free(display_manager.lvgl_buf2);

    ESP_LOGI("DisplayManager_Releasing", "Display fully released");
    return ESP_OK;
}

static uint32_t DisplayManagerSignalWait(uint32_t signal, uint32_t timeout_ms)
{
 uint32_t notifiedValue = 0;
    TickType_t ticks;

    if (timeout_ms == portMAX_DELAY) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS(timeout_ms);
        if (ticks == 0) ticks = 1; // avoid zero wait if ms < tick period
    }

    // Wait for notification bits (clear on exit specified by 'signal')
    xTaskNotifyWait(0x00, signal, &notifiedValue, ticks);

    return notifiedValue;
}