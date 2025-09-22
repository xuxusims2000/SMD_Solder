// useful.h


#ifndef USEFUL_H
#define USEFUL_H

#include <stdio.h>

#include <stdint.h>
#include "esp_log.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

uint32_t SignalWait (uint32_t signal, uint32_t timeout);
bool IsSignalSet (uint32_t signal , uint32_t flag);
void SetSignal (uint32_t signal);

#endif // USEFUL_H