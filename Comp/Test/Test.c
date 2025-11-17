// useful.c
#include "Test.h"

#include "ILI9341_screen.h"
#include "TestTemp_Sensing.h"
#include "TestDisplay_Manager.h"

TaskHandle_t task1Handle = NULL;
TaskHandle_t task2Handle = NULL;

void Test_task_manaement_signals(void);
void task1(void *pvParameters);
void task2(void *pvParameters);

void Test_smd_manager_1(void);
void SMD_Manager_Test_Task_1(void *pvParameters);
TaskHandle_t testSMDManagerTaskHandle = NULL;

void test_function() {
    // Test implementation goes here

    //-------Structur tests----------------
    //Test_task_manaement_signals();


    /*-------Module temperature sensing tests----------------*/
    //Test_temperature_sensing_0();
    Test_temperature_sensing_1();
    //Test_temperature_sensing_2();

    /*----------Module display----------------------*/
    //Test_main_screen();
    Test_display_manager_1();
    //Test_display_manager_2();


    /*----------Module SMD Manager----------------------*/
    Test_smd_manager_1();

    
    //Test_PID_control_();
}







/* Test sust to try signals and state diagram */


void Test_task_managment_signals(void){

    // Example test code for task management signals
    printf("Starting task management signals test...\n");

    xTaskCreate(task1, "Task1", 2048, NULL, 2, &task1Handle);
    xTaskCreate(task2, "Task2", 2048, NULL, 2, &task2Handle);

    printf("Task app_main() reached the end.\n");
}

void task1(void *pvParameters){

    Test_taskA_manager state = STATE1A;
    uint32_t notifyValue =0;

    while(1){
         switch (state)
         {
         case STATE1A:
            printf("Task1: Waiting for signal 0x01 or 0x02...\n");

            // Wait for any notification bits
            xTaskNotifyWait(
                0,            // don’t clear on entry
                ULONG_MAX,    // clear all bits on exit
                &notifyValue, // store value
                portMAX_DELAY // wait forever
            );

            if (notifyValue & 0x01) {
                    printf("Task1: Got SIGNAL 2 (0x01)\n");
                    state = STATE2A;
                } else if (notifyValue & 0x02) {
                    printf("Task1: Got SIGNAL 3 (0x02)\n");
                    state = STATE3A;
                }

            break;
        case STATE2A:
                /* code */
                vTaskDelay(pdMS_TO_TICKS(5000));
                break;  
        case STATE3A:
                /* code */
                break;
         
         default:
            break;
         }

    
    }
}

void task2(void *pvParameters){

    Test_taskB_manager state = STATE1B;
    while(1){
         switch (state)
         {
         case STATE1B:
            printf("Task2: Delaying for 5000 ms before sending SIGNAL A (0x01)\n");
            vTaskDelay(pdMS_TO_TICKS(5000));
                printf("Task2: Sending SIGNAL A (0x01)\n");
                xTaskNotify(task1Handle, 0x01, eSetBits);
                state = STATE2B;
            break;
        case STATE2B:
            vTaskDelay(pdMS_TO_TICKS(5000));
            break;  
        case STATE3B:
 
            break;
         
         default:
            break;
         }

    
    }
}

void Test_smd_manager_1(void){

    // Example test code for SMD Manager
    printf("Starting SMD Manager test...\n");

    // Temperature Sensor Initialization
    Temp_Sensing_Init();

    // Display Initialization
    Display_Init();

    /* Create and start 'test task thread'*/
    xTaskCreate(SMD_Manager_Test_Task_1, "SMD_Manager_Test_Task_1", 2048, NULL, 1, &testSMDManagerTaskHandle);


    printf("SMD Manager test completed.\n");
}

void SMD_Manager_Test_Task_1(void *pvParameters){

    ESP_LOGI("SMD_Manager_Test_Task", "--------------Started-----------------");
         
    for ( uint8_t i = 0; i < 2; i++ ) {

        DisplayManager_Request(&testDisplayManager.config);
        Temp_Sensing_Request(&testTempSensing.config);
     
        while(1)
          {
                
            vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1000 mseconds
          }
        }
}

void Test_SMD_Manager_SignalWait(uint32_t signal, uint32_t timeout_ms)
{
 uint32_t notifiedValue = 0;
    TickType_t ticks;

    if (timeout_ms == portMAX_DELAY) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS(timeout_ms);
        if (ticks == 0) ticks = 1; // avoid zero wait if ms < tick period
    }

    // Wait for notification bits (clear on exit specified by 'signal')
    xTaskNotifyWait(0x00, signal, &notifiedValue, ticks);

    return notifiedValue;
}   

void Test_SMD_Manager_OperationCompleteCallback(SMDManager_Result_t result)
{
    ESP_LOGI("SMD_Manager_OperationCompleteCallback", "Operation completed with result: %d", result);
}