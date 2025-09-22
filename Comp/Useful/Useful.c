// useful.c
#include "Useful.h"


uint32_t SignalWait (uint32_t signal, uint32_t timeout) {
    
    uint32_t notifiedValue = 0;

    // Implementation of waiting for a signal with a timeout
    printf("Waiting for signal %lu with timeout %lu ms\n", signal, timeout);

    xTaskNotifyWait(0x00, signal, &notifiedValue, pdMS_TO_TICKS(timeout));

 return 0; // Return 0 on success, or appropriate error code
}


bool IsSignalSet (uint32_t signal , uint32_t flag) {

    // Implementation to check if a signal is set
    if (signal == flag){
        return true;
    }
    else{
        return false;
    }
}

void SetSignal (uint32_t signal){
    // Implementation to set a signal
    printf("Setting signal %lu\n", signal);
    xTaskNotify( NULL, signal, eSetBits ); // Notify the current task
}
