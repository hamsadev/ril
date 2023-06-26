#ifndef __RIL_TELEPHONY_H__
#define __RIL_TELEPHONY_H__

#include <stdint.h>
#include "ril_error.h"

#define PHONE_NUMBER_MAX_LEN 41

typedef enum {
    CALL_STATE_ERROR = -1,
    CALL_STATE_OK = 0,
    CALL_STATE_BUSY,
    CALL_STATE_NO_ANSWER,
    CALL_STATE_NO_CARRIER,
    CALL_STATE_NO_DIALTONE,
    CALL_STATE_END
} Enum_CallState;

/**
 * @brief This function dials the specified phone number, only support voice call.
 *
 * @param type [In] Must be 0 , just support voice call.
 * @param phoneNumber [In] Phone number, null-terminated string.
 * @param result [Out] Result for dial, one value of Enum_CallState.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_Telephony_Dial(uint8_t type, char* phoneNumber, Enum_CallState* result);

/**
 * @brief This function answers a coming call.
 *
 * @param result [Out] Delete flag , which is one value of 'Enum_SMSDeleteFlag'.
 * @return int32_t
 */
RIL_ATSndError RIL_Telephony_Answer(int32_t* result);

/**
 * @brief This function answers a call.
 *
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_Telephony_Hangup(void);

#endif
