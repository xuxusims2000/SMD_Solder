#include <stdio.h>

#include <stdint.h>
#include "esp_log.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
//#include "driver/gpio.h"
#include "driver/ledc.h"

#include "Temp_Ctrl.h"
#include "Temp_Sensing.h"
#include "PWM_Config.h"
#include "ILI9341_screen.h"
#include "Tests.h"

#include "driver/spi_master.h"
#include "lvgl.h"



void app_main(void)
{

  #ifdef TEST 
  test_function();

  #else
esp_log_level_set("MAIN", ESP_LOG_INFO); // Set log level for MAIN tag

xTaskCreate(Manager_SMD, "Manager_SMD Task", 2048, NULL, 1, NULL);




    



    #endif
}

void Manager_SMD(){


}
