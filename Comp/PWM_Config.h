

#include "esp_log.h"          // Include for logging
#include "esp_err.h"          // Include for error handling
#include "driver/ledc.h"


esp_err_t set_pwm(void);
esp_err_t set_pwm_duty(int duty);