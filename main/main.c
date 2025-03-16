#include <stdio.h>

#include <stdint.h>
#include "esp_log.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "Temp_Ctrl.h"

#include "driver/spi_master.h"


/*--------------------------------------------------------------------------- SPI ----------------------------------------------------------------------------*/ 

// Define SPI pins
#define PIN_NUM_MISO GPIO_NUM_19
#define PIN_NUM_SCLK GPIO_NUM_18
#define PIN_NUM_CS   GPIO_NUM_5

// Declare functions
esp_err_t init_spi_bus(void); 
esp_err_t add_max6675_device(spi_device_handle_t *handle);
float read_max6675(spi_device_handle_t handle);

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

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SPI bus initialized successfully");
    return ESP_OK;
}

esp_err_t add_max6675_device(spi_device_handle_t *handle) {
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 4000000,   // 4 MHz
        .mode = 0,                   // SPI mode 0
        .spics_io_num = PIN_NUM_CS,  // CS pin
        .queue_size = 1,             // Transactions queued
    };

    esp_err_t ret = spi_bus_add_device(SPI2_HOST, &devcfg, handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add MAX6675 device: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "MAX6675 device added successfully");
    return ESP_OK;
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

    float temperature = (raw_data >> 3) * 0.25;
    ESP_LOGI(TAG, "Temperature read: %.2f°C", temperature);
    return temperature;
}

/*------------------------------------------------------------PWM------------------------------------------------------------------------*/
int duty = 0;

esp_err_t set_pwm(void){
    
    /*ledc_timer_config_t timerConfig = {0};
    timerConfig.speed_mode = LEDC_HIGH_SPEED_MODE;
    timerConfig.duty_resolution = LEDC_TIMER_10_BIT; // The ADC on the ESP32-WROOM-32 has a 12-bit resolution
    timerConfig.timer_num = LEDC_TIMER_0;
    timerConfig.freq_hz = 20000 ; //20 khz

    ledc_timer_config(&timerConfig);
    
    ledc_channel_config_t channelConfig = {0};
    channelConfig.gpio_num = GPIO_NUM_33;
    channelConfig.speed_mode = LEDC_HIGH_SPEED_MODE;
    channelConfig.channel = LEDC_CHANNEL_0;
    channelConfig.intr_type= LEDC_INTR_DISABLE;
    channelConfig.timer_sel = LEDC_TIMER_0;
    channelConfig.duty = 0;

    ledc_channel_config(&channelConfig);
*/
 // Configure the LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE, // High-speed mode
        .timer_num        = LEDC_TIMER_0,        // Timer 0
        .duty_resolution  = LEDC_TIMER_10_BIT,   // Resolution of PWM (13 bits)
        .freq_hz          = 5000,               // Frequency: 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK       // Auto clock source
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Configure the LEDC channel
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = 33,                  // GPIO pin number
        .speed_mode     = LEDC_HIGH_SPEED_MODE,// High-speed mode
        .channel        = LEDC_CHANNEL_0,     // Channel 0
        .timer_sel      = LEDC_TIMER_0,       // Use Timer 0
        .duty           = 0,                  // Initial duty cycle (0%)
        .hpoint         = 0                   // Set hpoint to 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  
    return ESP_OK;
}

esp_err_t set_pwm_duty(int duty){
    ledc_set_duty(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_0,duty);
    
    ledc_update_duty (LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_0);

    return ESP_OK;

}



void app_main(void)
{
//initialize spi
    esp_err_t ret = init_spi_bus();
    
    /* adding spi device 
    This initializes the internal structures for a device,
     plus allocates a CS pin on the indicated SPI master peripheral 
     and routes it to the indicated GPIO. 
     All SPI master devices have three CS pins and can thus control up to three devices.
     */ 
    spi_device_handle_t max6675;
    ret = add_max6675_device(&max6675);           
    set_pwm();
    set_pwm_duty(512);
    uint16_t a = reconocer();

    while (1) {
         
        set_pwm_duty(512);
        //read the temperature from the amplifier of the sensro (MAX6675)
        float temperature = read_max6675(max6675);
        if (temperature >= 0) {
            ESP_LOGI("MAIN", "Temperature: %.2f°C", temperature);
        } else {
            ESP_LOGE("MAIN", "Failed to read temperature");
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 second
    }
}
