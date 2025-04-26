#include <stdio.h>

#include <stdint.h>
#include "esp_log.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "driver/gpio.h"
#include "driver/ledc.h"

#include "Temp_Ctrl.h"
#include "SPI_Config.h"
#include "PWM_Config.h"

#include "driver/spi_master.h"



void app_main(void)
{
//initialize spi
    esp_err_t ret = init_spi_bus();
    spi_device_handle_t max6675;
    ret = add_max6675_device(&max6675);

    ret = set_pwm();
    set_pwm_duty(0);
    uint16_t new_temperature = 0;
    uint16_t new_duty = 0;
    
    while (1) {
               
        
        //read the temperature from the amplifier of the sensro (MAX6675)
        float current_temperature = read_max6675(max6675); // temperature in Celsius
        if (current_temperature >= 0) {
            ESP_LOGI("MAIN", "Temperature: %.2f°C", current_temperature);
        } else {
            ESP_LOGE("MAIN", "Failed to read temperature");
        }

        new_temperature = compute_pid(50,current_temperature );

        new_duty = Temperature2PWM(new_temperature);
        set_pwm_duty(new_duty);

        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 second
    }
}
