

// SMD_Manager.h
#ifndef SMD_MANAGER_H
#define SMD_MANAGER_H


#include <stdio.h>

#include <stdint.h>
#include <unistd.h>
#include "esp_log.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"






void Manager_SMD_Requesting(void);

void Manager_SMD_Task(void );

esp_err_t Manager_SMD_Starting(void);

typedef enum {
    POWER_OFF,
    REQUESTING,
    REQUESTED,
    SOLDERING,
    RELAXED,  
    REALISING
} SolderingManagerState;

typedef enum SMD_Manager_Result_e
{
	DOSEMANAGER_RESULT_UNDEFINED,
	DOSEMANAGER_RESULT_REQUEST,
	DOSEMANAGER_RESULT_START,
	DOSEMANAGER_RESULT_STOP,
	DOSEMANAGER_RESULT_RELEASE,
	DOSEMANAGER_RESULT_OPERATION_OK,
} SMD_Manager_Result_t;


typedef struct SMD_Manager_Callbacks_s
{
    void (*OperationCompleteCallback)(SMD_Manager_Result_t result);
    void (*ErrorCallback)(int errorCode);
    // Other callback function pointers...
} SMD_Manager_Callbacks_t;

typedef struct SMD_Manager_Configuration_s
{
    SMD_Manager_Callbacks_t callbacks;
} SMD_Manager_Configuration_t;

esp_err_t SMD_Manager_Request (void);


#endif // SMD_MANAGER_H