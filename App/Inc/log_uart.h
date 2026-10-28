#ifndef APP_LOG_UART_H
#define APP_LOG_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief Send a null-terminated string on the log UART.
 *
 * USART1 TX (PA9) is initialized by MX_USART1_UART_Init() in main.c.
 */
void LOG_Write(const char *text);

/**
 * @brief printf-style logging helper (blocking, polled transmit).
 */
void LOG_Printf(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* APP_LOG_UART_H */
