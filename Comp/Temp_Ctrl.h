

#include <stdint.h>


// PID Constants (Tune these values)
#define Kp 2.0      // Proportional gain
#define Ki 0.5      // Integral gain
#define Kd 1.0      // Derivative gain


uint64_t compute_pid(double setpoint, double current_temp);
uint16_t Temperature2PWM(uint16_t temperature);