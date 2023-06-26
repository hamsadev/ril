#ifndef RIL_MQTT_CLIENT_H
#define RIL_MQTT_CLIENT_H

#include <stdbool.h>
#include <stdint.h>

#define MQTT_MAX_CLIENTID_LEN 32
#define MQTT_MAX_TOPIC_LEN 128
#define MQTT_MAX_PAYLOAD_LEN 512

// MQTT error codes
typedef enum {
    RIL_MQTT_ERR_OK = 0,     // success
    RIL_MQTT_ERR_CFG_AT,     // configuration error
    RIL_MQTT_ERR_CFG_FAIL,   // configuration failed
    RIL_MQTT_ERR_OPEN_AT,    // open network connection error
    RIL_MQTT_ERR_OPEN_FAIL,  // open network connection failed
    RIL_MQTT_ERR_CONN_AT,    // connect to MQTT broker error
    RIL_MQTT_ERR_CONN_FAIL,  // connect to MQTT broker failed
    RIL_MQTT_ERR_SUB_AT,     // subscribe to topic error
    RIL_MQTT_ERR_SUB_FAIL,   // subscribe to topic failed
    RIL_MQTT_ERR_PUB_AT,     // publish message error
    RIL_MQTT_ERR_PUB_FAIL,   // publish message failed
    RIL_MQTT_ERR_DISC_AT,    // disconnect from MQTT broker error
    RIL_MQTT_ERR_DISC_FAIL,  // disconnect from MQTT broker failed
    RIL_MQTT_ERR_CLOSE_AT,   // close network connection error
    RIL_MQTT_ERR_CLOSE_FAIL, // close network connection failed
    RIL_MQTT_ERR_PARAM,      // invalid parameter
} RIL_MQTT_Err;

// MQTT quality of service
typedef enum { QOS_0 = 0, QOS_1 = 1, QOS_2 = 2 } Enum_Qos;

// MQTT receive mode configuration
typedef enum {
    MQTT_RECV_MODE_URC = 0,     // Receive data directly via URC (default)
    MQTT_RECV_MODE_BUFFER = 1   // Store received data in buffer
} MQTT_RecvMode;

// MQTT SSL configuration
typedef enum {
    MQTT_SSL_DISABLE = 0,       // Disable SSL
    MQTT_SSL_ENABLE = 1         // Enable SSL
} MQTT_SSLMode;

// MQTT version configuration
typedef enum {
    MQTT_VERSION_3_1 = 3,       // MQTT version 3.1
    MQTT_VERSION_3_1_1 = 4      // MQTT version 3.1.1 (default)
} MQTT_Version;

// MQTT will configuration structure
typedef struct {
    bool willEnable;            // Enable will message
    uint8_t willQos;           // Will message QoS (0-2)
    bool willRetain;           // Will message retain flag
    char willTopic[MQTT_MAX_TOPIC_LEN];  // Will message topic
    char willMessage[MQTT_MAX_PAYLOAD_LEN]; // Will message payload
} MQTT_WillConfig;

// MQTT client context
typedef struct {
    uint8_t clientIdx;                    // MQTT client index assigned by module
    char clientId[MQTT_MAX_CLIENTID_LEN]; // MQTT client identifier
    char username[MQTT_MAX_CLIENTID_LEN]; // username
    char password[MQTT_MAX_CLIENTID_LEN]; // password
    uint16_t keepAlive;                   // keepalive interval (seconds)
    bool cleanSession;                    // clean session flag
    bool isConnected;                     // connection status
} RIL_MQTTClient;

/**
 * @brief Configure MQTT receive mode
 * @param client: pointer to the MQTT client
 * @param recvMode: receive mode (URC or Buffer)
 * @param msgLenEnable: enable payload length indication (0=disable, 1=enable)
 * @return RIL_MQTT_Err: error code
 */
RIL_MQTT_Err RIL_MQTT_ConfigReceiveMode(RIL_MQTTClient* client, MQTT_RecvMode recvMode, uint8_t msgLenEnable);

/**
 * @brief Configure MQTT SSL mode
 * @param client: pointer to the MQTT client
 * @param sslMode: SSL mode (enable/disable)
 * @param sslCtxId: SSL context ID (0-5)
 * @return RIL_MQTT_Err: error code
 */
RIL_MQTT_Err RIL_MQTT_ConfigSSL(RIL_MQTTClient* client, MQTT_SSLMode sslMode, uint8_t sslCtxId);

/**
 * @brief Configure MQTT version
 * @param client: pointer to the MQTT client
 * @param version: MQTT version (3 or 4)
 * @return RIL_MQTT_Err: error code
 */
RIL_MQTT_Err RIL_MQTT_ConfigVersion(RIL_MQTTClient* client, MQTT_Version version);

/**
 * @brief Configure MQTT will message
 * @param client: pointer to the MQTT client
 * @param willConfig: will configuration structure
 * @return RIL_MQTT_Err: error code
 */
RIL_MQTT_Err RIL_MQTT_ConfigWill(RIL_MQTTClient* client, const MQTT_WillConfig* willConfig);

/**
 * @brief Configure MQTT keep alive timeout
 * @param client: pointer to the MQTT client
 * @param timeout: keep alive timeout in seconds (0-3600)
 * @return RIL_MQTT_Err: error code
 */
RIL_MQTT_Err RIL_MQTT_ConfigKeepAlive(RIL_MQTTClient* client, uint16_t timeout);

/**
 * @brief Configure MQTT clean session
 * @param client: pointer to the MQTT client
 * @param cleanSession: clean session flag (0=disable, 1=enable)
 * @return RIL_MQTT_Err: error code
 */
RIL_MQTT_Err RIL_MQTT_ConfigCleanSession(RIL_MQTTClient* client, uint8_t cleanSession);

/**
 * @brief Read MQTT message from buffer (for buffer mode)
 * @param clientIdx: MQTT client index
 * @param recvId: receive ID from URC
 * @param topic: buffer for topic (output)
 * @param topicLen: maximum topic length
 * @param payload: buffer for payload (output)
 * @param payloadLen: payload length (input: buffer size, output: actual length)
 * @return RIL_MQTT_Err: error code
 */
RIL_MQTT_Err RIL_MQTT_ReadMessage(uint8_t clientIdx, uint8_t recvId, char* topic, uint16_t topicLen,
                                 uint8_t* payload, uint16_t* payloadLen);

/**
 * @brief Initialize MQTT client parameters
 * @param client: pointer to the MQTT client
 * @param clientId: client identifier
 * @param username: username
 * @param password: password
 * @param keepAlive: keepalive interval
 * @param cleanSession: clean session flag
 */
RIL_MQTT_Err RIL_MQTT_InitClient(RIL_MQTTClient* client, const char* clientId, const char* username,
                                 const char* password, uint16_t keepAlive, bool cleanSession);

/**
 * @brief Open network connection
 * @param client: pointer to the MQTT client
 * @param host: host address
 * @param port: port number
 */
RIL_MQTT_Err RIL_MQTT_Open(RIL_MQTTClient* client, const char* host, uint16_t port);

/**
 * @brief Close network connection and disconnect from MQTT broker
 * @param client: pointer to the MQTT client
 */
RIL_MQTT_Err RIL_MQTT_Close(RIL_MQTTClient* client);

/**
 * @brief Connect to MQTT broker
 * @param client: pointer to the MQTT client
 */
RIL_MQTT_Err RIL_MQTT_Connect(RIL_MQTTClient* client);

/**
 * @brief Disconnect from MQTT broker
 * @param client: pointer to the MQTT client
 */
RIL_MQTT_Err RIL_MQTT_Disconnect(RIL_MQTTClient* client);

/**
 * @brief Subscribe to a topic
 * @param client: pointer to the MQTT client
 * @param msgId: message identifier
 * @param topic: topic
 * @param qos: quality of service
 */
RIL_MQTT_Err RIL_MQTT_Subscribe(RIL_MQTTClient* client, uint16_t msgId, const char* topic,
                                Enum_Qos qos);

/**
 * @brief Subscribe to multiple topics
 * @param client: pointer to the MQTT client
 * @param msgId: message identifier
 * @param topics: array of topics
 * @param qos: quality of service
 * @param topicCount: number of topics
 */
RIL_MQTT_Err RIL_MQTT_SubscribeMultiple(RIL_MQTTClient* client, uint16_t msgId,
                                        const char* topics[], Enum_Qos qos, uint8_t topicCount);

/**
 * @brief Publish a message to topic
 * @param client: pointer to the MQTT client
 * @param msgId: message identifier
 * @param topic: topic
 * @param payload: payload
 * @param qos: quality of service
 * @param retain: retain flag
 */
RIL_MQTT_Err RIL_MQTT_Publish(RIL_MQTTClient* client, uint16_t msgId, const char* topic,
                              const uint8_t* payload, uint32_t length, Enum_Qos qos, bool retain);

#endif
