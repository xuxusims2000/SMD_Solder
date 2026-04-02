#include "Debug.h"
#include "driver/uart.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define DEBUG_UART_BUF_SIZE 1024
#define DEBUG_BUFFER_SIZE   512

/* Estado del módulo Debug */
typedef struct {
    uart_port_t uart_num;
    int baudrate;
    int tx_io_num;
    int rx_io_num;
    bool initialized;
    SemaphoreHandle_t mutex;
} Debug_State_t;

static Debug_State_t s_debug_state = {
    .uart_num = UART_NUM_2,
    .baudrate = 115200,
    .tx_io_num = -1,
    .rx_io_num = -1,
    .initialized = false,
    .mutex = NULL
};

/* ============ CICLO DE VIDA DEL MÓDULO ============ */

esp_err_t Debug_Request(const Debug_Config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Crear mutex para thread-safety */
    if (s_debug_state.mutex == NULL) {
        s_debug_state.mutex = xSemaphoreCreateMutex();
        if (s_debug_state.mutex == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    /* Guardar configuración */
    s_debug_state.uart_num = config->uart_num;
    s_debug_state.baudrate = config->baudrate;
    s_debug_state.tx_io_num = config->tx_io_num;
    s_debug_state.rx_io_num = config->rx_io_num;

    return ESP_OK;
}

esp_err_t Debug_Start(void)
{
    if (s_debug_state.initialized) {
        return ESP_OK;  /* Ya inicializado */
    }

    uart_config_t uart_config = {
        .baud_rate = s_debug_state.baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    esp_err_t err;
    err = uart_driver_install(s_debug_state.uart_num, DEBUG_UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    if (err != ESP_OK) return err;

    err = uart_param_config(s_debug_state.uart_num, &uart_config);
    if (err != ESP_OK) return err;

    err = uart_set_pin(s_debug_state.uart_num, s_debug_state.tx_io_num, 
                       s_debug_state.rx_io_num, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) return err;

    s_debug_state.initialized = true;
    return ESP_OK;
}

esp_err_t Debug_Release(void)
{
    if (!s_debug_state.initialized) {
        return ESP_OK;
    }

    uart_driver_delete(s_debug_state.uart_num);
    s_debug_state.initialized = false;

    if (s_debug_state.mutex != NULL) {
        vSemaphoreDelete(s_debug_state.mutex);
        s_debug_state.mutex = NULL;
    }

    return ESP_OK;
}

/* ============ API PÚBLICA ============ */

esp_err_t Debug_Init(uart_port_t uart_num, int baudrate, int tx_io_num, int rx_io_num)
{
    Debug_Config_t config = {
        .uart_num = uart_num,
        .baudrate = baudrate,
        .tx_io_num = tx_io_num,
        .rx_io_num = rx_io_num
    };

    esp_err_t err = Debug_Request(&config);
    if (err != ESP_OK) return err;

    return Debug_Start();
}

int Debug_vPrintf(const char *fmt, va_list ap)
{
    if (!s_debug_state.initialized) {
        return -1;
    }

    if (s_debug_state.mutex != NULL) {
        xSemaphoreTake(s_debug_state.mutex, portMAX_DELAY);
    }

    char buf[DEBUG_BUFFER_SIZE];
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    if (len > 0) {
        uart_write_bytes(s_debug_state.uart_num, buf, len);
    }

    if (s_debug_state.mutex != NULL) {
        xSemaphoreGive(s_debug_state.mutex);
    }

    return len;
}

int Debug_Printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int len = Debug_vPrintf(fmt, ap);
    va_end(ap);
    return len;
}

/* ============ FUNCIONES DE LOGGING COLOREADO ============ */

void Debug_LogError(const char *msg)
{
    Debug_Printf(ANSI_COLOR_RED "[ERROR] %s" ANSI_COLOR_RESET "\n", msg);
}

void Debug_LogInfo(const char *msg)
{
    Debug_Printf(ANSI_COLOR_GREEN "[INFO] %s" ANSI_COLOR_RESET "\n", msg);
}

void Debug_LogWarning(const char *msg)
{
    Debug_Printf(ANSI_COLOR_YELLOW "[WARNING] %s" ANSI_COLOR_RESET "\n", msg);
}

void Debug_LogDebug(const char *msg)
{
    Debug_Printf(ANSI_COLOR_BLUE "[DEBUG] %s" ANSI_COLOR_RESET "\n", msg);
}

/* ============ TELEPLOT PARA TELEMETRÍA ============ */

void Debug_Teleplot(const char *var_name, float value)
{
    /* Formato Teleplot: >nombre_variable:valor\n */
    Debug_Printf(">%s:%.2f\n", var_name, value);
}