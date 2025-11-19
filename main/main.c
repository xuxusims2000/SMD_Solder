
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

    SMDManager_Configuration_t   SMD_ManagerConfig;

} MainApp_t;

MainApp_t mainApp = {
    .state = POWER_OFF
};



static esp_err_t MainApp_InitializeAndStart();
static esp_err_t MainApp_StopAndRelease();
static uint32_t MainAppSignalWait(uint32_t signal, uint32_t timeout);

static void MainApp_SMD_Manager_OperationCompleteCallback(SMDManager_Result_t result);



void app_main(void)
{

  #ifdef TEST
  ESP_LOGI("TEST", "TEST MODE"); 
  test_function();
    
  #else
    uint32_t signal;
    esp_err_t result;

    while (1) {

        switch (mainApp.state)
        {
            case MAINAPP_POWER_OFF:
                mainApp.state = MAINAPP_INITIALIZE_AND_START;
                break;
            
            case MAINAPP_INITIALIZE_AND_START:

                result = MainApp_InitializeAndStart();
                if (result == ESP_OK) {
                    ESP_LOGI("MAIN", "SMD Manager Initialized and Started successfully.");
                } else {
                    ESP_LOGE("MAIN", "Failed to Initialize and Start SMD Manager. Error: %d", result);
                }

                mainApp.state = MAINAPP_WAIT;
            break;

            case MAINAPP_WAIT:
                // Here you can implement any waiting logic if needed

                signal = MainAppSignalWait(0x01, 1000); // Example signal wait with 1 second timeout
                if (signal & 0x01) {
                    ESP_LOGI("MAIN", "Received stop signal.");
                    mainApp.state = MAINAPP_STOP_RELEASE_AND_RESET;
                }
                vTaskDelay(pdMS_TO_TICKS(1000)); // Just a placeholder delay
                break;
            
            case MAINAPP_STOP_RELEASE_AND_RESET:
                ESP_LOGI("MAIN", "Stopping and Releasing SMD Manager...");

                result = MainApp_StopAndRelease();
                if (result == ESP_OK) {
                    ESP_LOGI("MAIN", "SMD Manager Stopped and Released successfully.");
                } else {
                    ESP_LOGE("MAIN", "Failed to Stop and Release SMD Manager. Error: %d", result);
                }
                break;

            
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
    
    
    result = SMDManager_Request(&mainApp.SMD_ManagerConfig);

    return result;
}

esp_err_t MainApp_StopAndRelease(){

    esp_err_t result = ESP_FAIL;

    result = SMDManager_Stop();

    if (result != ESP_OK) {
        ESP_LOGE("MainApp_StopAndRelease", "Failed to stop SMD Manager. Error: %d", result);
        return result;
    }

    return result;
}



/*------------------Callbaks--------------------*/



void MainApp_SMD_Manager_OperationCompleteCallback(SMDManager_Result_t result){

    ESP_LOGI("MainApp_SMD_Manager_OperationCompleteCallback", "Operation Complete Callback executed with result: %d", result);

}


static uint32_t MainAppSignalWait(uint32_t signal, uint32_t timeout){

    uint32_t notifiedValue = 0;
    xTaskNotifyWait(0x00,          // Don't clear any bits on entry
                    ULONG_MAX,    // Clear all bits on exit
                    &notifiedValue, // Receives the notification value
                    pdMS_TO_TICKS(timeout)); // Wait time

    return (notifiedValue & signal);
}