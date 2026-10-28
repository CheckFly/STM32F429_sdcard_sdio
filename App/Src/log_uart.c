#include "log_uart.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart1;

void LOG_Write(const char *text)
{
    if (text == NULL)
    {
        return;
    }

    size_t length = strlen(text);
    if (length == 0U)
    {
        return;
    }

    if (length > 0xFFFFU)
    {
        length = 0xFFFFU;
    }

    (void)HAL_UART_Transmit(&huart1, (uint8_t *)text, (uint16_t)length, 1000U);
}

void LOG_Printf(const char *format, ...)
{
    char buffer[160];
    va_list args;

    if (format == NULL)
    {
        return;
    }

    va_start(args, format);
    (void)vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    buffer[sizeof(buffer) - 1U] = '\0';
    LOG_Write(buffer);
}
