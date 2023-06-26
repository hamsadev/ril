#include "ril_system.h"
#include "ril.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int32_t capacity;
    int32_t voltage;
} ST_SysPower;

static int32_t ATResponse_Handler(char* line, uint32_t len, void* param);
static int32_t Power_ATResponse_Hanlder(char* line, uint32_t len, void* param);
static int32_t PowerOff_ATResponse_Handler(char* line, uint32_t len, void* param);

RIL_ATSndError RIL_SYS_GetIMEI(char* imei) {
    if (imei == NULL)
        return RIL_AT_INVALID_PARAM;
    return RIL_SendATCmd("AT+GSN", 6, ATResponse_Handler, (void*) imei, 0);
}

RIL_ATSndError RIL_SYS_GetFirmwareVersion(char* firmware) {
    if (firmware == NULL)
        return RIL_AT_INVALID_PARAM;
    return RIL_SendATCmd("AT+CGMR", 7, ATResponse_Handler, (void*) firmware, 0);
}

RIL_ATSndError RIL_SYS_GetManufacturer(char* manufacturer) {
    if (manufacturer == NULL)
        return RIL_AT_INVALID_PARAM;
    return RIL_SendATCmd("AT+GMI", 6, ATResponse_Handler, (void*) manufacturer, 0);
}

RIL_ATSndError RIL_SYS_GetModel(char* model) {
    if (model == NULL)
        return RIL_AT_INVALID_PARAM;
    return RIL_SendATCmd("AT+GMM", 6, ATResponse_Handler, (void*) model, 0);
}

RIL_ATSndError RIL_SYS_GetSerialNumber(char* serial) {
    if (serial == NULL)
        return RIL_AT_INVALID_PARAM;
    return RIL_SendATCmd("AT+CGSN=2", 10, ATResponse_Handler, (void*) serial, 0);
}

RIL_ATSndError RIL_SYS_GetCCID(char* ccid) {
    if (ccid == NULL)
        return RIL_AT_INVALID_PARAM;
    return RIL_SendATCmd("AT+CCID", 7, ATResponse_Handler, (void*) ccid, 0);
}

RIL_ATSndError RIL_SYS_GetCoreVersion(char* coreVersion) {
    if (coreVersion == NULL)
        return RIL_AT_INVALID_PARAM;
    return RIL_SendATCmd("AT+SBLVER", 9, ATResponse_Handler, (void*) coreVersion, 0);
}


RIL_ATSndError RIL_SYS_PowerOff(RIL_SYS_PowerOffMode mode) {
    if (mode != RIL_SYS_POWER_OFF_MODE_NORMAL && mode != RIL_SYS_POWER_OFF_MODE_IMMEDIATE)
        return RIL_AT_INVALID_PARAM;

    char cmd[12];
    uint8_t len = sprintf(cmd, "AT+QPOWD=%d", mode);
    return RIL_SendATCmd(cmd, len, PowerOff_ATResponse_Handler, NULL, 500);
}

/**
 * @brief Retrieves all system details in a single formatted report.
 */
RIL_ATSndError RIL_SYS_GetFullSystemReport(char* report) {
    if (report == NULL)
        return RIL_AT_INVALID_PARAM;

    char imei[32] = {0};
    char firmware[64] = {0};
    char manufacturer[32] = {0};
    char model[32] = {0};
    char serial[32] = {0};
    char ccid[32] = {0};
    char coreVersion[32] = {0};

    RIL_SYS_GetIMEI(imei);
    RIL_SYS_GetFirmwareVersion(firmware);
    RIL_SYS_GetManufacturer(manufacturer);
    RIL_SYS_GetModel(model);
    RIL_SYS_GetSerialNumber(serial);
    RIL_SYS_GetCCID(ccid);
    RIL_SYS_GetCoreVersion(coreVersion);

    sprintf(report,
            "System Report:\n"
            "IMEI: %s\n"
            "Firmware Version: %s\n"
            "Manufacturer: %s\n"
            "Model: %s\n"
            "Serial Number: %s\n"
            "SIM CCID: %s\n"
            "Core Version: %s\n",
            imei, firmware, manufacturer, model, serial, ccid, coreVersion);

    return RIL_AT_SUCCESS;
}

RIL_ATSndError RIL_SYS_GetPowerSupply(uint32_t* capacity, uint32_t* voltage) {
    RIL_ATSndError ret;
    ST_SysPower PowerSupply;

    ret = RIL_SendATCmd("AT+CBC", 6, Power_ATResponse_Hanlder, (void*) &PowerSupply, 0);
    if (RIL_AT_SUCCESS == ret) {
        *capacity = PowerSupply.capacity;
        *voltage = PowerSupply.voltage;
    }
    return ret;
}

static int32_t ATResponse_Handler(char* line, uint32_t len, void* param) {
    char* _param = (char*) param;

    int cmpRet = NULL;
    cmpRet = strncmp(line, "OK", len);
    if (!cmpRet) {
        return RIL_ATRSP_SUCCESS;
    }

    return RIL_ATRSP_CONTINUE; // continue wait
}

static int32_t Power_ATResponse_Hanlder(char* line, uint32_t len, void* param) {
    ST_SysPower* PowerSupply;

    PowerSupply = (ST_SysPower*) param;
    char* head = strstr(line, "+CBC:"); // continue wait
    if (head) {
        char strTmp[10];
        char *p1, *p2;
        p1 = strstr(head, ":");
        p2 = strstr(p1 + 1, ",");
        if (p1 && p2) {
            p1 = p2;
            p2 = strstr(p1 + 1, ",");
            if (p1 && p2) {
                memset(strTmp, 0x0, sizeof(strTmp));
                memcpy(strTmp, p1 + 1, p2 - p1 - 1);
                PowerSupply->capacity = atoi(strTmp);
                p1 = p2;
                p2 = strstr(p1 + 1, "\r\n");
                if (p1 && p2) {
                    memset(strTmp, 0x0, sizeof(strTmp));
                    memcpy(strTmp, p1 + 1, p2 - p1 - 1);
                    PowerSupply->voltage = atoi(strTmp);
                }
            }
        }
        return RIL_ATRSP_CONTINUE;
    }

    int cmpRet = strncmp(line, "OK", len);
    if (cmpRet) {
        return RIL_ATRSP_SUCCESS;
    }

    return RIL_ATRSP_CONTINUE; // continue wait
}


static int32_t PowerOff_ATResponse_Handler(char* line, uint32_t len, void* param) {
    int cmpRet = strncmp(line, "OK", len);
    if (cmpRet) {
        return RIL_ATRSP_CONTINUE;
    }

    cmpRet = strncmp(line, "POWERED DOWN", len);
    if (cmpRet) {
        return RIL_ATRSP_SUCCESS;
    }

    return RIL_ATRSP_CONTINUE;
}
