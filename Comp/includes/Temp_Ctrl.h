

#ifndef TEMP_CTRL_H_
#define TEMP_CTRL_H_

#include <stdint.h>
#include "esp_err.h"          // Include for error handling
#include "esp_log.h" 
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

typedef enum {
    TEMP_CTRL_UNDEFINED, 
    TEMP_CTRL_POWER_OFF, 
    TEMP_CTRL_RQUESTING,
    TEMP_CTRL_REQUESTED,
    TEMP_CTRL_START,
    TEMP_CTRL_RELEASING
} TempCtrlState;

typedef enum TempCtrl_Result_e {
    TEMP_CTRL_RESULT_UNDEFINED ,
    TEMP_CTRL_RESULT_REQUEST,
    TEMP_CTRL_RESULT_START,
    TEMP_CTRL_RESULT_STOP,
    TEMP_CTRL_RESULT_RELEASE,
    TEMP_CTRL_RESULT_OPERATION_OK
} TempCtrl_Result_t;

typedef struct TempCtrl_Callbacks_e {
    void (*OperationCompleteCallback)(TempCtrl_Result_t result);

} TempCtrl_Callbacks_t;

typedef struct TempCtrl_Configuration_e {
   TempCtrl_Callbacks_t   callbacks;

} TempCtrl_Configuration_t;


// Declare functions

void Temp_Ctrl_Task(void *pvParameters);

void Temp_Ctrl_Init(void);
esp_err_t Temp_Ctrl_Request(TempCtrl_Configuration_t* config);
esp_err_t Temp_Ctrl_Start(void);
void Temp_Ctrl_Stop(void);
void Temp_Ctrl_Release(void);

esp_err_t config_pwm(void);
esp_err_t set_pwm_duty(int duty);


// PID Constants (Tune these values)
#define Kp 2.0      // Proportional gain
#define Ki 0.5      // Integral gain
#define Kd 1.0      // Derivative gain

#define FREQUENCY_PWM (1) //Frequency of PWM in Hz

#define Tmax 350 //Tempere max that hotplate can get


uint64_t compute_pid(double setpoint, double current_temp);
uint16_t Temperature2PWM(uint16_t temperature);

void Test_PID_control_(void);


#endif /* TEMP_CTRL_H_ */