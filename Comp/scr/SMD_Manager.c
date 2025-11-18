


#include "SMD_Manager.h"



typedef struct {

    SolderingManagerState   state;
    TimerHandle_t           Manager_SMD_UpdateTemperature_Timer;

    float                   temperature;
 
} Manager_SMD;


Manager_SMD mainSolder = {
    .state = POWER_OFF,
    .Manager_SMD_UpdateTemperature_Timer = NULL
};






esp_err_t SMD_Manager_Request (void){

    esp_err_t result = ESP_FAIL;

    // Code to handle the request

    // For example, transition to REQUESTED state
    mainSolder.state = REQUESTED;
    result = ESP_OK;

    return result;
}