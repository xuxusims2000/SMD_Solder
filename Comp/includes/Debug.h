#ifndef DEBUG_UART_H_
#define DEBUG_UART_H_

#include <stdarg.h>
#include "esp_err.h"
#include "driver/uart.h"

/* ANSI Color Codes */
#define ANSI_COLOR_RED     "\033[0;31m"
#define ANSI_COLOR_GREEN   "\033[0;32m"
#define ANSI_COLOR_YELLOW  "\033[0;33m"
#define ANSI_COLOR_BLUE    "\033[0;34m"
#define ANSI_COLOR_RESET   "\033[0;0m"

/* Configuración del módulo */
typedef struct {
    uart_port_t uart_num;
    int baudrate;
    int tx_io_num;
    int rx_io_num;
} Debug_Config_t;

/* Ciclo de vida del módulo */
esp_err_t Debug_Request(const Debug_Config_t *config);
esp_err_t Debug_Start(void);
esp_err_t Debug_Release(void);

/* API pública de logging */
esp_err_t Debug_Init(uart_port_t uart_num, int baudrate, int tx_io_num, int rx_io_num);
int Debug_Printf(const char *fmt, ...);
int Debug_vPrintf(const char *fmt, va_list ap);

/* Funciones de logging coloreado */
void Debug_LogError(const char *msg);
void Debug_LogInfo(const char *msg);
void Debug_LogWarning(const char *msg);
void Debug_LogDebug(const char *msg);

/* Teleplot para telemetría */
void Debug_Teleplot(const char *var_name, float value);

/* Macros para facilidad de uso */
#define DBG(...)           Debug_Printf(__VA_ARGS__)
#define DBG_ERROR(msg)     Debug_LogError(msg)
#define DBG_INFO(msg)      Debug_LogInfo(msg)
#define DBG_WARNING(msg)   Debug_LogWarning(msg)
#define DBG_DEBUG(msg)     Debug_LogDebug(msg)

#endif // DEBUG_UART_H_