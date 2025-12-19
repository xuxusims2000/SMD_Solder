


#include "SMD_Manager.h"

/*============================== Defines ==============================*/

#define SMD_MANAGER_SIGNAL_REQUESTED        (1 << 0)
#define SMD_MANAGER_SIGNAL_RELEASE          (1 << 1)
#define SMD_MANAGER_SIGNAL_START            (1 << 2)
#define SMD_MANAGER_SIGNAL_STOP             (1 << 3)
#define SMD_MANAGER_SIGNAL_UPDATE_TEMP      (1 << 4)
#define SMD_MANAGER_SIGNAL_SET_TEMP         (1 << 5)
#define SMD_MANAGER_SIGNAL_SETTINGS         (1 << 6)
#define SMD_MANAGER_SIGNAL_SOLDER           (1 << 7)
#define SMD_MANAGER_SIGNAL_KEY_MORE_TEMP    (1 << 8)
#define SMD_MANAGER_SIGNAL_KEY_LESS_TEMP    (1 << 9)
#define SMD_MANAGER_SIGNAL_KEY_HEAT         (1 << 10)
#define SMD_MANAGER_SIGNAL_HEAT             (1 << 11)


/*============================== Static Prototypes ==============================*/

static esp_err_t SMDManager_Requesting(void);
static esp_err_t SMDManager_Starting(void);
static uint32_t SMDManager_SignalWait(uint32_t signal, uint32_t timeout);

void Manager_SMD_UpdateTemperature_Timer_Callback (TimerHandle_t xTimer);
void SMDManager_HeatUp_timer_Callback(void* arg);

/*============================== Private Global Variables ==============================*/

typedef struct {

    TaskHandle_t                     taskHandle;
    SemaphoreHandle_t                xSemaphore;

    SolderingManagerState   state;
    TimerHandle_t           Manager_SMD_UpdateTemperature_Timer; // FreeRTOS timer handle
    esp_timer_create_args_t    heatUp_timer_args; // esp_timer arguments
    esp_timer_handle_t         heatUp_timer_handle; // esp_timer handle


    SMDManager_Configuration_t   config;

    DisplayManager_Configuration_t    displayManagerConfig;
    TempSensing_Configuration_t       tempSensingConfig;

    float                   temperature;
    float                   target_temperature;


 
} Manager_SMD;


Manager_SMD mainSolder = {
    .state = UNDDEFINED,
    .Manager_SMD_UpdateTemperature_Timer = NULL
};



void SMDManager_Init(void)
{
    if (mainSolder.state == UNDDEFINED && mainSolder.taskHandle == NULL) {
        ESP_LOGI("SMD_Manager", "INIT.");

        DisplayManager_Init();
        Temp_Sensing_Init();
        //TemperatureControl_Init();

        mainSolder.xSemaphore = xSemaphoreCreateBinary();
        if(mainSolder.xSemaphore == NULL) {
            ESP_LOGE("SMD_Manager", "Failed to create semaphore");
            return;
        }
        xSemaphoreGive(mainSolder.xSemaphore);
        
        mainSolder.state = POWER_OFF;
        
        // Create the SMD Manager Task
        xTaskCreate(SMDManager_Task, 
                    "SMDManagerTask", 
                    4096, 
                    NULL,
                    5, 
                    &mainSolder.taskHandle);

        
    }
}


esp_err_t SMDManager_Request (SMDManager_Configuration_t* config){

    esp_err_t result = ESP_FAIL;
    BaseType_t xResult ;

    xResult = xSemaphoreTake(mainSolder.xSemaphore, pdMS_TO_TICKS(1000));
    if ( xResult == pdTRUE) //Try to take the semaphore, wait 1000 ms if not available
    {
        ESP_LOGI("SMDManager_Request", "SMD Manager requested.");

        memcpy(&mainSolder.config, config, sizeof(SMDManager_Configuration_t));
        mainSolder.state = REQUESTING;
        xTaskNotify(mainSolder.taskHandle, SMD_MANAGER_SIGNAL_REQUESTED, eSetBits);
    }
    else
    {
        ESP_LOGE("SMDManager_Request", "Semaphore not available");
        return result;
    }


    return result= ESP_OK;
}

esp_err_t SMDManager_Release(void){

    if (xSemaphoreGive(mainSolder.xSemaphore) != pdTRUE) {
        ESP_LOGE("SMDManager_Release", "Failed to give semaphore");
        return ESP_FAIL;
    }
    else {        
        if (mainSolder.state == REQUESTED ) {
            mainSolder.state = RELEASING;
            xTaskNotify(mainSolder.taskHandle, SMD_MANAGER_SIGNAL_RELEASE, eSetBits);
            ESP_LOGI("SMDManager_Release", "SMD Manager release OK");
            return ESP_OK;
        } 
        else {
            ESP_LOGW("SMDManager_Release", "Invalid state (%d)", mainSolder.state);
            return ESP_FAIL;
        }     
        
    }

    return ESP_OK;
}

esp_err_t SMDManager_Start(void){

    esp_err_t result = ESP_FAIL;
    
    if (mainSolder.state == REQUESTED) {
        
        xTaskNotify(mainSolder.taskHandle, SMD_MANAGER_SIGNAL_START, eSetBits);
      result = ESP_OK;
        ESP_LOGI("SMDManager_Start", "Starting SMD Manager");
    }
    else {
        ESP_LOGE("SMDManager_Start", "Invalid state (%d)", mainSolder.state);
     
    }

  
    return result;
}

esp_err_t SMDManager_Starting(void){
    esp_err_t result = ESP_FAIL;

    /*---Timers starting ----------*/
    esp_timer_start_periodic(
        mainSolder.heatUp_timer_handle, 
        50000); // ms 

    result = Temp_Sensing_Start();
    vTaskDelay(pdMS_TO_TICKS(500)); // Delay for 500 mseconds
    if (result != ESP_OK) {
        ESP_LOGE("SMDManager_Starting", "Temp_Sensing_Start failed");
        return result;
    }

    result = DisplayManager_Start();
    vTaskDelay(pdMS_TO_TICKS(500)); // Delay for 500 mseconds
    if (result != ESP_OK) {
        ESP_LOGE("SMDManager_Starting", "DisplayManager_Start failed");
        return result;
    }


    return result;
}

esp_err_t SMDManager_Stop(void){

    if (mainSolder.state == SOLDERING) {
        xTaskNotify(mainSolder.taskHandle, SMD_MANAGER_SIGNAL_STOP, eSetBits);
        ESP_LOGI("SMDManager_Stop", "Stopping SMD Manager");
        return ESP_OK;
    } 
    else {
        ESP_LOGE("SMDManager_Stop", "Invalid state (%d)", mainSolder.state);
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

void SMDManager_Task(void *pvParameters){


    esp_err_t result = ESP_FAIL;
    uint32_t signal;

    void (*OperationCompleteCallback)(SMDManager_Result_t result);

    while(1){

        switch (mainSolder.state) {

            case POWER_OFF:
                ESP_LOGI("MAIN_SOLDER", "State: POWER_OFF");
                SMDManager_SignalWait(SMD_MANAGER_SIGNAL_REQUESTED, portMAX_DELAY);
                ESP_LOGI("MAIN_SOLDER", "State: POWER_OFF out of wait");
                break;

            case REQUESTING:
                ESP_LOGI("MAIN_SOLDER", "State: REQUESTING");

                result = SMDManager_Requesting();
                if( result == ESP_OK){
                    mainSolder.state = REQUESTED;
                    if(mainSolder.config.callbacks.OperationCompleteCallback != NULL)
                        mainSolder.config.callbacks.OperationCompleteCallback(DOSEMANAGER_RESULT_REQUEST);
                }
                else {
                    ESP_LOGE("MAIN_SOLDER", "SMDManager_Requesting failed");
                    
                    if (xSemaphoreGive(mainSolder.xSemaphore) != pdTRUE) {
                        ESP_LOGE("MAIN_SOLDER", "Failed to give semaphore");
                    }
                    mainSolder.state = RELEASING;   
                }

                break;

            case REQUESTED:
                ESP_LOGI("MAIN_SOLDER", "State: REQUESTED");

                signal = SMDManager_SignalWait(SMD_MANAGER_SIGNAL_START | 
                                                SMD_MANAGER_SIGNAL_RELEASE, 
                                                portMAX_DELAY);

                if (signal & SMD_MANAGER_SIGNAL_START) {
                    result = SMDManager_Starting();
                    
                    if (result == ESP_OK) {
                        mainSolder.state = IDLE;

                        if (mainSolder.config.callbacks.OperationCompleteCallback != NULL){
                            mainSolder.config.callbacks.OperationCompleteCallback(DOSEMANAGER_RESULT_START);
                        }                    
                    } else {
                        ESP_LOGE("MAIN_SOLDER", "SMDManager_Starting failed");
                        mainSolder.state = RELEASING;
                    }
                }
                else if (signal & SMD_MANAGER_SIGNAL_RELEASE) {
                    mainSolder.state = RELEASING;
                }

                break;

            case IDLE:
                ESP_LOGI("MAIN_SOLDER", "State: IDLE");

                // Identify the current screen 
                if (DisplayManager_GetScreen() != ui_Screen1) {
                    ESP_LOGI("MAIN_SOLDER", "Switching to Home Screen");
                }
         
                
                
                mainSolder.temperature = TempSensing_GetTemperature();
                DisplayManager_SetTemperature(mainSolder.temperature);
                ESP_LOGI("Display_Manager_Test_Task", "Setting temperature to: %.2f °C",  mainSolder.temperature);
                vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 2000 mseconds

                
                signal = SMDManager_SignalWait(SMD_MANAGER_SIGNAL_STOP | 
                                                SMD_MANAGER_SIGNAL_RELEASE|
                                                SMD_MANAGER_SIGNAL_SOLDER|
                                                SMD_MANAGER_SIGNAL_SET_TEMP|
                                                SMD_MANAGER_SIGNAL_SETTINGS| 
                                                SMD_MANAGER_SIGNAL_UPDATE_TEMP,
                                                portMAX_DELAY);

                if (signal & SMD_MANAGER_SIGNAL_STOP) {
                    

                }
                else if (signal & SMD_MANAGER_SIGNAL_RELEASE) {
                    

                }
                else if (signal & SMD_MANAGER_SIGNAL_SOLDER) {
                   

                }
                else if (signal & SMD_MANAGER_SIGNAL_SET_TEMP) {
                    
                    mainSolder.state = SET_TEMP;

                }
                else if (signal & SMD_MANAGER_SIGNAL_SETTINGS) {
      

                }
                else if (signal & SMD_MANAGER_SIGNAL_UPDATE_TEMP) {

                }

                break;
            
            case SET_TEMP:
                // Identify the current screen 
                if (DisplayManager_GetScreen() != ui_Screen1) {  // MODIFY THE ACTUALL SCREEN WHE SCREENS ARE DEFINED
                    ESP_LOGI("MAIN_SOLDER", "Switching to Home Screen");
                }

                mainSolder.temperature = TempSensing_GetTemperature();
                TempCtrl_UpdateTemperature(mainSolder.temperature);
                DisplayManager_SetTemperature(mainSolder.temperature);
                ESP_LOGI("SMD_Manager_Task", "Setting temperature to: %.2f °C",  mainSolder.temperature);
                vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 2000 mseconds

                signal = SMDManager_SignalWait(SMD_MANAGER_SIGNAL_STOP | 
                                                SMD_MANAGER_SIGNAL_KEY_HEAT|
                                                SMD_MANAGER_SIGNAL_KEY_MORE_TEMP|
                                                SMD_MANAGER_SIGNAL_KEY_LESS_TEMP|
                                                SMD_MANAGER_SIGNAL_HEAT,
                                                portMAX_DELAY);

                if (signal & SMD_MANAGER_SIGNAL_STOP) {

                    ESP_LOGI("SMD_Manager_Task", "SIGNAL_STOP");
                    result = SMDManager_Stop();
                    if (result == ESP_OK) {
                        mainSolder.state = REQUESTED;
                        if (mainSolder.config.callbacks.OperationCompleteCallback != NULL)
                            mainSolder.config.callbacks.OperationCompleteCallback(DOSEMANAGER_RESULT_STOP);
                    } else {
                        ESP_LOGE("SMD_Manager_Task", "SMDManager_Stop failed");
                    }
                }
                else if (signal & SMD_MANAGER_SIGNAL_KEY_MORE_TEMP) {
                    mainSolder.target_temperature += 5;
                    ESP_LOGI("SMD_Manager_Task", "Temperature increased to: %.2f °C", mainSolder.temperature);

                }
                else if (signal & SMD_MANAGER_SIGNAL_KEY_LESS_TEMP) {
                    mainSolder.target_temperature -= 5;
                    ESP_LOGI("SMD_Manager_Task", "Temperature decreased to: %.2f °C", mainSolder.temperature);

                }
                else if (signal & SMD_MANAGER_SIGNAL_KEY_HEAT) {
                    
                    TempCtrl_SetState(TEMP_CTRL_SET_TEMP);
                    flag_to_start_hit_up = true;

                }
                else if (signal & SMD_MANAGER_SIGNAL_HEAT) {                
                    ESP_LOGI("SMD_Manager_Task", "SIGNAL HEAT received");

                    if (flag_to_start_hit_up) {
                        ESP_LOGI("SMD_Manager_Task", "Starting Heat Up Process");

                        TempCtrl_SetTemperature(mainSolder.target_temperature);
                       
                    }
                }

                break;
            
            case SOLDERING:
                ESP_LOGI("MAIN_SOLDER", "State: SOLDERING");
                
                signal = SMDManager_SignalWait(SMD_MANAGER_SIGNAL_STOP | 
                                                SMD_MANAGER_SIGNAL_RELEASE, 
                                                portMAX_DELAY);
                break;

            case RELAXED:
                // Handle RELAXED state
                ESP_LOGI("MAIN_SOLDER", "State: RELAXED");
                break;

            case RELEASING:
                // Handle RELEASING state
                ESP_LOGI("MAIN_SOLDER", "State: RELEASING");
                break;

            default:
                ESP_LOGW("MAIN", "Unknown State");
                break;
        }   


        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 1 second
    }


}

esp_err_t SMDManager_Requesting(void){

   esp_err_t result = ESP_FAIL;
    //Resources needed for the module -> timers , submodules , callbaks

   /*------------Define timers---------------*/

    mainSolder.Manager_SMD_UpdateTemperature_Timer = xTimerCreate(
                    "Manager_SMD_UpdateTemperature_Timer", // Name 
                     pdMS_TO_TICKS(5000),                  // Period of the timer
                     pdTRUE,                             // Auto-reload        
                     ( void * ) 0,                      // Timer ID
                    Manager_SMD_UpdateTemperature_Timer_Callback);



    mainSolder.heatUp_timer_args = (esp_timer_create_args_t) {
        .callback = &SMDManager_HeatUp_timer_Callback,
        .arg = NULL,
        .name = "HeatUpTimer"
    };

    esp_timer_create(&mainSolder.heatUp_timer_args, &mainSolder.heatUp_timer_handle);

    /* ..........Display_Manager ----------------*/
        // Callbaks for the module
        result = DisplayManager_Request(&mainSolder.displayManagerConfig);
        vTaskDelay(pdMS_TO_TICKS(500)); // Delay for 500 mseconds

    /* ----------Temperature_Sensing--------------*/
        // Callbaks for the module
        result = Temp_Sensing_Request(&mainSolder.tempSensingConfig);
        vTaskDelay(pdMS_TO_TICKS(500)); // Delay for 500 mseconds


    /* ----------Temperature_Contrl---------------*/
        // Callbaks for the module

    
        result = ESP_OK;
    return result;
}


uint32_t SMDManager_SignalWait(uint32_t signal, uint32_t timeout){
    uint32_t notifiedValue = 0;
    TickType_t ticks;

    if (timeout == portMAX_DELAY) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS(timeout);
        if (ticks == 0) ticks = 1; // avoid zero wait if ms < tick period
    }

    // Wait for notification bits (clear on exit specified by 'signal')
    xTaskNotifyWait(0x00, signal, &notifiedValue, ticks);

    return notifiedValue;
}

/*----------------Callback----------------------------*/

void Manager_SMD_UpdateTemperature_Timer_Callback (TimerHandle_t xTimer){

    // Code to execute when the timer expires
    ESP_LOGI("Timer_Callback", "Manager_SMD_UpdateTemperature_Timer_Callback executed");

    //Manager_SMD_SignalSet(MANAGER_SMD_SIGNAL_UPDATE_TEMPERATURE);

}

void SMDManager_HeatUp_timer_Callback(void* arg)
{
    if (mainSolder.state == SET_TEMP )
    {
        xTaskNotify(mainSolder.taskHandle, SMD_MANAGER_SIGNAL_HEAT, eSetBits);
    }
    ESP_LOGI("Callback", "Timer triggered! Time since boot: %lld us", esp_timer_get_time());
}