


#include "SMD_Manager.h"

/*============================== Defines ==============================*/

#define SMD_MANAGER_SIGNAL_REQUESTED   (1 << 0)
#define SMD_MANAGER_SIGNAL_RELEASE     (1 << 1)
#define SMD_MANAGER_SIGNAL_START       (1 << 2)
#define SMD_MANAGER_SIGNAL_STOP        (1 << 3)

/*============================== Static Prototypes ==============================*/

static esp_err_t SMDManager_Requesting(void);
static esp_err_t SMDManager_Starting(void);
static uint32_t SMDManager_SignalWait(uint32_t signal, uint32_t timeout);

void Manager_SMD_UpdateTemperature_Timer_Callback (TimerHandle_t xTimer);

/*============================== Private Global Variables ==============================*/

typedef struct {

    TaskHandle_t                     taskHandle;
    SemaphoreHandle_t                xSemaphore;

    SolderingManagerState   state;
    TimerHandle_t           Manager_SMD_UpdateTemperature_Timer;

    SMDManager_Configuration_t   config;

    float                   temperature;
 
} Manager_SMD;


Manager_SMD mainSolder = {
    .state = UNDDEFINED,
    .Manager_SMD_UpdateTemperature_Timer = NULL
};



void SMDManager_Init(void)
{
    if (mainSolder.state == UNDDEFINED) {
        ESP_LOGI("SMD_Manager", "Initializing...");

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

    if( xSemaphoreTake(mainSolder.xSemaphore, 0) != pdTRUE ){
        ESP_LOGW("SMDManager_Request", "Semaphore not available");
        return ESP_FAIL;
    }
    ESP_LOGI("SMDManager_Request", "Semaphore taken, requesting SMD Manager");

    memcpy(&mainSolder.config, config, sizeof(SMDManager_Configuration_t));
    mainSolder.state = REQUESTING;
    
    xTaskNotify(&mainSolder.taskHandle, SMD_MANAGER_SIGNAL_REQUESTED, eSetBits);

    result = ESP_OK;
    return result;
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

    if (mainSolder.state == REQUESTED) {
        
        xTaskNotify(mainSolder.taskHandle, SMD_MANAGER_SIGNAL_START, eSetBits);
        return ESP_OK;
    }
    else {
        ESP_LOGE("SMDManager_Start", "Invalid state (%d)", mainSolder.state);
        return ESP_FAIL;
    }

    
    return ESP_OK;
}

esp_err_t SMDManager_Starting(void){
    esp_err_t result = ESP_FAIL;

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

                if ( lv_scr_act() != ui_Screen1 ) {
                    ESP_LOGI("MAIN_SOLDER", "State: IDLE");
                    DisplayManager_SetState(DISPLAY_MANAGER_IDLE);
            
                }

                mainSolder.temperature = TempSensing_GetTemperature();
                DisplayManager_SetTemperature(mainSolder.temperature);
                vTaskDelay(pdMS_TO_TICKS(2000)); // Delay for 2000 mseconds



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

   //Resources needed for the module -> timers , submodules , callbaks

   /*------------Define timers---------------*/

    
    
    mainSolder.Manager_SMD_UpdateTemperature_Timer = xTimerCreate(
                    "Manager_SMD_UpdateTemperature_Timer", // Name 
                     pdMS_TO_TICKS(5000),                  // Period of the timer
                     pdTRUE,                             // Auto-reload        
                     ( void * ) 0,                      // Timer ID
                    Manager_SMD_UpdateTemperature_Timer_Callback);



    /* ----------Temperature_Sensing--------------*/
        // Callbaks for the module
        
        //Temp_Sensing_Request(); mising config with callbacks


    /* ----------Temperature_Contrl---------------*/
        // Callbaks for the module

    /* ..........Display_Manager ----------------*/
        // Callbaks for the module

    return ESP_OK;
}

void Manager_SMD_UpdateTemperature_Timer_Callback (TimerHandle_t xTimer){

    // Code to execute when the timer expires
    ESP_LOGI("Timer_Callback", "Manager_SMD_UpdateTemperature_Timer_Callback executed");

    //Manager_SMD_SignalSet(MANAGER_SMD_SIGNAL_UPDATE_TEMPERATURE);

}