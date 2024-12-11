#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h" //define the number of the pins




void app_main(void)
{

spi_bus_config_t buscfg = {
    .miso_io_num = GPIO_NUM_19,
    .mosi_io_num = -1,          // No MOSI for MAX6675
    .sclk_io_num = GPIO_NUM_18,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
};

spi_device_interface_config_t devcfg = {
    .clock_speed_hz = 4000000,  // 4 MHz
    .mode = 0,                  // SPI mode 0
    .spics_io_num = GPIO_NUM_5, // CS pin
    .queue_size = 1,
};

// Initialize the SPI bus and add the device.
esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
if (ret != ESP_OK) {
    ESP_LOGE("SPI", "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
    return;
}

spi_device_handle_t spi;
spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
}
