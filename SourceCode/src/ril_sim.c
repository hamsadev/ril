#include "ril_sim.h"
#include "ril.h"
#include <stdio.h>
#include <string.h>

static int32_t ATRsp_CCID_Handler(char* line, uint32_t len, void* param);
static int32_t ATRsp_IMSI_Handler(char* line, uint32_t len, void* param);
static int32_t ATResponse_CPIN_Handler(char* line, uint32_t len, void* userdata);
static int32_t RIL_SIM_GetSimStateByErrCode(int32_t errCode);

RIL_ATSndError RIL_SIM_GetSimState(Enum_SIMState* stat) {
    int32_t retRes = -1;
    int32_t nStat = 0;
    static const char strAT[] = "AT+CPIN?";

    retRes = RIL_SendATCmd((char*) strAT, strlen(strAT), ATResponse_CPIN_Handler, &nStat, 0);
    if (RIL_AT_SUCCESS == retRes) {
        *stat = nStat;
    }
    return retRes;
}

RIL_ATSndError RIL_SIM_GetIMSI(char* imsi) {
    static const char strAT[] = "AT+CIMI";
    if (NULL == imsi) {
        return RIL_AT_INVALID_PARAM;
    }
    return RIL_SendATCmd((char*) strAT, strlen(strAT), ATRsp_IMSI_Handler, (void*) imsi, 0);
}

RIL_ATSndError RIL_SIM_GetCCID(char* ccid) {
    static const char strAT[] = "AT+CCID";
    if (NULL == ccid) {
        return RIL_AT_INVALID_PARAM;
    }
    return RIL_SendATCmd((char*) strAT, strlen(strAT), ATRsp_CCID_Handler, (void*) ccid, 0);
}

static int32_t ATRsp_CCID_Handler(char* line, uint32_t len, void* param) {
    char* _param = (char*) param;

    int cmpRet = strncmp(line, "OK", len);
    if (cmpRet == 0) {
        return RIL_ATRSP_SUCCESS;
    }

    memcpy(_param, line, len);
    _param[len] = 0;

    return RIL_ATRSP_CONTINUE; // continue wait
}

static int32_t ATRsp_IMSI_Handler(char* line, uint32_t len, void* param) {
    char* _param = (char*) param;

    int cmpRet = strncmp(line, "OK", len);
    if (cmpRet == 0) {
        return RIL_ATRSP_SUCCESS;
    }

    memcpy(_param, line, len);
    _param[len] = '\0';

    return RIL_ATRSP_CONTINUE; // continue wait
}

static int32_t ATResponse_CPIN_Handler(char* line, uint32_t len, void* userdata) {
    Enum_SIMState* simStat = (Enum_SIMState*) userdata;
    if (strstr(line, "READY")) {
        *simStat = SIM_STAT_READY;
        return RIL_ATRSP_SUCCESS;
    } else if (strstr(line, "NOT INSERTED")) {
        *simStat = SIM_STAT_NOT_INSERTED;
        return RIL_ATRSP_SUCCESS;
    } else if (strstr(line, "SIM PIN")) {
        *simStat = SIM_STAT_PIN_REQ;
        return RIL_ATRSP_SUCCESS;
    } else if (strstr(line, "SIM PUK")) {
        *simStat = SIM_STAT_PUK_REQ;
        return RIL_ATRSP_SUCCESS;
    } else if (strstr(line, "PH-SIM PIN")) {
        *simStat = SIM_STAT_PH_PIN_REQ;
        return RIL_ATRSP_SUCCESS;
    } else if (strstr(line, "PH-SIM PUK")) {
        *simStat = SIM_STAT_PH_PUK_REQ;
        return RIL_ATRSP_SUCCESS;
    } else if (strstr(line, "SIM PIN2")) {
        *simStat = SIM_STAT_PIN2_REQ;
        return RIL_ATRSP_SUCCESS;
    } else if (strstr(line, "SIM PUK2")) {
        *simStat = SIM_STAT_PUK2_REQ;
        return RIL_ATRSP_SUCCESS;
    } else if (strstr(line, "SIM BUSY")) {
        *simStat = SIM_STAT_BUSY;
        return RIL_ATRSP_SUCCESS;
    } else if (strstr(line, "NOT READY")) {
        *simStat = SIM_STAT_NOT_READY;
        return RIL_ATRSP_SUCCESS;
    }

    return RIL_ATRSP_CONTINUE; // continue wait
}

static int32_t RIL_SIM_GetSimStateByErrCode(int32_t errCode) {
    int32_t ss;
    switch (errCode) {
    case 10:
        ss = SIM_STAT_NOT_INSERTED;
        break;
    case 11:
        ss = SIM_STAT_PIN_REQ;
        break;
    case 12:
        ss = SIM_STAT_PUK_REQ;
        break;
    case 13:
    case 15:
    case 16:
        ss = SIM_STAT_UNSPECIFIED;
        break;
    case 14:
        ss = SIM_STAT_BUSY;
        break;
    case 17:
        ss = SIM_STAT_PIN2_REQ;
        break;
    case 18:
        ss = SIM_STAT_PUK2_REQ;
        break;
    default:
        ss = SIM_STAT_UNSPECIFIED;
        break;
    }
    return ss;
}
