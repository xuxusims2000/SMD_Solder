

#include "Display_Manager.h"



#define DISPLAY_MANEGER_SIGNAL_REQUESTED   (1 << 0)  // Signal to request display manager
#define DISPLAY_MANEGER_SIGNAL_RLEASE      (1 << 1)  // Signal to indicate display manager 
#define DISPLAY_MANEGER_SIGNAL_START       (1 << 2)  // Signal to indicate display manager 
#define DISPLAY_MANEGER_SIGNAL_STOP        (1 << 3)  // Signal to release display manager



esp_err_t DisplayManager_Requesting(void);

uint32_t DisplayManagerSignalWait(uint32_t signal, uint32_t timeout);

esp_err_t DisplayManager_Releasing(void);

typedef struct {
    DisplayManagerState                state;

    DisplayManager_Configuration_t     config;

    uint32_t                        signals;  // Bitmask for signals

    TaskHandle_t                    taskHandle;
    SemaphoreHandle_t               DisplayManager_xSemaphoreHandle; //Defines a semaphore to manage the resource
    

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
       
        
        /*---------------GPIO configuration--------------- */
        ESP_LOGI("Display_Manager_Init", "Turn off LCD backlight");  
        gpio_config_t bk_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT
            };
        ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

        /*---------------SPI configuration--------------- */
        ESP_LOGI("Display_Manager_Init", "Initialize SPI bus");
        spi_bus_config_t buscfg = {
            .sclk_io_num = EXAMPLE_PIN_NUM_SCLK,
            .mosi_io_num = EXAMPLE_PIN_NUM_MOSI,
            .miso_io_num = EXAMPLE_PIN_NUM_MISO,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = EXAMPLE_LCD_H_RES * 80 * sizeof(uint16_t),
            };
        ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

        /*------------Connect the LCD driver(ili9341) to SPI bus ----------------------*/
        ESP_LOGI("Display_Manager_Init", "Install panel IO");
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
        // Attach the LCD to the SPI bus
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

        esp_lcd_panel_handle_t panel_handle = NULL;
        esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
        };

        ESP_LOGI("Display_Manager_Init", "Install ILI9341 panel driver");
        ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));

        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));

        // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

        ESP_LOGI("Display_Manager_Init", "Turn on LCD backlight");
        gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);

        //Auria de mirar si necesita mutex o semaforo
        display_manager.DisplayManager_xSemaphoreHandle = xSemaphoreCreateBinary();
        xSemaphoreGive(display_manager.DisplayManager_xSemaphoreHandle); //Initialy available

        ESP_LOGI("Display_Manager_Init", "Initialize LVGL library");
        lv_init();
       
        xTaskCreate(Display_Manager_Task,
             "Display_Manager_Task", 
             4096, 
             NULL, 
             5, //!!!!!!!!!!!! Mirar prioritat
             &display_manager.taskHandle);
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
    
    if ( display_manager.state == DISPLAY_MANAGER_REQUESTED)
    {
        display_manager.state = DISPLAY_MANAGER_START;
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
        display_manager.state = DISPLAY_MANAGER_REQUESTED;
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

        case DISPLAY_MANAGER_START:
            
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
    }
}

esp_err_t DisplayManager_Requesting(void)
{
    esp_err_t result = ESP_FAIL;

    
    result = ESP_OK;

    return result;
}

esp_err_t DisplayManager_Releasing(void)
{
    esp_err_t result = ESP_FAIL;
    /*STOP and DELEATE timers*/
    /* Release resourses used like gpio spi uart etc*/

    //Release SPI resources
    //result = spi_bus_remove_device(???);
    //if (result != ESP_OK) {
        //ESP_LOGE(TAG, "Failed to remove SPI device: %s", esp_err_to_name(result));
        //return result;
    //}
    //By the moment just releases spi device, not the bus
    //result = spi_bus_free(SPI2_HOST);
    //if (result != ESP_OK) {
        //ESP_LOGE(TAG, "Failed to free SPI bus: %s", esp_err_to_name(result));
        //return;
    //}

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



































