
#include "main.h"



void Manager_SMD_UpdateTemperature_Timer_Callback (TimerHandle_t xTimer);

Manager_SMD mainSolder = {
    .state = POWER_OFF,
    .Manager_SMD_UpdateTemperature_Timer = NULL
};


void app_main(void)
{

  #ifdef TEST 
  test_function();

  #else
    esp_log_level_set("MAIN", ESP_LOG_INFO); // Set log level for MAIN tag
    ESP_LOGI("MAIN", "Starting Manager_SMD Task");

//shold do all the initializations here

    // Temperature Sensor Initialization
    Temp_Sensing_Init();
    // Temperature Control Initialization
    Temp_Ctrl_Init();
    // Display Initialization
    Display_Init();

    xTaskCreate(Manager_SMD_Task, "Manager_SMD Task", 2048, NULL, 1, NULL);

    #endif
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
        Temperature_Sensing_Request();


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




void Manager_SMD_Starting(){

    // Start or initialize resources needed for the module

    if (mainSolder.Manager_SMD_UpdateTemperature_Timer != NULL) {
        if (xTimerStart(mainSolder.Manager_SMD_UpdateTemperature_Timer, 0) != pdPASS) {
            ESP_LOGE("TIMER", "Failed to start Manager_SMD_UpdateTemperature_Timer");
        } else {
            ESP_LOGI("TIMER", "Manager_SMD_UpdateTemperature_Timer started successfully");
        }
    } else {
        ESP_LOGE("TIMER", "Manager_SMD_UpdateTemperature_Timer is NULL");
    }

    mainSolder.state = REQUESTED; // Transition to the next state
}

/*------------------Callbaks--------------------*/

void Manager_SMD_UpdateTemperature_Timer_Callback (TimerHandle_t xTimer){

    // Code to execute when the timer expires
    ESP_LOGI("Timer_Callback", "Manager_SMD_UpdateTemperature_Timer_Callback executed");

    //Manager_SMD_SignalSet(MANAGER_SMD_SIGNAL_UPDATE_TEMPERATURE);


}