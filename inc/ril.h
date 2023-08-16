#ifndef _RIL_H_
#define _RIL_H_

#define RIL_RX_STREAM_SIZE  256
#define RIL_TX_STREAM_SIZE  256

#include "usart.h"
#include "StreamBuffer.h"
#include "ril_error.h"

/*******************************************************************************
* RIL_SendATCmd response callback type
******************************************************************************/
typedef uint32_t (*Callback_ATResponse)(char* line, uint32_t len, void* userData);

/*******************************************************************************
 * @brief This function initializes RIl-related functions.
 * Set the initial AT commands, please refer to "m_InitCmds".
 ******************************************************************************/
RIL_ATSndError RIL_initialize(UART_HandleTypeDef* uart);

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

/******************************************************************************  
* @brief
*   This function implements sending AT command with the result  
*   being returned synchronously.The response of the AT command 
*   will be reported to the callback function,you can get the results
*   you want in it.
*    
* @param atCmd [in]AT command string.
* @param atCmdLen [in]The length of AT command string.
* @param atRsp_callBack [in]Callback function for handle the response of the AT command.
* @param userData [out]Used to transfer the customer's parameter.
* @param timeOut [in]Timeout for the AT command, unit in ms. 
*                    *If set it to 0, RIL uses the default timeout time (3min).
*
* @return A member of RIL_ATSndError enum
******************************************************************************/
RIL_ATSndError RIL_SendATCmd(char*  atCmd, uint32_t atCmdLen, Callback_ATResponse atRsp_callBack, void* userData, uint32_t timeOut);


/******************************************************************************  
* @brief This function retrieves the specific error code after executing AT failed.
* @return //TODO: Write return description
******************************************************************************/
RIL_Error Ql_RIL_AT_GetErrCode(void);

#endif //_RIL_H_
