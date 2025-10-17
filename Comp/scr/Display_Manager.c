

#include "Display_Manager.h"



#define DISPLAY_MANEGER_SIGNAL_REQUESTED   (1 << 0)  // Signal to request display manager
#define DISPLAY_MANEGER_SIGNAL_RLEASE      (1 << 1)  // Signal to indicate display manager 
#define DISPLAY_MANEGER_SIGNAL_START       (1 << 2)  // Signal to indicate display manager 
#define DISPLAY_MANEGER_SIGNAL_STOP        (1 << 3)  // Signal to release display manager



esp_err_t DisplayManager_Requesting(void);

uint32_t DisplayManagerSignalWait(uint32_t signal, uint32_t timeout);

esp_err_t DisplayManager_Releasing(void);

typedef struct {
    DisplayManagerState                state;

    DisplayManager_Configuration_t     config;

    uint32_t                        signals;  // Bitmask for signals

    TaskHandle_t                    taskHandle;
    SemaphoreHandle_t               DisplayManager_xSemaphoreHandle; //Defines a semaphore to manage the resource
    

} DisplayManager_t;

/* Private variables --------------------------------------------------*/

static DisplayManager_t display_manager = {
    .state = DISPLAY_MANAGER_UNDEFINED,
    .signals = 0,
    .taskHandle= NULL,
    .DisplayManager_xSemaphoreHandle = NULL,
    // Initialize other members as needed
};

void Display_Manager_Init(void){

   
    if (   (display_manager.state == DISPLAY_MANAGER_UNDEFINED) 
        && (display_manager.taskHandle == NULL) )
    {
        ESP_LOGI("Display_Manager", "INIT");
        
       //Auria de mirar si necesita mutex o semaforo
       
        xTaskCreate(Display_Manager_Task,
             "Display_Manager_Task", 
             4096, 
             NULL, 
             5, //!!!!!!!!!!!! Mirar prioritat
             &display_manager.taskHandle);
    }
}

esp_err_t DisplayManager_Request(DisplayManager_Configuration_t* config){

    esp_err_t result = ESP_FAIL;
    BaseType_t xResult ;
    
    //Resources needed for the module -> timers , submodules , callbaks

    //chck if the semaphore is available
    
    xResult =  xSemaphoreTake(display_manager.DisplayManager_xSemaphoreHandle, 0);
    //ESP_LOGI("TasDisplayManager_Request", "Try to request display manager: %d",xResult);
    if ( xResult == pdTRUE) //Try to take the semaphore, wait 0 ticks if not available
    {
        ESP_LOGI("DisplayManager_Request", "Semaphore taken immediately!"); // if yes set the application callbacks
        //Probably here goes a callback for interruptuions to the application

        memcpy(&display_manager.config, config, sizeof(DisplayManager_Configuration_t));

        //change state
        display_manager.state = DISPLAY_MANAGER_RQUESTING;
        xTaskNotify(display_manager.taskHandle, DISPLAY_MANEGER_SIGNAL_REQUESTED, eSetBits);
    } 
    else
    {
        ESP_LOGE("DisplayManager_Request", "Semaphore not available");
    }

   return result = ESP_OK;
}



































