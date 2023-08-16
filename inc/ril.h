#ifndef _RIL_H_
#define _RIL_H_

#define RIL_RX_STREAM_SIZE  256
#define RIL_TX_STREAM_SIZE  256

#include "usart.h"
#include "StreamBuffer.h"

/*******************************************************************************
 * @brief This function initializes RIl-related functions.
 * Set the initial AT commands, please refer to "m_InitCmds".
 ******************************************************************************/
void RIL_initialize(UART_HandleTypeDef* uart);

/*******************************************************************************
* @brief call in interrupt or RxCplt callback for Async Receive
* @return Stream_Result
******************************************************************************/
Stream_Result RIL_rxCpltHandle(void);

/*******************************************************************************
* @brief call in interrupt or TxCplt callback for Async Receive
* @return Stream_Result
******************************************************************************/
Stream_Result RIL_txCpltHandle(void);

#endif //_RIL_H_
