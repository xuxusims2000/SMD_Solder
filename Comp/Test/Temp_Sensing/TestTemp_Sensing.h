//TestTemp_Sensing.h

#ifndef TESTTEMP_SENSING_H
#define TESTTEMP_SENSING_H

#include <stdio.h>
#include <stdint.h>
#include "esp_log.h" 
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "Temp_Sensing.h"


//Test function

/* Prove of consep no task just reeds temperature*/
void Test_temperature_sensing_0(void);


/*Request , ... , Rlease*/
void Test_temperature_sensing_1(void);




#endif // TESTTEMP_SENSING_H