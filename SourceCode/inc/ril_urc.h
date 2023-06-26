#ifndef _RIL_URC_H_
#define _RIL_URC_H_

#include "Param.h"
#include <stdint.h>
#include <stdlib.h>

#define MAX_URC_PARAMS 8 // Maximum number of parameters a URC can have
#define RIL_UTIL_PARAM_MAX_SIZE 512

// Define the list of URCs with activation flags and AT commands
#define URC_LIST                                                                                   \
    /* ===== Network Registration URCs ===== */                                                    \
    X(URC_CREG, "+CREG", "AT+CREG=1", 0)        /* Basic network registration status */            \
    X(URC_CREG_LOC, "+CREG", "AT+CREG=2", 0)    /* Registration + LAC/CI info */                   \
    X(URC_CEREG, "+CEREG", "AT+CEREG=2", 0)     /* LTE registration with TAC/CI info */            \
    X(URC_CGREG, "+CGREG", "AT+CGREG=1", 0)     /* GPRS registration status */                     \
    X(URC_CGREG_LOC, "+CGREG", "AT+CGREG=2", 0) /* GPRS registration + LAC/CI info */              \
    /* ===== Time Zone URCs ===== */                                                               \
    X(URC_CTZV, "+CTZV", "AT+CTZR=1", 0) /* Basic time zone update */                              \
    X(URC_CTZE, "+CTZE", "AT+CTZR=2", 0) /* Extended time zone info */                             \
    /* ===== SMS and CBM URCs ===== */                                                             \
    X(URC_CMTI, "+CMTI", "AT+CNMI=2,1,0,1,0", 0) /* New SMS received in memory */                  \
    X(URC_CMT_TEXT, "+CMT", NULL, 0)             /* New SMS directly to TE (Text mode) */          \
    X(URC_CDS_TEXT, "+CDS", NULL, 0)             /* Delivery status report (Text mode) */          \
    X(URC_CDSI, "+CDSI", NULL, 0)                /* SMS status report stored in memory */          \
    /* ===== Call-related URCs ===== */                                                            \
    X(URC_COLP, "+COLP", "AT+COLP=1", 0)  /* Connected line presentation */                        \
    X(URC_CLIP, "+CLIP", "AT+CLIP=1", 0)  /* Caller ID presentation */                             \
    X(URC_CRING, "+CRING", "AT+CRC=1", 0) /* Incoming call with type info */                       \
    /* ===== System Initialization ===== */                                                        \
    X(URC_RDY, "RDY", NULL, 0)                       /* Module ready */                            \
    X(URC_CFUN, "+CFUN: 1", NULL, 0)                 /* All functions available */                 \
    X(URC_CPIN, "+CPIN", NULL, 0)                    /* SIM card status */                         \
    X(URC_QIND_SMS_DONE, "+QIND: SMS DONE", NULL, 0) /* SMS stack initialized */                   \
    X(URC_QIND_PB_DONE, "+QIND: PB DONE", NULL, 0)   /* Phonebook initialized */                   \
    /* ===== PDP and Network Events ===== */                                                       \
    X(URC_CGEREP_REJECT, "+CGEV: REJECT", "AT+CGEREP=1,1", 1)       /* PDP request rejected */     \
    X(URC_CGEREP_NW_REACT, "+CGEV: NW REACT", "AT+CGEREP=1,1", 0)   /* PDP reactivation */         \
    X(URC_CGEREP_NW_DEACT, "+CGEV: NW DEACT", "AT+CGEREP=1,1", 0)   /* Network deactivation */     \
    X(URC_CGEREP_ME_DEACT, "+CGEV: ME DEACT", "AT+CGEREP=1,1", 0)   /* Local deactivation */       \
    X(URC_CGEREP_NW_DETACH, "+CGEV: NW DETACH", "AT+CGEREP=1,1", 0) /* Network detach */           \
    X(URC_CGEREP_ME_DETACH, "+CGEV: ME DETACH", "AT+CGEREP=1,1", 0) /* ME detach */                \
    X(URC_CGEREP_NW_CLASS, "+CGEV: NW CLASS", "AT+CGEREP=1,1", 0)   /* Network class change */     \
    X(URC_CGEREP_ME_CLASS, "+CGEV: ME CLASS", "AT+CGEREP=1,1", 0)   /* ME class change */          \
    X(URC_CGEREP_PDN_ACT, "+CGEV: PDN ACT", "AT+CGEREP=1,1", 0)     /* PDN activated */            \
    X(URC_CGEREP_PDN_DEACT, "+CGEV: PDN DEACT", "AT+CGEREP=1,1", 0) /* PDN deactivated */          \
    /* ===== SIM Usage ===== */                                                                    \
    X(URC_USIM_0, "+USIM: 0", NULL, 0) /* SIM card inserted */                                     \
    X(URC_USIM_1, "+USIM: 1", NULL, 0) /* USIM card inserted */                                    \
    /* ===== QINDCFG-based URCs ===== */                                                           \
    X(URC_QIND_CSQ, "+QIND: \"csq\"", "AT+QINDCFG=\"csq\",0,0", 0) /* Signal strength changed */   \
    X(URC_QIND_SMSFULL, "+QIND: \"smsfull\"", "AT+QINDCFG=\"smsfull\",1,0",                        \
      1)                                                           /* SMS memory full */           \
    X(URC_QIND_ACT, "+QIND: \"act\"", "AT+QINDCFG=\"act\",1,0", 0) /* RAT change (e.g. LTE) */     \
    /* ===== Quectel-specific Status ===== */                                                      \
    X(URC_QSIMSTAT, "+QSIMSTAT", "AT+QSIMSTAT=1", 0) /* SIM card insertion/removal */              \
    X(URC_QCSQ, "+QCSQ", "AT+QCSQ=0", 0)             /* Detailed signal quality */                 \
    X(URC_QNETDEVSTATUS, "+QNETDEVSTATUS", NULL, 0)  /* Network attachment state */                \
    X(URC_QMTSTAT, "+QMTSTAT", NULL, 0)              /* MQTT connection state */                   \
    X(URC_QMTRECV, "+QMTRECV", NULL, 0)              /* MQTT message received */                   \
    X(URC_QMTPING, "+QMTPING", NULL, 0)              /* MQTT ping */

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

typedef struct {
    RIL_URCType urcType;                // URC type identifier
    uint8_t paramCount;                 // Number of extracted parameters
    Param_Value params[MAX_URC_PARAMS]; // Array of extracted parameters using Param library
} RIL_URCInfo;

#endif // _RIL_URC_H_
