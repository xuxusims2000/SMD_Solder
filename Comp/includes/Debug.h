#ifndef DEBUG_UART_H_
#define DEBUG_UART_H_

#include <stdarg.h>
#include "esp_err.h"
#include "driver/uart.h"

#define DEBUG_TX_PIN 17
#define DEBUG_RX_PIN 16

/* ANSI Color Codes */
#define ANSI_COLOR_RED     "\033[0;31m"
#define ANSI_COLOR_GREEN   "\033[0;32m"
#define ANSI_COLOR_YELLOW  "\033[0;33m"
#define ANSI_COLOR_BLUE    "\033[0;34m"
#define ANSI_COLOR_MAGENTA "\033[0;35m"
#define ANSI_COLOR_CYAN    "\033[0;36m"
#define ANSI_COLOR_WHITE   "\033[0;37m"

/* Bold Colors */
#define ANSI_COLOR_BOLD_RED     "\033[1;31m"
#define ANSI_COLOR_BOLD_GREEN   "\033[1;32m"
#define ANSI_COLOR_BOLD_YELLOW  "\033[1;33m"
#define ANSI_COLOR_BOLD_BLUE    "\033[1;34m"
#define ANSI_COLOR_BOLD_MAGENTA "\033[1;35m"
#define ANSI_COLOR_BOLD_CYAN    "\033[1;36m"

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
#define DBG(fmt, ...)      Debug_Printf(fmt "\r\n", ##__VA_ARGS__)
#define DBG_ERROR(msg)     Debug_LogError(msg)
#define DBG_INFO(msg)      Debug_LogInfo(msg)
#define DBG_WARNING(msg)   Debug_LogWarning(msg)
#define DBG_DEBUG(msg)     Debug_LogDebug(msg)

#endif // DEBUG_UART_H_