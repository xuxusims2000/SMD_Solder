

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "SPI_Comunication.h"
#include "driver/gpio.h"

void app_main(void) {
    
    
    //initialize spi
    esp_err_t ret = init_spi_bus();
    
    /* adding spi device 
    This initializes the internal structures for a device,
     plus allocates a CS pin on the indicated SPI master peripheral 
     and routes it to the indicated GPIO. 
     All SPI master devices have three CS pins and can 
     thus control up to three devices.
     */ 
    spi_device_handle_t max6675;
    ret = add_max6675_device(&max6675);           
  

    while (1) {
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
