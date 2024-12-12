#include "SPI_Comunication.h"
#include "driver/spi_master.h"
#include "esp_log.h"

// TAG for logging
static const char *TAG = "MAX6675";

esp_err_t init_spi_bus() {
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = -1,          // Not used for MAX6675
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,        // Not used
        .quadhd_io_num = -1,        // Not used
        .max_transfer_sz = 16,      // MAX6675 only transfers 16 bits
    };

    return spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
}

esp_err_t add_max6675_device(spi_device_handle_t *handle) {
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 4000000,   // 4 MHz
        .mode = 0,                   // SPI mode 0
        .spics_io_num = PIN_NUM_CS,  // CS pin
        .queue_size = 1,             // Transactions queued
    };

    return spi_bus_add_device(SPI2_HOST, &devcfg, handle);
}

float read_max6675(spi_device_handle_t handle) {
    spi_transaction_t trans = {
        .flags = SPI_TRANS_USE_RXDATA, // Use internal RX buffer
        .length = 16,                 // 16 bits transfer
    };

    esp_err_t ret = spi_device_transmit(handle, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transaction failed: %s", esp_err_to_name(ret));
        return -1.0; // Indicate an error
    }

    uint16_t raw_data = (trans.rx_data[0] << 8) | trans.rx_data[1];

    if (raw_data & 0x04) {
        ESP_LOGW(TAG, "No thermocouple connected!");
        return -1.0; // Indicate an error
    }

    return (raw_data >> 3) * 0.25;
}