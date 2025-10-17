


#include "driver/spi_master.h"
#include "esp_err.h"          // Include for error handling
#include "esp_log.h"
#include "driver/gpio.h"
#include <string.h>





typedef enum {
    DISPLAY_MANAGER_UNDEFINED, 
    DISPLAY_MANAGER_POWER_OFF, 
    DISPLAY_MANAGER_RQUESTING,
    DISPLAY_MANAGER_REQUESTED,
    DISPLAY_MANAGER_START,
    DISPLAY_MANAGER_RELEASING
} DisplayManagerState;

typedef enum DisplayManager_Result_e {
    DISPLAY_MANAGER_RESULT_UNDEFINED ,
    DISPLAY_MANAGER_RESULT_REQUEST,
    DISPLAY_MANAGER_RESULT_START,
    DISPLAY_MANAGER_RESULT_STOP,
    DISPLAY_MANAGER_RESULT_RELEASE,
    DISPLAY_MANAGER_RESULT_OPERATION_OK
} DisplayManager_Result_t;

typedef struct DisplayManager_Callbacks_e {
    void (*OperationCompleteCallback)(DisplayManager_Result_t result);

} DisplayManager_Callbacks_t;

typedef struct DisplayManager_Configuration_e {
   DisplayManager_Callbacks_t   callbacks;

} DisplayManager_Configuration_t;

// Declare functions

void Display_Manager_Task(void *pvParameters);

void DisplayManager_Init(void);
esp_err_t DisplayManager_Request(DisplayManager_Configuration_t* config);
esp_err_t DisplayManager_Start(void);
void DisplayManager_Stop(void);
esp_err_t DisplayManager_Release(void);

