


#include "Temp_Ctrl.h"


#define TEMP_CTRL_SIGNAL_REQUEST_COMPLETE   (1 << 0)  // Signal to request temperature control
#define TEMP_CTRL_SIGNAL_START_COMPLETE     (1 << 1)  // Signal to indicate temperature control 
#define TEMP_CTRL_SIGNAL_STOP_COMPLETE      (1 << 2)  // Signal to indicate temperature
#define TEMP_CTRL_SIGNAL_RELEASE_COMPLETE   (1 << 3)  // Signal to release temperature control


typedef struct TestTempCtrl_e
{
    TaskHandle_t            taskHandle;
    TempCtrl_Configuration_t   config;

    spi_device_handle_t max6675;


    /* data */  
}TestTempCtrl_t;

TestTempCtrl_t testTempCtrl ;

void TestTemp_Ctrl_Task_1(void *pvParameters); // Test task function prototype
void TestTemp_Ctrl_Task_PID_TUNE(void *pvParameters); // Test task function prototype

uint32_t TestTemp_Ctrl_SignalWait(uint32_t signal, uint32_t timeout);

void TestTemp_Ctrl_OperationCompleteCallback(TempCtrl_Result_t result);

// -------------------- Test functions ------------------------

void Test_TempCtrl_1(void){

    ESP_LOGI("Test_Temp_Ctrl_1", "Starting Temperature Control Test 1");

    TempCtrl_Init();
    //esp_err_t ret = init_spi_bus();
    
    //ret = add_max6675_device(&testTempCtrl.max6675);

    //uint16_t new_temperature = 0;

    /* Create and start 'test task thread'*/
    xTaskCreate(TestTemp_Ctrl_Task_1, "Test_Temp_Ctrl_Task_1", 2048, NULL, 1, &testTempCtrl.taskHandle);

}

void Test_Temp_Ctrl_PID_TUNE(void){

    ESP_LOGI("Test_Temp_Ctrl_PID_TUNE", "Starting Temperature Control PID Tuning Test");

    TempCtrl_Init();

    /* Create and start 'test task thread'*/
    xTaskCreate(TestTemp_Ctrl_Task_PID_TUNE, "Test_Temp_Ctrl_Task_PID_TUNE", 2048, NULL, 1, &testTempCtrl.taskHandle);

}


//-----------------------TASKS----------------------------------------------------//

void TestTemp_Ctrl_Task_1(void *pvParameters) // Test task function prototype
{
    ESP_LOGI("TestTemp_Ctrl_Task_1", "--------------Started-----------------");
    
    /*callbacs*/
        
    testTempCtrl.config.callbacks.OperationCompleteCallback = TestTemp_Ctrl_OperationCompleteCallback;  

    for(uint8_t i=0; i<2; i++)
    {
        ESP_LOGI("TestTemp_Ctrl_Task_1", "Requesting Temperature Control...");

        Temp_Ctrl_Request(&testTempCtrl.config);
        TestTemp_Ctrl_SignalWait( TEMP_CTRL_SIGNAL_REQUEST_COMPLETE,  portMAX_DELAY);
        ESP_LOGI("TestTemp_Ctrl_Task_1", "Temperature Control Request OK");

        ESP_LOGI("TestTemp_Ctrl_Task_1", "Starting Temperature Control...");
        Temp_Ctrl_Start();
        TestTemp_Ctrl_SignalWait( TEMP_CTRL_SIGNAL_START_COMPLETE,  portMAX_DELAY);
        ESP_LOGI("TestTemp_Ctrl_Task_1", "Temperature Control Start OK");

       while(1)
          {

            for ( uint8_t j = 0; j < 5; j++) {
                ESP_LOGI("TestTemp_Ctrl_Task_1", "Temperature Control Running... %d", j);
                
                TempCtrl_UpdateTemperature(25);

                TempCtrl_SetTemperature(40);

                vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1000 mseconds
            }

            vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1000 mseconds

          }
        vTaskDelay(pdMS_TO_TICKS(5000)); // Delay for 5000 mseconds

        ESP_LOGI("TestTemp_Ctrl_Task_1", "Stopping Temperature Control...");
        Temp_Ctrl_Stop();
        TestTemp_Ctrl_SignalWait( TEMP_CTRL_SIGNAL_STOP_COMPLETE,  portMAX_DELAY);
        ESP_LOGI("TestTemp_Ctrl_Task_1", "Temperature Control Stop OK");

        ESP_LOGI("TestTemp_Ctrl_Task_1", "Releasing Temperature Control...");
        Temp_Ctrl_Release();
        TestTemp_Ctrl_SignalWait( TEMP_CTRL_SIGNAL_RELEASE_COMPLETE,  portMAX_DELAY);
        ESP_LOGI("TestTemp_Ctrl_Task_1", "Temperature Control Release OK");

        ESP_LOGI("TestTemp_Ctrl_Task_1", "-------------END----------------");

        vTaskDelay(pdMS_TO_TICKS(2000)); // Delay for 2000 mseconds before next iteration
    }   

}


void TestTemp_Ctrl_Task_PID_TUNE(void *pvParameters) // Test task function prototype
{
    float current_temperature = 0.0;
    uint32_t target_temp = 50; // Target temperature for PID tuning
    ESP_LOGI("TestTemp_Ctrl_Task_PID_TUNE", "--------------Started-----------------");
    
    /*callbacs*/
        
    testTempCtrl.config.callbacks.OperationCompleteCallback = TestTemp_Ctrl_OperationCompleteCallback;  

    ESP_LOGI("TestTemp_Ctrl_Task_PID_TUNE", "Requesting Temperature Control...");

    Temp_Ctrl_Request(&testTempCtrl.config);
    TestTemp_Ctrl_SignalWait( TEMP_CTRL_SIGNAL_REQUEST_COMPLETE,  portMAX_DELAY);
    ESP_LOGI("TestTemp_Ctrl_Task_PID_TUNE", "Temperature Control Request OK");

    ESP_LOGI("TestTemp_Ctrl_Task_PID_TUNE", "Starting Temperature Control...");
    Temp_Ctrl_Start();
    TestTemp_Ctrl_SignalWait( TEMP_CTRL_SIGNAL_START_COMPLETE,  portMAX_DELAY);
    ESP_LOGI("TestTemp_Ctrl_Task_PID_TUNE", "Temperature Control Start OK");

   
    // PID Tuning loop
   while(1)
   {
        //ESP_LOGI("TestTemp_Ctrl_Task_PID_TUNE", "Setting Temperature to %lu°C", temp);
        current_temperature = read_max6675(testTempCtrl.max6675); // temperature in Celsius
        printf(">Desired Temperature to %lu°C\n", target_temp);
        printf(">Current Temperature: %.2f°C\n", current_temperature);
        TempCtrl_UpdateTemperature(current_temperature);
        TempCtrl_SetTemperature(target_temp);
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Hold each temperature for 10 seconds
   }

    ESP_LOGI("TestTemp_Ctrl_Task_PID_TUNE", "Stopping Temperature Control...");
    Temp_Ctrl_Stop();
    TestTemp_Ctrl_SignalWait( TEMP_CTRL_SIGNAL_STOP_COMPLETE,  portMAX_DELAY);
    ESP_LOGI("TestTemp_Ctrl_Task_PID_TUNE", "Temperature Control Stop OK");

    ESP_LOGI("TestTemp_Ctrl_Task_PID_TUNE", "Releasing Temperature Control...");
    Temp_Ctrl_Release();
    TestTemp_Ctrl_SignalWait( TEMP_CTRL_SIGNAL_RELEASE_COMPLETE,  portMAX_DELAY);
    ESP_LOGI("TestTemp_Ctrl_Task_PID_TUNE", "Temperature Control Release OK");

    ESP_LOGI("TestTemp_Ctrl_Task_PID_TUNE", "-------------END----------------");

}

/* ----------------- Callback Functions -------------------------*/

void TestTemp_Ctrl_OperationCompleteCallback(TempCtrl_Result_t result){

     ESP_LOGI("TestSMD_Manager_Callback", "Operation Complete Callback with result: %d", result);
    
    switch (result)
    {
    case TEMP_CTRL_RESULT_REQUEST:
        xTaskNotify(testTempCtrl.taskHandle, TEMP_CTRL_SIGNAL_REQUEST_COMPLETE, eSetBits);
        break;
    case TEMP_CTRL_RESULT_START:
        xTaskNotify(testTempCtrl.taskHandle, TEMP_CTRL_SIGNAL_START_COMPLETE, eSetBits);
        break;
    case TEMP_CTRL_RESULT_STOP:
        xTaskNotify(testTempCtrl.taskHandle, TEMP_CTRL_SIGNAL_STOP_COMPLETE, eSetBits);
        break;
    case TEMP_CTRL_RESULT_RELEASE:
        xTaskNotify(testTempCtrl.taskHandle, TEMP_CTRL_SIGNAL_RELEASE_COMPLETE, eSetBits);
        break;
    default:
        break;
    }

}

uint32_t TestTemp_Ctrl_SignalWait(uint32_t signal, uint32_t timeout){
    uint32_t notifyValue = 0;
    TickType_t ticks;

 if (timeout == portMAX_DELAY) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS(timeout);
        if (ticks == 0) ticks = 1; // avoid zero wait if ms < tick period
    }

    xTaskNotifyWait(0x00,          // Don't clear any bits on entry
                    signal,       // Clear the bits being waited for on exit
                    &notifyValue, // Receives the notified value
                    ticks);     // Wait time
    return notifyValue;
}   















