

#include <Temp_Ctrl.h>


// PID Variables
double last_error = 0.0;
double integral = 0.0;
const double dt = 0.5; // Time step (in seconds), adjust based on your loop timing

// PID Controller Function
uint64_t compute_pid(double setpoint, double current_temp) {
    // Compute error
    double error = setpoint - current_temp;

    // Integral term (accumulated error)
    integral += error * dt;

    // Derivative term (rate of change of error)
    double derivative = (error - last_error) / dt;

    // Compute PID output
    double output = (Kp * error) + (Ki * integral) + (Kd * derivative);

    // Save current error for next iteration
    last_error = error;

return output;
}

uint16_t Temperature2PWM(uint16_t temperature){
    uint16_t new_duty = 0;

    new_duty = (temperature * 1023) / Tmax;
    return new_duty;
}

void Test_PID_control_(){

    //uint16_t new_duty = 0;
    //ret = set_pwm();
    //set_pwm_duty(0);
   
    //uint16_t new_duty = 0;
    //new_temperature = compute_pid(50,current_temperature );

    //new_duty = Temperature2PWM(new_temperature);
    //set_pwm_duty(new_duty);

    //vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 second
    

}




