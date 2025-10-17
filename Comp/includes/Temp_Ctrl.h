

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


// PID Constants (Tune these values)
#define Kp 2.0      // Proportional gain
#define Ki 0.5      // Integral gain
#define Kd 1.0      // Derivative gain

#define Tmax 350 //Tempere max that hotplate can get

void Temp_Ctrl_Init(void);
void Temp_Ctrl_Task(void);

uint64_t compute_pid(double setpoint, double current_temp);
uint16_t Temperature2PWM(uint16_t temperature);

void Test_PID_control_(void);


