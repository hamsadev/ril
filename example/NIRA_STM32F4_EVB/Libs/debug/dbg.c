#include "dbg.h"

static UARTStream uart;
static uint8_t uart1RxBuffer[DBG_BUFFER_SIZE];
static uint8_t uart1TxBuffer[DBG_BUFFER_SIZE];
static char DBG_TX_BUFFER[DBG_BUFFER_SIZE];
static char DBG_RX_BUFFER[DBG_BUFFER_SIZE];
static DBG_LineReceived _onLineReceived;

static const char CRLF[] = "\r\n";
static const uint8_t CRLFLen = sizeof(CRLF) - 1;

UART_HandleTypeDef* _huart;

void* DBG_GetArgs(void)
{
    return &uart;
}

void DBG_Init(UART_HandleTypeDef* huart, DBG_LineReceived onLineReceived)
{
    _huart = huart;
    _onLineReceived = onLineReceived;
    UARTStream_init(&uart, huart, uart1RxBuffer, DBG_BUFFER_SIZE, uart1TxBuffer, DBG_BUFFER_SIZE);
    IStream_receive(&uart.Input);
}

void DBG_Routine(void)
{
    if (IStream_available(&uart.Input) > 0)
    {
        Stream_LenType len = IStream_readBytesUntilPattern(&uart.Input, (uint8_t*)CRLF, CRLFLen, (uint8_t*)DBG_RX_BUFFER, DBG_BUFFER_SIZE);
        if (len > CRLFLen)
        {
            DBG_RX_BUFFER[len] = 0;

            _onLineReceived(DBG_RX_BUFFER, len);
        }
    }
}

void DBG_RxCpltCallback(void)
{
    UARTStream_rxHandle(&uart);
}

void DBG_TxCpltCallback(void)
{
    UARTStream_txHandle(&uart);
}

void DBG_ErrorCallback(void)
{
    UARTStream_errorHandle(&uart);
}

void DBG(const char* format, ...) {
    va_list args;
    va_start(args, format);

    uint32_t len = vsnprintf(DBG_TX_BUFFER, DBG_BUFFER_SIZE, format, args);

    OStream_writeBytes(&uart.Output, (uint8_t*)DBG_TX_BUFFER, len);
    if (!uart.Output.Buffer.InTransmit)
    {
        OStream_flush(&uart.Output);
    }

    va_end(args);
}

