#ifndef APP_LOG_UART_H
#define APP_LOG_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/** Send a null-terminated string on the log UART. */
void LOG_Write(const char *text);

/** printf-style logging helper (blocking, polled transmit). */
void LOG_Printf(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* APP_LOG_UART_H */
