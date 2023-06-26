#ifndef _RIL_SIM_H_
#define _RIL_SIM_H_

#include "ril.h"
#include <stdint.h>

/**
 * @brief  Definition for SIM Card State
 */
typedef enum {
    SIM_STAT_NOT_INSERTED = 0,
    SIM_STAT_READY,
    SIM_STAT_PIN_REQ,
    SIM_STAT_PUK_REQ,
    SIM_STAT_PH_PIN_REQ,
    SIM_STAT_PH_PUK_REQ,
    SIM_STAT_PIN2_REQ,
    SIM_STAT_PUK2_REQ,
    SIM_STAT_BUSY,
    SIM_STAT_NOT_READY,
    SIM_STAT_UNSPECIFIED
} Enum_SIMState;

/**
 * @brief This function gets the state of SIM card.
 * Related AT: "AT+CPIN?".
 *
 * @param state [out]SIM card State code, one value of Enum_SIMState.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_SIM_GetSimState(Enum_SIMState* state);

/**
 * @brief This function gets the state of SIM card.
 * Related AT: "AT+CIMI".
 *
 * @param imsi [out]IMSI number, a string of 15-byte.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_SIM_GetIMSI(char* imsi);

/**
 * @brief This function gets the CCID of SIM card.
 * Related AT: "AT+CCID".
 *
 * @param ccid [out] CCID number, a string of 20-byte.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_SIM_GetCCID(char* ccid);

#endif // _RIL_SIM_H_
