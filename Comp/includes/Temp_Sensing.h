

#include "driver/spi_master.h"
#include "esp_err.h"          // Include for error handling
#include "esp_log.h" 
#include "driver/gpio.h"



// Define SPI pins
#define PIN_NUM_MISO GPIO_NUM_19
#define PIN_NUM_SCLK GPIO_NUM_18
#define PIN_NUM_CS   GPIO_NUM_5

typedef enum {
    TEMP_SENSING_UNDEFINED, 
    TEMP_SENSING_POWER_OFF, 
    TEMP_SENSING_RQUESTING,
    TEMP_SENSING_REQUESTED,
    TEMP_SENSING_START,
    TEMP_SENSING_RELEASING
} TempSensingState;



// Declare functions

void Temp_Sensing_Task(void *pvParameters);

void Temp_Sensing_Init(void);
void Temp_Sensing_Request(void);
esp_err_t Temp_Sensing_Start(void);
void Temp_Sensing_Stop(void);
void Temp_Sensing_Release(void);




esp_err_t init_spi_bus(void); 
esp_err_t add_max6675_device(spi_device_handle_t *handle);
float read_max6675(spi_device_handle_t handle);

