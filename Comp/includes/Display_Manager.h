


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
#include <string.h>

#include "esp_lcd_ili9341.h"
#include "esp_lcd_touch_xpt2046.h"
#include "lvgl.h"

#include "ILI9341_screen.h"
#include "Screen_design.h"
#include "ui.h"



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


typedef enum {
    DISPLAY_MANAGER_UNDEFINED, 
    DISPLAY_MANAGER_POWER_OFF, 
    DISPLAY_MANAGER_REQUESTING,
    DISPLAY_MANAGER_REQUESTED,
    //DISPLAY_MANAGER_START,
    DISPLAY_MANAGER_MAIN_SCREEN,
    DISPLAY_MANAGER_RELEASING
} DisplayManagerState;

typedef enum DisplayManager_Result_e {
    DISPLAY_MANAGER_RESULT_UNDEFINED ,
    DISPLAY_MANAGER_RESULT_REQUEST,
    DISPLAY_MANAGER_RESULT_START,
    DISPLAY_MANAGER_RESULT_STOP,
    DISPLAY_MANAGER_RESULT_RELEASE,
    DISPLAY_MANAGER_RESULT_OPERATION_OK
} DisplayManager_Result_t;

typedef struct DisplayManager_Callbacks_e {
    void (*OperationCompleteCallback)(DisplayManager_Result_t result);

} DisplayManager_Callbacks_t;

typedef struct DisplayManager_Configuration_e {
   DisplayManager_Callbacks_t   callbacks;

} DisplayManager_Configuration_t;

// Declare functions

void Display_Manager_Task(void *pvParameters);

void DisplayManager_Init(void);
esp_err_t DisplayManager_Request(DisplayManager_Configuration_t* config);
esp_err_t DisplayManager_Start(void);
void DisplayManager_Stop(void);
esp_err_t DisplayManager_Release(void);

esp_err_t DisplayManager_SetTemperature(float temperature);

