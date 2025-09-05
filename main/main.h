
// main.h
#ifndef MAIN_H
#define MAIN_H


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

void Manager_SMD(void );

#endif