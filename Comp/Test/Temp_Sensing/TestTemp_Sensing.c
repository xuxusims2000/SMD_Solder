



#include "TestTemp_Sensing.h"


#define TEST_TEMP_SENSING_SIGNAL_REQUEST_COMPLETE       (1 << 0)  // Signal to request temperature sensing
#define TEST_TEMP_SENSING_SIGNAL_START_COMPLETE         (1 << 1)  // Signal to indicate temperature sensing 
#define TEST_TEMP_SENSING_SIGNAL_RELEASE_COMPLETE       (1 << 2)  // Signal to indicate temperature sensing 
#define TEST_TEMP_SENSING_SIGNAL_STOP_COMPLETE          (1 << 3)  // Signal to release temperature sensing

typedef struct TestTempSensing_e
{
    TaskHandle_t            taskHandle;
    TempSensing_Configuration_t   config;
    /* data */
}TestTempSensing_t;

TestTempSensing_t testTempSensing ;

void Temp_Sensing_Test_Task_1(void *pvParameters); // Test task function prototype
void Temp_Sensing_Test_Task_2(void *pvParameters); // Test task function prototype


uint32_t TestTemp_Sensing_SignalWait(uint32_t signal, uint32_t timeout);

void TestTempSensing_OperationCompleteCallback(TempSensing_Result_t result);

// -------------------- Test functions ------------------------

void Test_temperature_sensing_0(){

    //initialize spi
    esp_err_t ret = init_spi_bus();
    spi_device_handle_t max6675;
    ret = add_max6675_device(&max6675);

    uint16_t new_temperature = 0;
     
    while (1) {
               
        
        //read the temperature from the amplifier of the sensro (MAX6675)
        float current_temperature = read_max6675(max6675); // temperature in Celsius
        if (current_temperature >= 0) {
            ESP_LOGI("MAIN", "Temperature: %.2f°C", current_temperature);
        } else {
            ESP_LOGE("MAIN", "Failed to read temperature");
        }
    }
}

void Test_temperature_sensing_1(){

    ESP_LOGI("TEST", "Test_temperature_sensing_1 started"); 
    Temp_Sensing_Init();

    /* Create and start 'test task thread'*/
    xTaskCreate(Temp_Sensing_Test_Task_1, "Temp_Sensing_Test_Task_1", 2048, NULL, 1, &testTempSensing.taskHandle);

}

void Test_temperature_sensing_2(){

    ESP_LOGI("TEST", "Test_temperature_sensing_2 started"); 
    Temp_Sensing_Init();

    /* Create and start 'test task thread'*/
    xTaskCreate(Temp_Sensing_Test_Task_2, "Temp_Sensing_Test_Task_2", 2048, NULL, 1, &testTempSensing.taskHandle);

}



/*-----------------------TASKS----------------------------------------------------*/

void Temp_Sensing_Test_Task_1(void *pvParameters){

    ESP_LOGI("Temp_Sensing_Test_Task", "--------------Started-----------------");
    
    /*callbacs*/
        
        testTempSensing.config.callbacks.OperationCompleteCallback = TestTempSensing_OperationCompleteCallback; 


    for(uint8_t i=0; i<2; i++){
               
        // Request temperature sensing
        Temp_Sensing_Request(&testTempSensing.config);
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100 mseconds

        TestTemp_Sensing_SignalWait( TEST_TEMP_SENSING_SIGNAL_REQUEST_COMPLETE,  portMAX_DELAY);

        ESP_LOGE(" Temp_Sensing_Test_Task", "Request OK");

        Temp_Sensing_Start();
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100 mseconds

        TestTemp_Sensing_SignalWait( TEST_TEMP_SENSING_SIGNAL_START_COMPLETE,  portMAX_DELAY);

        ESP_LOGE("Temp_Sensing_Test_Task", "Start OK");

        Temp_Sensing_Stop();
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100 mseconds
        TestTemp_Sensing_SignalWait( TEST_TEMP_SENSING_SIGNAL_STOP_COMPLETE,  portMAX_DELAY);
        ESP_LOGE("Temp_Sensing_Test_Task", "Stop OK");

        Temp_Sensing_Release();
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100 mseconds
        TestTemp_Sensing_SignalWait( TEST_TEMP_SENSING_SIGNAL_RELEASE_COMPLETE,  portMAX_DELAY);
        ESP_LOGE("Temp_Sensing_Test_Task", "Release OK");

        ESP_LOGE("Temp_Sensing_Test_Task", "-------------END----------------");

        while(1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1000 mseconds
        }
    }
}

void Temp_Sensing_Test_Task_2(void *pvParameters) // Test task function prototype
{
    for(uint8_t i=0; i<2; i++)
    {        
        // Request temperature sensing
        Temp_Sensing_Request(&testTempSensing.config);
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100 mseconds

        TestTemp_Sensing_SignalWait( TEST_TEMP_SENSING_SIGNAL_REQUEST_COMPLETE,  portMAX_DELAY);

        ESP_LOGE(" Temp_Sensing_Test_Task", "Request OK");

        Temp_Sensing_Start();
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100 mseconds

        TestTemp_Sensing_SignalWait( TEST_TEMP_SENSING_SIGNAL_START_COMPLETE,  portMAX_DELAY);

        ESP_LOGE("Temp_Sensing_Test_Task", "Start OK");

        while(1)
        {
            

        ESP_LOGE("Temp_Sensing_Test_Task", "-------------END----------------");

        while(1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1000 mseconds
        }
    }
}



uint32_t TestTemp_Sensing_SignalWait(uint32_t signal, uint32_t timeout)
{
    uint32_t notifiedValue = 0;

    printf("Waiting for signal %" PRIu32 " with timeout %" PRIu32 " ms\n", signal, timeout);

    xTaskNotifyWait(0x00, signal, &notifiedValue, pdMS_TO_TICKS(timeout));

    return notifiedValue;
}


/*------------------Callbacks-------------------------*/

void TestTempSensing_OperationCompleteCallback(TempSensing_Result_t result)
{
    switch (result)
    {
    case TEMP_SENSING_RESULT_REQUEST:
        xTaskNotify(testTempSensing.taskHandle, TEST_TEMP_SENSING_SIGNAL_REQUEST_COMPLETE, eSetBits);
        break;
    case TEMP_SENSING_RESULT_START:
        xTaskNotify(testTempSensing.taskHandle, TEST_TEMP_SENSING_SIGNAL_START_COMPLETE, eSetBits);
        break;
    case TEMP_SENSING_RESULT_STOP:
        xTaskNotify(testTempSensing.taskHandle, TEST_TEMP_SENSING_SIGNAL_STOP_COMPLETE, eSetBits);
        break;
    case TEMP_SENSING_RESULT_RELEASE:
        xTaskNotify(testTempSensing.taskHandle, TEST_TEMP_SENSING_SIGNAL_RELEASE_COMPLETE, eSetBits);
        break;
    default:
        // Handle undefined result
        break;
    }
}


