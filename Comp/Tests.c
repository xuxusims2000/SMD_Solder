

//In this file diferent tests can be run just uncoment the one you want to test
#include "Tests.h"
#include "ILI9341_screen.h"

TaskHandle_t task1Handle = NULL;
TaskHandle_t task2Handle = NULL;

void Test_task_manaement_signals(void);
void task1(void *pvParameters);
void task2(void *pvParameters);

void test_function() {
    // Test implementation goes here

    //Test_main_screen();
    Test_task_manaement_signals();
    //Test_temperature_sensing();
    //Test_PID_control_();
}


void Test_task_manaement_signals(void){

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
            
            break;  
        case STATE3B:
 
            break;
         
         default:
            break;
         }

    
    }
}