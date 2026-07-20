/*
 * DEBUG_CONFIG_TEMPLATE.h - Plantilla para configurar colores de debug por módulo
 * 
 * Instrucciones:
 * 1. Copia este archivo en tu módulo o includes
 * 2. Renómbralo a algo como "MODULE_DEBUG_CONFIG.h"
 * 3. Personaliza los colores según necesites
 * 4. Inclúyelo en tu módulo antes de usar DBG
 * 
 * Ejemplo en Temp_Sensing.c:
 *    #include "Temp_Sensing_Debug.h"
 *    DBG_TS("Temperatura: %d°C", temp);  // Imprime en color Cyan
 */

#ifndef MODULE_DEBUG_CONFIG_H
#define MODULE_DEBUG_CONFIG_H

#include "Debug.h"

/* ============ PERSONALIZA LOS COLORES AQUÍ ============ */

/* Color principal para mensajes informativos (elige uno) */
#define MODULE_COLOR_INFO       ANSI_COLOR_CYAN
/* Otras opciones:
 *   ANSI_COLOR_BLUE
 *   ANSI_COLOR_GREEN
 *   ANSI_COLOR_MAGENTA
 *   ANSI_COLOR_BOLD_CYAN
 *   ANSI_COLOR_BOLD_BLUE
 */

/* Color para warnings del módulo (siempre amarillo o naranja) */
#define MODULE_COLOR_WARNING    ANSI_COLOR_YELLOW

/* Color para errores del módulo (SIEMPRE ROJO - no cambiar) */
#define MODULE_COLOR_ERROR      ANSI_COLOR_RED

/* ============ MACROS DEL MÓDULO (NO MODIFICAR) ============ */

/* Mensaje genérico con color del módulo */
#define DBG_MODULE(fmt, ...) \
    Debug_Printf(MODULE_COLOR_INFO fmt ANSI_COLOR_RESET "\r\n", ##__VA_ARGS__)

/* Mensaje de información del módulo */
#define DBG_MODULE_INFO(fmt, ...) \
    Debug_Printf(MODULE_COLOR_INFO "[INFO] " fmt ANSI_COLOR_RESET "\r\n", ##__VA_ARGS__)

/* Mensaje de warning del módulo */
#define DBG_MODULE_WARNING(fmt, ...) \
    Debug_Printf(MODULE_COLOR_WARNING "[WARNING] " fmt ANSI_COLOR_RESET "\r\n", ##__VA_ARGS__)

/* Mensaje de error del módulo (SIEMPRE ROJO) */
#define DBG_MODULE_ERROR(fmt, ...) \
    Debug_Printf(ANSI_COLOR_RED "[ERROR] " fmt ANSI_COLOR_RESET "\r\n", ##__VA_ARGS__)

#endif // MODULE_DEBUG_CONFIG_H
