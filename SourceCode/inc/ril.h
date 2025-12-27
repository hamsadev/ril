#ifndef _RIL_H_
#define _RIL_H_

#define RIL_RX_STREAM_SIZE 512
#define RIL_TX_STREAM_SIZE 512

#include "StreamBuffer.h"
#include "log.h"
#include "ril_error.h"
#include "ril_urc.h"
#include <stdbool.h>

#define RIL_USE_OS 0
#define RIL_LOG_ENABLE 0

#if RIL_LOG_ENABLE
#define RIL_LOG_TRACE(fmt, ...) logTrace(fmt, ##__VA_ARGS__)
#define RIL_LOG_WARN(fmt, ...) logWarn(fmt, ##__VA_ARGS__)
#define RIL_LOG_ERROR(fmt, ...) logError(fmt, ##__VA_ARGS__)
#define RIL_LOG_DEBUG(fmt, ...) logDebug(fmt, ##__VA_ARGS__)
#define RIL_LOG_INFO(fmt, ...) logInfo(fmt, ##__VA_ARGS__)
#else
#define RIL_LOG_TRACE(fmt, ...)
#define RIL_LOG_WARN(fmt, ...)
#define RIL_LOG_ERROR(fmt, ...)
#define RIL_LOG_DEBUG(fmt, ...)
#define RIL_LOG_INFO(fmt, ...)
#endif

#define RIL_STR(S) S, sizeof(S) - 1

typedef enum {
    RIL_OperationMode_Normal = 0, // Normal AT command mode
    RIL_OperationMode_Binary = 1, // Binary data mode
} RIL_OperationMode;

typedef enum {
    RIL_BUSY,
    RIL_READY,
} RILState;

typedef enum {
    RIL_POWER_COMMAND_OFF,
    RIL_POWER_COMMAND_ON,
    RIL_POWER_COMMAND_RESTART,
} RIL_PowerCommand;

/**
 * @brief RIL_SendATCmd response callback type
 * @param line [in]Response line
 * @param len [in]Response length
 * @param userData [in]User data
 * @return Response result
 *         - RIL_ATRSP_SUCCESS: Success
 *         - RIL_ATRSP_CONTINUE: Continue
 *         - RIL_ATRSP_FAILED: Failed
 */
typedef int32_t (*Callback_ATResponse)(char* line, uint32_t len, void* userData);

/**
 * @brief URC indication callback type
 * @param info [in]URC information
 */
typedef void (*RIL_URCIndicationCallback)(RIL_URCInfo* info);


/**
 * @brief Power command callback type
 * @param powerCmd [in]Power command
 * @param delayMs [in]Delay in milliseconds
 */
typedef void (*RIL_PowerCommandCallback)(RIL_PowerCommand powerCmd, uint32_t delayMs);

/**
 * @brief Initialization result callback type
 * @param result [in]Initialization result (RIL_ATSndError)
 */
typedef void (*RIL_InitialResultCallback)(RIL_ATSndError result);

/**
 * @brief Deinitialize the RIL device, this function is used to deinitialize the RIL device
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_deInitialize(void);

/**
 * @brief This function initializes RIl-related functions.
 *
 * @param uart
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_initialize(UART_HandleTypeDef* uart, RIL_URCIndicationCallback urcCb, RIL_PowerCommandCallback powerCommandCb, RIL_InitialResultCallback initialResultCb);

/**
 * @brief Service routine for processing URC (Unsolicited Response Codes)
 * 
 * This function should be called continuously to detect and process URC messages
 * from the cellular module. It checks for incoming URC messages when RIL is in
 * READY state and forwards them to the registered URC callback.
 * 
 * @note This function is thread-safe and uses mutex to prevent conflicts with
 *       concurrent AT command processing.
 * @note RIL_ServiceRoutine should be called with a short delay (e.g., 50ms)
 *       in a dedicated task to ensure responsive URC detection.
 */
void RIL_ServiceRoutine(void);

/**
 * @brief call in interrupt or RxCplt callback for Async Receive
 *
 * @return Stream_Result
 */
Stream_Result RIL_rxCpltHandle(void);

Stream_Result RIL_errorHandle(void);

/**
 * @brief Get the current operation mode
 *
 * @param expectedBytes [out]Expected bytes to receive in binary mode
 * @return RIL_OperationMode
 */
RIL_OperationMode RIL_GetOperationMode(uint16_t* expectedBytes);

/**
 * @brief Set the operation mode to normal
 *
 */
void RIL_SetOperationNormal(void);

/**
 * @brief Set the operation mode to binary
 * @param expectedBytes [in]Expected bytes to receive in binary mode
 *
 */
void RIL_SetOperationBinary(uint16_t expectedBytes);

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
 * 
 * @note This function is thread-safe and uses mutex to ensure exclusive access.
 *       Multiple threads can safely call this function concurrently, and they will
 *       be serialized to prevent interference with each other.
 * 
 * @param atCmd [in]AT command string.
 * @param atCmdLen [in]The length of AT command string.
 * @param atRsp_callBack [in]Callback function for handle the response of the AT command.
 * @param userData [out]Used to transfer the customer's parameter.
 * @param waitForPrompt [in]If true, RIL will wait for the prompt character '>' before returning.
 * @param timeOut [in]Timeout for the AT command, unit in ms.
 *                    *If set it to 0, RIL uses the default timeout time (3min).
 * @return A member of RIL_ATSndError enum
 */
RIL_ATSndError _RIL_SendATCmd(const char* atCmd, uint32_t atCmdLen,
                              Callback_ATResponse atRsp_callBack, void* userData,
                              bool waitForPrompt, uint32_t timeOut);

/**
 * @brief This function implements sending binary data with the result
 *        being returned synchronously.
 *
 * @note This function is thread-safe and uses mutex to ensure exclusive access.
 * 
 * @param data [in]Pointer to the binary data to be sent.
 * @param dataLen [in]Length of the binary data to be sent.
 * @param atRsp_callBack [in]Callback function for handle the response of the AT command.
 * @param userData [out]Used to transfer the customer's parameter.
 * @param timeOut [in]Timeout for the AT command, unit in ms.
 *                    *If set it to 0, RIL uses the default timeout time (3min).
 * @return A member of RIL_ATSndError enum
 */
RIL_ATSndError RIL_SendBinaryData(const uint8_t* data, uint32_t dataLen,
                                  Callback_ATResponse atRsp_callBack, void* userData,
                                  uint32_t timeOut);

/**
 * @brief This function retrieves the specific error code after executing AT failed.
 *
 * @return uint16_t error code of the last AT command
 */
uint16_t RIL_AT_GetErrCode(void);

/**
 * @brief This function sets the specific error code after executing AT failed.
 *
 * @param errCode [in]error code of the last AT command
 */
void RIL_AT_SetErrCode(uint16_t errCode);

/**
 * @brief Get the current state of the RIL device
 *
 * @return RILState
 */
RILState RIL_GetState(void);

/**
 * @brief Check if the RIL module is powered and responding
 *
 * @return bool true if module is powered and responding, false otherwise
 */
bool RIL_IsModulePowered(void);

static inline RIL_ATSndError RIL_SendATCmd(const char* atCmd, uint32_t atCmdLen,
                                           Callback_ATResponse atRsp_callBack, void* userData,
                                           uint32_t timeOut) {
    return _RIL_SendATCmd(atCmd, atCmdLen, atRsp_callBack, userData, false, timeOut);
}

static inline RIL_ATSndError RIL_SendATCmdWithPrompt(const char* atCmd, uint32_t atCmdLen,
                                                     Callback_ATResponse atRsp_callBack,
                                                     void* userData, uint32_t timeOut) {
    return _RIL_SendATCmd(atCmd, atCmdLen, atRsp_callBack, userData, true, timeOut);
}

#endif //_RIL_H_
