

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "SPI_Comunication.h"
#include "driver/gpio.h"

void app_main(void) {
    esp_err_t ret = init_spi_bus();
    if (ret != ESP_OK) {
        ESP_LOGE("MAIN", "Failed to initialize SPI bus");
        return;
    }

    spi_device_handle_t max6675;
    ret = add_max6675_device(&max6675);
    if (ret != ESP_OK) {
        ESP_LOGE("MAIN", "Failed to add MAX6675 device");
        return;
    }

    while (1) {
        float temperature = read_max6675(max6675);
        if (temperature >= 0) {
            ESP_LOGI("MAIN", "Temperature: %.2f°C", temperature);
        } else {
            ESP_LOGE("MAIN", "Failed to read temperature");
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 second
    }
}
