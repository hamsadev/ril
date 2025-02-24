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
#include "ril_urc.h"
#include <stdbool.h>
#include <stdio.h>

#define STR(S)  S, sizeof(S) - 1

#define RIL_INIT_RETRY  10
#define RIL_LINE_LEN    64
#define _RIL_ERROR_SET(TYPE, ERRCODE)   \
    rilCtx.error.type = TYPE; \
    rilCtx.error.atError = ERRCODE; 

static const char CRLF[] = "\r\n";
static const uint8_t CRLFLen = sizeof(CRLF) - 1;

typedef enum {
    RIL_BUSY,
    RIL_READY,
}RILState;
typedef struct
{
    UARTStream stream;
    RIL_Error error;
    RILState state;
    RIL_URCIndicationCallback urcIndicationCallback;
    uint8_t streamRxBuff[RIL_RX_STREAM_SIZE];
    uint8_t streamTxBuff[RIL_TX_STREAM_SIZE];
    uint8_t initialized : 1;
    char lineCache[RIL_LINE_LEN];
}RILContext;

static RILContext rilCtx;

static int16_t _lineIsError(const char* line, uint32_t len, uint16_t* errCode);
static void RIL_ParseURC(const char* line, RIL_URCInfo* urcInfo);

RIL_ATSndError RIL_initialize(UART_HandleTypeDef* uart, RIL_URCIndicationCallback urcCb) {
    rilCtx.urcIndicationCallback = urcCb;
    rilCtx.error.type = RIL_ERROR_AT;
    rilCtx.error.atError = RIL_AT_UNINITIALIZED;
    rilCtx.state = RIL_READY;

    UARTStream_init(&rilCtx.stream, uart, rilCtx.streamRxBuff, RIL_RX_STREAM_SIZE, rilCtx.streamTxBuff, RIL_TX_STREAM_SIZE);

    // start receive
    IStream_receive(&rilCtx.stream.Input);
    rilCtx.initialized = true;

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
            RIL_SendATCmd(STR("ATE0"), NULL, NULL, 500);

            /* Use AT+CMEE=1 to enable result code */
            RIL_SendATCmd(STR("AT+CMEE=1"), NULL, NULL, 500);

            // Use ATV1 to set the response format 
            RIL_SendATCmd(STR("ATV1"), NULL, NULL, 500);

            // URC activation
            for (size_t i = 0; i < URC_MAX; i++)
            {
                if ((char*)URC_AT_COMMANDS[i] != NULL)
                {
                    RIL_SendATCmd((char*)URC_AT_COMMANDS[i], strlen(URC_AT_COMMANDS[i]), NULL, NULL, 500);
                }
            }

            return RIL_AT_SUCCESS;
        }

    }
    return atErrCode;

}

void RIL_ServiceRoutine(void)
{
    if (rilCtx.state == RIL_READY)
    {
        if (IStream_available(&rilCtx.stream.Input) > 0) {
            Stream_LenType len = IStream_readBytesUntilPattern(&rilCtx.stream.Input, (uint8_t*)CRLF, CRLFLen, (uint8_t*)rilCtx.lineCache, RIL_LINE_LEN);
            if (len > CRLFLen)
            {
                len -= CRLFLen;
                rilCtx.lineCache[len] = 0; // Null-terminate the received line

                Str_MultiResult ret;
                const char* head = Str_findStrs(rilCtx.lineCache, URC_STRINGS, URC_MAX, &ret);
                if (head)
                {
                    RIL_URCInfo urcInfo = { 0 };
                    urcInfo.urcType = ret.Position;

                    RIL_ParseURC(rilCtx.lineCache, &urcInfo); // Parse URC parameters

                    rilCtx.urcIndicationCallback(&urcInfo); // Call user-defined callback
                }
            }
        }
    }
}

Stream_Result RIL_rxCpltHandle(void) {
    UARTStream_rxHandle(&rilCtx.stream);
    return Stream_Ok;
}

Stream_Result RIL_txCpltHandle(void) {
    UARTStream_txHandle(&rilCtx.stream);
    return Stream_Ok;
}

RIL_ATSndError RIL_SendATCmd(char* atCmd, uint32_t atCmdLen, Callback_ATResponse atRsp_callBack, void* userData, uint32_t timeOut) {
    if (!rilCtx.initialized) {
        return RIL_AT_UNINITIALIZED;
    }

    rilCtx.state = RIL_BUSY;

    if (timeOut == 0) {
        /* 3min -> (3*50*1000)ms */
        timeOut += 180000;
    }
    timeOut += HAL_GetTick();

    // Copy command to buffer
    Str_copyFix(rilCtx.lineCache, atCmd, atCmdLen);
    // Put a null at the end of command
    rilCtx.lineCache[atCmdLen] = 0;
    // Append CRLF at the end of command
    Str_append(rilCtx.lineCache, CRLF);
    // Update atCmd and cmdLen
    atCmd = rilCtx.lineCache;
    atCmdLen = strlen(rilCtx.lineCache);

    Stream_Result streamErrCode = OStream_writeBytes(&rilCtx.stream.Output, (uint8_t*)atCmd, atCmdLen);
    OStream_flush(&rilCtx.stream.Output);
    if (streamErrCode)
    {
        _RIL_ERROR_SET(RIL_ERROR_EQPT, streamErrCode);
        rilCtx.state = RIL_READY;
        return RIL_AT_FAILED;
    }
    streamErrCode = OStream_flush(&rilCtx.stream.Output);

    while (HAL_GetTick() < timeOut) {
        if (IStream_available(&rilCtx.stream.Input) > 0) {
            Stream_LenType len = IStream_readBytesUntilPattern(&rilCtx.stream.Input, (uint8_t*)CRLF, CRLFLen, (uint8_t*)rilCtx.lineCache, RIL_LINE_LEN);
            if (len > CRLFLen) {

                len -= CRLFLen;
                rilCtx.lineCache[len] = 0;

                // Check that the response is not an error 
                uint16_t errCode;
                if (_lineIsError(rilCtx.lineCache, len, &errCode) > 0) {
                    _RIL_ERROR_SET(RIL_ERROR_AT, errCode);
                    rilCtx.state = RIL_READY;
                    return RIL_AT_FAILED;
                }

                if (atRsp_callBack != NULL)
                {
                    int32_t ret = atRsp_callBack(rilCtx.lineCache, len, userData);
                    if (ret == RIL_ATRSP_CONTINUE)
                    {
                        continue;
                    }
                }

                rilCtx.state = RIL_READY;
                return RIL_AT_SUCCESS;
            }
        }
    }

    rilCtx.state = RIL_READY;
    return RIL_AT_TIMEOUT;
}

RIL_Error Ql_RIL_AT_GetErrCode(void) {
    return rilCtx.error;
}

static int16_t _lineIsError(const char* line, uint32_t len, uint16_t* errCode) {
    return sscanf(line, "+CME ERROR: %hu", errCode);
}

static void RIL_ParseURC(const char* line, RIL_URCInfo* urcInfo)
{
    urcInfo->paramCount = 0; // Reset parameter count

    const char* paramStart = strchr(line, ':'); // Find the start of parameters
    if (!paramStart) return; // No parameters found, exit

    paramStart++; // Move past ':'
    while (*paramStart == ' ') paramStart++; // Remove extra spaces

    while (*paramStart != '\0' && urcInfo->paramCount < MAX_URC_PARAMS) {
        URC_Param* param = &urcInfo->params[urcInfo->paramCount];

        if (*paramStart == '"') {  // Check for string parameter
            paramStart++; // Skip opening quote
            char* end = strchr(paramStart, '"'); // Find closing quote
            if (end) {
                size_t len = end - paramStart;
                if (len < sizeof(param->strValue)) {
                    memcpy(param->strValue, paramStart, len);
                    param->strValue[len] = '\0';
                }
                param->type = URC_PARAM_STRING;
                paramStart = end + 1; // Move past closing quote
            }
        }
        else if ((*paramStart >= '0' && *paramStart <= '9') || *paramStart == '-') {  // Check for integer parameter
            param->type = URC_PARAM_INT;
            param->intValue = 0;
            int sign = (*paramStart == '-') ? -1 : 1;
            if (*paramStart == '-') paramStart++;

            while (*paramStart >= '0' && *paramStart <= '9') {
                param->intValue = param->intValue * 10 + (*paramStart - '0');
                paramStart++;
            }
            param->intValue *= sign;
        }

        urcInfo->paramCount++; // Increment parameter count

        while (*paramStart == ',' || *paramStart == ' ') paramStart++; // Skip ',' and spaces
    }
}

