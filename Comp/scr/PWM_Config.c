

#include "PWM_Config.h"


/*------------------------------------------------------------PWM------------------------------------------------------------------------*/


esp_err_t set_pwm(void){
    

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
           .gpio_num       = GPIO_NUM_33,                  // GPIO pin number
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
   