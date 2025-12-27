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
#if RIL_USE_OS
#include "cmsis_os2.h"
#endif

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
    RIL_PowerCommandCallback powerCommandCallback;
    RIL_OperationMode operationMode;
    uint16_t expectedBytes; // expected bytes to receive in binary mode
    uint16_t error;
    uint8_t streamRxBuff[RIL_RX_STREAM_SIZE];
    uint8_t streamTxBuff[RIL_TX_STREAM_SIZE];
    uint8_t initialized : 1;
#if RIL_USE_OS
    osMutexId_t mutex;
    osMutexAttr_t mutex_attr;
#endif
} RILContext;

static RILContext rilCtx;

static int16_t _lineIsError(const char* line, uint32_t len, uint16_t* errCode);
static bool _lineIsURC(const char* line);
static bool RIL_ParseURC(const char* line, RIL_URCInfo* urcInfo);
static int32_t okRespCallback(char* line, uint32_t len, void* userData);
static void RIL_PowerRestart(RIL_PowerCommandCallback powerCommandCb);
static void _RIL_Lock(void);
static void _RIL_Unlock(void);

// Helper functions for common operations
static RIL_ATSndError _RIL_AcquireAccess(void);
static void _RIL_ReleaseAccess(bool resetOperationMode);
static uint32_t _RIL_GetCurrentTick(void);
static void _RIL_Delay(uint32_t ms);
static RIL_ATSndError _RIL_WaitForResponse(char* lineCache, uint32_t timeOut,
                                           Callback_ATResponse atRsp_callBack, void* userData,
                                           bool checkEcho, const char* echoCmd);

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
            if (rilCtx.urcIndicationCallback) {
                RIL_SendATCmd(RIL_STR("AT+QURCCFG=\"urcport\",\"uart1\""), okRespCallback, NULL,
                              500);

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

RIL_ATSndError RIL_initialize(UART_HandleTypeDef* uart, RIL_URCIndicationCallback urcCb,
                              RIL_PowerCommandCallback powerCommandCb,
                              RIL_InitialResultCallback initialResultCb) {
    if (rilCtx.initialized) {
        if (initialResultCb) {
            initialResultCb(RIL_AT_SUCCESS);
        }
        return RIL_AT_SUCCESS;
    }

    rilCtx.urcIndicationCallback = urcCb;
    rilCtx.powerCommandCallback = powerCommandCb;
    rilCtx.error = RIL_AT_UNINITIALIZED;
    rilCtx.state = RIL_READY;

    UARTStream_init(&rilCtx.stream, uart, rilCtx.streamRxBuff, RIL_RX_STREAM_SIZE,
                    rilCtx.streamTxBuff, RIL_TX_STREAM_SIZE);

    // start receive
    IStream_receive(&rilCtx.stream.Input);
    rilCtx.operationMode = RIL_OperationMode_Normal;
    rilCtx.initialized = true;

#if RIL_USE_OS
    // Initialize mutex for thread-safe access
    rilCtx.mutex_attr.name = "RIL_Mutex";
    rilCtx.mutex_attr.attr_bits = osMutexRecursive;
    rilCtx.mutex = osMutexNew(&rilCtx.mutex_attr);
    if (rilCtx.mutex == NULL) {
        return RIL_AT_FAILED;
    }
#endif

    // Initialize with power management and restart logic
    uint8_t retryCount = 0;
    bool moduleRestarted = false;
    RIL_ATSndError ret;

    while (retryCount < 3) {
        // Try to initialize the module
        ret = RIL_deInitialize();

        if (ret == RIL_AT_SUCCESS) {
            // Check if we need to restart the module
            if (moduleRestarted) {
                if (initialResultCb) {
                    initialResultCb(RIL_AT_SUCCESS);
                }
                return RIL_AT_SUCCESS;
            } else {
                // Power off and restart the module
                RIL_PowerRestart(rilCtx.powerCommandCallback);
				_RIL_Delay(1000);

                moduleRestarted = true;
                retryCount++;
            }
        } else {
            // Initialization failed, try power cycle
            RIL_PowerRestart(rilCtx.powerCommandCallback);
			_RIL_Delay(1000);
            moduleRestarted = true;
            retryCount++;
        }
    }

    return RIL_AT_TIMEOUT;
}

void RIL_ServiceRoutine(void) {
    // Only process URC when RIL is ready (not busy with AT command)
    // This is safe because AT commands use Mutex for exclusive access
    if (rilCtx.state == RIL_READY && IStream_available(&rilCtx.stream.Input) > 0) {
        char lineCache[RIL_LINE_LEN];
        Stream_LenType len = IStream_readBytesUntilPattern(
            &rilCtx.stream.Input, (uint8_t*) CRLF, CRLFLen, (uint8_t*) lineCache, RIL_LINE_LEN);
        if (len > CRLFLen) {
            len -= CRLFLen;
            lineCache[len] = 0; // Null-terminate the received line

            RIL_LOG_TRACE("RIL_ServiceRoutine received: %s", lineCache);

            RIL_URCInfo urcInfo;
            bool urcDetected = RIL_ParseURC(lineCache, &urcInfo); // Parse URC parameters
            if (urcDetected) {
                RIL_LOG_TRACE("URC detected: %u ,%s", urcInfo.urcType, lineCache);
                if (rilCtx.urcIndicationCallback != NULL) {
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

Stream_Result RIL_errorHandle(void) {
    UARTStream_errorHandle(&rilCtx.stream);
    return Stream_Ok;
}

// ============================================================================
// Helper Functions for Common Operations
// ============================================================================

/**
 * @brief Get current system tick
 */
static uint32_t _RIL_GetCurrentTick(void) {
#if RIL_USE_OS
    return osKernelGetTickCount();
#else
    return HAL_GetTick();
#endif
}

/**
 * @brief Delay for specified milliseconds
 */
static void _RIL_Delay(uint32_t ms) {
#if RIL_USE_OS
    osDelay(ms);
#else
    HAL_Delay(ms);
#endif
}

/**
 * @brief Acquire exclusive access to RIL and validate initialization
 * @return RIL_AT_SUCCESS if acquired, RIL_AT_UNINITIALIZED otherwise
 */
static RIL_ATSndError _RIL_AcquireAccess(void) {
    _RIL_ERROR_RESET();
    _RIL_Lock();

    if (!rilCtx.initialized) {
        _RIL_Unlock();
        return RIL_AT_UNINITIALIZED;
    }

    rilCtx.state = RIL_BUSY;
    return RIL_AT_SUCCESS;
}

/**
 * @brief Release RIL access and reset state
 * @param resetOperationMode If true, reset operation mode to normal
 */
static void _RIL_ReleaseAccess(bool resetOperationMode) {
    rilCtx.state = RIL_READY;
    if (resetOperationMode) {
        RIL_SetOperationNormal();
    }
    _RIL_Unlock();
}

/**
 * @brief Wait for and process response from module
 * @param lineCache Buffer to store response line
 * @param timeOut Absolute timeout tick value
 * @param atRsp_callBack Response callback function
 * @param userData User data for callback
 * @param checkEcho If true, check for command echo
 * @param echoCmd Command string to match for echo (only used if checkEcho is true)
 * @return RIL_AT_SUCCESS, RIL_AT_FAILED, or RIL_AT_TIMEOUT
 */
static RIL_ATSndError _RIL_WaitForResponse(char* lineCache, uint32_t timeOut,
                                           Callback_ATResponse atRsp_callBack, void* userData,
                                           bool checkEcho, const char* echoCmd) {
    bool echoSeen = !checkEcho; // If not checking echo, consider it already seen

    while (_RIL_GetCurrentTick() < timeOut) {
        if (IStream_available(&rilCtx.stream.Input) > 0) {
            Stream_LenType len = IStream_readBytesUntilPattern(
                &rilCtx.stream.Input, (uint8_t*) CRLF, CRLFLen, (uint8_t*) lineCache, RIL_LINE_LEN);

            if (len > CRLFLen) {
                len -= CRLFLen;
                if (lineCache[len - 1] == '\r') {
                    len--;
                }
                lineCache[len] = 0;

                // Check for echo if needed
                if (checkEcho && !echoSeen && echoCmd != NULL) {
                    if (Str_compareFix(lineCache, echoCmd, len) == 0) {
                        RIL_LOG_TRACE("Echo seen: %s", lineCache);
                        echoSeen = true;
                        continue;
                    } else {
                        RIL_LOG_TRACE("Response line: %s", lineCache);
                    }
                }

                if (echoSeen) {
                    // Check for error response
                    uint16_t errCode;
                    _RIL_ERROR_SET(0);
                    if (_lineIsError(lineCache, len, &errCode) > 0) {
                        RIL_LOG_ERROR("Error response: %s", lineCache);
                        _RIL_ERROR_SET(errCode);
                        return RIL_AT_FAILED;
                    }

                    // Process through callback if provided
                    if (atRsp_callBack != NULL) {
                        // Check if response is printable for logging
                        bool allPrintable = true;
                        for (uint32_t i = 0; i < len; i++) {
                            if (lineCache[i] >= 127) {
                                allPrintable = false;
                                break;
                            }
                        }
                        if (allPrintable) {
                            RIL_LOG_TRACE("Response: %s", lineCache);
                        }

                        int32_t ret = atRsp_callBack(lineCache, len, userData);
                        if (ret == RIL_ATRSP_CONTINUE) {
                            if (_lineIsURC(lineCache)) {
                                RIL_URCInfo urc;
                                RIL_ParseURC(lineCache, &urc);
                                RIL_LOG_TRACE("URC received: %s", lineCache);
                            }
                            continue;
                        }
                    }
                }

                return RIL_AT_SUCCESS;
            }
        }
        _RIL_Delay(1);
    }

    RIL_LOG_TRACE("Response timeout");
    return RIL_AT_TIMEOUT;
}

/**
 * @brief Send chunked binary data with buffer management
 * @param data Data to send
 * @param dataLen Length of data
 * @return Stream_Ok on success, error code otherwise
 */
static Stream_Result _RIL_SendChunkedData(const uint8_t* data, uint32_t dataLen) {
    uint32_t totalSent = 0;

    while (totalSent < dataLen) {
        Stream_LenType availableSpace = OStream_space(&rilCtx.stream.Output);
        uint32_t remainingData = dataLen - totalSent;
        uint32_t chunkSize = (remainingData < availableSpace) ? remainingData : availableSpace;

        if (chunkSize == 0) {
            RIL_LOG_TRACE("Buffer full, waiting for space...");
            uint32_t waitStart = _RIL_GetCurrentTick();
            while (OStream_space(&rilCtx.stream.Output) == 0) {
                if (_RIL_GetCurrentTick() - waitStart > 5000) {
                    RIL_LOG_ERROR("Timeout waiting for buffer space");
                    return Stream_NoSpace;
                }
                _RIL_Delay(10);
            }
            continue;
        }

        Stream_Result err =
            OStream_writeBytes(&rilCtx.stream.Output, (uint8_t*) (data + totalSent), chunkSize);
        if (err) {
            RIL_LOG_ERROR("Failed to write chunk: %u bytes at offset %u, error=%d", chunkSize,
                          totalSent, err);
            return err;
        }

        totalSent += chunkSize;
        RIL_LOG_TRACE("Chunk written: %u bytes (total: %u/%u)", chunkSize, totalSent, dataLen);

        err = OStream_flush(&rilCtx.stream.Output);
        if (err && err != Stream_InTransmit) {
            RIL_LOG_ERROR("Failed to flush: error=%d", err);
            return err;
        }
    }

    return Stream_Ok;
}

/**
 * @brief Wait for all pending data to be transmitted
 * @param dataLen Length of data sent (used for timeout calculation)
 * @return Stream_Ok on success, error code otherwise
 */
static Stream_Result _RIL_WaitForTransmitComplete(uint32_t dataLen) {
    uint32_t flushTimeout = (dataLen / 10) + 2000;
    uint32_t flushStart = _RIL_GetCurrentTick();
    uint32_t lastPending = OStream_pendingBytes(&rilCtx.stream.Output);
    uint32_t stuckCount = 0;
    const uint32_t progressCheckInterval = 10;

    while (OStream_pendingBytes(&rilCtx.stream.Output) > 0) {
        if (_RIL_GetCurrentTick() - flushStart > flushTimeout) {
            RIL_LOG_ERROR("Transmission timeout: %u bytes still pending",
                          OStream_pendingBytes(&rilCtx.stream.Output));
            return Stream_TransmitFailed;
        }

        uint32_t currentPending = OStream_pendingBytes(&rilCtx.stream.Output);
        if (currentPending == lastPending) {
            stuckCount++;
            if (stuckCount > 100) {
                RIL_LOG_ERROR("Transmission stuck: %u bytes pending, no progress for 1s",
                              currentPending);
                return Stream_TransmitFailed;
            }
        } else {
            if (stuckCount > 0) {
                RIL_LOG_TRACE("Progress: %u bytes remaining", currentPending);
            }
            stuckCount = 0;
            lastPending = currentPending;
        }

        _RIL_Delay(progressCheckInterval);
    }

    RIL_LOG_TRACE("All data transmitted successfully: %u bytes", dataLen);
    return Stream_Ok;
}

// ============================================================================
// Main API Functions
// ============================================================================

RIL_ATSndError _RIL_SendATCmd(const char* atCmd, uint32_t atCmdLen,
                              Callback_ATResponse atRsp_callBack, void* userData,
                              bool waitForPrompt, uint32_t timeOut) {
    // Acquire exclusive access
    RIL_ATSndError accessResult = _RIL_AcquireAccess();
    if (accessResult != RIL_AT_SUCCESS) {
        return accessResult;
    }

    static char lineCache[RIL_LINE_LEN];
    RIL_LOG_TRACE("Sending AT command: %s", atCmd);

    // Set default timeout if not specified (5 seconds)
    if (timeOut == 0) {
        timeOut = 5000;
    }
    uint32_t absoluteTimeout = _RIL_GetCurrentTick() + timeOut;

    // Prepare command: copy to buffer and append CRLF
    Str_copyFix(lineCache, atCmd, atCmdLen);
    lineCache[atCmdLen] = 0;
    Str_append(lineCache, CRLF);
    const char* cmdWithCRLF = lineCache;
    uint32_t cmdLen = strlen(lineCache);

    // Send command
    Stream_Result streamErr =
        OStream_writeBytes(&rilCtx.stream.Output, (uint8_t*) cmdWithCRLF, cmdLen);
    if (streamErr == Stream_Ok) {
        streamErr = OStream_flush(&rilCtx.stream.Output);
    }
    if (streamErr) {
        RIL_LOG_TRACE("AT command send failed");
        _RIL_ReleaseAccess(true);
        return RIL_AT_FAILED;
    }

    // Wait for response with special handling for prompt and binary modes
    bool echoSeen = false;
    while (_RIL_GetCurrentTick() < absoluteTimeout) {
        if (IStream_available(&rilCtx.stream.Input) > 0) {
            Stream_LenType len;

            // Check for prompt character if waiting for it
            if (waitForPrompt) {
                len = IStream_readBytesUntil(&rilCtx.stream.Input, (uint8_t) '>',
                                             (uint8_t*) lineCache, RIL_LINE_LEN);
                if (len > 0) {
                    RIL_LOG_TRACE("Prompt character '>' detected");
                    _RIL_ReleaseAccess(true);
                    return RIL_AT_SUCCESS;
                }
            }

            // Handle based on operation mode
            if (rilCtx.operationMode == RIL_OperationMode_Normal) {
                len = IStream_readBytesUntilPattern(&rilCtx.stream.Input, (uint8_t*) CRLF, CRLFLen,
                                                    (uint8_t*) lineCache, RIL_LINE_LEN);
                if (len > CRLFLen) {
                    len -= CRLFLen;
                    if (lineCache[len - 1] == '\r') {
                        len--;
                    }
                    lineCache[len] = 0;

                    // Check for command echo
                    if (!echoSeen && Str_compareFix(lineCache, cmdWithCRLF, len) == 0) {
                        RIL_LOG_TRACE("Echo seen: %s", lineCache);
                        echoSeen = true;
                        continue;
                    } else {
                        RIL_LOG_TRACE("AT command response: %s", lineCache);
                    }

                    if (echoSeen) {
                        // Check for error response
                        uint16_t errCode;
                        _RIL_ERROR_SET(0);
                        if (_lineIsError(lineCache, len, &errCode) > 0) {
                            RIL_LOG_ERROR("AT command failed: %s", atCmd);
                            _RIL_ERROR_SET(errCode);
                            _RIL_ReleaseAccess(true);
                            return RIL_AT_FAILED;
                        }

                        // Process through callback
                        if (atRsp_callBack != NULL) {
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
                                }
                                continue;
                            }
                        }
                    }

                    RIL_LOG_TRACE("AT command success");
                    _RIL_ReleaseAccess(true);
                    return RIL_AT_SUCCESS;
                }
            } else if (rilCtx.operationMode == RIL_OperationMode_Binary &&
                       rilCtx.expectedBytes > 0) {
                // Binary mode: read expected number of bytes
                streamErr = IStream_readBytes(&rilCtx.stream.Input, (uint8_t*) lineCache,
                                              rilCtx.expectedBytes);
                if (streamErr == Stream_Ok) {
                    if (atRsp_callBack != NULL) {
                        int32_t ret = atRsp_callBack(lineCache, rilCtx.expectedBytes, userData);
                        if (ret == RIL_ATRSP_CONTINUE) {
                            RIL_SetOperationNormal();
                            continue;
                        } else if (ret == RIL_ATRSP_SUCCESS) {
                            _RIL_ReleaseAccess(true);
                            return RIL_AT_SUCCESS;
                        } else {
                            _RIL_ReleaseAccess(true);
                            return RIL_AT_FAILED;
                        }
                    }
                    _RIL_ReleaseAccess(true);
                    return RIL_AT_SUCCESS;
                }
            }
        }
        _RIL_Delay(1);
    }

    RIL_LOG_TRACE("AT command timeout");
    _RIL_ReleaseAccess(true);
    return RIL_AT_TIMEOUT;
}

RIL_ATSndError RIL_SendBinaryData(const uint8_t* data, uint32_t dataLen,
                                  Callback_ATResponse atRsp_callBack, void* userData,
                                  uint32_t timeOut) {
    // Acquire exclusive access
    RIL_ATSndError accessResult = _RIL_AcquireAccess();
    if (accessResult != RIL_AT_SUCCESS) {
        return accessResult;
    }

    char lineCache[RIL_LINE_LEN];
    RIL_LOG_TRACE("Sending raw data, length: %u", dataLen);

    // Set default timeout if not specified (3 minutes)
    if (timeOut == 0) {
        timeOut = 180000;
    }
    uint32_t absoluteTimeout = _RIL_GetCurrentTick() + timeOut;

    // Send data in chunks with buffer management
    Stream_Result streamErr = _RIL_SendChunkedData(data, dataLen);
    if (streamErr != Stream_Ok) {
        RIL_LOG_TRACE("Binary data send failed");
        _RIL_ReleaseAccess(false);
        return RIL_AT_FAILED;
    }

    RIL_LOG_TRACE("All data written to buffer, waiting for transmission to complete...");

    // Wait for transmission to complete
    streamErr = _RIL_WaitForTransmitComplete(dataLen);
    if (streamErr != Stream_Ok) {
        RIL_LOG_TRACE("Binary data transmission failed");
        _RIL_ReleaseAccess(false);
        return RIL_AT_FAILED;
    }

    // Wait for response using shared helper (no echo checking for binary data)
    RIL_ATSndError result =
        _RIL_WaitForResponse(lineCache, absoluteTimeout, atRsp_callBack, userData, false, NULL);

    if (result == RIL_AT_SUCCESS) {
        RIL_LOG_TRACE("Binary data success");
    } else if (result == RIL_AT_TIMEOUT) {
        RIL_LOG_TRACE("Binary data timeout");
    } else {
        RIL_LOG_TRACE("Binary data failed");
    }

    _RIL_ReleaseAccess(false);
    return result;
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

bool RIL_IsModulePowered(void) {
    RIL_ATSndError ret = RIL_SendATCmd("AT", 2, okRespCallback, NULL, 500);
    return ret == RIL_AT_SUCCESS;
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

static void RIL_PowerRestart(RIL_PowerCommandCallback powerCommandCb) {
    if (powerCommandCb) {
        powerCommandCb(RIL_POWER_COMMAND_RESTART);
    }
}

static void _RIL_Lock(void) {
#if RIL_USE_OS
    if (rilCtx.mutex != NULL) {
        osMutexAcquire(rilCtx.mutex, osWaitForever);
    }
#endif
}

static void _RIL_Unlock(void) {
#if RIL_USE_OS
    if (rilCtx.mutex != NULL) {
        osMutexRelease(rilCtx.mutex);
    }
#endif
}