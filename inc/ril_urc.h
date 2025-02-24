#ifndef _RIL_URC_H_
#define _RIL_URC_H_

#include "ril_urc_params.h"

#define MAX_URC_PARAMS 8 // Maximum number of parameters a URC can have

// Define the list of URCs with activation flags and AT commands
#define URC_LIST \
    /* Call-related URCs */ \
    X(URC_CALL_WAITING, "+CCWA", "AT+CCWA=1", 1) /* Call waiting indication */ \
    X(URC_CALLER_ID, "+CLIP", "AT+CLIP=1", 1) /* Caller ID presentation */ \
    X(URC_INCOMING_CALL, "+CRING", NULL, 0) /* Incoming call indication (no activation needed) */ \
    X(URC_NETWORK_REGISTRATION, "+CREG", "AT+CREG=1", 1) /* Network registration status */ \
    X(URC_CALL_WARNING, "+CCWV", NULL, 0) /* Call meter warning */ \
    X(URC_ACTIVE_CALL_LIST, "+CLCC", "AT+CLCC=1", 1) /* List of current active calls */ \
    \
    /* SMS-related URCs */ \
    X(URC_SMS_NEW_MESSAGE, "+CMTI", "AT+CNMI=2,1,0,0,0", 1) /* New SMS message indication */ \
    X(URC_SMS_RECEIVED, "+CMT", "AT+CNMI=2,1,0,0,0", 1) /* Incoming SMS message */ \
    X(URC_CELL_BROADCAST, "+CBM", "AT+CNMI=2,2,0,0,0", 1) /* New cell broadcast message */ \
    X(URC_SMS_STATUS_REPORT, "+CDS", "AT+CNMI=2,1,1,0,0", 1) /* SMS status report received */ \
    \
    /* Network-related URCs */ \
    X(URC_NETWORK_INFO, "+CENG", "AT+CENG=1", 1) /* Network information report */ \
    X(URC_SIM_STATUS, "+CPIN", NULL, 0) /* SIM card status (always active) */ \
    X(URC_SIGNAL_QUALITY, "+CSQN", NULL, 0) /* Signal strength and quality (no activation needed) */ \
    X(URC_TIMEZONE_UPDATE, "+CTZV", "AT+CTZU=1", 1) /* Time zone update */ \
    X(URC_SIM_INSERT_STATUS, "+CSMINS", NULL, 0) /* SIM card insertion status */ \
    X(URC_CALL_DATA_TERMINATION, "+CDRIND", NULL, 0) /* Call/data termination status */ \
    \
    /* Power and system-related URCs */ \
    X(URC_SYSTEM_READY, "RDY", NULL, 0) /* Module is powered on and ready */ \
    X(URC_CALL_READY, "Call Ready", NULL, 0) /* Phone is initialized and ready for calls */ \
    X(URC_SMS_READY, "SMS Ready", NULL, 0) /* SMS functionality is ready */ \
    X(URC_POWER_DOWN, "NORMAL POWER DOWN", NULL, 0) /* Normal shutdown */ \
    X(URC_UNDER_VOLTAGE_WARNING, "UNDER-VOLTAGE WARNING", NULL, 0) /* Low voltage warning */ \
    X(URC_UNDER_VOLTAGE_SHUTDOWN, "UNDER-VOLTAGE POWER DOWN", NULL, 0) /* Low voltage shutdown */ \
    X(URC_OVER_VOLTAGE_WARNING, "OVER-VOLTAGE WARNING", NULL, 0) /* High voltage warning */ \
    X(URC_OVER_VOLTAGE_SHUTDOWN, "OVER-VOLTAGE POWER DOWN", NULL, 0) /* High voltage shutdown */ \
    X(URC_CHARGE_MODE, "CHARGE-ONLY MODE", NULL, 0) /* Charging mode enabled */ \
    \
    /* Data connection and internet-related URCs */ \
    X(URC_TCP_UDP_CONNECTED, "CONNECT OK", "AT+CIURC=1", 1) /* TCP/UDP connection established */ \
    X(URC_TCP_UDP_FAILED, "CONNECT FAIL", NULL, 0) /* TCP/UDP connection failed */ \
    X(URC_ALREADY_CONNECTED, "ALREADY CONNECT", NULL, 0) /* Connection already exists */ \
    X(URC_DATA_SENT, "SEND OK", NULL, 0) /* Data sent successfully */ \
    X(URC_CONNECTION_CLOSED, "CLOSED", NULL, 0) /* Connection closed */ \
    X(URC_GPRS_DEACTIVATED, "+PDP: DEACT", "AT+CIURC=1", 1) /* GPRS connection deactivated */ \
    X(URC_BEARER_DEACTIVATED, "+SAPBR: DEACT", "AT+CIURC=1", 1) /* Bearer service deactivated */ \
    X(URC_HTTP_RESPONSE, "+HTTPACTION", "AT+CIURC=1", 1) /* HTTP request response */ \
    \
    /* Miscellaneous URCs */ \
    X(URC_USSD_RESPONSE, "+CUSD", "AT+CUSD=1", 1) /* USSD response */ \
    X(URC_INCOMING_RING, "RING", NULL, 0) /* Incoming call signal */ \
    X(URC_ALARM_NOTIFICATION, "+CALV", NULL, 0) /* Alarm expired notification */

// Generate enum from the URC list
typedef enum {
#define X(name, str, cmd, flag) name,
    URC_LIST
#undef X
    URC_MAX
} RIL_URCType;

// Generate an array of URC strings
static const char* URC_STRINGS[] = {
#define X(name, str, cmd, flag) str,
    URC_LIST
#undef X
};

// Generate an array of AT commands for URC activation
static const char* URC_AT_COMMANDS[] = {
#define X(name, str, cmd, flag) cmd,
    URC_LIST
#undef X
};

// Generate an array of flags indicating whether activation is needed
static const uint8_t URC_ENABLE_FLAGS[] = {
#define X(name, str, cmd, flag) flag,
    URC_LIST
#undef X
};

typedef enum {
    URC_PARAM_INT,   // Integer parameter
    URC_PARAM_STRING // String parameter
} URC_ParamType;

typedef struct {
    URC_ParamType type; // Parameter type
    union {
        uint32_t intValue;   // Integer value
        char strValue[32];   // String value (for text-based URCs)
    };
} URC_Param;

typedef struct {
    RIL_URCType urcType;       // URC type identifier
    uint8_t paramCount;     // Number of extracted parameters
    URC_Param params[MAX_URC_PARAMS]; // Array of extracted parameters
} RIL_URCInfo;

#endif // _RIL_URC_H_
