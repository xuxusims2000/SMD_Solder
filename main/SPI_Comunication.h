#ifndef SPI_COMUNICATION_H
#define SPI_COMUNICATION_H

#include <stdint.h>
#include <esp_err.h>
#include "driver/spi_master.h"
#include "esp_log.h" 
#include "driver/gpio.h"

// Define SPI pins
#define PIN_NUM_MISO GPIO_NUM_19
#define PIN_NUM_SCLK GPIO_NUM_18
#define PIN_NUM_CS   GPIO_NUM_5

// Declare functions
esp_err_t init_spi_bus(void); 
esp_err_t add_max6675_device(spi_device_handle_t *handle);
float read_max6675(spi_device_handle_t handle);

#endif 
