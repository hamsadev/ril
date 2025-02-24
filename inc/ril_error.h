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
  RIL_AT_SUCCESS = 0, /**< succeed in executing AT command, and the response is OK. */
  RIL_AT_TIMEOUT = -2, /**< Send AT timeout. */
  RIL_AT_FAILED = -1, /**< Fail to execute AT command, or the response is ERROR.*/
  RIL_AT_BUSY = -3, /**< Sending AT. */
  RIL_AT_INVALID_PARAM = -4, /**< Invalid input parameter. */
  RIL_AT_UNINITIALIZED = -5, /**< RIL is not ready, need to call RIL_initialize function.
                                      you may call Ql_RIL_AT_GetErrCode() to get the
                                      specific error code.*/
}RIL_ATSndError;

typedef enum {
  RIL_ATRSP_FAILED = -1, // AT command run FAIL
  RIL_ATRSP_SUCCESS = 0, // AT command run SUCCESS.
  RIL_ATRSP_CONTINUE = 1, // Need to wait later AT response
}RIL_ATRspError;

typedef enum {
  RIL_ERROR_AT = 0, /**< The response is ERROR.*/
  RIL_ERROR_EQPT = 1, /**< Fail to execute AT command.*/
}RIL_ErrorType;

typedef struct {
  union {
    uint32_t atError;            /**< Check it if type is RIL_ERROR_AT.*/
    Stream_Result eqptError;     /**< Check it if type is RIL_ERROR_EQPT.*/
  };
  RIL_ErrorType type;
}RIL_Error;

#endif //_RIL_ERROR_H_