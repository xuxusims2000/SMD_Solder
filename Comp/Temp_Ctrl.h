

#include <stdint.h>


// PID Constants (Tune these values)
#define Kp 2.0      // Proportional gain
#define Ki 0.5      // Integral gain
#define Kd 1.0      // Derivative gain


double compute_pid(double setpoint, double current_temp);