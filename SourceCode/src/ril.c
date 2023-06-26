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
#include "ril_urc.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define RIL_INIT_RETRY 10
#define RIL_LINE_LEN 512

#define _RIL_ERROR_SET(ERRCODE) rilCtx.error = ERRCODE;
#define _RIL_ERROR_RESET() rilCtx.error = 0;

static const char CRLF[] = "\r\n";
static const uint8_t CRLFLen = sizeof(CRLF) - 1;
typedef struct {
    UARTStream stream;
    RILState state;
    RIL_URCIndicationCallback urcIndicationCallback;
    RIL_OperationMode operationMode;
    uint16_t expectedBytes; // expected bytes to receive in binary mode
    uint16_t error;
    uint8_t streamRxBuff[RIL_RX_STREAM_SIZE];
    uint8_t streamTxBuff[RIL_TX_STREAM_SIZE];
    uint8_t initialized : 1;
} RILContext;

static RILContext rilCtx;

static int16_t _lineIsError(const char* line, uint32_t len, uint16_t* errCode);
static bool _lineIsURC(const char* line);
static bool RIL_ParseURC(const char* line, RIL_URCInfo* urcInfo);
static int32_t okRespCallback(char* line, uint32_t len, void* userData);

RILState RIL_GetState(void) {
    return rilCtx.state;
}

RIL_OperationMode RIL_GetOperationMode(uint16_t* expectedBytes) {
    *expectedBytes = rilCtx.expectedBytes;
    return rilCtx.operationMode;
}

void RIL_SetOperationNormal(void) {
    rilCtx.operationMode = RIL_OperationMode_Normal;
    rilCtx.expectedBytes = 0;
}

void RIL_SetOperationBinary(uint16_t expectedBytes) {
    rilCtx.operationMode = RIL_OperationMode_Binary;
    rilCtx.expectedBytes = expectedBytes;
}

RIL_ATSndError RIL_deInitialize(void) {
    RIL_ATSndError atErrCode;

    uint16_t tryCount = RIL_INIT_RETRY;
    while (tryCount--) {
        /* Start AT SYNC: Send AT every 500ms, if receive OK, SYNC success,
           if no OK return after sending AT RIL_INIT_RETRY times, SYNC fail */
        atErrCode = RIL_SendATCmd("AT", 2, okRespCallback, NULL, 500);
        if (atErrCode == RIL_AT_SUCCESS) {
            /* Use ATE1 to enable echo mode */
            RIL_SendATCmd(RIL_STR("ATE1"), okRespCallback, NULL, 500);

            /* Use AT+CMEE=1 to enable result code */
            RIL_SendATCmd(RIL_STR("AT+CMEE=1"), okRespCallback, NULL, 500);

            // Use ATV1 to set the response format
            RIL_SendATCmd(RIL_STR("ATV1"), okRespCallback, NULL, 500);

            // URC activation
						if(rilCtx.urcIndicationCallback)
						{
							RIL_SendATCmd(RIL_STR("AT+QURCCFG=\"urcport\",\"uart1\""), okRespCallback, NULL, 500);
							
							for (size_t i = 0; i < URC_MAX; i++) {
                if ((char*) URC_AT_COMMANDS[i] != NULL) {
                    RIL_SendATCmd((char*) URC_AT_COMMANDS[i], strlen(URC_AT_COMMANDS[i]),
                                  okRespCallback, NULL, 500);
                }
							}
						}

            return RIL_AT_SUCCESS;
        }
    }
    return RIL_AT_FAILED;
}

RIL_ATSndError RIL_initialize(UART_HandleTypeDef* uart, RIL_URCIndicationCallback urcCb) {
    if (rilCtx.initialized) {
        return RIL_AT_SUCCESS;
    }

    rilCtx.urcIndicationCallback = urcCb;
    rilCtx.error = RIL_AT_UNINITIALIZED;
    rilCtx.state = RIL_READY;

    UARTStream_init(&rilCtx.stream, uart, rilCtx.streamRxBuff, RIL_RX_STREAM_SIZE,
                    rilCtx.streamTxBuff, RIL_TX_STREAM_SIZE);

    // start receive
    IStream_receive(&rilCtx.stream.Input);
    rilCtx.operationMode = RIL_OperationMode_Normal;
    rilCtx.initialized = true;

    RIL_ATSndError ret = RIL_deInitialize();

    return ret;
}

void RIL_ServiceRoutine(void) {
    if (rilCtx.state == RIL_READY) {
        if (IStream_available(&rilCtx.stream.Input) > 0) {
            char lineCache[RIL_LINE_LEN];
            Stream_LenType len = IStream_readBytesUntilPattern(
                &rilCtx.stream.Input, (uint8_t*) CRLF, CRLFLen, (uint8_t*) lineCache, RIL_LINE_LEN);
            if (len > CRLFLen) {
                len -= CRLFLen;
                lineCache[len] = 0; // Null-terminate the received line

                RIL_URCInfo urcInfo;
                bool urcDetected = RIL_ParseURC(lineCache, &urcInfo); // Parse URC parameters
                if (urcDetected) {
                    RIL_LOG_TRACE("URC detected: %u ,%s", urcInfo.urcType, lineCache);
                    if(rilCtx.urcIndicationCallback != NULL)
                    {
                        rilCtx.urcIndicationCallback(&urcInfo); // Call user-defined callback
                    }
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

Stream_Result RIL_errorHandle(void) {
    UARTStream_errorHandle(&rilCtx.stream);
    return Stream_Ok;
}

RIL_ATSndError _RIL_SendATCmd(const char* atCmd, uint32_t atCmdLen,
                              Callback_ATResponse atRsp_callBack, void* userData,
                              bool waitForPrompt, uint32_t timeOut) {
    _RIL_ERROR_RESET();

    if (!rilCtx.initialized) {
        return RIL_AT_UNINITIALIZED;
    }

    static char lineCache[RIL_LINE_LEN];
    rilCtx.state = RIL_BUSY;

    RIL_LOG_TRACE("Sending AT command: %s", atCmd);

    if (timeOut == 0) {
        /* 5 sec -> 5000ms */
        timeOut += 5000;
    }
    timeOut += HAL_GetTick();

    // Copy command to buffer
    Str_copyFix(lineCache, atCmd, atCmdLen);
    // Put a null at the end of command
    lineCache[atCmdLen] = 0;
    // Append CRLF at the end of command
    Str_append(lineCache, CRLF);
    // Update atCmd and cmdLen
    atCmd = lineCache;
    atCmdLen = strlen(lineCache);

    Stream_Result streamErrCode =
        OStream_writeBytes(&rilCtx.stream.Output, (uint8_t*) atCmd, atCmdLen);
    streamErrCode = OStream_flush(&rilCtx.stream.Output);
    if (streamErrCode) {
        goto onError;
    }

    bool echoSeen = false;
    while (HAL_GetTick() < timeOut) {
        if (IStream_available(&rilCtx.stream.Input) > 0) {
            Stream_LenType len;
            if (waitForPrompt) {
                len = IStream_readBytesUntil(&rilCtx.stream.Input, (uint8_t) '>',
                                             (uint8_t*) lineCache, RIL_LINE_LEN);

                if (len > 0) {
                    RIL_LOG_TRACE("Prompt character '>' detected");
                    goto onSuccess;
                }
            }

            if (rilCtx.operationMode == RIL_OperationMode_Normal) {
                len = IStream_readBytesUntilPattern(&rilCtx.stream.Input, (uint8_t*) CRLF, CRLFLen,
                                                    (uint8_t*) lineCache, RIL_LINE_LEN);
                if (len > CRLFLen) {

                    len -= CRLFLen;
                    if (lineCache[len - 1] == '\r') {
                        len--;
                    }

                    lineCache[len] = 0;

                    if (!echoSeen && Str_compareFix(lineCache, atCmd, len) == 0) {
                        RIL_LOG_TRACE("Echo seen: %s", lineCache);
                        echoSeen = true;
                        continue;
                    }

                    if (echoSeen) {
                        // Check that the response is not an error
                        uint16_t errCode;
                        _RIL_ERROR_SET(0);
                        if (_lineIsError(lineCache, len, &errCode) > 0) {
                            RIL_LOG_ERROR("AT command failed: %s", atCmd);
                            _RIL_ERROR_SET(errCode);
                            goto onError;

                        } else if (atRsp_callBack != NULL) {
                            // Check if the response is printable
                            bool allPrintable = true;
                            for (uint32_t i = 0; i < len; i++) {
                                if (lineCache[i] >= 127) {
                                    allPrintable = false;
                                    break;
                                }
                            }
                            if (allPrintable) {
                                RIL_LOG_TRACE("AT command response: %s", lineCache);
                            }

                            int32_t ret = atRsp_callBack(lineCache, len, userData);
                            if (ret == RIL_ATRSP_CONTINUE) {
                                if (_lineIsURC(lineCache)) {

                                    RIL_URCInfo urc;
                                    RIL_ParseURC(lineCache, &urc);
                                    // TODO: handle URC later
                                }
                                continue;
                            }
                        }
                    }

                    goto onSuccess;
                }
            } else if (rilCtx.operationMode == RIL_OperationMode_Binary &&
                       rilCtx.expectedBytes > 0) {
                streamErrCode = IStream_readBytes(&rilCtx.stream.Input, (uint8_t*) lineCache,
                                                  rilCtx.expectedBytes);
                if (streamErrCode == Stream_Ok) {
                    if (atRsp_callBack != NULL) {
                        int32_t ret = atRsp_callBack(lineCache, rilCtx.expectedBytes, userData);
                        if (ret == RIL_ATRSP_CONTINUE) {
                            RIL_SetOperationNormal();
                            continue;
                        } else if (ret == RIL_ATRSP_SUCCESS) {
                            goto onSuccess;
                        } else {
                            goto onError;
                        }
                    }
                    goto onSuccess;
                }
            }
        }
    }

    rilCtx.state = RIL_READY;
    RIL_SetOperationNormal();
    return RIL_AT_TIMEOUT;

onError:
    rilCtx.state = RIL_READY;
    RIL_SetOperationNormal();
    return RIL_AT_FAILED;

onSuccess:
    rilCtx.state = RIL_READY;
    RIL_SetOperationNormal();
    return RIL_AT_SUCCESS;
}

RIL_ATSndError RIL_SendBinaryData(const uint8_t* data, uint32_t dataLen,
                                  Callback_ATResponse atRsp_callBack, void* userData,
                                  uint32_t timeOut) {
    _RIL_ERROR_RESET();

    if (!rilCtx.initialized) {
        return RIL_AT_UNINITIALIZED;
    }

    char lineCache[RIL_LINE_LEN];
    rilCtx.state = RIL_BUSY;

    RIL_LOG_TRACE("Sending raw data, length: %u", dataLen);

    if (timeOut == 0) {
        /* 3min -> (3*50*1000)ms */
        timeOut += 1800;
    }
    timeOut += HAL_GetTick();

    // Send the raw data
    Stream_Result streamErrCode =
        OStream_writeBytes(&rilCtx.stream.Output, (uint8_t*) data, dataLen);
    streamErrCode = OStream_flush(&rilCtx.stream.Output);
    if (streamErrCode) {
        goto onError;
    }

    while (HAL_GetTick() < timeOut) {
        if (IStream_available(&rilCtx.stream.Input) > 0) {
            // Check for complete line
            Stream_LenType len = IStream_readBytesUntilPattern(
                &rilCtx.stream.Input, (uint8_t*) CRLF, CRLFLen, (uint8_t*) lineCache, RIL_LINE_LEN);
            if (len > CRLFLen) {
                len -= CRLFLen;
                lineCache[len] = 0;

                // Check for errors
                uint16_t errCode;
                _RIL_ERROR_SET(0);
                if (_lineIsError(lineCache, len, &errCode) > 0) {
                    RIL_LOG_ERROR("Error response received: %s", lineCache);
                    _RIL_ERROR_SET(errCode);
                    goto onError;
                }

                // Process response through callback if provided
                if (atRsp_callBack != NULL) {
                    RIL_LOG_TRACE("Response received: %s", lineCache);
                    int32_t ret = atRsp_callBack(lineCache, len, userData);
                    if (ret == RIL_ATRSP_CONTINUE) {
                        if (_lineIsURC(lineCache)) {
                            RIL_URCInfo urc;
                            RIL_ParseURC(lineCache, &urc);
                            RIL_LOG_TRACE("URC received: %s", lineCache);
                            // TODO: handle URC if needed
                        }
                        continue;
                    }
                }

                goto onSuccess;
            }
        }
    }

    rilCtx.state = RIL_READY;
    return RIL_AT_TIMEOUT;

onError:
    rilCtx.state = RIL_READY;
    return RIL_AT_FAILED;

onSuccess:
    rilCtx.state = RIL_READY;
    return RIL_AT_SUCCESS;
}

uint16_t RIL_AT_GetErrCode(void) {
    return rilCtx.error;
}

void RIL_AT_SetErrCode(uint16_t errCode) {
    rilCtx.error = errCode;
}

static int16_t _lineIsError(const char* line, uint32_t len, uint16_t* errCode) {
    int ret = sscanf(line, "+CME ERROR: %hu", errCode);
    if (ret) {
        return ret;
    } else {
        ret = sscanf(line, "+CMS ERROR: %hu", errCode);
        if (ret) {
            return ret;
        }
        return !strncmp(line, "ERROR", len);
    }
}

static bool _lineIsURC(const char* line) {
    // If line strated with '+'
    return line[0] == '+';
}

static int32_t okRespCallback(char* line, uint32_t len, void* userData) {
    int8_t cmpRet = strncmp(line, "OK", len);
    if (!cmpRet) {
        return RIL_ATRSP_SUCCESS;
    }
    return RIL_ATRSP_CONTINUE;
}

static bool RIL_ParseURC(const char* line, RIL_URCInfo* urcInfo) {

    if (!_lineIsURC(line)) {
        return false;
    }

    Str_MultiResult ret;
    const char* head = Str_findStrs(line, URC_STRINGS, URC_MAX, &ret);
    if (!head) {
        return false;
    }

    urcInfo->urcType = ret.Position;
    urcInfo->paramCount = 0; // Reset parameter count

    const char* paramStart = strchr(line, ':'); // Find the start of parameters
    if (!paramStart)
        return true; // No parameters found, exit

    paramStart++; // Move past ':'

    // Create a copy of parameter string for parsing (Param library modifies the string)
    char paramBuffer[RIL_UTIL_PARAM_MAX_SIZE];
    size_t paramLen = strlen(paramStart);
    if (paramLen >= RIL_UTIL_PARAM_MAX_SIZE) {
        paramLen = RIL_UTIL_PARAM_MAX_SIZE - 1;
    }
    memcpy(paramBuffer, paramStart, paramLen);
    paramBuffer[paramLen] = '\0';

    // Initialize Param cursor
    Param_Cursor cursor;
    Param_initCursor(&cursor, paramBuffer, paramLen, ',');

    // Parse parameters using Param library
    Param param;
    while (Param_next(&cursor, &param) && urcInfo->paramCount < MAX_URC_PARAMS) {
        // Copy parsed parameter to URC info
        urcInfo->params[urcInfo->paramCount] = param.Value;
        urcInfo->paramCount++;
    }

    return true;
}
