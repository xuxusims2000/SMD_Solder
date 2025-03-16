

#include <Temp_Ctrl.h>


// PID Variables
double last_error = 0.0;
double integral = 0.0;
const double dt = 0.5; // Time step (in seconds), adjust based on your loop timing

// PID Controller Function
double compute_pid(double setpoint, double current_temp) {
    // Compute error
    double error = setpoint - current_temp;

    // Integral term (accumulated error)
    integral += error * dt;

    // Derivative term (rate of change of error)
    double derivative = (error - last_error) / dt;

    // Compute PID output
    double output = (Kp * error) + (Ki * integral) + (Kd * derivative);

    // Constrain output to valid PWM range (0 to 1023 for 10-bit PWM)
    if (output > 1023) output = 1023;
    if (output < 0) output = 0;

    // Save current error for next iteration
    last_error = error;

    return output;
}



