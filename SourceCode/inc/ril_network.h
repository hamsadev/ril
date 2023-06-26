#ifndef _RIL_NETWORK_H_
#define _RIL_NETWORK_H_

#include "ril_sim.h"
#include <stdbool.h>
#include <stdint.h>

/****************************************************************************
 * Definition for network State (Compatible with EC200U module)
 ***************************************************************************/
typedef enum {
    RIL_NW_State_NotRegistered = 0,    // Not registered to network
    RIL_NW_State_Registered = 1,       // Registered, home network
    RIL_NW_State_Searching = 2,        // Not registered, but searching for operator
    RIL_NW_State_RegDenied = 3,        // Registration denied
    RIL_NW_State_Unknown = 4,          // Unknown status
    RIL_NW_State_RegisteredRoaming = 5 // Registered, roaming
} RIL_NW_State;

typedef enum {
    RIL_NW_CtxIPState_IPInitial = 0,
    RIL_NW_CtxIPState_IPStart,
    RIL_NW_CtxIPState_IPConfig,
    RIL_NW_CtxIPState_IPGPRSACT,
    RIL_NW_CtxIPState_IPStatus,
    RIL_NW_CtxIPState_TCPConnecting,
    RIL_NW_CtxIPState_UDPConnecting = RIL_NW_CtxIPState_TCPConnecting,
    RIL_NW_CtxIPState_ServerListening = RIL_NW_CtxIPState_TCPConnecting,
    RIL_NW_CtxIPState_ConnectOK,
    RIL_NW_CtxIPState_TCPClosing,
    RIL_NW_CtxIPState_UDPClosing = RIL_NW_CtxIPState_TCPClosing,
    RIL_NW_CtxIPState_TCPClosed,
    RIL_NW_CtxIPState_UDPClosed = RIL_NW_CtxIPState_TCPClosed,
    RIL_NW_CtxIPState_GPRSContextDeact,
    RIL_NW_CtxIPState_IPStatusEnd
} RIL_NW_CtxIPState;

// EC200U specific network access technology enum
typedef enum {
    RIL_NW_AccessTech_GSM = 0,
    RIL_NW_AccessTech_GSM_COMPACT = 1,
    RIL_NW_AccessTech_UTRAN = 2,
    RIL_NW_AccessTech_GSM_EGPRS = 3,
    RIL_NW_AccessTech_UTRAN_HSDPA = 4,
    RIL_NW_AccessTech_UTRAN_HSUPA = 5,
    RIL_NW_AccessTech_UTRAN_HSPA = 6,
    RIL_NW_AccessTech_E_UTRAN = 7,
    RIL_NW_AccessTech_E_UTRAN_CA = 8
} RIL_NW_AccessTech;

typedef enum {
    RIL_NW_Error_OperateSuccessfully = 0,
    RIL_NW_Error_UnknownError = 550,
    RIL_NW_Error_OperationBlocked = 551,
    RIL_NW_Error_InvalidParameters = 552,
    RIL_NW_Error_MemoryAllocationFailed = 553,
    RIL_NW_Error_SocketCreationFailed = 554,
    RIL_NW_Error_OperationNotSupported = 555,
    RIL_NW_Error_SocketBindFailed = 556,
    RIL_NW_Error_SocketListenFailed = 557,
    RIL_NW_Error_SocketWriteFailed = 558,
    RIL_NW_Error_SocketReadFailed = 559,
    RIL_NW_Error_SocketAcceptFailed = 560,
    RIL_NW_Error_ActivatePDPContextFailed = 561,
    RIL_NW_Error_DeactivatePDPContextFailed = 562,
    RIL_NW_Error_SocketIdentityHasBeenUsed = 563,
    RIL_NW_Error_DNSBusy = 564,
    RIL_NW_Error_DNSParseFailed = 565,
    RIL_NW_Error_SocketConnectFailed = 566,
    RIL_NW_Error_ConnectionReset = 567,
    RIL_NW_Error_SystemBusy = 568,
    RIL_NW_Error_OperationTimeout = 569,
    RIL_NW_Error_PDPContextDeactivated = 570,
    RIL_NW_Error_CancelSending = 571,
    RIL_NW_Error_OperationNotAllowed = 572,
    RIL_NW_Error_APNNotConfigured = 573,
    RIL_NW_Error_PortBusy = 574,
} RIL_NW_Error;

typedef enum {
    RIL_NW_AAuthType_None = 0,
    RIL_NW_AAuthType_PAP = 1,
    RIL_NW_AAuthType_CHAP = 2,
} RIL_NW_AuthType;

// Do not change the order of the enum values
typedef enum {
    RIL_NW_ContextType_IPV4 = 1,
    RIL_NW_ContextType_IPV6 = 2,
    RIL_NW_ContextType_IPv6V4 = 3,
    RIL_NW_ContextType_PPP = 4,
} RIL_NW_ContextType;

typedef struct {
    int rssi; // Signal strength (0-31, 99=unknown)
    int ber;  // Bit Error Rate (0-7, 99=unknown)
} RIL_NW_CSQReponse;

// EC200U extended signal quality structure
typedef struct {
    int rssi; // Received Signal Strength Indicator
    int rsrp; // Reference Signal Received Power (LTE)
    int sinr; // Signal to Interference plus Noise Ratio (LTE)
    int rsrq; // Reference Signal Received Quality (LTE)
} RIL_NW_ExtendedSignalInfo;

/**
 * @brief This function gets the GSM network register state.
 *
 * @param stat [out] GSM State.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_NW_GetGSMState(RIL_NW_State* stat);

/**
 * @brief This function gets the GPRS network register state.
 *
 * @param stat [out] GPRS State.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_NW_GetGPRSState(RIL_NW_State* stat);

/**
 * @brief This function gets the signal quality level and bit error rate.
 *
 * @param rssi [out] Signal quality level, 0~31 or 99. 99 indicates module
 *                   doesn't register to GSM network.
 * @param ber [out] The bit error code of signal.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_NW_GetSignalQuality(uint8_t* rssi, uint8_t* ber);

/**
 * @brief This function sets the APN for the current context.
 *
 * @param pdp_id [in] the PDP context id (1-15).
 * @param authType [in] the authentication type.
 * @param contextType [in] the context type. (RIL_NW_ContextType_PPP Not supported).
 * @param apn [in] pointer to the APN name.
 * @param userName [in] pointer to the user name (can be NULL). Max 127 characters.
 * @param pw [in] pointer to the password (can be NULL). Max 127 characters.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_NW_SetAPNEx(uint8_t pdp_id, RIL_NW_AuthType authType,
                               RIL_NW_ContextType contextType, char* apn, char* userName, char* pw);

/**
 * @brief This function sets the APN for the current context.
 *
 * @param pdp_id [in] the PDP context id (1-15).
 * @param contextType [in] the context type.
 * @param apn [in] pointer to the APN name.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_NW_SetAPN(uint8_t pdp_id, RIL_NW_ContextType contextType, char* apn);

/**
 * @brief This function sets the DNS for the current context.
 *
 * @param pdp_id [in] the PDP context id (1-15).
 * @param primaryServer [in] pointer to the primary DNS server.
 * @param secondaryServer [in] pointer to the secondary DNS server.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_NW_SetDNS(uint8_t pdp_id, const char* primaryServer,
                             const char* secondaryServer);

/**
 * @brief This function gets the status of IP protocol stack.
 *
 * @param state [out] The status of IP protocol stack.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_NW_GetIPStatus(uint8_t* state);

/**
 * @brief This function gets the IP address of the current PDP context.
 *
 * @param ip [out] Buffer to store IP address (minimum 16 bytes).
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_NW_GetIP(char* ip);

/**
 * @brief This function sets the NTP server and port.
 *
 * @param pdp_id [in] PDP context ID (1-15).
 * @param server [in] NTP server address.
 * @param port [in] NTP server port. Usually 123.
 * @param autoSetTime [in] Whether to automatically set the time.
 * @param retryCount [in] Retry count. (1 - 10), default is 3
 * @param retryInterval [in] Retry interval. (5-60 seconds), default is 15
 * @param err [out] Error code. Set to NULL to ignore error.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_NW_SetNTPEx(uint8_t pdp_id, const char* server, uint16_t port, bool autoSetTime,
                               uint8_t retryCount, uint8_t retryInterval, RIL_NW_Error* err);

/**
 * @brief This function sets the NTP server and port.
 *
 * @param pdp_id [in] PDP context ID (1-15).
 * @param server [in] NTP server address.
 * @param port [in] NTP server port. Usually 123.
 * @param autoSetTime [in] Whether to automatically set the time.
 * @param err [out] Error code. Set to NULL to ignore error.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_NW_SetNTP(uint8_t pdp_id, const char* server, uint16_t port, RIL_NW_Error* err);

/**
 * @brief This function opens/activates the GPRS PDP context.
 *
 * @param pdp_id [in] PDP context ID (1-15).
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_NW_OpenPDPContext(uint8_t pdp_id);

/**
 * @brief This function closes/deactivates the GPRS PDP context.
 *
 * @param pdp_id [in] PDP context ID (1-15).
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_NW_ClosePDPContext(uint8_t pdp_id);

/**
 * @brief This function gets the network operator that module registered.
 *
 * @param operator [out] a string with max 32 characters, which indicates the
 *                 network operator that module registered.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_NW_GetOperator(char* operator);

// EC200U specific functions
/**
 * @brief Get extended signal quality information.
 *
 * @param signalInfo [out] Extended signal information structure.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_NW_GetExtendedSignalQuality(RIL_NW_ExtendedSignalInfo* signalInfo);

/**
 * @brief Get current network access technology.
 *
 * @param act [out] Current access technology.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_NW_GetAccessTechnology(RIL_NW_AccessTech* act);

/**
 * @brief Enable/disable network registration URC.
 *
 * @param enable [in] true to enable URC, false to disable.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_NW_SetRegistrationURC(bool enable);

/**
 * @brief Set network selection mode.
 *
 * @param mode [in] 0=automatic, 1=manual, 2=deregister, 3=set format only, 4=manual with fallback.
 * @param operatorCode [in] operator code (can be NULL for automatic mode).
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_NW_SetNetworkSelection(uint8_t mode, const char* operatorCode);

#endif // _RIL_NETWORK_H_
