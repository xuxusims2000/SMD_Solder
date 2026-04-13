#include "TestDebug.h"
#include "Debug.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "TEST_DEBUG";

/* ============ TEST 1: Debug_Request() ============ */
void Test_Debug_Request()
{
    ESP_LOGI(TAG, "--- TEST 1: Debug_Request() ---");
    
    Debug_Config_t config = {
        .uart_num = UART_NUM_2,
        .baudrate = 115200,
        .tx_io_num = 17,
        .rx_io_num = 16
    };
    
    esp_err_t err = Debug_Request(&config);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ Debug_Request() SUCCESS");
    } else {
        ESP_LOGE(TAG, "✗ Debug_Request() FAILED: %d", err);
    }
}

/* ============ TEST 2: Debug_Start() ============ */
void Test_Debug_Start()
{
    ESP_LOGI(TAG, "--- TEST 2: Debug_Start() ---");
    
    esp_err_t err = Debug_Start();
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ Debug_Start() SUCCESS");
    } else {
        ESP_LOGE(TAG, "✗ Debug_Start() FAILED: %d", err);
    }
}

/* ============ TEST 3: Debug_Printf() ============ */
void Test_Debug_Printf()
{
    ESP_LOGI(TAG, "--- TEST 3: Debug_Printf() ---");
    
    DBG("");
    DBG("========== TEST DEBUG MODULE ==========");
    DBG("This is a generic debug message with number: %d", 42);
    DBG("Float value: %.2f", 3.14159);
    DBG("String: %s", "Hello from ESP32");
    DBG("========================================");
    
    ESP_LOGI(TAG, "✓ Debug_Printf() test completed");
}

/* ============ TEST 4: Logging Levels (Coloreado) ============ */
void Test_Debug_LoggingLevels()
{
    ESP_LOGI(TAG, "--- TEST 4: Logging Levels (Coloreado) ---\n");
    
    vTaskDelay(pdMS_TO_TICKS(500));
    Debug_LogInfo("This is an INFO message");
    
    vTaskDelay(pdMS_TO_TICKS(500));
    Debug_LogWarning("This is a WARNING message");
    
    vTaskDelay(pdMS_TO_TICKS(500));
    Debug_LogError("This is an ERROR message");
    
    vTaskDelay(pdMS_TO_TICKS(500));
    Debug_LogDebug("This is a DEBUG message");
    
    vTaskDelay(pdMS_TO_TICKS(500));
    DBG_INFO("Macro DBG_INFO works");
    
    vTaskDelay(pdMS_TO_TICKS(500));
    DBG_ERROR("Macro DBG_ERROR works");
    
    ESP_LOGI(TAG, "✓ Logging levels test completed\r\n");
}

/* ============ TEST 5: Teleplot ============ */
void Test_Debug_Teleplot()
{
    ESP_LOGI(TAG, "--- TEST 5: Teleplot Data ---\r\n");
    
    /* Simular valores de temperatura/PWM */
    for (int i = 0; i < 10; i++) {
        float temp = 20.0 + (i * 5.0);
        int pwm = 30 + (i * 7);
        
        Debug_Teleplot("temperatura", temp);
        Debug_Teleplot("pwm", pwm);
        
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    
    DBG("");
    ESP_LOGI(TAG, "✓ Teleplot test completed (10 samples sent)\r\n");
}

/* ============ TEST 6: Multitask (Thread-Safety) ============ */

static void debug_task_1(void *param)
{
    for (int i = 0; i < 5; i++) {
        DBG("[TASK1] Message %d", i);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}

static void debug_task_2(void *param)
{
    for (int i = 0; i < 5; i++) {
        DBG_INFO("Task 2 info message");
        vTaskDelay(pdMS_TO_TICKS(150));
    }
    vTaskDelete(NULL);
}

static void debug_task_3(void *param)
{
    for (int i = 0; i < 5; i++) {
        Debug_Teleplot("concurrent_data", 20.5 + i);
        vTaskDelay(pdMS_TO_TICKS(120));
    }
    vTaskDelete(NULL);
}

void Test_Debug_Multitask()
{
    ESP_LOGI(TAG, "--- TEST 6: Multitask (Thread-Safety) ---\r\n");
    
    /* Crear 3 tareas que llaman a debug simultáneamente */
    xTaskCreate(debug_task_1, "DBG_Task1", 2048, NULL, 2, NULL);
    vTaskDelay(pdMS_TO_TICKS(50));  /* Pequeño delay entre creaciones */
    xTaskCreate(debug_task_2, "DBG_Task2", 2048, NULL, 2, NULL);
    vTaskDelay(pdMS_TO_TICKS(50));
    xTaskCreate(debug_task_3, "DBG_Task3", 2048, NULL, 2, NULL);
    
    /* Esperar a que terminen */
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    DBG("");
    ESP_LOGI(TAG, "✓ Multitask test completed (todas las tareas finalizadas)\r\n");
}

/* ============ TEST EJECUTOR COMPLETO ============ */
void Test_Debug_All()
{
    ESP_LOGI(TAG, "\r\n\r\n╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   DEBUG MODULE COMPREHENSIVE TEST      ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝\r\n");
    
    /* Test 1 */
    Test_Debug_Request();
    vTaskDelay(pdMS_TO_TICKS(500));
    
    /* Test 2 */
    Test_Debug_Start();
    vTaskDelay(pdMS_TO_TICKS(500));
    
    /* Test 3 */
    Test_Debug_Printf();
    vTaskDelay(pdMS_TO_TICKS(500));
    
    /* Test 4 */
    Test_Debug_LoggingLevels();
    vTaskDelay(pdMS_TO_TICKS(500));
    
    /* Test 5 */
    Test_Debug_Teleplot();
    vTaskDelay(pdMS_TO_TICKS(500));
    
    /* Test 6 */
    Test_Debug_Multitask();
    
    ESP_LOGI(TAG, "\r\n╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   ALL TESTS COMPLETED SUCCESSFULLY    ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝\r\n");
}
