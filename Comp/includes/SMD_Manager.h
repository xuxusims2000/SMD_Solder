

// SMD_Manager.h
#ifndef SMD_MANAGER_H
#define SMD_MANAGER_H


#include <stdio.h>

#include <stdint.h>
#include <unistd.h>
#include <sys/lock.h>
#include <sys/param.h>
#include "esp_log.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"

#include "Display_Manager.h"
#include "Temp_Sensing.h"
#include "Temp_Ctrl.h"


//Signal and Defines for the SMD Manager Module




typedef enum {
    UNDDEFINED,
    POWER_OFF,
    REQUESTING,
    REQUESTED,
    IDLE,
    SOLDERING,
    RELAXED,  
    RELEASING
} SolderingManagerState;

typedef enum SMDManager_Result_e
{
	DOSEMANAGER_RESULT_UNDEFINED,
	DOSEMANAGER_RESULT_REQUEST,
	DOSEMANAGER_RESULT_START,
	DOSEMANAGER_RESULT_STOP,
	DOSEMANAGER_RESULT_RELEASE,
	DOSEMANAGER_RESULT_OPERATION_OK,
} SMDManager_Result_t;


typedef struct SMDManager_Callbacks_s
{
    void (*OperationCompleteCallback)(SMDManager_Result_t result);
    void (*ErrorCallback)(int errorCode);
    // Other callback function pointers...
} SMDManager_Callbacks_t;

typedef struct SMDManager_Configuration_s
{
    SMDManager_Callbacks_t callbacks;
} SMDManager_Configuration_t;


void SMDManager_Task(void *pvParameters);

void SMDManager_Init(void);
esp_err_t SMDManager_Request(SMDManager_Configuration_t* config);
esp_err_t SMDManager_Start(void);
esp_err_t SMDManager_Stop(void);
esp_err_t SMDManager_Release(void);



#endif // SMD_MANAGER_H
