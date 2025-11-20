


#include "TestSMD_Manager.h"


#define TEST_SMD_MANAGER_SIGNAL_REQUEST_COMPLETE       (1 << 0)  // Signal to request temperature sensing
#define TEST_SMD_MANAGER_SIGNAL_START_COMPLETE         (1 << 1)  // Signal to indicate temperature sensing 
#define TEST_SMD_MANAGER_SIGNAL_STOP_COMPLETE          (1 << 2)  // Signal to indicate temperature sensing 
#define TEST_SMD_MANAGER_SIGNAL_RELEASE_COMPLETE       (1 << 3)  // Signal to release temperature sensing


typedef struct TestSMDManager_e
{
    TaskHandle_t            taskHandle;
    SMDManager_Configuration_t   config;
    /* data */
}TestSMDManager_t;

TestSMDManager_t testSMDManager ;

void TestSMD_Manager_Task_1(void *pvParameters); // Test task function prototype
void TestSMD_Manager_Task_2(void *pvParameters); // Test task function prototype

uint32_t TestSMD_Manager_SignalWait(uint32_t signal, uint32_t timeout);

void TestSMD_Manager_OperationCompleteCallback(SMDManager_Result_t result);



// -------------------- Test functions ------------------------

void Test_smd_manager_1(void){

    ESP_LOGI("Test_smd_manager_1", "Starting SMD Manager Test 1");

    SMDManager_Init();

    /* Create and start 'test task thread'*/
    xTaskCreate(TestSMD_Manager_Task_1, "TestSMD_Manager_Task_1", 2048, NULL, 1, &testSMDManager.taskHandle);

    ESP_LOGI("Test_smd_manager_1", "SMD Manager Test 1 Completed");
}




//-----------------------TASKS----------------------------------------------------//

void TestSMD_Manager_Task_1(void *pvParameters) // Test task function prototype
{
    ESP_LOGI("TestSMD_Manager_Task_1", "--------------Started-----------------");
    
    /*callbacs*/
        
    testSMDManager.config.callbacks.OperationCompleteCallback = TestSMD_Manager_OperationCompleteCallback;  

    for(;;)
    {
        ESP_LOGI("TestSMD_Manager_Task_1", "Requesting SMD Manager...");
        SMDManager_Request(&testSMDManager.config);
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100 mseconds

        TestSMD_Manager_SignalWait( TEST_SMD_MANAGER_SIGNAL_REQUEST_COMPLETE,  portMAX_DELAY);
        ESP_LOGI("TestSMD_Manager_Task_1", "SMD Manager Request OK");

        ESP_LOGI("TestSMD_Manager_Task_1", "Starting SMD Manager...");
        SMDManager_Start();
        TestSMD_Manager_SignalWait( TEST_SMD_MANAGER_SIGNAL_START_COMPLETE,  portMAX_DELAY);
        ESP_LOGI("TestSMD_Manager_Task_1", "SMD Manager Start OK");

        while(1){
            vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1000 mseconds

        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // Delay for 5000 mseconds

        ESP_LOGI("TestSMD_Manager_Task_1", "Stopping SMD Manager...");
        SMDManager_Stop();
        TestSMD_Manager_SignalWait( TEST_SMD_MANAGER_SIGNAL_STOP_COMPLETE,  portMAX_DELAY);
        ESP_LOGI("TestSMD_Manager_Task_1", "SMD Manager Stop OK");

        ESP_LOGI("TestSMD_Manager_Task_1", "Releasing SMD Manager...");
        SMDManager_Release();
        TestSMD_Manager_SignalWait( TEST_SMD_MANAGER_SIGNAL_RELEASE_COMPLETE,  portMAX_DELAY);
        ESP_LOGI("TestSMD_Manager_Task_1", "SMD Manager Release OK");

        ESP_LOGI("TestSMD_Manager_Task_1", "-------------END----------------");

        vTaskDelay(pdMS_TO_TICKS(2000)); // Delay for 2000 mseconds before next iteration
    }


}


void TestSMD_Manager_OperationCompleteCallback(SMDManager_Result_t result){

    ESP_LOGI("TestSMD_Manager_Callback", "Operation Complete Callback with result: %d", result);

    switch(result){
        case DOSEMANAGER_RESULT_REQUEST:
            xTaskNotify(testSMDManager.taskHandle, TEST_SMD_MANAGER_SIGNAL_REQUEST_COMPLETE, eSetBits);
            break;
        case DOSEMANAGER_RESULT_START:
            xTaskNotify(testSMDManager.taskHandle, TEST_SMD_MANAGER_SIGNAL_START_COMPLETE, eSetBits);
            break;
        case DOSEMANAGER_RESULT_STOP:
            xTaskNotify(testSMDManager.taskHandle, TEST_SMD_MANAGER_SIGNAL_STOP_COMPLETE, eSetBits);
            break;
        case DOSEMANAGER_RESULT_RELEASE:
            xTaskNotify(testSMDManager.taskHandle, TEST_SMD_MANAGER_SIGNAL_RELEASE_COMPLETE, eSetBits);
            break;
        default:
            ESP_LOGW("TestSMD_Manager_Callback", "Unknown result in callback: %d", result);
            break;
    }
}
