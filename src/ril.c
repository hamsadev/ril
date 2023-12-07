/**
 * @file ril.c
 * @author Hamid Salehi (hamsame.dev@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-22
 * 
 * @copyright Copyright (c) 2023 Hamid Salehi
 * 
 */

#include "ril.h"
#include "UARTStream.h"
#include "Str.h"
#include <stdbool.h>
#include <stdio.h>

#define RIL_INIT_RETRY  10
#define RIL_LINE_LEN    64
#define _RIL_ERROR_SET(TYPE, ERRCODE)   \
    error.type = TYPE; \
    error.atError = ERRCODE; 

static const char CRLF[] = "\r\n";
static const uint8_t CRLFLen = sizeof(CRLF) -1;

static UARTStream stream;
static uint8_t streamRxBuff[RIL_RX_STREAM_SIZE];
static uint8_t streamTxBuff[RIL_TX_STREAM_SIZE];
static bool rilInitialized = false;
static RIL_Error error = {
    .type = RIL_ERROR_AT,
    .atError = RIL_AT_UNINITIALIZED,
};

static int16_t _lineIsError(const char* line, uint32_t len, uint16_t* errCode);

RIL_ATSndError RIL_initialize(UART_HandleTypeDef *uart){
    stream.HUART = uart;
    IStream_init(&stream.Input, UARTStream_receive, streamRxBuff, sizeof(streamRxBuff));
    IStream_setCheckReceive(&stream.Input, UARTStream_checkReceivedBytes);
    IStream_setArgs(&stream.Input, &stream);
    OStream_init(&stream.Output, UARTStream_transmit, streamTxBuff, sizeof(streamTxBuff));
    OStream_setArgs(&stream.Output, &stream);
    // start receive
    IStream_receive(&stream.Input);
    rilInitialized = true;

    uint16_t try = RIL_INIT_RETRY;

    RIL_ATSndError atErrCode;
    while (try--)
    {
        /* Start AT SYNC: Send AT every 500ms, if receive OK, SYNC success,
           if no OK return after sending AT RIL_INIT_RETRY times, SYNC fail */
        atErrCode = RIL_SendATCmd("AT", 2, NULL, NULL, 500);
        if (atErrCode == RIL_AT_SUCCESS)
        {
            /* Use ATE0 to disable echo mode */
            RIL_SendATCmd("ATE0", 3, NULL, NULL, 500);

            /* Use AT+CMEE=1 to enable result code */
            RIL_SendATCmd("AT+CMEE=1", 9, NULL, NULL, 500);

            // Use ATV1 to set the response format 
            RIL_SendATCmd("ATE1", 3, NULL, NULL, 500);
            
            return RIL_AT_SUCCESS;
        }
        
    }
    return atErrCode;

}

Stream_Result RIL_rxCpltHandle(void){
    Stream_Result streamErrCode = IStream_handle(&stream.Input, IStream_incomingBytes(&stream.Input));
    _RIL_ERROR_SET(RIL_ERROR_EQPT, streamErrCode);
    return streamErrCode;
}

Stream_Result RIL_txCpltHandle(void){
    Stream_Result streamErrCode = OStream_handle(&stream.Output, OStream_outgoingBytes(&stream.Output));
    _RIL_ERROR_SET(RIL_ERROR_EQPT, streamErrCode);
    return streamErrCode;
}

RIL_ATSndError RIL_SendATCmd(char *atCmd, uint32_t atCmdLen, Callback_ATResponse atRsp_callBack, void *userData, uint32_t timeOut){
    char buffer[RIL_LINE_LEN];
    if (!rilInitialized){
        return RIL_AT_UNINITIALIZED;
    }  

    if (timeOut == 0){
        /* 3min -> (3*50*1000)ms */
        timeOut += 180000;
    }
    timeOut += HAL_GetTick();

    // Copy command to buffer
    Str_copyFix(buffer, atCmd, atCmdLen);
    // Put a null at the end of command
    buffer[atCmdLen] = 0;
    // Append CRLF at the end of command
    Str_append(buffer, CRLF);
    // Update atCmd and cmdLen
    atCmd = buffer;
    atCmdLen = strlen(buffer);

    Stream_Result streamErrCode = OStream_writeBytes(&stream.Output, (uint8_t *)atCmd, atCmdLen);
    OStream_flush(&stream.Output);
    if (streamErrCode)
    {
        _RIL_ERROR_SET(RIL_ERROR_EQPT, streamErrCode);
        return RIL_AT_FAILED;
    }
    streamErrCode = OStream_flush(&stream.Output);

    while (HAL_GetTick() < timeOut){
        if(IStream_available(&stream.Input) > 0){
            Stream_LenType len = IStream_readBytesUntilPattern(&stream.Input, (uint8_t *)CRLF, CRLFLen, (uint8_t *)buffer, RIL_LINE_LEN);
            if(len > CRLFLen){
                buffer[len] = 0;
                // Check that the response is not an error 
                uint16_t errCode;
                if(_lineIsError(buffer, len, &errCode) > 0){
                    _RIL_ERROR_SET(RIL_ERROR_AT, errCode);
                    return RIL_AT_FAILED;
                }

                if (atRsp_callBack != NULL)
                {
                    atRsp_callBack(buffer, len, userData);
                }
                            
                return RIL_AT_SUCCESS;
            }
        }        
    }
    return RIL_AT_TIMEOUT;
}

RIL_Error Ql_RIL_AT_GetErrCode(void){
    return error;
}

static int16_t _lineIsError(const char* line, uint32_t len, uint16_t* errCode){
    return sscanf(line, "+CME ERROR: %hu", errCode); 
}