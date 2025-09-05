
#include "main.h"

void Manager_SMD(){


    while(1){


        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }


}


void app_main(void)
{

  #ifdef TEST 
  test_function();

  #else
esp_log_level_set("MAIN", ESP_LOG_INFO); // Set log level for MAIN tag

xTaskCreate(Manager_SMD, "Manager_SMD Task", 2048, NULL, 1, NULL);




    



    #endif
}


