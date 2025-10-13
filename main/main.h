
// main.h
#ifndef MAIN_H
#define MAIN_H


#include <stdio.h>

#include <stdint.h>
#include "esp_log.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
//#include "driver/gpio.h"
#include "driver/ledc.h"

#include "Temp_Ctrl.h"
#include "Temp_Sensing.h"
#include "PWM_Config.h"
#include "ILI9341_screen.h"
#include "Test.h"

#include "driver/spi_master.h"
#include "lvgl.h"

void Manager_SMD_Requesting(void);

void Manager_SMD_Task(void );

esp_err_t Manager_SMD_Starting(void);

typedef enum {
    POWER_OFF,
    REQUESTING,
    REQUESTED,
    SOLDERING,
    RELAXED,  
    REALISING
} SolderingManagerState;

typedef struct {

    SolderingManagerState state;
    TimerHandle_t Manager_SMD_UpdateTemperature_Timer;
 
} Manager_SMD;




#endif