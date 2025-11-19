


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
        //TemperatureSensing_Init();
        //TemperatureControl_Init();
        
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

    memcpy(&mainSolder.config, config, sizeof(SMDManager_Configuration_t));

    mainSolder.state = REQUESTING;
    xTaskNotify(&mainSolder.taskHandle, SMD_MANAGER_SIGNAL_REQUESTED, eSetBits);

    result = ESP_OK;
    return result;
}

void SMDManager_Task(void *pvParameters){


    esp_err_t result = ESP_FAIL;
    uint32_t signal;

    void (*OperationCompleteCallback)(SMDManager_Result_t result);

    while(1){

        switch (mainSolder.state) {

            case POWER_OFF:
                // Handle POWER_OFF state
                ESP_LOGI("MAIN_SOLDER", "State: POWER_OFF");
                SMDManager_SignalWait(SMD_MANAGER_SIGNAL_REQUESTED, portMAX_DELAY);

                break;

            case REQUESTING:
                // Handle REQUESTING state
                ESP_LOGI("MAIN_SOLDER", "State: REQUESTING");

            // Request of the TEMPERATURE SENCE module has to call Request() functions

                SMDManager_Requesting();

                break;

            case REQUESTED:
                // Handle REQUESTED state
                ESP_LOGI("MAIN_SOLDER", "State: REQUESTED");

                SMDManager_Starting();

                break;

            case SOLDERING:
                // Handle SOLDERING state
                ESP_LOGI("MAIN_SOLDER", "State: SOLDERING");
                break;

            case RELAXED:
                // Handle RELAXED state
                ESP_LOGI("MAIN_SOLDER", "State: RELAXED");
                break;

            case REALISING:
                // Handle REALISING state
                ESP_LOGI("MAIN_SOLDER", "State: REALISING");
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
                     pdMS_TO_TICKS(500),                  // Period of the timer
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