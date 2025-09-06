
#include "main.h"

    Manager_SMD mainSolder = {
        .state = POWER_OFF,
        .targetTemperature = 0
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

void Manager_SMD_Task(){


    while(1){

        switch (mainSolder.state) {
            case POWER_OFF:
                // Handle POWER_OFF state
                ESP_LOGI("MAIN", "State: POWER_OFF");
                mainSolder.state = REQUESTING; // Example transition
                break;

            case REQUESTING:
                // Handle REQUESTING state
                ESP_LOGI("MAIN", "State: REQUESTING");

            // Request of the TEMPERATURE SENCE module


                break;

            case REQUESTED:
                // Handle REQUESTED state
                ESP_LOGI("MAIN", "State: REQUESTED");
                break;

            case SOLDERING:
                // Handle SOLDERING state
                ESP_LOGI("MAIN", "State: SOLDERING");
                break;

            case RELAXED:
                // Handle RELAXED state
                ESP_LOGI("MAIN", "State: RELAXED");
                break;

            case REALISING:
                // Handle REALISING state
                ESP_LOGI("MAIN", "State: REALISING");
                break;

            default:
                ESP_LOGW("MAIN", "Unknown State");
                break;
        }   


        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }


}


