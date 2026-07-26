

#include "Temp_Ctrl.h"
#include "Debug.h"

#define TEMP_CTRL_SIGNAL_REQUESTED   (1 << 0)  // Signal to request temperature control
#define TEMP_CTRL_SIGNAL_RELEASE     (1 << 1)  // Signal to indicate temperature control 
#define TEMP_CTRL_SIGNAL_START       (1 << 2)  // Signal to indicate temperature control
#define TEMP_CTRL_SIGNAL_STOP        (1 << 3)  // Signal to release temperature control
#define TEMP_CTRL_SIGNAL_SET_TEMP    (1 << 4)  // Signal to set temperature

#define TEMP_CTRL_DEBUG 

#ifdef TEMP_CTRL_DEBUG
    #define DBG_TC(fmt, ...)  Debug_Printf(ANSI_COLOR_GREEN "[Temp_Ctrl] " fmt "\r\n", ##__VA_ARGS__)
    #define TOP_DBG_TC(title) \
        do { \
            Debug_Printf("//====================================//\r\n"); \
            Debug_Printf("// %s //\r\n", title); \
            Debug_Printf("//====================================//\r\n"); \
        } while (0)
    #define TELEPLOT_TC(var_name, value)  Debug_Teleplot(var_name, value)
#else
    #define DBG_TC(fmt, ...)  ((void)0)
    #define TOP_DBG_TC(title) ((void)0)
    #define TELEPLOT_TC(var_name, value)  ((void)0)
#endif


esp_err_t tempCtrl_Requesting(void);
uint32_t tempCtrlSignalWait(uint32_t signal, uint32_t timeout);
esp_err_t tempCtrl_Releasing(void);

void TempCtrl_PID_Callback(TimerHandle_t xTimer);

esp_err_t config_pwm(void);
//esp_err_t set_pwm_duty(uint32_t duty);
void TempCtrl_CalculateTemp(void);


typedef struct {
    TempCtrlState                state;

    TempCtrl_Configuration_t     config;

    uint32_t                        signals;  // Bitmask for signals

    TaskHandle_t                    taskHandle;
    SemaphoreHandle_t               TempCtrl_xSemaphoreHandle; //Defines a semaphore to manage the resource

    TimerHandle_t                   pid_timer; // Timer for PID control
    
    uint32_t                        pwm_duty;
 
    double                          last_error;
    double                          integral;
    const double                    dt;
   
    uint32_t                        target_temperature;
    uint32_t                        temp;
    double                          pid_output;

    bool                            heat_up_process_active;

} TempCtrl_t;



/* Private variables --------------------------------------------------*/

static TempCtrl_t temp_ctrl = {
    .state = TEMP_CTRL_UNDEFINED,
    .signals = 0,
    .taskHandle= NULL,
    .TempCtrl_xSemaphoreHandle = NULL,
    .dt = 0.1, // Example time step for PID calculations (100 ms)
    .heat_up_process_active = false,
    // Initialize other members as needed
};


void TempCtrl_Init(void){

    if (   (temp_ctrl.state == TEMP_CTRL_UNDEFINED) 
        && (temp_ctrl.taskHandle == NULL) )
    {
        ESP_LOGI("Temp_ctrl", "INIT");
        
       //Auria de mirar si necesita mutex o semaforo
       
        esp_err_t ret = config_pwm(); 
        if (ret != ESP_OK) {
            ESP_LOGE("Temp_Ctrl_Init", "Failed to initialize PWM: %s", esp_err_to_name(ret));
            return;
        }

        temp_ctrl.TempCtrl_xSemaphoreHandle = xSemaphoreCreateBinary();
        if(temp_ctrl.TempCtrl_xSemaphoreHandle == NULL){
            ESP_LOGE("Temp_Ctrl_Init", "Failed to create semaphore");
            return;
        }
        xSemaphoreGive(temp_ctrl.TempCtrl_xSemaphoreHandle); //Initialy the semaphore is available
       
        temp_ctrl.state = TEMP_CTRL_POWER_OFF;
       
        //Create Task
        xTaskCreate(TempCtrl_Task, 
                    "Temperature_control_Task", 
                    2048, 
                    NULL, 
                    1, 
                    &temp_ctrl.taskHandle);
    }
}

esp_err_t Temp_Ctrl_Request(TempCtrl_Configuration_t* config){

    esp_err_t result = ESP_FAIL;
    BaseType_t xResult ;
    
    //chck if the semaphore is available
    
    xResult =  xSemaphoreTake(temp_ctrl.TempCtrl_xSemaphoreHandle, 0);
    if ( xResult == pdTRUE) //Try to take the semaphore, wait 0 ticks if not available
    {
        ESP_LOGI("Temp_Ctrl_Request", "Semaphore taken immediately!"); // if yes set the application callbacks
        //Probably here goes a callback for interruptuions to the application

        memcpy(&temp_ctrl.config, config, sizeof(TempCtrl_Configuration_t));

        //change state
        temp_ctrl.state = TEMP_CTRL_RQUESTING;
        xTaskNotify(temp_ctrl.taskHandle, TEMP_CTRL_SIGNAL_REQUESTED, eSetBits);
    } 
    else
    {
        ESP_LOGE("Temp_Ctrl_Request", "Semaphore not available");
    }

   return result = ESP_OK;
}

esp_err_t Temp_Ctrl_Start(void){
    esp_err_t result = ESP_FAIL;

    if ( temp_ctrl.state == TEMP_CTRL_REQUESTED)
    {
        xTaskNotify(temp_ctrl.taskHandle, TEMP_CTRL_SIGNAL_START, eSetBits);
        result = ESP_OK;
    }
    else
    {
        ESP_LOGE("Temp_Ctrl_Start", "Error: Temp_Ctrl not in REQUESTED state");
    }
    return result;
}

void Temp_Ctrl_Stop(void){

    if( temp_ctrl.state == TEMP_CTRL_START )
    {
         xTaskNotify(temp_ctrl.taskHandle, TEMP_CTRL_SIGNAL_STOP, eSetBits);
    }
    else
    {
        ESP_LOGE("Temp_Ctrl_Stop", "Error: Temp_Ctrl not in START state");
    }

}


void Temp_Ctrl_Release(void){
    
    esp_err_t result = ESP_FAIL;
    result = xSemaphoreGive(temp_ctrl.TempCtrl_xSemaphoreHandle);
    if ( result == pdTRUE)
    {
        if (temp_ctrl.state == TEMP_CTRL_REQUESTED)
        {
            temp_ctrl.state = TEMP_CTRL_RELEASING;
            xTaskNotify(temp_ctrl.taskHandle, TEMP_CTRL_SIGNAL_RELEASE, eSetBits);
            ESP_LOGI("Temp_Ctrl_Release", "Temperature Control Release OK.");
        }
        else
        {
            ESP_LOGE("Temp_Ctrl_Release", "Temperature Control Release ERROR.");
        }
    }
    else
    {
        ESP_LOGE("Temp_Ctrl_Release", "Error: Semaphore not released");
    }
}  

 void TempCtrl_Task(void *pvParameters){

     esp_err_t result = ESP_FAIL;
     uint32_t signal = 0;

     void (*OperationCompleteCallback)(TempCtrl_Result_t result);

    while(1){

        switch (temp_ctrl.state)
        {
        case TEMP_CTRL_POWER_OFF:
            
            ESP_LOGI("Temp_Ctrl_Task", "STATE: POWER_OFF");
            signal = tempCtrlSignalWait( TEMP_CTRL_SIGNAL_REQUESTED,  portMAX_DELAY);

            break;

        case TEMP_CTRL_RQUESTING:
           
            ESP_LOGI("Temp_Ctrl_Task", "STATE: REQUESTING");
            result = tempCtrl_Requesting();
            if ( result == ESP_OK)
            {
                temp_ctrl.state = TEMP_CTRL_REQUESTED;
                ESP_LOGI("Temp_Ctrl_Task", "REQUESTED");
                if (temp_ctrl.config.callbacks.OperationCompleteCallback != NULL)
                {
                    temp_ctrl.config.callbacks.OperationCompleteCallback(TEMP_CTRL_RESULT_REQUEST);
                }

            }
            else
            {
                ESP_LOGE("Temp_Ctrl_Task", "REQUESTING ERROR -> RELEASING");
                xSemaphoreGive(temp_ctrl.TempCtrl_xSemaphoreHandle);
                temp_ctrl.state = TEMP_CTRL_RELEASING;
                
            }
            break;
    
        case TEMP_CTRL_REQUESTED:
            ESP_LOGI("Temp_Ctrl_Task", "STATE: REQUESTED");

            signal = tempCtrlSignalWait( TEMP_CTRL_SIGNAL_START | TEMP_CTRL_SIGNAL_RELEASE,  portMAX_DELAY);

            if (signal & TEMP_CTRL_SIGNAL_START)
            {
                temp_ctrl.state = TEMP_CTRL_START;
                ESP_LOGI("Temp_Ctrl_Task", "STATE: START");
                if (temp_ctrl.config.callbacks.OperationCompleteCallback != NULL)
                {
                    temp_ctrl.config.callbacks.OperationCompleteCallback(TEMP_CTRL_RESULT_START);
                }
            }
            else if (signal & TEMP_CTRL_SIGNAL_RELEASE)
            {
                temp_ctrl.state = TEMP_CTRL_RELEASING;
                ESP_LOGI("Temp_Ctrl_Task", "STATE: RELEASING");
            }

            break;

        case TEMP_CTRL_START:
            ESP_LOGI("Temp_Ctrl_Task", "STATE: START");

            //Here so far doing test in the future shold do the curve of temperature
    
            signal = tempCtrlSignalWait( TEMP_CTRL_SIGNAL_STOP | 
                                         TEMP_CTRL_SIGNAL_SET_TEMP ,  
                                         portMAX_DELAY);


            if (signal & TEMP_CTRL_SIGNAL_SET_TEMP)
            {
                if (temp_ctrl.heat_up_process_active)
                {
                    TELEPLOT_TC("Current_Temperature", temp_ctrl.temp);
                    TELEPLOT_TC("Target_Temperature", temp_ctrl.target_temperature);
                    TempCtrl_CalculateTemp();
                    
                }
            }
            else if (signal & TEMP_CTRL_SIGNAL_STOP)
            {
                temp_ctrl.heat_up_process_active = false;
                TempCtrl_StopTemperatureControl();
                temp_ctrl.state = TEMP_CTRL_REQUESTED;
                ESP_LOGI("Temp_Ctrl_Task", "STATE: REQUESTED");
                if (temp_ctrl.config.callbacks.OperationCompleteCallback != NULL)
                {
                    temp_ctrl.config.callbacks.OperationCompleteCallback(TEMP_CTRL_RESULT_STOP);
                }
            }
            
            break;

        case TEMP_CTRL_RELEASING:
            ESP_LOGI("Temp_Ctrl_Task", "STATE: RELEASING");

            OperationCompleteCallback = temp_ctrl.config.callbacks.OperationCompleteCallback;

            result = tempCtrl_Releasing();
            if ( result == ESP_OK)
            {
                temp_ctrl.state = TEMP_CTRL_POWER_OFF;
                ESP_LOGI("Temp_Ctrl_Task", "POWER OFF");
                if (temp_ctrl.config.callbacks.OperationCompleteCallback != NULL)
                {
                    temp_ctrl.config.callbacks.OperationCompleteCallback(TEMP_CTRL_RESULT_RELEASE);
                }
            }
            else
            {
                ESP_LOGE("Temp_Ctrl_Task", "RELEASING ERROR -> POWER OFF");
                temp_ctrl.state = TEMP_CTRL_POWER_OFF;
            }
            break;
        
        
        default:
                ESP_LOGE("Temp_Ctrl_Task", "STATE: UNDEFINED");
               
            break;
        }
        
    }
}



/*------------------------------------------------------------PWM------------------------------------------------------------------------*/


esp_err_t config_pwm(void){
    

    // Configure the LEDC timer
       ledc_timer_config_t ledc_timer = {
           .speed_mode       = LEDC_HIGH_SPEED_MODE, // High-speed mode
           .timer_num        = LEDC_TIMER_0,        // Timer 0
           .duty_resolution  = LEDC_TIMER_10_BIT,   // Resolution of PWM (13 bits)
           .freq_hz          = FREQUENCY_PWM,               // Frequency: 5 kHz
           .clk_cfg          = LEDC_AUTO_CLK       // Auto clock source
       };
       ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
   
       // Configure the LEDC channel
       ledc_channel_config_t ledc_channel = {
           .gpio_num       = GPIO_NUM_33,                  // GPIO pin number
           .speed_mode     = LEDC_HIGH_SPEED_MODE,// High-speed mode
           .channel        = LEDC_CHANNEL_0,     // Channel 0
           .intr_type      = LEDC_INTR_DISABLE,        // No interrupt
           .timer_sel      = LEDC_TIMER_0,       // Use Timer 0
           .duty           = (0),                  // Initial duty cycle (0%)
           .hpoint         = 0                   // Set hpoint to 0
       };
       ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
   
     
       return ESP_OK;
   }
   
   esp_err_t set_pwm_duty(uint32_t duty)
   { // set_temperature
       
        ledc_set_duty(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_0,duty);
       
        ledc_update_duty (LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_0);

       ESP_LOGI("set_pwm_duty", "PWM duty set to %lu", duty);
   
       return ESP_OK;
   
   }



/*------------------PID Controller Function--------------------------*/ 
double Temp_Compute_pid(double setpoint, double current_temp) {
    // Compute error as a signed value
    double error = setpoint - current_temp;
    //DBG_TC("Error: %.2f", error);

    // Integral term (accumulated error)
    temp_ctrl.integral += error * temp_ctrl.dt;

    // Derivative term (rate of change of error)
    double derivative = (error - temp_ctrl.last_error) / temp_ctrl.dt;

    // Compute PID output
    double output = (Kp * error) + (Ki * temp_ctrl.integral) + (Kd * derivative);

    // Save current error for next iteration
    temp_ctrl.last_error = error;

    return output;
}

uint16_t Temperature2PWM(uint16_t temperature){ // doit as define
    uint16_t new_duty = 0;

    new_duty = (temperature * 1023) / Tmax;
    return new_duty;
}


esp_err_t TempCtrl_UpdateTemperature(uint32_t temperature){ //fpaso de flaot a uint32
    esp_err_t result = ESP_FAIL;

    temp_ctrl.temp = temperature;

    return result = ESP_OK;
}


esp_err_t TempCtrl_SetTemperature(uint32_t temp) //fpaso de flaot a uint32
{
    temp_ctrl.integral = 0.0;
    temp_ctrl.last_error = 0.0;

    temp_ctrl.target_temperature = temp;
    temp_ctrl.heat_up_process_active = true;

    //activar el timer de pid
    if (xTimerStart(temp_ctrl.pid_timer, 0) != pdPASS) {
        ESP_LOGE("Temp_Ctrl", "Failed to start PID timer");
        return ESP_FAIL;
    }
    DBG_TC("STARTED TIMER HEAT");

    return ESP_OK;
}

void TempCtrl_StopTemperatureControl(void)
{
    temp_ctrl.heat_up_process_active = false;
    
    if (xTimerStop(temp_ctrl.pid_timer, 0) != pdPASS) {
        ESP_LOGE("Temp_Ctrl", "Failed to stop PID timer");
    }

    set_pwm_duty(0); //Example set duty cycle to 50% 
    ESP_LOGI("Temp_Ctrl_Task", "Stop temperature %d", temp_ctrl.heat_up_process_active);
    DBG_TC("Stop Heat Up Process %d", temp_ctrl.heat_up_process_active);


}

void TempCtrl_CalculateTemp(void)
{
    double raw_pid_output;
    uint32_t aux_duty;

    //DBG_TC("CalculateTemp");
    raw_pid_output = Temp_Compute_pid((double)temp_ctrl.target_temperature, (double)temp_ctrl.temp);

    if (raw_pid_output < 0.0) {
        raw_pid_output = 0.0;
    } else if (raw_pid_output > 1023.0) {
        raw_pid_output = 1023.0;
    }

    temp_ctrl.pid_output = raw_pid_output;
    aux_duty = (uint32_t)raw_pid_output;
    set_pwm_duty(aux_duty);
}

   
/* ---------------Signals---------------*/

uint32_t tempCtrlSignalWait(uint32_t signal, TickType_t timeout)
{
    uint32_t notifiedValue = 0;

    xTaskNotifyWait(0x00, signal, &notifiedValue, timeout);

    return notifiedValue;
}

esp_err_t tempCtrl_Requesting(void)
{
    esp_err_t result = ESP_FAIL;

    temp_ctrl.pid_timer = xTimerCreate("PID_Timer",
                         pdMS_TO_TICKS(150), pdTRUE, NULL, TempCtrl_PID_Callback);

    if (temp_ctrl.pid_timer == NULL) {
        ESP_LOGE("Temp_Ctrl", "Failed to create PID timer");
        return ESP_FAIL;
    }

    
    result = ESP_OK;

    return result;
}

esp_err_t tempCtrl_Releasing(void)
{
    esp_err_t result = ESP_FAIL;
    /*STOP and DELEATE timers*/
    /* Release resourses used like gpio spi uart etc*/

    //Stop PWM
    result = set_pwm_duty(0);
    if (result != ESP_OK) {
        ESP_LOGE("Temp_Ctrl_Releasing", "Failed to stop PWM: %s", esp_err_to_name(result));
        return result;
    }
    ESP_LOGI("Temp_Ctrl_Releasing", "PWM stopped successfully");
    
    return result ;
} 

 void TempCtrl_SetState(TempCtrlState state)
{
    //TODO 
    temp_ctrl.state = state;
}

/*------------------------Callbacks----------------------------*/

void TempCtrl_PID_Callback(TimerHandle_t xTimer)
{
    // This callback will be called every 100 ms
    // Notify the task only when heat-up process is active and controller is running.
    if (temp_ctrl.heat_up_process_active && temp_ctrl.state == TEMP_CTRL_START)
    {
        //DBG_TC("Callback SET_TEMP");
        xTaskNotify(temp_ctrl.taskHandle, TEMP_CTRL_SIGNAL_SET_TEMP, eSetBits);
    }
}

