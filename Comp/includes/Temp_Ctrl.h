

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
#include "Temp_Sensing.h"

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

void TempCtrl_Task(void *pvParameters);

void TempCtrl_Init(void);
esp_err_t Temp_Ctrl_Request(TempCtrl_Configuration_t* config);
esp_err_t Temp_Ctrl_Start(void);
void Temp_Ctrl_Stop(void);
void Temp_Ctrl_Release(void);

void TempCtrl_SetState(TempCtrlState state);

esp_err_t config_pwm(void);
esp_err_t set_pwm_duty(uint32_t duty);
esp_err_t TempCtrl_UpdateTemperature(uint32_t temperature);
esp_err_t TempCtrl_SetTemperature(uint32_t temp);
void TempCtrl_StopTemperatureControl(void);


// PID Constants (Tune these values)
// These values are deliberately more conservative to avoid overshoot and oscillations.
#define Kp 1.3      // Proportional gain
#define Ki 0.04     // Integral gain
#define Kd 0.35     // Derivative gain
#define MAX_PID_INTEGRAL 300.0

#define PID_SAMPLE_TIME_MS (150)
#define PID_SAMPLE_TIME_S (0.15f)

#define FREQUENCY_PWM (1000) // Frequency of PWM in Hz
#define MAX_PWM_DUTY (512)   // Cap the maximum PWM duty to reduce current spikes
#define PWM_RAMP_STEP (5)    // Smooth duty changes to avoid sudden power jumps

#define Tmax 350 // Tempere max that hotplate can get


double Temp_Compute_pid(double setpoint, double current_temp);
uint16_t Temperature2PWM(uint16_t temperature);




#endif /* TEMP_CTRL_H_ */