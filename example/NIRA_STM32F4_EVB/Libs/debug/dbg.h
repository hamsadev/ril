#ifndef _MODEM_DEBUG_H_
#define _MODEM_DEBUG_H_

#include "stm32f4xx_hal.h"
#include "UARTStream.h"
#include "OutputStream.h"
#include <stdarg.h>
#include <stdio.h>

#define DBG_BUFFER_SIZE     (128)

typedef void (*DBG_LineReceived) (char*, uint16_t);

void DBG(const char* format, ...);
void DBG_Init(UART_HandleTypeDef* huart, DBG_LineReceived onLineReceived);
void DBG_Routine(void);
void DBG_RxCpltCallback(void);
void DBG_TxCpltCallback(void);
void DBG_ErrorCallback(void);
void* DBG_GetArgs(void);


#endif //_MODEM_DEBUG_H_
