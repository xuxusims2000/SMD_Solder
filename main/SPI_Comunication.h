
#ifndef SPI_Comunication_H
#define SPI_Comunication_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Define constants or macros
#define PIN_NUM_MISO GPIO_NUM_19
#define PIN_NUM_SCLK GPIO_NUM_18
#define PIN_NUM_CS   GPIO_NUM_5

// Declare functions
esp_err_t init_spi_bus(void);
esp_err_t add_max6675_device(spi_device_handle_t *handle);
float read_max6675(spi_device_handle_t handle);

#endif // MY_FUNCTIONS_H
