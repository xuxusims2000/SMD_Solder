




#include "TestDisplay_Manager.h"

#define TEST_DISPLAY_MANAGER_SIGNAL_REQUEST_COMPLETE       (1 << 0)  // Signal to request display manager
#define TEST_DISPLAY_MANAGER_SIGNAL_START_COMPLETE         (1 << 1)  // Signal to indicate
#define TEST_DISPLAY_MANAGER_SIGNAL_STOP_COMPLETE          (1 << 2)  // Signal to indicate
#define TEST_DISPLAY_MANAGER_SIGNAL_RELEASE_COMPLETE       (1 << 3)  // Signal to release display manager

typedef struct TestDisplayManager_e
{
    TaskHandle_t            taskHandle;
    DisplayManager_Configuration_t   config;
    /* data */
}TestDisplayManager_t;

TestDisplayManager_t testDisplayManager ;

void Display_Manager_Test_Task_1(void *pvParameters); // Test task function prototype
void Display_Manager_Test_Task_2(void *pvParameters); // Test task function prototype

uint32_t TestDisplay_Manager_SignalWait(uint32_t signal, uint32_t timeout);

void TestDisplayManager_OperationCompleteCallback(DisplayManager_Result_t result);

// -------------------- Test functions ------------------------

void Test_display_manager_1(){

    ESP_LOGI("TEST", "Test_display_manager_1 started"); 
    DisplayManager_Init();

    /* Create and start 'test task thread'*/
    xTaskCreate(Display_Manager_Test_Task_1, "Display_Manager_Test_Task_1", 2048, NULL, 1, &testDisplayManager.taskHandle);

}

/*-----------------------TASKS----------------------------------------------------*/

void Display_Manager_Test_Task_1(void *pvParameters){

    ESP_LOGI("Display_Manager_Test_Task", "--------------Started-----------------");
    
    /*callbacs*/
        
        testDisplayManager.config.callbacks.OperationCompleteCallback = TestDisplayManager_OperationCompleteCallback;
    for(uint8_t i=0; i<2; i++){
                
          ESP_LOGI("Display_Manager_Task", "Time Test Iteration: %d", i);
          // Request display manager
          DisplayManager_Request(&testDisplayManager.config);
          vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100 mseconds
    
          TestDisplay_Manager_SignalWait( TEST_DISPLAY_MANAGER_SIGNAL_REQUEST_COMPLETE,  portMAX_DELAY);
    
          ESP_LOGI(" Display_Manager_Test_Task", "Request OK");
    
          DisplayManager_Start();
          vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100 mseconds
    
          TestDisplay_Manager_SignalWait( TEST_DISPLAY_MANAGER_SIGNAL_START_COMPLETE,  portMAX_DELAY);
    
          ESP_LOGI("Display_Manager_Test_Task", "Start OK");
    
          DisplayManager_Stop();
          vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100 mseconds
          TestDisplay_Manager_SignalWait( TEST_DISPLAY_MANAGER_SIGNAL_STOP_COMPLETE,  portMAX_DELAY);
          ESP_LOGI("Display_Manager_Test_Task", "Stop OK");
    
          DisplayManager_Release();
          vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100 mseconds
          TestDisplay_Manager_SignalWait( TEST_DISPLAY_MANAGER_SIGNAL_RELEASE_COMPLETE,  portMAX_DELAY);
          ESP_LOGI("Display_Manager_Test_Task", "Release OK");
    
          ESP_LOGI("Display_Manager_Test_Task", "-------------END----------------");
    
        
     }
        while(1)
          {
                vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1000 mseconds
          }
}

uint32_t TestDisplay_Manager_SignalWait(uint32_t signal, uint32_t timeout)
{
    uint32_t notifiedValue = 0;

    printf("Waiting for signal %" PRIu32 " with timeout %" PRIu32 " ms\n", signal, timeout);

    xTaskNotifyWait(0x00, signal, &notifiedValue, pdMS_TO_TICKS(timeout));

    return notifiedValue;
}

void TestDisplayManager_OperationCompleteCallback(DisplayManager_Result_t result)
{
    switch (result)
    {
    case DISPLAY_MANAGER_RESULT_REQUEST:
        ESP_LOGI("TestDisplayManager_Callback", "DISPLAY_MANAGER_RESULT_REQUEST received");
        xTaskNotify(testDisplayManager.taskHandle, TEST_DISPLAY_MANAGER_SIGNAL_REQUEST_COMPLETE, eSetBits);
        break;
    case DISPLAY_MANAGER_RESULT_START:
        ESP_LOGI("TestDisplayManager_Callback", "DISPLAY_MANAGER_RESULT_START received");
        xTaskNotify(testDisplayManager.taskHandle, TEST_DISPLAY_MANAGER_SIGNAL_START_COMPLETE, eSetBits);
        break;
    case DISPLAY_MANAGER_RESULT_STOP:
        ESP_LOGI("TestDisplayManager_Callback", "DISPLAY_MANAGER_RESULT_STOP received");
        xTaskNotify(testDisplayManager.taskHandle, TEST_DISPLAY_MANAGER_SIGNAL_STOP_COMPLETE, eSetBits);
        break;
    case DISPLAY_MANAGER_RESULT_RELEASE:
        ESP_LOGI("TestDisplayManager_Callback", "DISPLAY_MANAGER_RESULT_RELEASE received");
        xTaskNotify(testDisplayManager.taskHandle, TEST_DISPLAY_MANAGER_SIGNAL_RELEASE_COMPLETE, eSetBits);
        break;
    default:
        ESP_LOGW("TestDisplayManager_Callback", "Unknown result received: %d", result);
        break;
    }
}





