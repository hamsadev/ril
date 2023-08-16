#include "ril.h"
#include "UARTStream.h"

static UARTStream stream;
static uint8_t streamRxBuff[RIL_RX_STREAM_SIZE];
static uint8_t streamTxBuff[RIL_TX_STREAM_SIZE];

void RIL_initialize(UART_HandleTypeDef* uart){
    stream.HUART = uart;
    IStream_init(&stream.Input, UARTStream_receive, streamRxBuff, sizeof(streamRxBuff));
    IStream_setCheckReceive(&stream.Input, UARTStream_checkReceivedBytes);
    IStream_setArgs(&stream.Input, &stream);
    OStream_init(&stream.Output, UARTStream_transmit, streamTxBuff, sizeof(streamTxBuff));
    OStream_setArgs(&stream.Output, &stream);
    // start receive
    IStream_receive(&stream.Input);
}


Stream_Result RIL_rxCpltHandle(void){
  return IStream_handle(&stream.Input, IStream_incomingBytes(&stream.Input));
}

Stream_Result RIL_txCpltHandle(void){
  return OStream_handle(&stream.Output, OStream_outgoingBytes(&stream.Output));
}