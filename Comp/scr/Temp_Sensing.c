

#include "Temp_Sensing.h"


#define TEMP_SENSING_SIGNAL_REQUESTED   (1 << 0)  // Signal to request temperature sensing
#define TEMP_SENSING_SIGNAL_RLEASE      (1 << 1)  // Signal to indicate temperature sensing 
#define TEMP_SENSING_SIGNAL_START       (1 << 2)  // Signal to indicate temperature sensing 
#define TEMP_SENSING_SIGNAL_STOP        (1 << 3)  // Signal to release temperature sensing

esp_err_t tempSensing_Requesting(void);

uint32_t tempSensingSignalWait(uint32_t signal, uint32_t timeout);

esp_err_t tempSensing_Releasing(void);

typedef struct {
    TempSensingState        state;

    uint32_t                signals;  // Bitmask for signals

    TaskHandle_t            taskHandle;
    SemaphoreHandle_t       TempSensing_xSemaphoreHandle; //Defines a semaphore to manage the resource
    
    float                temperature;

} TempSensing_t;




/* Private variables --------------------------------------------------*/

static TempSensing_t temp_sensing = {
    .state = TEMP_SENSING_UNDEFINED,
    .signals = 0,
    .taskHandle= NULL,
    .TempSensing_xSemaphoreHandle = NULL,
    // Initialize other members as needed
};



void Temp_Sensing_Init(void){

   
    if (   (temp_sensing.state == TEMP_SENSING_UNDEFINED) 
        && (temp_sensing.taskHandle == NULL) )
    {
        ESP_LOGI("Temp_sensing", "INIT");
        
       //Auria de mirar si necesita mutex o semaforo
       
        esp_err_t ret = init_spi_bus();
        spi_device_handle_t max6675;
        ret = add_max6675_device(&max6675);
        if (ret != ESP_OK) {
            ESP_LOGE("Temp_sensing", "Failed to initialize SPI for MAX6675");
            return;
        }

        // May need semaphore  
        temp_sensing.TempSensing_xSemaphoreHandle = xSemaphoreCreateBinary(); //Ceates a binary semaphore before using it
        if (temp_sensing.TempSensing_xSemaphoreHandle == NULL) {
            ESP_LOGE("Temp_sensing", "Failed to create semaphore");
            return;
        }

        xSemaphoreGive(temp_sensing.TempSensing_xSemaphoreHandle); //Initialy the semaphore is available


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

esp_err_t Temp_Sensing_Request(TempSensing_Configuration_t* config){

    esp_err_t result = ESP_FAIL;
    BaseType_t xResult ;
    
    //Resources needed for the module -> timers , submodules , callbaks
   
    //chck if the semaphore is available
    
    xResult =  xSemaphoreTake(temp_sensing.TempSensing_xSemaphoreHandle, 0);
    ESP_LOGI("TasTemp_Sensing_Request", "Try to request temperature: %d",xResult);
    if ( xResult == pdTRUE) //Try to take the semaphore, wait 0 ticks if not available
    {
        ESP_LOGI("Temp_Sensing_Request", "Semaphore taken immediately!"); // if yes set the application callbacks
        //Probably here goes a callback for interruptuions to the application

        //memcpy(&temp_sensing.config, config, sizeof(TempSensing_Configuration_t));

        //change state
        temp_sensing.state = TEMP_SENSING_RQUESTING;
        xTaskNotify(temp_sensing.taskHandle, TEMP_SENSING_SIGNAL_REQUESTED, eSetBits);
    } 
    else
    {
        ESP_LOGE("Temp_Sensing_Request", "Semaphore not available");
    }

   return result = ESP_OK;
}

esp_err_t Temp_Sensing_Start(void){

    esp_err_t result = ESP_FAIL;

    if ( temp_sensing.state == TEMP_SENSING_REQUESTED)
    {
        xTaskNotify(temp_sensing.taskHandle, TEMP_SENSING_SIGNAL_START, eSetBits);
        result = ESP_OK;
    }
    else
    {
        ESP_LOGE("Temp_Sensing_Start", "Error: Temp_Sensing not in REQUESTED state");
    }
    return result;
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
    if ( xSemaphoreGive(temp_sensing.TempSensing_xSemaphoreHandle) != pdTRUE)
    {
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
    else
    {
        ESP_LOGE("Temp_Sensing_Release", "Error: Semaphore not released");
    }
}


float TempSensing_GetTemperature(spi_device_handle_t handle)
{
    temp_sensing.temperature = read_max6675(handle); // Replace NULL with actual device handle if needed


    return temp_sensing.temperature; // Replace with actual temperature reading logic
}


void Temp_Sensing_Task(void *pvParameters){

     esp_err_t result = ESP_FAIL;
     uint32_t signal = 0;

    while (1) {
               
                
        switch (temp_sensing.state)
        {
        case TEMP_SENSING_POWER_OFF:
            
            ESP_LOGI("Temp_Sensing_Task", "STATE: POWER_OFF");

            //SignalWait wait for the request signal
            tempSensingSignalWait( TEMP_SENSING_SIGNAL_REQUESTED,  portMAX_DELAY);

            break;

        case TEMP_SENSING_RQUESTING:
        
            ESP_LOGI("Temp_Sensing_Task", "STATE: REQUESTING");

            result = tempSensing_Requesting();
            if ( result == ESP_OK)
            {
                temp_sensing.state = TEMP_SENSING_REQUESTED;
                ESP_LOGI("Temp_Sensing_Task", "REQUESTED");
            }
            else
            {
                ESP_LOGE("Temp_Sensing_Task", "REQUESTING ERROR -> RELEASING");
                xSemaphoreGive(temp_sensing.TempSensing_xSemaphoreHandle);
                temp_sensing.state = TEMP_SENSING_RELEASING;
                
            }
            break;

        case TEMP_SENSING_REQUESTED:
            ESP_LOGI("Temp_Sensing_Task", "STATE: REQUESTED");
            //Wait for start or release signal
            {
                signal = tempSensingSignalWait( TEMP_SENSING_SIGNAL_START | TEMP_SENSING_SIGNAL_RLEASE,  portMAX_DELAY);
                
                if (signal & TEMP_SENSING_SIGNAL_START)
                {
                    temp_sensing.state = TEMP_SENSING_START;
                    ESP_LOGI("Temp_Sensing_Task", "STATE: START");
                }
                else if (signal & TEMP_SENSING_SIGNAL_RLEASE)
                {
                    temp_sensing.state = TEMP_SENSING_RELEASING;
                    ESP_LOGI("Temp_Sensing_Task", "STATE: RELEASING");
                }
            }  

            break;

        case TEMP_SENSING_START:
            
            signal = tempSensingSignalWait( TEMP_SENSING_SIGNAL_STOP ,  portMAX_DELAY);
            
            if (signal & TEMP_SENSING_SIGNAL_STOP)
            {
                temp_sensing.state = TEMP_SENSING_REQUESTED;
                ESP_LOGI("Temp_Sensing_Task", "STATE: REQUESTED");
            }
           

            break;

        case TEMP_SENSING_RELEASING:
            
            ESP_LOGI("Temp_Sensing_Task", "STATE: RELEASING");

            result = tempSensing_Releasing();
            if ( result == ESP_OK)
            {
                temp_sensing.state = TEMP_SENSING_POWER_OFF;
                ESP_LOGI("Temp_Sensing_Task", "STATE: POWER_OFF");
            }
            else
            {
                ESP_LOGE("Temp_Sensing_Task", "RELEASING ERROR -> POWER_OFF");
                temp_sensing.state = TEMP_SENSING_POWER_OFF;   
            }
            break;
        default:
            ESP_LOGE("Temp_Sensing_Task", "STATE: UNDEFINED");
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


esp_err_t tempSensing_Requesting(void)
{
    esp_err_t result = ESP_FAIL;

    
    result = ESP_OK;

    return result;
}

esp_err_t tempSensing_Releasing(void)
{
    esp_err_t result = ESP_FAIL;
    /*STOP and DELEATE timers*/
    /* Release resourses used like gpio spi uart etc*/
    return result ;
}




uint32_t tempSensingSignalWait(uint32_t signal, uint32_t timeout)
{
    uint32_t notifiedValue = 0;

    printf("Waiting for signal %" PRIu32 " with timeout %" PRIu32 " ms\n", signal, timeout);

    xTaskNotifyWait(0x00, signal, &notifiedValue, pdMS_TO_TICKS(timeout));

    return notifiedValue;
}