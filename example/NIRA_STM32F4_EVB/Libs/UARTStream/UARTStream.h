#ifndef _UART_STREAM_H_
#define _UART_STREAM_H_

#include "InputStream.h"
#include "OutputStream.h"
#include "dma.h"

typedef struct {
	UART_HandleTypeDef* 		HUART;
	IStream									Input;
	OStream									Output;
} UARTStream;

void UARTStream_stopReceiveDMA(UARTStream* stream);
void UARTStream_stopTransmitDMA(UARTStream* stream);
Stream_Result UARTStream_receive(IStream* stream, uint8_t* buff, Stream_LenType len);
Stream_Result UARTStream_transmit(OStream* stream, uint8_t* buff, Stream_LenType len);
Stream_LenType UARTStream_checkReceivedBytes(IStream* stream);

#endif //_UART_STREAM_H_
