#include "ril_socket.h"
#include <string.h>

static int32_t ATHandler_QIOPEN(char* line, uint32_t len, void* userData) {
    uint8_t* pCid = (uint8_t*) userData;
    int cid = -1, err = -1;
    if (strncmp(line, "+QIOPEN:", 8) == 0) {
        // +QIOPEN: <connectId>,<err>
        sscanf(line, "+QIOPEN: %d,%d", &cid, &err);
        if (err == 0 && pCid)
            *pCid = (uint8_t) cid;
        return RIL_ATRSP_SUCCESS;
    }
    if (strncmp(line, "OK", len) == 0) {
        return RIL_ATRSP_CONTINUE;
    }

    return RIL_ATRSP_CONTINUE;
}

RIL_SocErrCode RIL_SOC_Open(uint8_t contextId, RIL_SocType type, const char* host,
                            uint16_t remotePort, uint16_t localPort, RIL_SocAccessMode mode,
                            uint8_t* out_connectId) {
    char cmd[128];
    const char* srv = (type == RIL_SOC_TYPE_TCP            ? "TCP"
                       : type == RIL_SOC_TYPE_UDP          ? "UDP"
                       : type == RIL_SOC_TYPE_TCP_LISTENER ? "TCP LISTENER"
                                                           : "UDP SERVICE");
    snprintf(cmd, sizeof(cmd), "AT+QIOPEN=%d,0,\"%s\",\"%s\",%u,%u,%u", contextId, srv, host,
             remotePort, localPort, mode);
    RIL_ATSndError res = RIL_SendATCmd(cmd, strlen(cmd), ATHandler_QIOPEN, out_connectId, 0);
    return (res == RIL_AT_SUCCESS ? RIL_SOC_SUCCESS : RIL_SOC_ERR_GENERAL);
}

// Close socket
static int32_t ATHandler_QICLOSE(char* line, uint32_t len, void* userData) {
    if (strncmp(line, "OK", len) == 0)
        return RIL_ATRSP_SUCCESS;
    if (strstr(line, "+QICLOSE:"))
        return RIL_ATRSP_SUCCESS;

    return RIL_ATRSP_CONTINUE;
}

RIL_SocErrCode RIL_SOC_Close(uint8_t connectId) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+QICLOSE=%u", connectId);
    RIL_ATSndError res = RIL_SendATCmd(cmd, strlen(cmd), ATHandler_QICLOSE, NULL, 0);
    return (res == RIL_AT_SUCCESS ? RIL_SOC_SUCCESS : RIL_SOC_ERR_GENERAL);
}

// Send data (buffer/direct modes)
static int32_t ATHandler_QISEND(char* line, uint32_t len, void* userData) {
    if (strncmp(line, ">", 1) == 0)
        return RIL_ATRSP_CONTINUE; // prompt
    if (strstr(line, "SEND OK"))
        return RIL_ATRSP_SUCCESS;
    if (strstr(line, "SEND FAIL"))
        return RIL_ATRSP_FAILED;
    if (strstr(line, "ERROR"))
        return RIL_ATRSP_FAILED;
    return RIL_ATRSP_CONTINUE;
}

RIL_SocErrCode RIL_SOC_Send(uint8_t connectId, const uint8_t* data, uint32_t length,
                            const char* peerIp, uint16_t peerPort) {
    char cmd[64];
    if (peerIp) {
        snprintf(cmd, sizeof(cmd), "AT+QISEND=%u,%u,\"%s\",%u", connectId, length, peerIp,
                 peerPort);
    } else {
        snprintf(cmd, sizeof(cmd), "AT+QISEND=%u,%u", connectId, length);
    }
    RIL_ATSndError res = RIL_SendATCmd(cmd, strlen(cmd), ATHandler_QISEND, NULL, 0);
    if (res != RIL_AT_SUCCESS)
        return RIL_SOC_ERR_GENERAL;
    // send actual data
    RIL_ATSndError r2 = RIL_SendATCmd((char*) data, length, ATHandler_QISEND, NULL, 0);
    return (r2 == RIL_AT_SUCCESS ? RIL_SOC_SUCCESS : RIL_SOC_ERR_GENERAL);
}

// Receive into internal buffer (buffer mode only)
static int32_t ATHandler_QIRD(char* line, uint32_t len, void* userData) {
    uint32_t* outLen = (uint32_t*) userData;
    if (strncmp(line, "+QIURC: \"recv\"", 12) == 0) {
        // URC only indicates data ready
        return RIL_ATRSP_CONTINUE;
    }
    if (strncmp(line, "+QIRD:", 6) == 0) {
        int cid = 0, rd = 0;
        sscanf(line, "+QIRD: %d,%d", &cid, &rd);
        if (outLen)
            *outLen = rd;
        return RIL_ATRSP_CONTINUE;
    }
    if (strncmp(line, "OK", len) == 0)
        return RIL_ATRSP_SUCCESS;
    if (strstr(line, "ERROR"))
        return RIL_ATRSP_FAILED;
    return RIL_ATRSP_CONTINUE;
}

RIL_SocErrCode RIL_SOC_Recv(uint8_t connectId, uint8_t* buffer, uint32_t bufferLen,
                            uint32_t* out_len) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+QIRD=%u,%u", connectId, bufferLen);
    // first send QIRD to get length and data
    RIL_ATSndError res = RIL_SendATCmd(cmd, strlen(cmd), ATHandler_QIRD, out_len, 0);
    if (res != RIL_AT_SUCCESS)
        return RIL_SOC_ERR_GENERAL;
    // then read the actual data frame via RIL_SendATCmd with buffer and length
    // (left as exercise)
    return RIL_SOC_SUCCESS;
}

// State query
static int32_t ATHandler_QISTATE(char* line, uint32_t len, void* userData) {
    if (strncmp(line, "+QISTATE:", 9) == 0) {
        // Example: +QISTATE: 0,"TCP","10.137.3.138",12345,1,1,1
        int cid = 0, rport = 0, state = 0;
        char type[8] = {0};
        char ip[32] = {0};
        sscanf(line, "+QISTATE: %d,\\\"%7[^\\\"]\\\",\\\"%31[^\\\"]\\\",%d,%*d,%*d,%d", &cid, type,
               ip, &rport, &state);
        RIL_LOG_TRACE("Socket[%d]: %s %s:%d (state=%d)\n", cid, type, ip, rport, state);
        return RIL_ATRSP_CONTINUE;
    }
    if (strncmp(line, "OK", len) == 0)
        return RIL_ATRSP_SUCCESS;
    if (strstr(line, "ERROR"))
        return RIL_ATRSP_FAILED;
    return RIL_ATRSP_CONTINUE;
}

RIL_SocErrCode RIL_SOC_GetStates(void) {
    RIL_ATSndError res =
        RIL_SendATCmd("AT+QISTATE", strlen("AT+QISTATE"), ATHandler_QISTATE, NULL, 0);
    return (res == RIL_AT_SUCCESS ? RIL_SOC_SUCCESS : RIL_SOC_ERR_GENERAL);
}
