#ifndef _MODEM_DEBUG_H_
#define _MODEM_DEBUG_H_

#include "stm32f4xx_hal.h"
#include "UARTStream.h"
#include <stdarg.h>
#include <stdio.h>

#define UART_STREAM_BUFFER_TX_SIZE 		(512)
#define UART_STREAM_BUFFER_RX_SIZE 		(512)
#define UART_STREAM_LINE_MAX_LEN 		UART_STREAM_BUFFER_RX_SIZE

void SerialLog_print(const char *format, ...);
void SerialLog_Init(UART_HandleTypeDef *huart);
void SerialLog_RxCpltCallback(void);
void SerialLog_TxCpltCallback(void);
void SerialLog_ErrorCallback(void);
void SerialLog_Routine(void);
void SerialLog_ForceFlush(void);

#endif //_MODEM_DEBUG_H_
