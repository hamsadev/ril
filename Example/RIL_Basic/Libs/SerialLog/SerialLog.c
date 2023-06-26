#include "SerialLog.h"

typedef struct {
    UARTStream dbg;
    uint8_t rxBuffer[UART_STREAM_BUFFER_RX_SIZE];
    uint8_t txBuffer[UART_STREAM_BUFFER_TX_SIZE];
    char DBG_BUFFER[UART_STREAM_LINE_MAX_LEN]; // TODO: maybe dynamic or in stack
    UART_HandleTypeDef* _huart;
    uint8_t initalized : 1;
} SerialLog;

static SerialLog serialLog;

void SerialLog_Init(UART_HandleTypeDef* huart) {
    serialLog.initalized = 1;
    serialLog._huart = huart;
    UARTStream_init(&serialLog.dbg, huart, serialLog.rxBuffer, UART_STREAM_BUFFER_RX_SIZE,
                    serialLog.txBuffer, UART_STREAM_BUFFER_TX_SIZE);
    IStream_receive(&serialLog.dbg.Input);
}

void SerialLog_RxCpltCallback(void) {
    UARTStream_rxHandle(&serialLog.dbg);
}

void SerialLog_TxCpltCallback(void) {
    UARTStream_txHandle(&serialLog.dbg);
}

void SerialLog_ErrorCallback(void) {
    UARTStream_errorHandle(&serialLog.dbg);
}

void SerialLog_Routine(void) {
    if (!serialLog.initalized) {
        return;
    }

    OStream_flush(&serialLog.dbg.Output);
}

void SerialLog_ForceFlush(void) {
    if (!serialLog.initalized) {
        return;
    }

    OStream_flushBlocking(&serialLog.dbg.Output);
}

void SerialLog_print(const char* format, ...) {
    if (!serialLog.initalized) {
        return;
    }

    va_list args;
    va_start(args, format);

    uint32_t len = vsnprintf(serialLog.DBG_BUFFER, UART_STREAM_LINE_MAX_LEN, format, args);
    serialLog.DBG_BUFFER[len] = 0;
    if (len == 0) {
        va_end(args);
        return;
    }

    OStream_writeBytes(&serialLog.dbg.Output, (uint8_t*) serialLog.DBG_BUFFER, len);
    OStream_flush(&serialLog.dbg.Output);

    va_end(args);
}
