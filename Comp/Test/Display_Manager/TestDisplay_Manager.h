

#ifndef TESTDISPLAY_MANAGER_H
#define TESTDISPLAY_MANAGER_H

#include "Display_Manager.h"
#include <stdio.h>
#include <stdint.h>
#include "esp_log.h" 
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

//Test function

void Test_display_manager_1(void);
void Test_display_manager_2(void);

#endif // TESTDISPLAY_MANAGER_H