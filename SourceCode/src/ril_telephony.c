#include "ril_telephony.h"
#include "ril.h"
#include "ril_util.h"
#include <stdint.h>
#include <stdio.h>

static int32_t Telephony_Dial_AT_handler(char* line, uint32_t len, void* userdata) {
    int8_t cmpRet = strncmp(line, "NO DIALTONE", len);
    if (!cmpRet) {
        (*(int32_t*) userdata) = CALL_STATE_NO_DIALTONE;
        return RIL_ATRSP_SUCCESS;
    }

    cmpRet = strncmp(line, "BUSY", len);
    if (!cmpRet) {
        (*(int32_t*) userdata) = CALL_STATE_BUSY;
        return RIL_ATRSP_SUCCESS;
    }

    cmpRet = strncmp(line, "NO CARRIER", len);
    if (!cmpRet) {
        (*(int32_t*) userdata) = CALL_STATE_NO_CARRIER;
        return RIL_ATRSP_SUCCESS;
    }

    cmpRet = strncmp(line, "OK", len);
    if (!cmpRet) {
        (*(int32_t*) userdata) = CALL_STATE_OK;
        return RIL_ATRSP_SUCCESS;
    }

    return RIL_ATRSP_CONTINUE; // continue wait
}

RIL_ATSndError RIL_Telephony_Dial(uint8_t type, char* phoneNumber, Enum_CallState* result) {
    char strAT[40];
    if (NULL == phoneNumber) {
        return -1;
    }
    memset(strAT, 0x0, sizeof(strAT));
    sprintf(strAT, "ATD%s;", phoneNumber);
    return RIL_SendATCmd(strAT, strlen(strAT), Telephony_Dial_AT_handler, (void*) result, 0);
}

static int32_t Telephony_Answer_AT_handler(char* line, uint32_t len, void* userdata) {
    int8_t cmpRet;
    cmpRet = strncmp(line, "OK", len);
    if (!cmpRet) {
        (*(int32_t*) userdata) = CALL_STATE_OK;
        return RIL_ATRSP_SUCCESS;
    }

    cmpRet = strncmp(line, "NO CARRIER", len);
    if (!cmpRet) {
        (*(int32_t*) userdata) = CALL_STATE_NO_CARRIER;
        return RIL_ATRSP_SUCCESS;
    }

    return RIL_ATRSP_CONTINUE; // continue wait
}

RIL_ATSndError RIL_Telephony_Answer(int32_t* result) {
    return RIL_SendATCmd("ATA", 3, Telephony_Answer_AT_handler, result, 0);
}

RIL_ATSndError RIL_Telephony_Hangup(void) {
    return RIL_SendATCmd("ATH", 3, NULL, NULL, 0);
}
