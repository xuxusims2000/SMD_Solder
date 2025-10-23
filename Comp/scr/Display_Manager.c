

#include "Display_Manager.h"



#define DISPLAY_MANEGER_SIGNAL_REQUESTED   (1 << 0)  // Signal to request display manager
#define DISPLAY_MANEGER_SIGNAL_RLEASE      (1 << 1)  // Signal to indicate display manager 
#define DISPLAY_MANEGER_SIGNAL_START       (1 << 2)  // Signal to indicate display manager 
#define DISPLAY_MANEGER_SIGNAL_STOP        (1 << 3)  // Signal to release display manager



esp_err_t DisplayManager_Requesting(void);

uint32_t DisplayManagerSignalWait(uint32_t signal, uint32_t timeout);

esp_err_t DisplayManager_Releasing(void);

_lock_t lvgl_api_lock2;

typedef struct {
    DisplayManagerState                state;

    DisplayManager_Configuration_t     config;

    uint32_t                        signals;  // Bitmask for signals

    TaskHandle_t                    taskHandle;
    SemaphoreHandle_t               DisplayManager_xSemaphoreHandle; //Defines a semaphore to manage the resource
    
    lv_display_t*                  lvgl_display;

    TaskHandle_t                    lvgl_task_handle;

} DisplayManager_t;

/* Private variables --------------------------------------------------*/

static DisplayManager_t display_manager = {
    .state = DISPLAY_MANAGER_UNDEFINED,
    .signals = 0,
    .taskHandle= NULL,
    .DisplayManager_xSemaphoreHandle = NULL,
    // Initialize other members as needed
};

void DisplayManager_Init(void){

   
    if (   (display_manager.state == DISPLAY_MANAGER_UNDEFINED) 
        && (display_manager.taskHandle == NULL) )
    {
        ESP_LOGI("Display_Manager", "INIT");
       
        
        /*---------------GPIO configuration backlight pin--------------- */
        ESP_LOGI("Display_Manager_Init", "Turn off LCD backlight");  
        gpio_config_t bk_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT
            };
        ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

         // Optionally, turn the backlight off during setup
        gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, 0);
       
       // May need semaphore  
        display_manager.DisplayManager_xSemaphoreHandle = xSemaphoreCreateBinary(); //Ceates a binary semaphore before using it
        if (display_manager.DisplayManager_xSemaphoreHandle == NULL) {
            ESP_LOGE("Display_Manager_Init", "Failed to create semaphore");
            return;
        }
        xSemaphoreGive(display_manager.DisplayManager_xSemaphoreHandle);
       display_manager.state = DISPLAY_MANAGER_POWER_OFF;
        
        
        xTaskCreate(Display_Manager_Task,
             "Display_Manager_Task", 
             4096, 
             NULL, 
             5, //!!!!!!!!!!!! Mirar prioritat
             &display_manager.taskHandle);

        ESP_LOGI("Display_Manager_Init", "Starting LCD display and LVGL TASK");
        xTaskCreate(example_lvgl_port_task, 
                "LVGL", 
                EXAMPLE_LVGL_TASK_STACK_SIZE, 
                NULL, 
                EXAMPLE_LVGL_TASK_PRIORITY, 
                &display_manager.lvgl_task_handle);
    }
}

esp_err_t DisplayManager_Request(DisplayManager_Configuration_t* config){

    esp_err_t result = ESP_FAIL;
    BaseType_t xResult ;
    
    //chck if the semaphore is available
    
    xResult =  xSemaphoreTake(display_manager.DisplayManager_xSemaphoreHandle, 0);
    //ESP_LOGI("TasDisplayManager_Request", "Try to request display manager: %d",xResult);
    if ( xResult == pdTRUE) //Try to take the semaphore, wait 0 ticks if not available
    {
        ESP_LOGI("DisplayManager_Request", "Semaphore taken immediately!"); // if yes set the application callbacks
        //Probably here goes a callback for interruptuions to the application

        memcpy(&display_manager.config, config, sizeof(DisplayManager_Configuration_t));

 
        //change state
        display_manager.state = DISPLAY_MANAGER_RQUESTING;
        xTaskNotify(display_manager.taskHandle, DISPLAY_MANEGER_SIGNAL_REQUESTED, eSetBits);
    } 
    else
    {
        ESP_LOGE("DisplayManager_Request", "Semaphore not available");
    }

   return result = ESP_OK;
}

esp_err_t DisplayManager_Start(void)
{
    esp_err_t result = ESP_FAIL;

    ESP_LOGI("Display_Manager_Task", "DisplayManager_Start");
    if ( display_manager.state == DISPLAY_MANAGER_REQUESTED)
    {
        
        xTaskNotify(display_manager.taskHandle, DISPLAY_MANEGER_SIGNAL_START, eSetBits);
        result = ESP_OK;
    }
    else
    {
        ESP_LOGE("DisplayManager_Start", "Display Manager not in REQUESTED state");
        result = ESP_FAIL;
    }

    return result;
}

void DisplayManager_Stop(void)
{
    
    if ( display_manager.state == DISPLAY_MANAGER_START)
    {

        xTaskNotify(display_manager.taskHandle, DISPLAY_MANEGER_SIGNAL_STOP, eSetBits);
    }
    else
    {
        ESP_LOGE("DisplayManager_Stop", "Error: Display Manager not in START state");
    }

}

esp_err_t DisplayManager_Release(void)
{
    
    esp_err_t result = ESP_FAIL;
    result = xSemaphoreGive(display_manager.DisplayManager_xSemaphoreHandle);
    //ESP_LOGI("DisplayManager_Release", "Releasing display manager: %d",result);

    return result;
}



void Display_Manager_Task(void *pvParameters)
{

     esp_err_t result = ESP_FAIL;
     uint32_t signal = 0;

     void (*OperationCompleteCallback)(DisplayManager_Result_t result);

    while (1) {
               
                
        switch (display_manager.state)
        {
        case DISPLAY_MANAGER_POWER_OFF:
            
            ESP_LOGI("Display_Manager_Task", "STATE: POWER_OFfF");

            //SignalWait wait for the request signal
            DisplayManagerSignalWait( DISPLAY_MANEGER_SIGNAL_REQUESTED,  portMAX_DELAY);
            
            break;

        case DISPLAY_MANAGER_RQUESTING:
            
            result = DisplayManager_Requesting();
            if ( result == ESP_OK)
            {
                display_manager.state = DISPLAY_MANAGER_REQUESTED;
                ESP_LOGI("Display_Manager_Task", "STATE: REQUESTED");
                if (display_manager.config.callbacks.OperationCompleteCallback != NULL)
                {
                    display_manager.config.callbacks.OperationCompleteCallback(DISPLAY_MANAGER_RESULT_REQUEST);
                }
            }
            else
            {
                ESP_LOGE("Display_Manager_Task", "REQUESTING ERROR -> POWER_OFF");
                display_manager.state = DISPLAY_MANAGER_POWER_OFF;   
            }
            break;

        case DISPLAY_MANAGER_REQUESTED:
            ESP_LOGI("Display_Manager_Task", "STATE: REQUESTED");
            //Wait for start or release signal
            {
                signal = DisplayManagerSignalWait( DISPLAY_MANEGER_SIGNAL_START | DISPLAY_MANEGER_SIGNAL_RLEASE,  portMAX_DELAY);
                
                if (signal & DISPLAY_MANEGER_SIGNAL_START)
                {
                    
                    display_manager.state = DISPLAY_MANAGER_START;
                    ESP_LOGI("Display_Manager_Task", "STATE: START from rquested");
                    if (display_manager.config.callbacks.OperationCompleteCallback != NULL)
                    {
                        display_manager.config.callbacks.OperationCompleteCallback(DISPLAY_MANAGER_RESULT_START);
                        ESP_LOGI("Display_Manager_Task", " START callback defined");
                    }
                    else
                    {
                        ESP_LOGE("Display_Manager_Task", "No START callback defined");
                    }
                }
                else if (signal & DISPLAY_MANEGER_SIGNAL_RLEASE)
                {
                    display_manager.state = DISPLAY_MANAGER_RELEASING;
                    ESP_LOGI("Display_Manager_Task", "STATE: RELEASING");
                }
            }  

            break;

        case DISPLAY_MANAGER_START:
            
           ESP_LOGI("Display_Manager_Task", "STATE: START");
            // --- 2. Lock LVGL access (if using locking) --- 
            _lock_acquire(&lvgl_api_lock2);
                
            // --- 3. Display the main screen ---
            lvgl_main_screen(display_manager.lvgl_display); 

            // --- 4. Unlock LVGL ---
            _lock_release(&lvgl_api_lock2);
            vTaskDelay(pdMS_TO_TICKS(10)); // feeds watchdog
        
            signal = DisplayManagerSignalWait( DISPLAY_MANEGER_SIGNAL_STOP ,  portMAX_DELAY);
            
            if (signal & DISPLAY_MANEGER_SIGNAL_STOP)
            {
                display_manager.state = DISPLAY_MANAGER_REQUESTED;
                ESP_LOGI("Display_Manager_Task", "STATE: REQUESTED");
                if (display_manager.config.callbacks.OperationCompleteCallback != NULL)
                {
                    display_manager.config.callbacks.OperationCompleteCallback(DISPLAY_MANAGER_RESULT_STOP);
                }
            }
           
            break;

        case DISPLAY_MANAGER_RELEASING:
            
            ESP_LOGI("Display_Manager_Task", "STATE: RELEASING");

            OperationCompleteCallback = display_manager.config.callbacks.OperationCompleteCallback;

            result = DisplayManager_Releasing();
            if ( result == ESP_OK)
            {
                display_manager.state = DISPLAY_MANAGER_POWER_OFF;
                ESP_LOGI("Display_Manager_Task", "STATE: POWER_OFF");
                if (OperationCompleteCallback != NULL)
                {
                    OperationCompleteCallback(DISPLAY_MANAGER_RESULT_RELEASE);
                }
            }
            else
            {
                ESP_LOGE("Display_Manager_Task", "RELEASING ERROR -> POWER_OFF");
                display_manager.state = DISPLAY_MANAGER_POWER_OFF;   
            }
            break;
        default:
            ESP_LOGE("Display_Manager_Task", "STATE: UNDEFINED");
            display_manager.state = DISPLAY_MANAGER_POWER_OFF;
            break;
        }
         vTaskDelay(pdMS_TO_TICKS(10));  // small delay is always healthy  
    }
}

esp_err_t DisplayManager_Requesting(void)
{
    esp_err_t result = ESP_FAIL;

       ESP_LOGI("DisplayManager_Request", "Initializing SPI bus for LCD");

        spi_bus_config_t buscfg = {
            .sclk_io_num = EXAMPLE_PIN_NUM_SCLK,
            .mosi_io_num = EXAMPLE_PIN_NUM_MOSI,
            .miso_io_num = EXAMPLE_PIN_NUM_MISO,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = EXAMPLE_LCD_H_RES * 80 * sizeof(uint16_t),
        };
        ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

        ESP_LOGI("DisplayManager_Request", "Installing LCD panel IO driver");

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

        esp_lcd_panel_handle_t panel_handle = NULL;
        esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
            .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
            .bits_per_pixel = 16,
        };
        // "Install ILI9341 panel driver");
        ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));

        // Initialize panel
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

        ESP_LOGI("DisplayManager_Request", "Turning on LCD backlight");
        gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);

        ESP_LOGI("DisplayManager_Request", "Initializing LVGL");
        lv_init();

        // Create LVGL display and buffers
        display_manager.lvgl_display = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);

        size_t draw_buffer_sz = EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_DRAW_BUF_LINES * sizeof(lv_color16_t);
        void *buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_DMA);
        void *buf2 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_DMA);
        assert(buf1 && buf2);

        lv_display_set_buffers(display_manager.lvgl_display, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
        lv_display_set_user_data(display_manager.lvgl_display, panel_handle);
        lv_display_set_color_format(display_manager.lvgl_display, LV_COLOR_FORMAT_RGB565);
        lv_display_set_flush_cb(display_manager.lvgl_display, example_lvgl_flush_cb);

        // LVGL tick timer
        const esp_timer_create_args_t lvgl_tick_timer_args = {
            .callback = &example_increase_lvgl_tick,
            .name = "lvgl_tick"
        };
        esp_timer_handle_t lvgl_tick_timer = NULL;
        ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
        ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

        // Register flush done callback
        const esp_lcd_panel_io_callbacks_t cbs = {
            .on_color_trans_done = example_notify_lvgl_flush_ready,
        };
        ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, display_manager.lvgl_display));

        ESP_LOGI("DisplayManager_Request", "LCD and LVGL successfully requested");
    
            // ---------- TOUCH SETUP ----------
        ESP_LOGI("DisplayManager_Request", "Initializing touch controller XPT2046");

        // Create SPI IO for touch (shares the same SPI bus)
        esp_lcd_panel_io_handle_t tp_io_handle = NULL;
        esp_lcd_panel_io_spi_config_t tp_io_config =
            ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(EXAMPLE_PIN_NUM_TOUCH_CS);

        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &tp_io_config, &tp_io_handle));

        esp_lcd_touch_config_t tp_cfg = {
            .x_max = EXAMPLE_LCD_H_RES,
            .y_max = EXAMPLE_LCD_V_RES,
            .rst_gpio_num = -1,
            .int_gpio_num = -1,
            .flags = {
                .swap_xy = 0,
                .mirror_x = 0,
                .mirror_y = CONFIG_EXAMPLE_LCD_MIRROR_Y,
            },
        };

        esp_lcd_touch_handle_t tp = NULL;
        ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &tp));

        // Register LVGL input device for touch
        static lv_indev_t *indev;
        indev = lv_indev_create();
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_display(indev, display_manager.lvgl_display);
        lv_indev_set_user_data(indev, tp);
        lv_indev_set_read_cb(indev, example_lvgl_touch_cb);

        ESP_LOGI("DisplayManager_Request", "Touch controller initialized");
    
    result = ESP_OK;

    return result;
}

esp_err_t DisplayManager_Releasing(void)
{
    esp_err_t result = ESP_FAIL;
    
    ESP_LOGI("DisplayManager_Releasing", "Releasing LCD and freeing resources...");

    // --- 1. Stop the LVGL task ---
    // If you created a task for LVGL updates, delete it here
    if (display_manager.lvgl_task_handle != NULL) {
        vTaskDelete(display_manager.lvgl_task_handle);
        display_manager.lvgl_task_handle = NULL;
        ESP_LOGI("DisplayManager_Releasing", "LVGL task deleted");
    }
/*
    // --- 2. Deinit touch controller ---
    if (touch_handle) {
        esp_lcd_touch_del(touch_handle);
        touch_handle = NULL;
        ESP_LOGI("DisplayManager_Releasing", "Touch controller released");
    }

    // --- 3. Delete LVGL display driver and flush callback ---
    if (lv_display_get_default()) {
        lv_display_delete(lv_display_get_default());
        ESP_LOGI("DisplayManager_Releasing", "LVGL display driver deleted");
    }

    
    // --- 4. Delete panel handle ---
    if (panel_handle) {
        esp_lcd_panel_del(panel_handle);
        panel_handle = NULL;
        ESP_LOGI("DisplayManager_Releasing", "Panel handle deleted");
    }

    // --- 5. Delete SPI IO handle ---
    if (io_handle) {
        esp_lcd_panel_io_del(io_handle);
        io_handle = NULL;
        ESP_LOGI("DisplayManager_Releasing", "SPI IO handle deleted");
    }
*/
    // --- 6. Deinitialize the SPI bus ---
    spi_bus_free(LCD_HOST);
    ESP_LOGI("DisplayManager_Releasing", "SPI bus deinitialized");

    // --- 7. Turn off backlight ---
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, 0);
    ESP_LOGI("DisplayManager_Releasing", "Backlight turned off");



    ESP_LOGI("DisplayManager_Releasing", "SPI bus deinitialized successfully");
    
    return result ;
}



uint32_t DisplayManagerSignalWait(uint32_t signal, uint32_t timeout)
{
    uint32_t notifiedValue = 0;

    //ESP_LOGI("Display_Manager_SignalWait", "Waiting for signal %" PRIu32 " with timeout %" PRIu32 " ms", signal, timeout);
    printf("Waiting for signal %" PRIu32 " with timeout %" PRIu32 " ms\n", signal, timeout);
    xTaskNotifyWait(0x00, signal, &notifiedValue, pdMS_TO_TICKS(timeout));

    return notifiedValue;
}



































