

#include "Temp_Sensing.h"



void tempSensing_Requesting(void);


typedef struct {
    TempSensingState state;
    // Add other members as needed, e.g., timers, callbacks
} TempSensing_t;


static TempSensing_t temp_sensing = {
    .state = TEMP_SENSING_UNDEFINED,
    // Initialize other members as needed
};



void Temp_Sensing_Init(void){

    ESP_LOGI("Temp_sensing", "INIT");
   
    
    esp_err_t ret = init_spi_bus();
    spi_device_handle_t max6675;
    ret = add_max6675_device(&max6675);

    //NOT SHURE BUT MABY NEED SEMAPHORE OR MUTEX

    temp_sensing.state = TEMP_SENSING_POWER_OFF;
    
    xTaskCreate(Temp_Sensing_Task, "Temp_Sensing_Task_Task", 2048, NULL, 1, NULL);
}

void Temp_Sensing_Request(void){

    //Resources needed for the module -> timers , submodules , callbaks

    //need of semaphore or mutex
    //chck if the semaphore is available
    // if yes set the application callbacks 
        //signalsSet (signarl request)
        temp_sensing.state = TEMP_SENSING_RQUESTING;

}

void Temp_Sensing_Task(void){

     
    while (1) {
               
        switch (temp_sensing.state)
        {
        case TEMP_SENSING_POWER_OFF:
            
            ESP_LOGI("Temp_sensing", "STATE: POWER_OFF");

            //SignalWait wait for the request signal

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


void Test_temperature_sensing(){

    //initialize spi
    esp_err_t ret = init_spi_bus();
    spi_device_handle_t max6675;
    ret = add_max6675_device(&max6675);

    uint16_t new_temperature = 0;
     
    while (1) {
               
        
        //read the temperature from the amplifier of the sensro (MAX6675)
        float current_temperature = read_max6675(max6675); // temperature in Celsius
        if (current_temperature >= 0) {
            ESP_LOGI("MAIN", "Temperature: %.2f°C", current_temperature);
        } else {
            ESP_LOGE("MAIN", "Failed to read temperature");
        }
    }
}


/* -------------------- Sensing functions ------------------------*/


void tempSensing_Requesting(void)
{

}