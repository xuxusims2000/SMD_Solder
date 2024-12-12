#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Define GPIO pins
#define PIN_NUM_MISO GPIO_NUM_19 // MISO
#define PIN_NUM_SCLK GPIO_NUM_18 // SCLK
#define PIN_NUM_CS   GPIO_NUM_5  // CS

// SPI Host
#define SPI_HOST SPI2_HOST

// TAG for logging
static const char *TAG = "MAX6675";

// Function to initialize the SPI bus
static esp_err_t init_spi_bus() {
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = -1,          // Not used for MAX6675
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,        // Not used
        .quadhd_io_num = -1,        // Not used
        .max_transfer_sz = 16,      // MAX6675 only transfers 16 bits
    };

    // Initialize the SPI bus
    return spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
}

// Function to add the MAX6675 device to the SPI bus
static esp_err_t add_max6675_device(spi_device_handle_t *handle) {
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 4000000,   // 4 MHz
        .mode = 0,                   // SPI mode 0
        .spics_io_num = PIN_NUM_CS,  // CS pin
        .queue_size = 1,             // Transactions queued
    };

    // Add the device
    return spi_bus_add_device(SPI_HOST, &devcfg, handle);
}

// Function to read temperature from MAX6675
static float read_max6675(spi_device_handle_t handle) {
    spi_transaction_t trans = {
        .flags = SPI_TRANS_USE_RXDATA, // Use internal RX buffer
        .length = 16,                 // 16 bits transfer
    };

    esp_err_t ret = spi_device_transmit(handle, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transaction failed: %s", esp_err_to_name(ret));
        return -1.0; // Indicate an error
    }

    // Extract temperature data from received bits
    uint16_t raw_data = (trans.rx_data[0] << 8) | trans.rx_data[1];

    // Check for thermocouple connection error (bit 2)
    if (raw_data & 0x04) {
        ESP_LOGW(TAG, "No thermocouple connected!");
        return -1.0; // Indicate an error
    }

    // Shift to discard the lower 3 bits and multiply by 0.25°C per bit
    float temperature = (raw_data >> 3) * 0.25;
    return temperature;
}

// Main application
void app_main(void) {
    // Initialize SPI bus
    esp_err_t ret = init_spi_bus();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return;
    }

    // Add MAX6675 device to SPI bus
    spi_device_handle_t max6675;
    ret = add_max6675_device(&max6675);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add MAX6675 device: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "SPI bus and MAX6675 initialized successfully");

    // Periodically read the temperature
    while (1) {
        float temperature = read_max6675(max6675);
        if (temperature >= 0) {
            ESP_LOGI(TAG, "Temperature: %.2f°C", temperature);
        } else {
            ESP_LOGE(TAG, "Failed to read temperature");
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 second
    }
}
