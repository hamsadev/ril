#include "ril_mqtt_client.h"
#include "log.h"
#include "ril.h"
#include "ril_socket.h"
#include <string.h>

// Generic AT response handler for QMTCFG commands
static int32_t ATHandler_QMTCFG(char* line, uint32_t len, void* userData) {
    // QMTCFG commands typically return OK on success
    if (strncmp(line, "OK", 2) == 0) {
        return RIL_ATRSP_SUCCESS;
    }

    return RIL_ATRSP_CONTINUE;
}

// AT response handler for QMTRECV command
static int32_t ATHandler_QMTRECV(char* line, uint32_t len, void* userData) {
    typedef struct {
        char* topic;
        uint16_t topicLen;
        uint8_t* payload;
        uint16_t* payloadLen;
        uint16_t maxPayloadLen;
        bool dataFound;
    } QMTRECVData;

    QMTRECVData* data = (QMTRECVData*) userData;

    if (strncmp(line, "+QMTRECV:", 9) == 0) {
        // Parse: +QMTRECV: <client_idx>,<msgid>,"<topic>",<payload_len>,<payload>
        char* pos = line + 9; // Skip "+QMTRECV:"

        // Skip client_idx and msgid
        pos = strchr(pos, ',');
        if (pos)
            pos = strchr(pos + 1, ',');
        if (!pos)
            return RIL_ATRSP_CONTINUE;
        pos++;

        // Parse topic (quoted string)
        if (*pos == '"') {
            pos++;
            char* topicEnd = strchr(pos, '"');
            if (topicEnd) {
                size_t topicLength = topicEnd - pos;
                if (topicLength < data->topicLen) {
                    strncpy(data->topic, pos, topicLength);
                    data->topic[topicLength] = '\0';
                }
                pos = topicEnd + 1;
            }
        }

        // Skip to payload_len
        pos = strchr(pos, ',');
        if (pos) {
            pos++;
            uint16_t payloadLength = atoi(pos);

            // Skip to payload
            pos = strchr(pos, ',');
            if (pos) {
                pos += 2;
                // Copy payload data
                if (payloadLength <= data->maxPayloadLen) {
                    memcpy(data->payload, pos, payloadLength);
                    *(data->payloadLen) = payloadLength;
                    data->dataFound = true;
                    return RIL_ATRSP_SUCCESS;
                }
            }
        }
    }

    return RIL_ATRSP_CONTINUE;
}

RIL_MQTT_Err RIL_MQTT_ConfigReceiveMode(RIL_MQTTClient* client, MQTT_RecvMode recvMode,
                                        uint8_t msgLenEnable) {
    if (!client) {
        return RIL_MQTT_ERR_PARAM;
    }

    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+QMTCFG=\"recv/mode\",%u,%u,%u", client->clientIdx, recvMode,
             msgLenEnable);

    if (RIL_SendATCmd(cmd, strlen(cmd), ATHandler_QMTCFG, NULL, 5000) != RIL_AT_SUCCESS) {
        return RIL_MQTT_ERR_CFG_AT;
    }

    return RIL_MQTT_ERR_OK;
}

RIL_MQTT_Err RIL_MQTT_ConfigSSL(RIL_MQTTClient* client, MQTT_SSLMode sslMode, uint8_t sslCtxId) {
    if (!client || sslCtxId > 5) {
        return RIL_MQTT_ERR_PARAM;
    }

    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+QMTCFG=\"ssl\",%u,%u,%u", client->clientIdx, sslMode, sslCtxId);

    if (RIL_SendATCmd(cmd, strlen(cmd), ATHandler_QMTCFG, NULL, 5000) != RIL_AT_SUCCESS) {
        return RIL_MQTT_ERR_CFG_AT;
    }

    return RIL_MQTT_ERR_OK;
}

RIL_MQTT_Err RIL_MQTT_ConfigVersion(RIL_MQTTClient* client, MQTT_Version version) {
    if (!client || (version != MQTT_VERSION_3_1 && version != MQTT_VERSION_3_1_1)) {
        return RIL_MQTT_ERR_PARAM;
    }

    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+QMTCFG=\"version\",%u,%u", client->clientIdx, version);

    if (RIL_SendATCmd(cmd, strlen(cmd), ATHandler_QMTCFG, NULL, 5000) != RIL_AT_SUCCESS) {
        return RIL_MQTT_ERR_CFG_AT;
    }

    return RIL_MQTT_ERR_OK;
}

RIL_MQTT_Err RIL_MQTT_ConfigWill(RIL_MQTTClient* client, const MQTT_WillConfig* willConfig) {
    if (!client || !willConfig) {
        return RIL_MQTT_ERR_PARAM;
    }

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "AT+QMTCFG=\"will\",%u,%u,%u,%u,\"%s\",\"%s\"", client->clientIdx,
             willConfig->willEnable ? 1 : 0, willConfig->willQos, willConfig->willRetain ? 1 : 0,
             willConfig->willTopic, willConfig->willMessage);

    if (RIL_SendATCmd(cmd, strlen(cmd), ATHandler_QMTCFG, NULL, 5000) != RIL_AT_SUCCESS) {
        return RIL_MQTT_ERR_CFG_AT;
    }

    return RIL_MQTT_ERR_OK;
}

RIL_MQTT_Err RIL_MQTT_ConfigKeepAlive(RIL_MQTTClient* client, uint16_t timeout) {
    if (!client || timeout > 3600) {
        return RIL_MQTT_ERR_PARAM;
    }

    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+QMTCFG=\"keepalive\",%u,%u", client->clientIdx, timeout);

    if (RIL_SendATCmd(cmd, strlen(cmd), ATHandler_QMTCFG, NULL, 5000) != RIL_AT_SUCCESS) {
        return RIL_MQTT_ERR_CFG_AT;
    }

    return RIL_MQTT_ERR_OK;
}

RIL_MQTT_Err RIL_MQTT_ConfigCleanSession(RIL_MQTTClient* client, uint8_t cleanSession) {
    if (!client || cleanSession > 1) {
        return RIL_MQTT_ERR_PARAM;
    }

    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+QMTCFG=\"session\",%u,%u", client->clientIdx, cleanSession);

    if (RIL_SendATCmd(cmd, strlen(cmd), ATHandler_QMTCFG, NULL, 5000) != RIL_AT_SUCCESS) {
        return RIL_MQTT_ERR_CFG_AT;
    }

    return RIL_MQTT_ERR_OK;
}

RIL_MQTT_Err RIL_MQTT_ReadMessage(uint8_t clientIdx, uint8_t recvId, char* topic, uint16_t topicLen,
                                  uint8_t* payload, uint16_t* payloadLen) {
    if (!topic || !payload || !payloadLen || topicLen == 0 || *payloadLen == 0) {
        return RIL_MQTT_ERR_PARAM;
    }

    struct {
        char* topic;
        uint16_t topicLen;
        uint8_t* payload;
        uint16_t* payloadLen;
        uint16_t maxPayloadLen;
        bool dataFound;
    } recvData = {.topic = topic,
                  .topicLen = topicLen,
                  .payload = payload,
                  .payloadLen = payloadLen,
                  .maxPayloadLen = *payloadLen,
                  .dataFound = false};

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+QMTRECV=%u,%u", clientIdx, recvId);

    if (RIL_SendATCmd(cmd, strlen(cmd), ATHandler_QMTRECV, &recvData, 10000) != RIL_AT_SUCCESS) {
        return RIL_MQTT_ERR_CFG_AT;
    }

    if (!recvData.dataFound) {
        return RIL_MQTT_ERR_CFG_FAIL;
    }

    return RIL_MQTT_ERR_OK;
}

static int32_t ATHandler_QMTOPEN(char* line, uint32_t len, void* userData) {
    struct {
        int cid;
        int res;
    }* openRes = userData;
    if (strncmp(line, "+QMTOPEN:", 9) == 0) {
        sscanf(line, "+QMTOPEN: %d,%d", &openRes->cid, &openRes->res);
        return RIL_ATRSP_SUCCESS;
    }
    return RIL_ATRSP_CONTINUE;
}

// QMTCONN handler: checks return codes
static int32_t ATHandler_QMTCONN(char* line, uint32_t len, void* userData) {
    int* pres = (int*) userData;
    int cid, result, ret_code;
    if (strncmp(line, "+QMTCONN:", 9) == 0) {
        sscanf(line, "+QMTCONN: %d,%d,%d", &cid, &result, &ret_code);
        *pres = ((result == 0 && ret_code == 0) ? 0 : -1);
        return RIL_ATRSP_SUCCESS;
    }
    return RIL_ATRSP_CONTINUE;
}

RIL_MQTT_Err RIL_MQTT_InitClient(RIL_MQTTClient* client, const char* clientId, const char* username,
                                 const char* password, uint16_t keepAlive, bool cleanSession) {
    if (client == NULL || clientId == NULL || username == NULL || password == NULL) {
        return RIL_MQTT_ERR_PARAM;
    }

    memset(client, 0, sizeof(*client));
    strncpy(client->clientId, clientId, MQTT_MAX_CLIENTID_LEN - 1);
    strncpy(client->username, username, MQTT_MAX_CLIENTID_LEN - 1);
    strncpy(client->password, password, MQTT_MAX_CLIENTID_LEN - 1);
    client->keepAlive = keepAlive;
    client->cleanSession = cleanSession;

    return RIL_MQTT_ERR_OK;
}

RIL_MQTT_Err RIL_MQTT_Open(RIL_MQTTClient* client, const char* host, uint16_t port) {
    if (client == NULL || host == NULL) {
        return RIL_MQTT_ERR_PARAM;
    }
    if (port < 1 || port > 65535) {
        return RIL_MQTT_ERR_PARAM;
    }

    struct {
        int cid;
        int res;
    } openRes = {-1, -1};

    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT+QMTOPEN=0,\"%s\",%u", host, port);
    if (RIL_SendATCmd(cmd, strlen(cmd), ATHandler_QMTOPEN, &openRes, 30000) != RIL_AT_SUCCESS) {
        return RIL_MQTT_ERR_OPEN_AT;
    }

    if (openRes.res != 0) {
        RIL_LOG_ERROR("QMTOPEN failed, code=%d", openRes.res);
        return RIL_MQTT_ERR_OPEN_FAIL;
    }

    client->clientIdx = (uint8_t) openRes.cid;
    RIL_LOG_TRACE("QMTOPEN success, clientIdx=%u", client->clientIdx);

    return RIL_MQTT_ERR_OK;
}

RIL_MQTT_Err RIL_MQTT_Connect(RIL_MQTTClient* client) {
    if (client == NULL) {
        return RIL_MQTT_ERR_PARAM;
    }

    char cmd[128];
    int connRes = -1;
    snprintf(cmd, sizeof(cmd), "AT+QMTCONN=%u,\"%s\",\"%s\",\"%s\"", client->clientIdx,
             client->clientId, client->username, client->password);
    if (RIL_SendATCmd(cmd, strlen(cmd), ATHandler_QMTCONN, &connRes, 30000) != RIL_AT_SUCCESS) {
        return RIL_MQTT_ERR_CONN_AT;
    }

    if (connRes == 0) {
        client->isConnected = true;
        return RIL_MQTT_ERR_OK;
    } else {
        client->isConnected = false;
        return RIL_MQTT_ERR_CONN_FAIL;
    }
}

// QMTSUB handler
static int32_t ATHandler_QMTSUB(char* line, uint32_t len, void* userData) {
    int* pres = (int*) userData;
    int cid, msgid, res;
    if (strncmp(line, "+QMTSUB:", 8) == 0) {
        sscanf(line, "+QMTSUB: %d,%d,%d", &cid, &msgid, &res);
        *pres = (res == 0 ? 0 : -1);
        return RIL_ATRSP_SUCCESS;
    }
    return RIL_ATRSP_CONTINUE;
}

RIL_MQTT_Err RIL_MQTT_Subscribe(RIL_MQTTClient* client, uint16_t msgId, const char* topic,
                                Enum_Qos qos) {
    if (client == NULL || topic == NULL) {
        return RIL_MQTT_ERR_PARAM;
    }
    if (qos < QOS_0 || qos > QOS_2) {
        return RIL_MQTT_ERR_PARAM;
    }

    char cmd[200];
    int result = -1;
    snprintf(cmd, sizeof(cmd), "AT+QMTSUB=%u,%u,\"%s\",%u", client->clientIdx, msgId, topic, qos);
    if (RIL_SendATCmd(cmd, strlen(cmd), ATHandler_QMTSUB, &result, 10000) != RIL_AT_SUCCESS) {
        return RIL_MQTT_ERR_SUB_AT;
    }
    return (result == 0 ? RIL_MQTT_ERR_OK : RIL_MQTT_ERR_SUB_FAIL);
}

RIL_MQTT_Err RIL_MQTT_SubscribeMultiple(RIL_MQTTClient* client, uint16_t msgId,
                                        const char* topics[], Enum_Qos qos, uint8_t topicCount) {
    if (topicCount == 0 || topicCount > 10) {
        return RIL_MQTT_ERR_PARAM;
    }

    char cmd[512];
    int offset = snprintf(cmd, sizeof(cmd), "AT+QMTSUB=%u,%u", client->clientIdx, msgId);
    for (size_t i = 0; i < topicCount; ++i) {
        int n = snprintf(cmd + offset, sizeof(cmd) - offset, ",\"%s\",%u", topics[i], qos);
        if (n < 0 || n >= (int) (sizeof(cmd) - offset)) {
            return RIL_MQTT_ERR_PARAM;
        }
        offset += n;
    }
    int result = -1;
    if (RIL_SendATCmd(cmd, offset, ATHandler_QMTSUB, &result, 10000) != RIL_AT_SUCCESS) {
        return RIL_MQTT_ERR_SUB_AT;
    }
    return (result == 0 ? RIL_MQTT_ERR_OK : RIL_MQTT_ERR_SUB_FAIL);
}

// QMTPUBEX handler
static int32_t ATHandler_QMTPUBEX(char* line, uint32_t len, void* userData) {
    int* pres = (int*) userData;
    int cid, msgid, res;
    if (strncmp(line, "+QMTPUBEX:", 10) == 0) {
        sscanf(line, "+QMTPUBEX: %d,%d,%d", &cid, &msgid, &res);
        *pres = (res == 0 ? 0 : -1);
        return RIL_ATRSP_SUCCESS;
    }
    return RIL_ATRSP_CONTINUE;
}

RIL_MQTT_Err RIL_MQTT_Publish(RIL_MQTTClient* client, uint16_t msgId, const char* topic,
                              const uint8_t* payload, uint32_t length, Enum_Qos qos, bool retain) {
    if (client == NULL || topic == NULL || payload == NULL) {
        return RIL_MQTT_ERR_PARAM;
    }
    if (qos < QOS_0 || qos > QOS_2) {
        return RIL_MQTT_ERR_PARAM;
    }

    char cmd[256];
    int result = -1;
    // send header
    snprintf(cmd, sizeof(cmd), "AT+QMTPUBEX=%u,%u,%u,%u,\"%s\",%u", client->clientIdx, msgId, qos,
             retain ? 1 : 0, topic, length);
    if (RIL_SendATCmdWithPrompt(cmd, strlen(cmd), NULL, NULL, 10000) != RIL_AT_SUCCESS) {
        return RIL_MQTT_ERR_PUB_AT;
    }
    // send payload and wait URC
    if (RIL_SendBinaryData(payload, length, ATHandler_QMTPUBEX, &result, 30000) != RIL_AT_SUCCESS) {
        return RIL_MQTT_ERR_PUB_AT;
    }

    return (result == 0 ? RIL_MQTT_ERR_OK : RIL_MQTT_ERR_PUB_FAIL);
}

static int32_t ATHandler_QMTCLOSE(char* line, uint32_t len, void* userData) {
    int* pres = (int*) userData;
    int cid;
    if (strncmp(line, "+QMTCLOSE:", 10) == 0) {
        sscanf(line, "+QMTCLOSE: %d,%d", &cid, pres);
        return RIL_ATRSP_SUCCESS;
    }
    return RIL_ATRSP_CONTINUE;
}

RIL_MQTT_Err RIL_MQTT_Close(RIL_MQTTClient* client) {
    if (client == NULL) {
        return RIL_MQTT_ERR_PARAM;
    }

    RIL_MQTT_Err err = RIL_MQTT_Disconnect(client);
    if (err != RIL_MQTT_ERR_OK) {
        return err;
    }

    char cmd[32];
    int result = -1;
    snprintf(cmd, sizeof(cmd), "AT+QMTCLOSE=%u", client->clientIdx);
    if (RIL_SendATCmd(cmd, strlen(cmd), ATHandler_QMTCLOSE, &result, 10000) != RIL_AT_SUCCESS) {
        return RIL_MQTT_ERR_CLOSE_AT;
    }
    if (result == 0) {
        client->isConnected = false;
        return RIL_MQTT_ERR_OK;
    } else {
        return RIL_MQTT_ERR_CLOSE_FAIL;
    }
}

// QMTDISC handler
static int32_t ATHandler_QMTDISC(char* line, uint32_t len, void* userData) {
    int* pres = (int*) userData;
    int cid, res;
    if (strncmp(line, "+QMTDISC:", 9) == 0) {
        sscanf(line, "+QMTDISC: %d,%d", &cid, &res);
        *pres = (res == 0 ? 0 : -1);
        return RIL_ATRSP_SUCCESS;
    }
    return RIL_ATRSP_CONTINUE;
}

RIL_MQTT_Err RIL_MQTT_Disconnect(RIL_MQTTClient* client) {
    if (client == NULL) {
        return RIL_MQTT_ERR_PARAM;
    }

    char cmd[32];
    int result = -1;
    snprintf(cmd, sizeof(cmd), "AT+QMTDISC=%u", client->clientIdx);
    if (RIL_SendATCmd(cmd, strlen(cmd), ATHandler_QMTDISC, &result, 10000) != RIL_AT_SUCCESS) {
        return RIL_MQTT_ERR_DISC_AT;
    }

    if (result == 0) {
        client->isConnected = false;
        return RIL_MQTT_ERR_OK;
    } else {
        return RIL_MQTT_ERR_DISC_FAIL;
    }
}
