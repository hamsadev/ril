#ifndef _RIL_UARC_PARAMS_
#define _RIL_UARC_PARAMS_

/* Call-related URCs */
typedef enum {
    /* +CCWA: <number>, <type> */
    CCWA_PARAM_NUMBER, // Caller phone number
    CCWA_PARAM_TYPE    // Number type (129: National, 145: International)
} CCWA_Params;

typedef enum {
    /* +CLIP: <number>, <type>, [<subaddr>, <satype>, <alpha>, <CLI validity>] */
    CLIP_PARAM_NUMBER,      // Caller phone number
    CLIP_PARAM_TYPE,        // Number type (129: National, 145: International)
    CLIP_PARAM_SUBADDR,     // Sub-address (Optional)
    CLIP_PARAM_SATYPE,      // Sub-address type (Optional)
    CLIP_PARAM_ALPHA,       // Alpha (Contact name, Optional)
    CLIP_PARAM_CLI_VALIDITY // CLI validity (Optional)
} CLIP_Params;

typedef enum {
    /* +CRING: <call_type> */
    CRING_PARAM_TYPE // Call type (VOICE, DATA, FAX, etc.)
} CRING_Params;

typedef enum {
    /* +CREG: <status>, [<LAC>, <CellID>] */
    CREG_PARAM_STATUS, // Registration status (0: Not registered, 1: Registered)
    CREG_PARAM_LAC,    // Location Area Code (Optional)
    CREG_PARAM_CELLID  // Cell ID (Optional)
} CREG_Params;

typedef enum {
    /* +CCWV: <warning_level> */
    CCWV_PARAM_LEVEL // Warning level (Call charge warning)
} CCWV_Params;

typedef enum {
    /* +CLCC: <index>, <dir>, <status>, <mode>, <mpty>, <number>, <type> */
    CLCC_PARAM_INDEX,  // Call index
    CLCC_PARAM_DIR,    // Call direction (0: Outgoing, 1: Incoming)
    CLCC_PARAM_STATUS, // Call status (0: Active, 1: Held, 2: Dialing, etc.)
    CLCC_PARAM_MODE,   // Call mode (0: Voice, 1: Data, 2: Fax)
    CLCC_PARAM_MPTY,   // Multi-party call (0: No, 1: Yes)
    CLCC_PARAM_NUMBER, // Phone number
    CLCC_PARAM_TYPE    // Number type (129: National, 145: International)
} CLCC_Params;

/* SMS-related URCs */
typedef enum {
    /* +CMTI: <mem>, <index> */
    CMTI_PARAM_MEMORY, // Memory storage (e.g., "SM", "ME")
    CMTI_PARAM_INDEX   // Message index in memory
} CMTI_Params;

typedef enum {
    /* +CMT: <message_details> */
    CMT_PARAM_DETAILS // Full SMS message details
} CMT_Params;

typedef enum {
    /* +CBM: <message_details> */
    CBM_PARAM_DETAILS // Full Cell Broadcast Message details
} CBM_Params;

typedef enum {
    /* +CDS: <message_details> */
    CDS_PARAM_DETAILS // SMS status report details
} CDS_Params;

/* Network-related URCs */
typedef enum {
    /* +CENG: <mode>, <cell_info> */
    CENG_PARAM_MODE,    // Network information mode (0: Disabled, 1: Enabled)
    CENG_PARAM_CELLINFO // Cell information details
} CENG_Params;

typedef enum {
    /* +CPIN: <status> */
    CPIN_PARAM_STATUS // SIM card status (READY, SIM PIN, SIM PUK, etc.)
} CPIN_Params;

typedef enum {
    /* +CSQN: <signal_strength> */
    CSQN_PARAM_SIGNAL // Signal strength level (0-31, 99 = Unknown)
} CSQN_Params;

typedef enum {
    /* +CTZV: <timezone_info> */
    CTZV_PARAM_TIMEZONE // Time zone update details
} CTZV_Params;

typedef enum {
    /* +CSMINS: <status> */
    CSMINS_PARAM_STATUS // SIM insertion status (0: Not inserted, 1: Inserted)
} CSMINS_Params;

typedef enum {
    /* +CDRIND: <status> */
    CDRIND_PARAM_STATUS // Call/Data termination status
} CDRIND_Params;

/* Power and system-related URCs */
typedef enum {
    /* RDY */
    RDY_PARAM_NONE // No parameters, just a system ready notification
} RDY_Params;

typedef enum {
    /* Call Ready */
    CALL_READY_PARAM_NONE // No parameters, just a ready notification
} CALL_READY_Params;

typedef enum {
    /* SMS Ready */
    SMS_READY_PARAM_NONE // No parameters, just an SMS readiness notification
} SMS_READY_Params;

typedef enum {
    /* NORMAL POWER DOWN */
    NORMAL_POWER_DOWN_PARAM_NONE // No parameters, just a shutdown indication
} NORMAL_POWER_DOWN_Params;

typedef enum {
    /* UNDER-VOLTAGE WARNING */
    UNDER_VOLTAGE_WARN_PARAM_NONE // No parameters, just a warning
} UNDER_VOLTAGE_WARN_Params;

typedef enum {
    /* UNDER-VOLTAGE POWER DOWN */
    UNDER_VOLTAGE_PD_PARAM_NONE // No parameters, just a shutdown notification
} UNDER_VOLTAGE_PD_Params;

typedef enum {
    /* OVER-VOLTAGE WARNING */
    OVER_VOLTAGE_WARN_PARAM_NONE // No parameters, just a warning
} OVER_VOLTAGE_WARN_Params;

typedef enum {
    /* OVER-VOLTAGE POWER DOWN */
    OVER_VOLTAGE_PD_PARAM_NONE // No parameters, just a shutdown notification
} OVER_VOLTAGE_PD_Params;

typedef enum {
    /* CHARGE-ONLY MODE */
    CHARGE_ONLY_MODE_PARAM_NONE // No parameters, just a mode change notification
} CHARGE_ONLY_MODE_Params;

/* Data connection and internet-related URCs */
typedef enum {
    /* CONNECT OK */
    CONNECT_OK_PARAM_NONE // No parameters, just a success notification
} CONNECT_OK_Params;

typedef enum {
    /* CONNECT FAIL */
    CONNECT_FAIL_PARAM_NONE // No parameters, just a failure notification
} CONNECT_FAIL_Params;

typedef enum {
    /* ALREADY CONNECT */
    ALREADY_CONNECT_PARAM_NONE // No parameters, just a status notification
} ALREADY_CONNECT_Params;

typedef enum {
    /* SEND OK */
    SEND_OK_PARAM_NONE // No parameters, just a success notification
} SEND_OK_Params;

typedef enum {
    /* CLOSED */
    CLOSED_PARAM_NONE // No parameters, just a connection closed notification
} CLOSED_Params;

typedef enum {
    /* +PDP: DEACT */
    PDP_DEACT_PARAM_NONE // No parameters, just a deactivation notification
} PDP_DEACT_Params;

typedef enum {
    /* +SAPBR: DEACT */
    SAPBR_DEACT_PARAM_NONE // No parameters, just a deactivation notification
} SAPBR_DEACT_Params;

typedef enum {
    /* +HTTPACTION: <method>, <status_code>, <data_length> */
    HTTPACTION_PARAM_METHOD,      // HTTP method (0 = GET, 1 = POST, 2 = HEAD)
    HTTPACTION_PARAM_STATUS_CODE, // HTTP response status code (e.g., 200, 404)
    HTTPACTION_PARAM_DATA_LENGTH  // Number of bytes received
} HTTPACTION_Params;

/* Miscellaneous URCs */
typedef enum {
    /* +CUSD: <status>, <response> */
    CUSD_PARAM_STATUS,  // USSD status (0: No action, 1: Response received)
    CUSD_PARAM_RESPONSE // USSD response message
} CUSD_Params;

typedef enum {
    /* RING */
    RING_PARAM_NONE // No parameters, just a call ring notification
} RING_Params;

typedef enum {
    /* +CALV */
    CALV_PARAM_NONE // No parameters, just an alarm notification
} CALV_Params;

#endif //_RIL_UARC_PARAMS_
