
 #ifndef TESTS_H
 #define TESTS_H
 
#include <stdio.h>
#include <stdint.h>
#include "esp_log.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "Useful.h"



 // #include "ILI9341_screen.h"
void test_function(void);

typedef enum { 
    STATE1A,
    STATE2A,
    STATE3A,
    STATE4A
} Test_taskA_manager;

typedef enum { 
    STATE1B,
    STATE2B,
    STATE3B,
    STATE4B
} Test_taskB_manager;




#endif /*TESTS_H*/
