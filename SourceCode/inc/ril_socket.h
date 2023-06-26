#ifndef __RIL_SOCKET_H__
#define __RIL_SOCKET_H__

#include "ril.h"
#include <stdbool.h>
#include <stdint.h>

// Socket types
typedef enum {
    RIL_SOC_TYPE_TCP = 0,
    RIL_SOC_TYPE_UDP,
    RIL_SOC_TYPE_TCP_LISTENER,
    RIL_SOC_TYPE_UDP_SERVICE
} RIL_SocType;

// Access modes
typedef enum {
    RIL_SOC_ACCESS_BUFFER = 0,
    RIL_SOC_ACCESS_DIRECT,
    RIL_SOC_ACCESS_TRANSPARENT
} RIL_SocAccessMode;

// Error codes
typedef enum {
    RIL_SOC_SUCCESS = 0,
    RIL_SOC_ERR_GENERAL = -1,
    RIL_SOC_ERR_TIMEOUT = -2,
    RIL_SOC_ERR_AT = -3,
    RIL_SOC_ERR_PARAM = -4
} RIL_SocErrCode;

// Open a socket: returns a connectId (0-11) via out_connectId
RIL_SocErrCode RIL_SOC_Open(uint8_t contextId, RIL_SocType type, const char* host,
                            uint16_t remotePort, uint16_t localPort, RIL_SocAccessMode mode,
                            uint8_t* out_connectId);

// Close a socket
RIL_SocErrCode RIL_SOC_Close(uint8_t connectId);

// Send data: for UDP specify peer IP/port, for TCP ignore peer
RIL_SocErrCode RIL_SOC_Send(uint8_t connectId, const uint8_t* data, uint32_t length,
                            const char* peerIp, uint16_t peerPort);

// Receive data: returns length via out_len
RIL_SocErrCode RIL_SOC_Recv(uint8_t connectId, uint8_t* buffer, uint32_t bufferLen,
                            uint32_t* out_len);

// Query state of all sockets
RIL_SocErrCode RIL_SOC_GetStates(void);

#endif
