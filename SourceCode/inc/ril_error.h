/**
 * @file ril_error.h
 * @author Hamid Salehi (hamsam.dev@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-12-07
 *
 * @copyright Copyright (c) 2023 Hamid Salehi
 *
 */

#ifndef _RIL_ERROR_H_
#define _RIL_ERROR_H_

#include "StreamBuffer.h"

/**
 * Define the return value of RIL_SendATCmd.
 */
typedef enum {
    RIL_AT_SUCCESS = 0,        /**< succeed in executing AT command, and the response is OK. */
    RIL_AT_TIMEOUT = -2,       /**< Send AT timeout. */
    RIL_AT_FAILED = -1,        /**< Fail to execute AT command, or the response is ERROR.*/
    RIL_AT_BUSY = -3,          /**< Sending AT. */
    RIL_AT_INVALID_PARAM = -4, /**< Invalid input parameter. */
    RIL_AT_UNINITIALIZED = -5, /**< RIL is not ready, need to call RIL_initialize function.
                                        you may call Ql_RIL_AT_GetErrCode() to get the
                                        specific error code.*/
} RIL_ATSndError;

typedef enum {
    RIL_ATRSP_FAILED = -1,  // AT command run FAIL
    RIL_ATRSP_SUCCESS = 0,  // AT command run SUCCESS.
    RIL_ATRSP_CONTINUE = 1, // Need to wait later AT response
} RIL_ATRspError;

typedef enum {
    RIL_Error_Success = -1,
    RIL_Error_PhoneFailure = 0,
    RIL_Error_NoConnectionToPhone = 1,
    RIL_Error_PhoneAdaptorLinkReserved = 2,
    RIL_Error_OperationNotAllowed = 3,
    RIL_Error_OperationNotSupported = 4,
    RIL_Error_PhSimPinRequired = 5,
    RIL_Error_PhFsimPinRequired = 6,
    RIL_Error_PhFsimPukRequired = 10,
    RIL_Error_SimNotInserted = 11,
    RIL_Error_SimPinRequired = 12,
    RIL_Error_SimPukRequired = 13,
    RIL_Error_SimFailure = 14,
    RIL_Error_SimBusy = 15,
    RIL_Error_SimWrong = 16,
    RIL_Error_IncorrectPassword = 17,
    RIL_Error_SimPin2Required = 18,
    RIL_Error_SimPuk2Required = 19,
    RIL_Error_MemoryFull = 20,
    RIL_Error_InvalidIndex = 21,
    RIL_Error_NotFound = 22,
    RIL_Error_MemoryFailure = 23,
    RIL_Error_TextStringTooLong = 24,
    RIL_Error_InvalidCharactersInTextString = 25,
    RIL_Error_DialStringTooLong = 26,
    RIL_Error_InvalidCharactersInDialString = 27,
    RIL_Error_NoNetworkService = 30,
    RIL_Error_NetworkTimeout = 31,
    RIL_Error_NetworkNotAllowed = 32, // emergency calls only
    RIL_Error_NetworkPersonalizationPinRequired = 40,
    RIL_Error_NetworkPersonalizationPukRequired = 41,
    RIL_Error_NetworkSubsetPersonalizationPinRequired = 42,
    RIL_Error_NetworkSubsetPersonalizationPukRequired = 43,
    RIL_Error_ServiceProviderPersonalizationPinRequired = 44,
    RIL_Error_ServiceProviderPersonalizationPukRequired = 45,
    RIL_Error_CorporatePersonalizationPinRequired = 46,
    RIL_Error_CorporatePersonalizationPukRequired = 47,
    RIL_Error_AudioUnknownError = 901,
    RIL_Error_AudioInvalidParameters = 902,
    RIL_Error_AudioOperationNotSupported = 903,
    RIL_Error_AudioDeviceBusy = 904,
} RIL_Error;

#endif //_RIL_ERROR_H_