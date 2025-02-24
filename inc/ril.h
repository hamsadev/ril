#ifndef _RIL_H_
#define _RIL_H_

#define RIL_RX_STREAM_SIZE  512
#define RIL_TX_STREAM_SIZE  256

#include "usart.h"
#include "StreamBuffer.h"
#include "ril_error.h"
#include "ril_urc.h"


/**
 * @brief RIL_SendATCmd response callback type
 *
 */
typedef int32_t(*Callback_ATResponse)(char* line, uint32_t len, void* userData);

/**
 * @brief // TODO: Write a brief
 *
 */
typedef void (*RIL_URCIndicationCallback)(RIL_URCInfo* info);

/**
 * @brief This function initializes RIl-related functions.
 *
 * @param uart
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_initialize(UART_HandleTypeDef* uart, RIL_URCIndicationCallback urcCb);

/**
 * @brief // TODO: Write a brief
 *
 */
void RIL_ServiceRoutine(void);

/**
 * @brief call in interrupt or RxCplt callback for Async Receive
 *
 * @return Stream_Result
 */
Stream_Result RIL_rxCpltHandle(void);

/**
 * @brief call in interrupt or TxCplt callback for Async Receive
 *
 * @return Stream_Result
 */
Stream_Result RIL_txCpltHandle(void);

/**
 * @brief
 *   This function implements sending AT command with the result
 *   being returned synchronously.The response of the AT command
 *   will be reported to the callback function,you can get the results
 *   you want in it.
 * @param atCmd [in]AT command string.
 * @param atCmdLen [in]The length of AT command string.
 * @param atRsp_callBack [in]Callback function for handle the response of the AT command.
 * @param userData [out]Used to transfer the customer's parameter.
 * @param timeOut [in]Timeout for the AT command, unit in ms.
 *                    *If set it to 0, RIL uses the default timeout time (3min).
 * @return A member of RIL_ATSndError enum
 */
RIL_ATSndError RIL_SendATCmd(char* atCmd, uint32_t atCmdLen, Callback_ATResponse atRsp_callBack, void* userData, uint32_t timeOut);

/**
 * @brief This function retrieves the specific error code after executing AT failed.
 *
 * @return RIL_Error  //TODO: Write return description
 */
RIL_Error Ql_RIL_AT_GetErrCode(void);

#endif //_RIL_H_
