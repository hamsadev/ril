#include "UARTStream.h"

/**
 * @brief Initialize UARTStream object
 * 
 * @param stream 
 * @param huart 
 * @param rxBuff 
 * @param rxBuffSize 
 * @param txBuff 
 * @param txBuffSize 
 */
void UARTStream_init(
	UARTStream* stream, UART_HandleTypeDef* huart, 
	uint8_t* rxBuff, Stream_LenType rxBuffSize, 
	uint8_t* txBuff, Stream_LenType txBuffSize
) {
    stream->HUART = huart;

    if (rxBuff) {
        IStream_init(&stream->Input, UARTStream_receive, rxBuff, rxBuffSize);
        IStream_setCheckReceive(&stream->Input, UARTStream_checkReceive);
        IStream_setArgs(&stream->Input, stream);
    }

    if (txBuff) {
        OStream_init(&stream->Output, UARTStream_transmit, txBuff, txBuffSize);
        OStream_setCheckTransmit(&stream->Output, UARTStream_checkTransmit);
        OStream_setArgs(&stream->Output, stream);  
    }
}
/**
 * @brief Handle Receive Data for Input Stream, user must put it in 
 * HAL_UART_RxCpltCallback or other callbacks
 * 
 * @param stream 
 */
void UARTStream_rxHandle(UARTStream* stream) {
    IStream_handle(&stream->Input, IStream_incomingBytes(&stream->Input));
}
/**
 * @brief Handle Transmit Data for Output Stream, user must put it in 
 * HAL_UART_TxCpltCallback or other callbacks
 * 
 * @param stream 
 */
void UARTStream_txHandle(UARTStream* stream) {
    OStream_handle(&stream->Output, OStream_outgoingBytes(&stream->Output));
}
/**
 * @brief Handle Error for Stream, user must put it in HAL_UART_ErrorCallback
 * 
 * @param stream 
 */
void UARTStream_errorHandle(UARTStream* stream) {
    IStream_resetIO(&stream->Input);
    OStream_resetIO(&stream->Output);
    IStream_receive(&stream->Input);
}
#if UARTSTREAM_SUPPORT_IDLE
/**
 * @brief Initialize UARTStream object for Idle
 * 
 * @param stream 
 * @param huart 
 * @param rxBuff 
 * @param rxBuffSize 
 * @param txBuff 
 * @param txBuffSize 
 */
void UARTStream_initIdle(
    UARTStream* stream, UART_HandleTypeDef* huart, 
    uint8_t* rxBuff, Stream_LenType rxBuffSize, 
    uint8_t* txBuff, Stream_LenType txBuffSize
) {
    UARTStream_init(stream, huart, rxBuff, rxBuffSize, txBuff, txBuffSize);
    stream->Input.receive = UARTStream_receiveIdle;
}
/**
 * @brief Handle Idle for Stream, user must put it in HAL_UART_RxIdleCallback
 * 
 * @param stream 
 * @param len 
 */
void UARTStream_rxHandleIdle(UARTStream* stream, Stream_LenType len) {
    IStream_handle(&stream->Input, len);
}
/**
 * @brief Stream receive function for Idle
 * 
 * @param stream 
 * @param buff 
 * @param len 
 */
Stream_Result UARTStream_receiveIdle(IStream* stream, uint8_t* buff, Stream_LenType len) {
    HAL_StatusTypeDef status;
    UARTStream* uartStream = (UARTStream*) IStream_getArgs(stream);
    if (uartStream->HUART->hdmarx) {
        status = HAL_UARTEx_ReceiveToIdle_DMA(uartStream->HUART, buff, len);
    }
    else {
        status = HAL_UARTEx_ReceiveToIdle_IT(uartStream->HUART, buff, len);
    }
    return status == HAL_OK ? Stream_Ok : (Stream_Result) (Stream_CustomError | status);
}
#endif
/**
 * @brief Check Received bytes in DMA or IT and add to stream
 * 
 * @param stream 
 * @return Stream_LenType 
 */
Stream_LenType UARTStream_checkReceive(IStream* stream) {
    UARTStream* uartStream = (UARTStream*) IStream_getArgs(stream);
    if (uartStream->HUART->hdmarx) {
        return IStream_incomingBytes(stream) - __HAL_DMA_GET_COUNTER(uartStream->HUART->hdmarx);
    }
    else {
        return IStream_incomingBytes(stream) - uartStream->HUART->RxXferCount;
    }
}
/**
 * @brief Check Transmitted bytes in DMA or IT and removed from stream 
 * 
 * @param stream 
 * @return Stream_LenType 
 */
Stream_LenType UARTStream_checkTransmit(OStream* stream) {
    UARTStream* uartStream = (UARTStream*) OStream_getArgs(stream);
    if (uartStream->HUART->hdmatx) {
        return OStream_outgoingBytes(stream) - __HAL_DMA_GET_COUNTER(uartStream->HUART->hdmatx);
    }
    else {
        return OStream_outgoingBytes(stream) - uartStream->HUART->TxXferCount;
    }
}
/**
 * @brief Stream receive function
 * 
 * @param stream 
 * @param buff 
 * @param len 
 */
Stream_Result UARTStream_receive(IStream* stream, uint8_t* buff, Stream_LenType len) {
    HAL_StatusTypeDef status;
    UARTStream* uartStream = (UARTStream*) IStream_getArgs(stream);
    if (uartStream->HUART->hdmarx) {
        status = HAL_UART_Receive_DMA(uartStream->HUART, buff, len);
    }
    else {
        status = HAL_UART_Receive_IT(uartStream->HUART, buff, len);
    }
    return status == HAL_OK ? Stream_Ok : (Stream_Result) (Stream_CustomError | status);
}
/**
 * @brief Stream transmit function
 * 
 * @param stream 
 * @param buff 
 * @param len 
 */
Stream_Result UARTStream_transmit(OStream* stream, uint8_t* buff, Stream_LenType len) {
    HAL_StatusTypeDef status;
    UARTStream* uartStream = (UARTStream*) OStream_getArgs(stream);
    if (uartStream->HUART->hdmatx) {
        status = HAL_UART_Transmit_DMA(uartStream->HUART, buff, len);
    }
    else {
        status = HAL_UART_Transmit_IT(uartStream->HUART, buff, len);
    }
    return status == HAL_OK ? Stream_Ok : (Stream_Result) (Stream_CustomError | status);
}
