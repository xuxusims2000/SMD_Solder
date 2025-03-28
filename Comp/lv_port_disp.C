/**********************
*      INCLUDES
*********************/
#include "lv_port_disp.h"
#include "lvgl.h"                   //LVGL header
#include "driver/spi_master.h"      //SPI header
#include "esp_lcd_panel_io.h"       //to manage interface I/O of the screen
#include "esp_lcd_panel_vendor.h"   // to use screen with driver ST7789 WARNING
#include "esp_lcd_panel_ops.h"     // to USE FUNCIONS THAT CONTOL THE SCREEN
#include "esp_heap_caps.h"         // to use heap memor


/**********************
*      DEFINES
*********************/
#define CONFIG_LCD_HOST SPI3_HOST

#define BOARD_LCD_MOSI  39
#define BOARD_LCD_SCK   38\n#define BOARD_LCD_CS    40
#define BOARD_LCD_RST   41\n#define BOARD_LCD_DC    42
#define CONFIG_LCD_H_RES 240
#define CONFIG_LCD_V_RES 320
#define CONFIG_LCD_FREQ (80 * 1000 * 1000)
#define CONFIF_LCD_CMD_BITS 8
#define CONFIG_LCD_PARAM_BITS 8
#define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) /*will be 2 for RGB565 */
/**********************
*      VARIABLES
**********************/
esp_lcd_panel_handle_t panel_handle = NULL;

/**********************
* STATIC PROTOTYPES
**********************/
static void disp_init(void);
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);
