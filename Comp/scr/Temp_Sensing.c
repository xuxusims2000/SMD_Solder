

#include "Temp_Sensing.h"


#define TEMP_SENSING_SIGNAL_REQUESTED   (1 << 0)  // Signal to request temperature sensing
#define TEMP_SENSING_SIGNAL_RLEASE      (1 << 1)  // Signal to indicate temperature sensing 
#define TEMP_SENSING_SIGNAL_START       (1 << 2)  // Signal to indicate temperature sensing 
#define TEMP_SENSING_SIGNAL_STOP        (1 << 3)  // Signal to release temperature sensing

void tempSensing_Requesting(void);

uint32_t tempSensingSignalWait(uint32_t signal, uint32_t timeout);



typedef struct {
    TempSensingState state;

    uint32_t signals;  // Bitmask for signals

    TaskHandle_t taskHandle;
    SemaphoreHandle_t xSemaphoreHandle;
    // Add other members as needed, e.g., timers, callbacks
} TempSensing_t;




/* Private variables --------------------------------------------------*/

static TempSensing_t temp_sensing = {
    .state = TEMP_SENSING_UNDEFINED,
    .signals = 0,
    .taskHandle= NULL,
    .xSemaphoreHandle = NULL,
    // Initialize other members as needed
};



void Temp_Sensing_Init(void){

   
    if (   (temp_sensing.state == TEMP_SENSING_UNDEFINED) 
        && (temp_sensing.taskHandle == NULL) )
    {
        ESP_LOGI("Temp_sensing", "INIT");
        
        esp_err_t ret = init_spi_bus();
        spi_device_handle_t max6675;
        ret = add_max6675_device(&max6675);
        if (ret != ESP_OK) {
            ESP_LOGE("Temp_sensing", "Failed to initialize SPI for MAX6675");
            return;
        }

        // May need semaphore  

        /* Initialize state */
        temp_sensing.state = TEMP_SENSING_POWER_OFF;

        // May need mutex

        /* Create and launch Task*/
        xTaskCreate(Temp_Sensing_Task, 
                    "Temp_Sensing_Task",
                    2048, 
                    NULL, 
                    1, 
                    &temp_sensing.taskHandle);

    }
    
}

void Temp_Sensing_Request(void){

    //Resources needed for the module -> timers , submodules , callbaks

    //need of semaphore or mutex
    //chck if the semaphore is available
    // if yes set the application callbacks 
        //signalsSet (signarl request)
    temp_sensing.state = TEMP_SENSING_RQUESTING;
    xTaskNotify(temp_sensing.taskHandle, TEMP_SENSING_SIGNAL_REQUESTED, eSetBits);
   

}

void Temp_Sensing_Start(void){


    if ( temp_sensing.state == TEMP_SENSING_REQUESTED)
    {
        xTaskNotify(temp_sensing.taskHandle, TEMP_SENSING_SIGNAL_START, eSetBits);
    }
    else
    {
        ESP_LOGE("Temp_Sensing_Start", "Error: Temp_Sensing not in REQUESTED state");
    }

}

void Temp_Sensing_Stop(void){

    if( temp_sensing.state == TEMP_SENSING_START )
    {
         xTaskNotify(temp_sensing.taskHandle, TEMP_SENSING_SIGNAL_STOP, eSetBits);
    }
    else
    {
        ESP_LOGE("Temp_Sensing_Stop", "Error: Temp_Sensing not in START state");
    }

}

void Temp_Sensing_Release(void)
{
    /*Gestionar semaforo*/

    if (temp_sensing.state == TEMP_SENSING_REQUESTED)
    {
        temp_sensing.state = TEMP_SENSING_RELEASING;
        xTaskNotify(temp_sensing.taskHandle, TEMP_SENSING_SIGNAL_RLEASE, eSetBits);
        ESP_LOGI("Temp_Sensing_Release", "Temperature Sensing Release OK.");

    }
    else
    {
        ESP_LOGE("Temp_Sensing_Release", "Temperature Sensing Release ERROR.");
    }
}

void Temp_Sensing_Task(void *pvParameters){

     
    while (1) {
               
                
        switch (temp_sensing.state)
        {
        case TEMP_SENSING_POWER_OFF:
            
            ESP_LOGI("Temp_sensing", "STATE: POWER_OFF");

            //SignalWait wait for the request signal
            tempSensingSignalWait( TEMP_SENSING_SIGNAL_REQUESTED,  portMAX_DELAY);

            break;

        case TEMP_SENSING_RQUESTING:
        
            ESP_LOGI("Temp_sensing", "STATE: REQUESTING");

            tempSensing_Requesting();


            break;

        case TEMP_SENSING_REQUESTED:
        /* code */
            break;

        case TEMP_SENSING_RELEASING:
            /* code */
            break;

        
        default:
            break;
        } 

    }
}

/*--------------------------------------------------------------------------- SPI ----------------------------------------------------------------------------*/ 



// TAG for logging
static const char *TAG = "MAX6675";

esp_err_t init_spi_bus() {
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = -1,          // Not used for MAX6675
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,        // Not used
        .quadhd_io_num = -1,        // Not used
        .max_transfer_sz = 16,      // MAX6675 only transfers 16 bits
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SPI bus initialized successfully");
    return ESP_OK;
}

esp_err_t add_max6675_device(spi_device_handle_t *handle) {
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 4000000,   // 4 MHz
        .mode = 0,                   // SPI mode 0
        .spics_io_num = PIN_NUM_CS,  // CS pin
        .queue_size = 1,             // Transactions queued
    };

    esp_err_t ret = spi_bus_add_device(SPI2_HOST, &devcfg, handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add MAX6675 device: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "MAX6675 device added successfully");
    return ESP_OK;
}

float read_max6675(spi_device_handle_t handle) {
    spi_transaction_t trans = {
        .flags = SPI_TRANS_USE_RXDATA, // Use internal RX buffer
        .length = 16,                 // 16 bits transfer
    };

    esp_err_t ret = spi_device_transmit(handle, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transaction failed: %s", esp_err_to_name(ret));
        return -1.0; // Indicate an error
    }

    uint16_t raw_data = (trans.rx_data[0] << 8) | trans.rx_data[1];

    if (raw_data & 0x04) {
        ESP_LOGW(TAG, "No thermocouple connected!");
        return -1.0; // Indicate an error
    }

    float temperature = (raw_data >> 3) * 0.25;
    ESP_LOGI(TAG, "Temperature read: %.2f°C", temperature);
    return temperature;
}



/* -------------------- Sensing functions ------------------------*/


void tempSensing_Requesting(void)
{

}




uint32_t tempSensingSignalWait(uint32_t signal, uint32_t timeout)
{
    uint32_t notifiedValue = 0;

    printf("Waiting for signal %" PRIu32 " with timeout %" PRIu32 " ms\n", signal, timeout);

    xTaskNotifyWait(0x00, signal, &notifiedValue, pdMS_TO_TICKS(timeout));

    return notifiedValue;
}