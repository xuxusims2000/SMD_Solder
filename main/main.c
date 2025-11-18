
#include "main.h"

typedef enum MainApp_State_e{
    MAINAPP_POWER_OFF,
    MAINAPP_INITIALIZE_AND_START,
    MAINAPP_WAIT,
    MAINAPP_STOP_RELEASE_AND_RESET
} MainAppState_t;

typedef struct MainApp_s
{
    SolderingManagerState   state;
    esp_err_t               result;

    SMD_Manager_Configuration_t   SMD_ManagerConfig;

} MainApp_t;

MainApp_t mainApp = {
    .state = POWER_OFF
};



static esp_err_t MainApp_InitializeAndStart();
void Manager_SMD_UpdateTemperature_Timer_Callback (TimerHandle_t xTimer);

static void MainApp_SMD_Manager_OperationCompleteCallback(SMD_Manager_Result_t result);



void app_main(void)
{

  #ifdef TEST
  ESP_LOGI("TEST", "TEST MODE"); 
  test_function();
    
  #else

    while (1) {

        switch (mainApp.state)
        {
            case MAINAPP_POWER_OFF:
                mainApp.state = MAINAPP_INITIALIZE_AND_START;
                break;
            
            case MAINAPP_INITIALIZE_AND_START:

                result = MainApp_InitializeAndStart();
            
            default:
                break;
            }
            ESP_LOGI("MAIN", "Starting Soldering Manager...");
            vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1000 milliseconds
            break;
    }

    xTaskCreate(Manager_SMD_Task, "Manager_SMD Task", 2048, NULL, 1, NULL);

    #endif
}

esp_err_t MainApp_InitializeAndStart(){

    esp_err_t result = ESP_FAIL;

    mainApp.SMD_ManagerConfig.callbacks.OperationCompleteCallback = MainApp_SMD_Manager_OperationCompleteCallback; // Assign appropriate callback

    // Initialize SMD Manager
    
    
    result = SMD_Manager_Request(&mainApp.SMD_ManagerConfig);

    return result;
}




void Manager_SMD_Requesting(void){

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


}

void Manager_SMD_Task(){


    while(1){

        switch (mainSolder.state) {
            case POWER_OFF:
                // Handle POWER_OFF state
                ESP_LOGI("MAIN_SOLDER", "State: POWER_OFF");
                mainSolder.state = REQUESTING; // Example transition
                break;

            case REQUESTING:
                // Handle REQUESTING state
                ESP_LOGI("MAIN_SOLDER", "State: REQUESTING");

            // Request of the TEMPERATURE SENCE module has to call Request() functions

                Manager_SMD_Requesting();

                break;

            case REQUESTED:
                // Handle REQUESTED state
                ESP_LOGI("MAIN_SOLDER", "State: REQUESTED");

                Manager_SMD_Starting();

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


        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }


}

esp_err_t Manager_SMD_Starting(){

    esp_err_t result = ESP_FAIL;

    // Start  resources needed for the modules 

   
    
    /* -----------Start Temperature Sensing-------------*/ 
    //if ( result == ESP_OK )      
    //{
       result = Temp_Sensing_Start();
        if ( result == ESP_OK)
        {
            ESP_LOGI("Manager_SMD_Starting", "Temperature Sensing Started Successfully");
            result = ESP_OK;
            vTaskDelay(pdMS_TO_TICKS(10));  
        }
        else
         {
            ESP_LOGE("Manager_SMD_Starting", "Failed to Start Temperature Sensing");
            vTaskDelay(pdMS_TO_TICKS(10));  
         }
    //}  
   // mainSolder.state = REQUESTED; // Transition to the next state

    return result;
}

/*------------------Callbaks--------------------*/

void Manager_SMD_UpdateTemperature_Timer_Callback (TimerHandle_t xTimer){

    // Code to execute when the timer expires
    ESP_LOGI("Timer_Callback", "Manager_SMD_UpdateTemperature_Timer_Callback executed");

    //Manager_SMD_SignalSet(MANAGER_SMD_SIGNAL_UPDATE_TEMPERATURE);

}

void MainApp_SMD_Manager_OperationCompleteCallback(SMD_Manager_Result_t result){

    ESP_LOGI("MainApp_SMD_Manager_OperationCompleteCallback", "Operation Complete Callback executed with result: %d", result);

}