#include "ril_network.h"
#include "ril.h"
#include "ril_util.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// EC200U AT Command Timeout Values (in milliseconds)
#define EC200U_TIMEOUT_BASIC 5000          // Basic commands (CSQ, CREG, CGREG)
#define EC200U_TIMEOUT_CONFIG 30000        // Configuration commands (APN, DNS)
#define EC200U_TIMEOUT_PDP_ACTIVATE 150000 // PDP context activation/deactivation
#define EC200U_TIMEOUT_NETWORK_REG 180000  // Network registration commands
#define EC200U_TIMEOUT_EXTENDED 10000      // Extended signal quality, access tech
#define EC200U_TIMEOUT_OPERATOR 15000      // Operator selection/query

// AT response handlers for EC200U network commands

static int32_t ATResponse_CREG_Handler(char* line, uint32_t len, void* userdata) {
    char* head = strstr(line, "+CREG:");
    if (head) {
        int32_t n = 0;
        int32_t* state = (int32_t*) userdata;
        // EC200U format: +CREG: n,stat[,lac,ci[,act]]
        sscanf(head, "+CREG: %d,%d", &n, state);
        return RIL_ATRSP_CONTINUE;
    }

    int cmpRet = strncmp(line, "OK", len);
    if (cmpRet == 0) {
        return RIL_ATRSP_SUCCESS;
    }

    return RIL_ATRSP_CONTINUE;
}

RIL_ATSndError RIL_NW_GetGSMState(RIL_NW_State* stat) {
    if (stat == NULL) {
        return RIL_AT_INVALID_PARAM;
    }

    static const char strAT[] = "AT+CREG?";
    RIL_ATSndError retRes = RIL_SendATCmd((char*) strAT, sizeof(strAT) - 1, ATResponse_CREG_Handler,
                                          stat, EC200U_TIMEOUT_BASIC);
    return retRes;
}

static int32_t ATResponse_CGREG_Handler(char* line, uint32_t len, void* userdata) {
    char* head = strstr(line, "+CGREG:");
    if (head) {
        int32_t n = 0;
        int32_t* state = (int32_t*) userdata;
        // EC200U format: +CGREG: n,stat[,lac,ci[,act,rac]]
        sscanf(head, "+CGREG: %d,%d", &n, state);
        return RIL_ATRSP_CONTINUE;
    }

    int cmpRet = strncmp(line, "OK", len);
    if (cmpRet == 0) {
        return RIL_ATRSP_SUCCESS;
    }

    return RIL_ATRSP_CONTINUE;
}

RIL_ATSndError RIL_NW_GetGPRSState(RIL_NW_State* stat) {
    if (stat == NULL) {
        return RIL_AT_INVALID_PARAM;
    }

    RIL_NW_State nStat = RIL_NW_State_NotRegistered;
    static const char strAT[] = "AT+CGREG?";

    RIL_ATSndError retRes = RIL_SendATCmd((const char*) strAT, sizeof(strAT) - 1,
                                          ATResponse_CGREG_Handler, &nStat, EC200U_TIMEOUT_BASIC);
    if (retRes == RIL_AT_SUCCESS) {
        *stat = nStat;
    }
    return retRes;
}

static int32_t ATResponse_CSQ_Handler(char* line, uint32_t len, void* userdata) {
    RIL_NW_CSQReponse* CSQ_Reponse = (RIL_NW_CSQReponse*) userdata;

    char* head = strstr(line, "+CSQ:");
    if (head) {
        // EC200U format: +CSQ: rssi,ber
        sscanf(head, "+CSQ: %d,%d", &CSQ_Reponse->rssi, &CSQ_Reponse->ber);
        return RIL_ATRSP_CONTINUE;
    }

    int cmpRet = strncmp(line, "OK", len);
    if (cmpRet == 0) {
        return RIL_ATRSP_SUCCESS;
    }

    return RIL_ATRSP_CONTINUE;
}

RIL_ATSndError RIL_NW_GetSignalQuality(uint8_t* rssi, uint8_t* ber) {
    if (rssi == NULL || ber == NULL) {
        return RIL_AT_INVALID_PARAM;
    }

    static const char strAT[] = "AT+CSQ";
    RIL_NW_CSQReponse pCSQ_Reponse;
    memset(&pCSQ_Reponse, 0, sizeof(pCSQ_Reponse));

    RIL_ATSndError retRes = RIL_SendATCmd(strAT, sizeof(strAT) - 1, ATResponse_CSQ_Handler,
                                          (void*) &pCSQ_Reponse, EC200U_TIMEOUT_BASIC);
    if (retRes == RIL_AT_SUCCESS) {
        *rssi = pCSQ_Reponse.rssi;
        *ber = pCSQ_Reponse.ber;
    }

    return retRes;
}

static int32_t ATResponse_QICSGP_Handler(char* line, uint32_t len, void* userdata) {
    if (strncmp(line, "OK", len) == 0)
        return RIL_ATRSP_SUCCESS;

    return RIL_ATRSP_CONTINUE;
}

RIL_ATSndError RIL_NW_SetAPNEx(uint8_t pdp_id, RIL_NW_AuthType authType,
                               RIL_NW_ContextType contextType, char* apn, char* userName,
                               char* pw) {
    if (apn == NULL) {
        return RIL_AT_INVALID_PARAM;
    }

    char strAT[512];
    // EC200U Format:
    // AT+QICSGP=<contextID>,<context_type>,"<APN>"[,"<username>","<password>"[,<authentication>]]
    uint32_t len = snprintf(strAT, sizeof(strAT), "AT+QICSGP=%d,%d,\"%s\",\"%s\",\"%s\",%d", pdp_id,
                            contextType, apn, userName ? userName : "", pw ? pw : "", authType);

    if (len >= sizeof(strAT)) {
        return RIL_AT_INVALID_PARAM; // Command too long
    }

    RIL_ATSndError res =
        RIL_SendATCmd(strAT, len, ATResponse_QICSGP_Handler, NULL, EC200U_TIMEOUT_CONFIG);
    return res;
}

RIL_ATSndError RIL_NW_SetAPN(uint8_t pdp_id, RIL_NW_ContextType contextType, char* apn) {
    if (apn == NULL) {
        return RIL_AT_INVALID_PARAM;
    }

    char* contextTypeStr = (contextType == RIL_NW_ContextType_IPV4)     ? "IP"
                           : (contextType == RIL_NW_ContextType_IPV6)   ? "IPV6"
                           : (contextType == RIL_NW_ContextType_IPv6V4) ? "IPV4V6"
                           : (contextType == RIL_NW_ContextType_PPP)    ? "PPP"
                                                                        : NULL;

    if (contextTypeStr == NULL) {
        return RIL_AT_INVALID_PARAM;
    }

    char strAT[512];
    // EC200U Format:
    // AT+CGDCONT=<contextID>[,<context_type>,"<APN>"]
    uint32_t len =
        snprintf(strAT, sizeof(strAT), "AT+CGDCONT=%d,\"%s\",\"%s\"", pdp_id, contextTypeStr, apn);

    if (len >= sizeof(strAT)) {
        return RIL_AT_INVALID_PARAM; // Command too long
    }

    RIL_ATSndError res =
        RIL_SendATCmd(strAT, len, ATResponse_QICSGP_Handler, NULL, EC200U_TIMEOUT_CONFIG);
    return res;
}

static int32_t ATResponse_IPStatus_Handler(char* line, uint32_t len, void* userdata) {
    int32_t* state = (int32_t*) userdata;
    char* head = strstr(line, "+QIACT:");
    if (head) {
        int cid = 0, ctx_state = 0, ctx_type = 0;
        // EC200U format: +QIACT: <contextID>,<context_state>,<context_type>[,<IP_address>]
        int parsed = sscanf(head, "+QIACT: %d,%d,%d", &cid, &ctx_state, &ctx_type);
        if (parsed >= 2) {
            *state = ctx_state;
        }
        return RIL_ATRSP_CONTINUE;
    }

    if (strncmp(line, "OK", len) == 0)
        return RIL_ATRSP_SUCCESS;

    return RIL_ATRSP_CONTINUE;
}

RIL_ATSndError RIL_NW_GetIPStatus(uint8_t* state) {
    if (state == NULL) {
        return RIL_AT_INVALID_PARAM;
    }

    int32_t activeState = 0;
    const char strAT[] = "AT+QIACT?";
    RIL_ATSndError retRes = RIL_SendATCmd(strAT, sizeof(strAT) - 1, ATResponse_IPStatus_Handler,
                                          &activeState, EC200U_TIMEOUT_BASIC);
    if (retRes == RIL_AT_SUCCESS) {
        *state = activeState;
    }
    return retRes;
}

static int32_t ATResponse_IP_Handler(char* line, uint32_t len, void* userdata) {
    char* ip = (char*) userdata;
    char* head = strstr(line, "+QIACT:");
    if (head) {
        int cid = 0, ctx_state = 0, ctx_type = 0;
        char temp_ip[16] = {0};
        // EC200U format: +QIACT: <contextID>,<context_state>,<context_type>[,<IP_address>]
        int parsed =
            sscanf(head, "+QIACT: %d,%d,%d,\"%15[^\"]\"", &cid, &ctx_state, &ctx_type, temp_ip);
        if (parsed >= 4) {
            strcpy(ip, temp_ip);
        }
        return RIL_ATRSP_CONTINUE;
    }

    if (strncmp(line, "OK", len) == 0)
        return RIL_ATRSP_SUCCESS;

    return RIL_ATRSP_CONTINUE;
}

RIL_ATSndError RIL_NW_GetIP(char* ip) {
    if (ip == NULL) {
        return RIL_AT_INVALID_PARAM;
    }

    const char strAT[] = "AT+QIACT?";
    return RIL_SendATCmd(strAT, sizeof(strAT) - 1, ATResponse_IP_Handler, ip, EC200U_TIMEOUT_BASIC);
}

static int32_t ATResponse_QIACT_Handler(char* line, uint32_t len, void* userdata) {
    if (strstr(line, "OK"))
        return RIL_ATRSP_SUCCESS;

    return RIL_ATRSP_CONTINUE;
}

static int32_t ATResponse_QNTP_Handler(char* line, uint32_t len, void* userdata) {
    RIL_ATRspError ret = RIL_ATRSP_CONTINUE;
    if (strncmp(line, "OK", 2) == 0) {
        ret = RIL_ATRSP_CONTINUE;
    } else if (strncmp(line, "+QNTP:", 6) == 0) {
        int _err;
        int* err = userdata == NULL ? &_err : (int*) userdata;

        //+QNTP: <err>,"<time>"
        uint8_t parsed = sscanf(line, "+QNTP: %d,\"%*[^\"]\"", err);
        if (parsed == 1) {
            ret = RIL_ATRSP_SUCCESS;
        } else {
            parsed = sscanf(line, "+QNTP: %d", err);
            if (parsed == 1) {
                ret = RIL_ATRSP_FAILED;
            }
        }
        RIL_AT_SetErrCode(*err);
    }

    return ret;
}

RIL_ATSndError RIL_NW_SetNTPEx(uint8_t pdp_id, const char* server, uint16_t port, bool autoSetTime,
                               uint8_t retryCount, uint8_t retryInterval, RIL_NW_Error* err) {
    if (server == NULL) {
        return RIL_AT_INVALID_PARAM;
    }

    char strAT[100];
    uint8_t strAtLen = snprintf(strAT, sizeof(strAT), "AT+QNTP=%d,\"%s\",%d,%d,%d,%d", pdp_id,
                                server, port, autoSetTime, retryCount, retryInterval);
    return RIL_SendATCmd(strAT, strAtLen, ATResponse_QNTP_Handler, err, EC200U_TIMEOUT_CONFIG);
}

RIL_ATSndError RIL_NW_SetNTP(uint8_t pdp_id, const char* server, uint16_t port, RIL_NW_Error* err) {
    return RIL_NW_SetNTPEx(pdp_id, server, port, true, 3, 15, err);
}

RIL_ATSndError RIL_NW_GetNTP(uint8_t pdp_id, char* time) {
    if (time == NULL) {
        return RIL_AT_INVALID_PARAM;
    }

    static const char strAT[] = "AT+QNTP?";
    return RIL_SendATCmd(strAT, sizeof(strAT) - 1, ATResponse_QNTP_Handler, time,
                         EC200U_TIMEOUT_CONFIG);
}

// EC200U supports PDP context activation using QIACT command
RIL_ATSndError RIL_NW_OpenPDPContext(uint8_t pdp_id) {
    if (pdp_id < 1 || pdp_id > 15) {
        return RIL_AT_INVALID_PARAM;
    }

    char strAT[20];
    uint8_t strAtLen = snprintf(strAT, sizeof(strAT), "AT+QIACT=%d", pdp_id);
    return RIL_SendATCmd(strAT, strAtLen, ATResponse_QIACT_Handler, NULL,
                         EC200U_TIMEOUT_PDP_ACTIVATE);
}

RIL_ATSndError RIL_NW_ClosePDPContext(uint8_t pdp_id) {
    if (pdp_id < 1 || pdp_id > 15) {
        return RIL_AT_INVALID_PARAM;
    }

    char strAT[20];
    uint8_t strAtLen = snprintf(strAT, sizeof(strAT), "AT+QIDEACT=%d", pdp_id);
    return RIL_SendATCmd(strAT, strAtLen, ATResponse_QIACT_Handler, NULL,
                         EC200U_TIMEOUT_PDP_ACTIVATE);
}

RIL_ATSndError RIL_NW_SetDNS(uint8_t pdp_id, const char* primaryServer,
                             const char* secondaryServer) {
    if (primaryServer == NULL || secondaryServer == NULL) {
        return RIL_AT_INVALID_PARAM;
    }

    char strAT[100];
    // EC200U supports DNS configuration via QIDNSCFG
    uint8_t strAtLen = snprintf(strAT, sizeof(strAT), "AT+QIDNSCFG=%d,\"%s\",\"%s\"", pdp_id,
                                primaryServer, secondaryServer);
    return RIL_SendATCmd(strAT, strAtLen, ATResponse_QIACT_Handler, NULL, EC200U_TIMEOUT_CONFIG);
}

static int32_t ATRsp_COPS_Handler(char* line, uint32_t len, void* param) {
    char* pStr = (char*) param;
    char* pHead = strstr(line, "+COPS:");
    if (pHead) {
        char* p = NULL;
        char* q = NULL;
        p = pHead + strlen("+COPS: ");

        // Skip the first parameter (mode)
        while (*p && *p != ',')
            p++;
        if (*p == ',')
            p++;

        // Skip the second parameter (format)
        while (*p && *p != ',')
            p++;
        if (*p == ',')
            p++;

        // Now we're at the operator name
        q = strstr(p, "\"");
        if (q) { // Format: +COPS: 0,0,"OPERATOR_NAME"
            p = q + 1;
            q = strstr(p, "\"");
            if (q != NULL) {
                memcpy(pStr, p, q - p);
                pStr[q - p] = '\0';
            }
        } else { // Format: +COPS: 0
            *pStr = '\0';
        }
        return RIL_ATRSP_SUCCESS;
    }

    int cmpRet = strncmp(line, "OK", len);
    if (cmpRet == 0) {
        return RIL_ATRSP_SUCCESS;
    }

    return RIL_ATRSP_CONTINUE;
}

RIL_ATSndError RIL_NW_GetOperator(char* operator) {
    if (operator == NULL) {
        return RIL_AT_INVALID_PARAM;
    }

    char strAT[] = "AT+COPS?";
    return RIL_SendATCmd(strAT, sizeof(strAT) - 1, ATRsp_COPS_Handler, (void*) operator,
                         EC200U_TIMEOUT_OPERATOR);
}

// ===== EC200U Specific Functions =====

static int32_t ATResponse_ExtSignal_Handler(char* line, uint32_t len, void* userdata) {
    RIL_NW_ExtendedSignalInfo* signalInfo = (RIL_NW_ExtendedSignalInfo*) userdata;

    // Handle QCSQ response for extended signal quality
    char* head = strstr(line, "+QCSQ:");
    if (head) {
        char sysmode[10] = {0};
        // EC200U format: +QCSQ: "sysmode",rssi,rsrp,sinr,rsrq
        sscanf(head, "+QCSQ: \"%9[^\"]\",,%d,%d,%d,%d", sysmode, &signalInfo->rssi,
               &signalInfo->rsrp, &signalInfo->sinr, &signalInfo->rsrq);
        return RIL_ATRSP_CONTINUE;
    }

    if (strncmp(line, "OK", len) == 0) {
        return RIL_ATRSP_SUCCESS;
    }

    return RIL_ATRSP_CONTINUE;
}

RIL_ATSndError RIL_NW_GetExtendedSignalQuality(RIL_NW_ExtendedSignalInfo* signalInfo) {
    if (signalInfo == NULL) {
        return RIL_AT_INVALID_PARAM;
    }

    memset(signalInfo, 0, sizeof(RIL_NW_ExtendedSignalInfo));
    const char strAT[] = "AT+QCSQ";
    return RIL_SendATCmd(strAT, sizeof(strAT) - 1, ATResponse_ExtSignal_Handler, signalInfo,
                         EC200U_TIMEOUT_EXTENDED);
}

static int32_t ATResponse_AccessTech_Handler(char* line, uint32_t len, void* userdata) {
    RIL_NW_AccessTech* act = (RIL_NW_AccessTech*) userdata;

    char* head = strstr(line, "+CREG:");
    if (head) {
        int n = 0, stat = 0, act_val = 0;
        // Try to parse with access technology: +CREG: n,stat[,lac,ci[,act]]
        int parsed = sscanf(head, "+CREG: %d,%d,%*d,%*d,%d", &n, &stat, &act_val);
        if (parsed >= 3) {
            *act = (RIL_NW_AccessTech) act_val;
        }
        return RIL_ATRSP_CONTINUE;
    }

    if (strncmp(line, "OK", len) == 0) {
        return RIL_ATRSP_SUCCESS;
    }

    return RIL_ATRSP_CONTINUE;
}

RIL_ATSndError RIL_NW_GetAccessTechnology(RIL_NW_AccessTech* act) {
    if (act == NULL) {
        return RIL_AT_INVALID_PARAM;
    }

    *act = RIL_NW_AccessTech_GSM; // Default value
    const char strAT[] = "AT+CREG?";
    return RIL_SendATCmd(strAT, sizeof(strAT) - 1, ATResponse_AccessTech_Handler, act,
                         EC200U_TIMEOUT_EXTENDED);
}

static int32_t ATResponse_Simple_Handler(char* line, uint32_t len, void* userdata) {
    if (strncmp(line, "OK", len) == 0) {
        return RIL_ATRSP_SUCCESS;
    }

    return RIL_ATRSP_CONTINUE;
}

RIL_ATSndError RIL_NW_SetRegistrationURC(bool enable) {
    char strAT[30];

    if (enable) {
        // Enable network registration URC with location info
        snprintf(strAT, sizeof(strAT), "AT+CREG=2");
    } else {
        // Disable network registration URC
        snprintf(strAT, sizeof(strAT), "AT+CREG=0");
    }

    return RIL_SendATCmd(strAT, sizeof(strAT) - 1, ATResponse_Simple_Handler, NULL,
                         EC200U_TIMEOUT_NETWORK_REG);
}

RIL_ATSndError RIL_NW_SetNetworkSelection(uint8_t mode, const char* operatorCode) {
    char strAT[100];

    if (mode > 4) {
        return RIL_AT_INVALID_PARAM;
    }

    uint8_t strAtLen = 0;
    if (mode == 0) {
        // Automatic network selection
        strAtLen = snprintf(strAT, sizeof(strAT), "AT+COPS=0");
    } else if (mode == 1 && operatorCode != NULL) {
        // Manual network selection with operator code
        strAtLen = snprintf(strAT, sizeof(strAT), "AT+COPS=1,2,\"%s\"", operatorCode);
    } else if (mode == 2) {
        // Deregister from network
        strAtLen = snprintf(strAT, sizeof(strAT), "AT+COPS=2");
    } else if (mode == 4 && operatorCode != NULL) {
        // Manual with automatic fallback
        strAtLen = snprintf(strAT, sizeof(strAT), "AT+COPS=4,2,\"%s\"", operatorCode);
    } else {
        return RIL_AT_INVALID_PARAM;
    }

    return RIL_SendATCmd(strAT, strAtLen, ATResponse_Simple_Handler, NULL, EC200U_TIMEOUT_OPERATOR);
}
