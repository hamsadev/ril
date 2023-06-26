/**
 * @file   ril_sms.c
 * @brief  Implementation of SMS client for Quectel EC200/EG915U modems
 * @details Provides implementation for SMS operations using Quectel
 *          modem AT commands. This module covers the following operations:
 *          - SMS storage management (CPMS)
 *          - SMS reading in PDU/Text format (CMGR)
 *          - SMS sending in PDU/Text format (CMGS)
 *          - SMS deletion (CMGD)
 *          - Character set conversion and formatting
 *          - Concatenated SMS support
 *
 * @author Sepahtan Project Team
 * @version 1.0
 * @date 2024
 */
#include "ril_sms.h"
#include "lib_ril_sms.h"
#include "ril.h"
#include "ril_util.h"
#include <stdio.h>

/* ══════════════════════════════════════════════════════════════════════ */
/*                           Macro Definitions                            */
/* ══════════════════════════════════════════════════════════════════════ */
#define DBG_EN 1
#define DBG_BUF_MAX_LEN (512)

/** Maximum length for AT command strings */
#define SMS_CMD_MAX_LEN (30)

/** AT response key strings - DO NOT CHANGE these values */
#define CPMS_KEY_STR "+CPMS: "
#define CMGR_KEY_STR "+CMGR: "
#define CMGS_KEY_STR "+CMGS: "

/** Control characters for SMS operations */
#define STR_CMGS_HINT "\r\n>"
#define STR_CR_LF "\r\n"
#define STR_COMMA ","

/** GSM character set conversion flags */
#define CHARSET_GSM_INSTEAD_0X1B_CHAR_FLAG (0x80)
#define CHARSET_GSM_INSTEAD_0X1A_CHAR_FLAG (0x81)

/* ══════════════════════════════════════════════════════════════════════ */
/*                             Type Definitions                           */
/* ══════════════════════════════════════════════════════════════════════ */
/**
 * @brief Handler types for AT command responses
 * @details Used to identify which type of SMS operation is being handled
 */
typedef enum {
    HDLR_TYPE_CPMS_READ_CMD = 0, /**< Read SMS storage info (AT+CPMS?) */
    HDLR_TYPE_CPMS_SET_CMD = 1,  /**< Set SMS storage type (AT+CPMS=...) */
    HDLR_TYPE_CMGR_PDU_CMD = 2,  /**< Read SMS in PDU format (AT+CMGR=...) */
    HDLR_TYPE_CMGS_PDU_CMD = 3,  /**< Send SMS in PDU format (AT+CMGS=...) */

    /** WARNING: Add new handler types above this line */
    HDLR_TYPE_INVALID = 0xFFFFFFFF
} Enum_HdlrType;

/**
 * @brief User data structure for AT command handlers
 * @details Contains handler type and user-specific data pointer
 */
typedef struct {
    uint32_t uHdlrType; /**< Handler type from Enum_HdlrType */
    void* pUserData;    /**< Pointer to handler-specific data */
} ST_SMS_HdlrUserData;

/**
 * @brief SMS storage information structure
 * @details Contains SMS storage status and capacity information
 */
typedef struct {
    uint8_t storage; /**< Storage type (SM/ME/MT) */
    uint32_t used;   /**< Number of used slots */
    uint32_t total;  /**< Total number of slots */
} ST_SMSStorage;

/* ══════════════════════════════════════════════════════════════════════ */
/*                       Function Declarations                            */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Platform adapter functions
 * @note If porting to another platform, these functions may need modification
 */
/**
 * @brief Convert SMS index between RIL and platform-specific formats
 * @param uStoType Storage type identifier
 * @param uRILIdx RIL-specific index
 * @param uRILMaxIdx Maximum RIL index
 * @return Platform-specific core index
 */
static uint32_t ADP_SMS_ConvIdxToCoreIdx(uint8_t uStoType, uint32_t uRILIdx, uint32_t uRILMaxIdx);

/**
 * @brief Internal utility functions
 * @note These functions are only used within this file
 */

/** Validation functions */
static bool IsValidConParam(ST_RIL_SMS_Con* pCon);

/** Conversion utility functions */
static bool ConvStringToPhoneNumber(char* pString, uint32_t uStrLen,
                                    LIB_SMS_PhoneNumberStruct* pNumber);
static bool ConvPhoneNumberToString(LIB_SMS_PhoneNumberStruct* pNumber, char* pString,
                                    uint32_t uStrLen);
static bool ConvTimeStampToString(LIB_SMS_TimeStampStruct* pTimeStamp, char* pString,
                                  uint32_t uStrLen);
static bool ConvDeliverSMSParamToTextInfo(LIB_SMS_DeliverPDUParamStruct* pSMSParam,
                                          ST_RIL_SMS_DeliverParam* pTextInfo);
static bool ConvSubmitSMSParamToTextInfo(LIB_SMS_SubmitPDUParamStruct* pSMSParam,
                                         ST_RIL_SMS_SubmitParam* pTextInfo);
static bool ConvSMSParamToTextInfo(uint8_t uCharSet, LIB_SMS_PDUParamStruct* pSMSParam,
                                   ST_RIL_SMS_TextInfo* pTextInfo);

/** Storage management functions */
static bool GetStorageType(char* pStr, uint8_t* pType);

/** AT command response handlers */
static char* HdlrGetStorageInfo(char* pLine, uint32_t uLen, ST_SMSStorage* pInfo);
static char* HdlrSetStorage(char* pLine, uint32_t uLen, ST_SMSStorage* pInfo);
static char* HdlrReadPDUMsg(char* pLine, uint32_t uLen, ST_RIL_SMS_PDUInfo* pPDU);
static char* HdlrSendPDUMsg(char* pLine, uint32_t uLen, ST_RIL_SMS_SendPDUInfo* pInfo);

/** General AT command handler */
static int32_t SMS_CMD_GeneralHandler(char* pLine, uint32_t uLen, void* pUserData);

/** Core SMS command functions */
static int32_t CmdGetStorageInfo(uint8_t* pCurrMem, uint32_t* pUsed, uint32_t* pTotal);
static int32_t CmdSetStorageInfo(uint8_t uStoType, uint32_t* pUsed, uint32_t* pTotal);
static int32_t CmdReadPDUMsg(uint32_t uIndex, ST_RIL_SMS_PDUInfo* pPDU);
static int32_t CmdSendPDUMsg(char* pPDUStr, uint32_t uPDUStrLen, uint32_t* pMsgRef);

/* ══════════════════════════════════════════════════════════════════════ */
/*                         Adapter Layer Macros                          */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Check if storage type is supported by adapter layer
 * @param StorageType Storage type to check
 * @return true if supported, false otherwise
 * @note May need modification for different platforms
 */
#define ADP_IS_SUPPORT_STORAGE_TYPE(StorageType)                                                   \
    (((RIL_SMS_STORAGE_TYPE_SM == (StorageType))) ? true : false)

/* ══════════════════════════════════════════════════════════════════════ */
/*                          Internal Helper Macros                       */
/* ══════════════════════════════════════════════════════════════════════ */
/**
 * @brief Validate PDU information structure
 * @param PDUInfo Pointer to PDU info structure
 * @return true if valid, false otherwise
 */
#define IS_VALID_PDU_INFO(PDUInfo)                                                                 \
    ((((((ST_RIL_SMS_PDUInfo*) (PDUInfo))->status) <= RIL_SMS_STATUS_TYPE_STO_SENT) &&             \
      ((((ST_RIL_SMS_PDUInfo*) (PDUInfo))->length) <=                                              \
       sizeof(((ST_RIL_SMS_PDUInfo*) (PDUInfo))->data)))                                           \
         ? true                                                                                    \
         : false)

/**
 * @brief Convert string to integer value
 * @param pStr Pointer to string
 * @param uLen Length of string
 * @param uVal Output integer value
 */
#define CONV_STRING_TO_INTEGER(pStr, uLen, uVal)                                                   \
    do {                                                                                           \
        char aBufTmpZ[40] = {                                                                      \
            0,                                                                                     \
        };                                                                                         \
                                                                                                   \
        memcpy(aBufTmpZ, pStr, uLen);                                                              \
        uVal = atoi(aBufTmpZ);                                                                     \
    } while (0)

/**
 * @brief Initialize PDU info structure to invalid state
 * @param PDUInfo Pointer to PDU info structure
 */
#define SMS_SET_INVALID_PDU_INFO(PDUInfo)                                                          \
    do {                                                                                           \
        memset(((ST_RIL_SMS_PDUInfo*) (PDUInfo)), 0x00, sizeof(ST_RIL_SMS_PDUInfo));               \
                                                                                                   \
        (((ST_RIL_SMS_PDUInfo*) (PDUInfo))->status) = RIL_SMS_STATUS_TYPE_INVALID;                 \
    } while (0)

/**
 * @brief Initialize text info structure to invalid state
 * @param TextInfo Pointer to text info structure
 */
#define SMS_SET_INVALID_TEXT_INFO(TextInfo)                                                        \
    do {                                                                                           \
        memset(((ST_RIL_SMS_TextInfo*) (TextInfo)), 0x00, sizeof(ST_RIL_SMS_TextInfo));            \
                                                                                                   \
        (((ST_RIL_SMS_TextInfo*) (TextInfo))->status) = RIL_SMS_STATUS_TYPE_INVALID;               \
    } while (0)

/**
 * @brief Set SMS module to PDU mode and handle errors
 * @param ErrCode Variable to store error code
 * @param DbgFunName Function name for debugging
 */
#define SMS_SET_PDU_MODE(ErrCode, DbgFunName)                                                      \
    do {                                                                                           \
        (ErrCode) = RIL_SendATCmd("AT+CMGF=0", strlen("AT+CMGF=0"), NULL, NULL, 0);                \
        if (RIL_ATRSP_SUCCESS != (ErrCode)) {                                                      \
            RIL_LOG_TRACE("Enter " DbgFunName ",FAIL! AT+CMGF=0 execute error! Code:%d\r\n",       \
                          (ErrCode));                                                              \
            return (ErrCode);                                                                      \
        }                                                                                          \
    } while (0)

/**
 * @brief Check AT command response for error conditions
 * @param pLine Response line from modem
 * @param uLen Length of response line
 * @param DbgFunName Function name for debugging
 * @return Appropriate RIL response code
 */
#define SMS_HDLR_CHECK_ERROR(pLine, uLen, DbgFunName)                                              \
    do {                                                                                           \
        if (NULL != strncmp(pLine, "OK", uLen)) {                                                  \
            RIL_LOG_TRACE("Enter " DbgFunName ",SUCCESS. Find string: \"OK\"\r\n");                \
            return RIL_ATRSP_SUCCESS;                                                              \
        }                                                                                          \
    } while (0);

/**
 * @brief Get storage name string from storage type enum
 * @param StorageType Storage type enum value
 * @param StorageName Output string buffer for storage name
 */
#define SMS_GET_STORAGE_NAME(StorageType, StorageName)                                             \
    do {                                                                                           \
        if (RIL_SMS_STORAGE_TYPE_MT == (StorageType)) {                                            \
            strcpy(StorageName, "MT");                                                             \
        } else if (RIL_SMS_STORAGE_TYPE_ME == (StorageType)) {                                     \
            strcpy(StorageName, "ME");                                                             \
        } else {                                                                                   \
            strcpy(StorageName, "SM");                                                             \
        }                                                                                          \
    } while (0)

/* ══════════════════════════════════════════════════════════════════════ */
/*                        Adapter Layer Functions                        */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Convert RIL SMS index to platform-specific core SMS index
 * @param uStoType SMS storage type (Enum_RIL_SMS_StorageType)
 * @param uRILIdx The RIL SMS index to convert
 * @param uRILMaxIdx Maximum valid RIL SMS index
 * @return Core SMS index on success, 0 on failure
 * @details Converts RIL layer SMS indices to platform-specific indices.
 *          For this implementation, indices are passed through unchanged.
 * @note May require modification for different platform implementations
 */
static uint32_t ADP_SMS_ConvIdxToCoreIdx(uint8_t uStoType, uint32_t uRILIdx, uint32_t uRILMaxIdx) {
    uint32_t uCoreIdx = 0;

    if (false == ADP_IS_SUPPORT_STORAGE_TYPE(uStoType)) {
        RIL_LOG_TRACE(
            "Enter ADP_SMS_ConvIdxToCoreIdx,FAIL! ADP_IS_SUPPORT_STORAGE_TYPE FAIL! uStoType:%u",
            uStoType);
        return 0;
    }

    if ((uRILIdx < 1) || (uRILIdx > uRILMaxIdx)) {
        RIL_LOG_TRACE("Enter ADP_SMS_ConvIdxToCoreIdx,FAIL! uRILIdx:%u,uRILMaxIdx:%u\r\n", uRILIdx,
                      uRILMaxIdx);
        return 0;
    }

    uCoreIdx = uRILIdx;

    RIL_LOG_TRACE(
        "Enter ADP_SMS_ConvIdxToCoreIdx,SUCCESS. uRILIdx:%u,uRILMaxIdx:%u,uCoreIdx:%u\r\n", uRILIdx,
        uRILMaxIdx, uCoreIdx);

    return uCoreIdx;
}

/* ══════════════════════════════════════════════════════════════════════ */
/*                         Internal Utility Functions                    */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Validate concatenated SMS parameters
 * @param pCon Pointer to SMS concatenation parameters
 * @return true if parameters are valid, false otherwise
 * @details Validates concatenated SMS message parameters including:
 *          - Message type (6-byte or 7-byte format)
 *          - Message reference number
 *          - Current segment number
 *          - Total segment count
 * @note This function is only used within this file
 */
static bool IsValidConParam(ST_RIL_SMS_Con* pCon) {
    if (NULL == pCon) {
        RIL_LOG_TRACE("Enter IsValidConParam,FAIL! Parameter is NULL.");
        return false;
    }

    if (((pCon->msgType) != LIB_SMS_UD_TYPE_CON_6_BYTE) &&
        ((pCon->msgType) != LIB_SMS_UD_TYPE_CON_7_BYTE)) {
        RIL_LOG_TRACE("Enter IsValidConParam,FAIL! msgType:%d is INVALID.", pCon->msgType);
        return false;
    }

    if (((pCon->msgSeg) < 1) || ((pCon->msgSeg) > (pCon->msgTot))) {
        RIL_LOG_TRACE("Enter IsValidConParam,FAIL! msgSeg:%d or msgTot: %d INVALID.", pCon->msgSeg,
                      pCon->msgTot);
        return false;
    }

    return true;
}

/**
 * @brief Convert string representation to phone number structure
 * @param pString Pointer to phone number string
 * @param uStrLen Length of phone number string
 * @param pNumber Pointer to phone number structure to fill
 * @return true on successful conversion, false on failure
 * @details Converts a string phone number to LIB_SMS_PhoneNumberStruct format.
 *          Handles international (+) and national number formats.
 *          Validates number length and format.
 * @note This function is only used within this file
 */
static bool ConvStringToPhoneNumber(char* pString, uint32_t uStrLen,
                                    LIB_SMS_PhoneNumberStruct* pNumber) {
    uint8_t uType = 0;
    char* pTmp = NULL;
    uint32_t i = 0;

    if ((NULL == pString) || (NULL == pNumber)) {
        RIL_LOG_TRACE("Enter ConvStringToPhoneNumber,FAIL! Parameter is NULL.");
        return false;
    }

    if (0 == uStrLen) // Not given number string
    {
        memset(pNumber, 0x00, sizeof(LIB_SMS_PhoneNumberStruct));
        (pNumber->uType) = LIB_SMS_PHONE_NUMBER_TYPE_UNKNOWN;

        RIL_LOG_TRACE("Enter ConvStringToPhoneNumber,SUCCESS. NOTE: uStrLen is 0.");
        return true;
    }

    // Initialize
    pTmp = pString;

    if (LIB_SMS_CHAR_PLUS == (*pTmp)) {
        uType = LIB_SMS_PHONE_NUMBER_TYPE_INTERNATIONAL;
        pTmp += 1;
    } else {
        uType = LIB_SMS_PHONE_NUMBER_TYPE_NATIONAL;
    }

    if ((uStrLen - (pTmp - pString)) > LIB_SMS_PHONE_NUMBER_MAX_LEN) {
        RIL_LOG_TRACE("Enter ConvStringToPhoneNumber,FAIL! Phone number is too long. "
                      "LIB_SMS_PHONE_NUMBER_MAX_LEN:%d,Now:%d",
                      LIB_SMS_PHONE_NUMBER_MAX_LEN, (uStrLen - (pTmp - pString)));
        return false;
    }

    // Check the given number's validity
    for (i = 0; i < (uStrLen - (pTmp - pString)); i++) {
        if (false == LIB_SMS_IS_VALID_ASCII_NUMBER_CHAR(pTmp[i])) {
            RIL_LOG_TRACE("Enter ConvStringToPhoneNumber,FAIL! LIB_SMS_IS_VALID_ASCII_NUMBER_CHAR "
                          "FAIL! Char[%d]:0x%x",
                          i, pTmp[i]);
            return false;
        }
    }

    pNumber->uType = uType;
    (pNumber->uLen) = (uStrLen - (pTmp - pString));
    memcpy((pNumber->aNumber), pTmp, (pNumber->uLen));

    RIL_LOG_TRACE("Enter ConvStringToPhoneNumber,SUCCESS. uLen:%d,uType:%d", (pNumber->uLen),
                  uType);

    return true;
}

/**
 * @brief Convert phone number to string
 *
 * @param pNumber [In] The pointer of phone number
 * @param pString [In] The pointer of string
 * @param uStrLen [In] The length of string
 * @return true This function works SUCCESS.
 * @return false This function works FAIL!
 *
 * NOTE:
 *      1. This function ONLY used in AT handler function.
 */
static bool ConvPhoneNumberToString(LIB_SMS_PhoneNumberStruct* pNumber, char* pString,
                                    uint32_t uStrLen) {
    uint32_t uLimitLen = 0;
    char* pTmp = NULL;

    if ((NULL == pNumber) || (NULL == pString) || (0 == uStrLen)) {
        RIL_LOG_TRACE("Enter ConvPhoneNumberToString,FAIL! Parameter is NULL.");
        return false;
    }

    if (0 == (pNumber->uLen)) {
        RIL_LOG_TRACE("Enter ConvPhoneNumberToString,SUCCESS. NOTE: Number length is 0.");

        memset(pString, 0x00, uStrLen);
        return true;
    }

    // Check <uStrLen> is VALID or not
    if (LIB_SMS_PHONE_NUMBER_TYPE_INTERNATIONAL == (pNumber->uType)) {
        uLimitLen = ((pNumber->uLen) + 1 +
                     1); // It will add '+' at the first position,'\0' at the end position.
    } else if (LIB_SMS_PHONE_NUMBER_TYPE_NATIONAL == (pNumber->uType)) {
        uLimitLen = ((pNumber->uLen) + 1); // It will add '\0' at the end position.
    } else if (LIB_SMS_PHONE_NUMBER_TYPE_UNKNOWN == (pNumber->uType)) {
        uLimitLen = ((pNumber->uLen) + 1); // It will add '\0' at the end position.
    }
    //<2015/03/23-ROTVG00006-P04-Vicent.Gao,<[SMS] Segment 4==>Fix issues of RIL SMS LIB.>
    else if (LIB_SMS_PHONE_NUMBER_TYPE_ALPHANUMERIC == (pNumber->uType)) {
        uLimitLen = ((pNumber->uLen) + 1); // It will add '\0' at the end position.
    }
    //>2015/03/23-ROTVG00006-P04-Vicent.Gao
    else {
        RIL_LOG_TRACE("Enter ConvPhoneNumberToString,FAIL! Number type is INVALID. uType:%u",
                      (pNumber->uType));
        return false;
    }

    if (uStrLen < uLimitLen) {
        RIL_LOG_TRACE("Enter ConvPhoneNumberToString,FAIL! uStrLen is less than uLimitLen. "
                      "uStrLen:%u,uLimitLen:%u",
                      uStrLen, uLimitLen);
        return false;
    }

    // Initialize
    memset(pString, 0x00, uStrLen);
    pTmp = pString;

    if ((LIB_SMS_PHONE_NUMBER_TYPE_INTERNATIONAL == (pNumber->uType))) {
        pTmp[0] = LIB_SMS_CHAR_PLUS;
        pTmp += 1;
    }

    memcpy(pTmp, (pNumber->aNumber), (pNumber->uLen));

    RIL_LOG_TRACE("Enter ConvPhoneNumberToString,SUCCESS. pString: %s", pString);

    return true;
}

/**
 * @brief Convert 'LIB_SMS_TimeStampStruct' data to string.
 *
 * @param pTimeStamp [In] The pointer of 'LIB_SMS_TimeStampStruct' data
 * @param pString [In] The pointer of string
 * @param uStrLen [In] The maximum length of string
 * @return true This function works SUCCESS.
 * @return false This function works FAIL!
 *
 * NOTE:
 *      1. This function ONLY used in AT handler function.
 */
static bool ConvTimeStampToString(LIB_SMS_TimeStampStruct* pTimeStamp, char* pString,
                                  uint32_t uStrLen) {
    int32_t iLen = 0;
    int32_t iTimeZone = 0;

    if ((NULL == pTimeStamp) || (NULL == pString)) {
        RIL_LOG_TRACE("Enter ConvTimeStampToString,FAIL! Parameter is NULL.");
        return false;
    }

    if (uStrLen < RIL_SMS_TIME_STAMP_STR_MAX_LEN) {
        RIL_LOG_TRACE("Enter ConvTimeStampToString,FAIL! uStrLen is too less. "
                      "uStrLen:%u,RIL_SMS_TIME_STAMP_STR_MAX_LEN:%u",
                      uStrLen, RIL_SMS_TIME_STAMP_STR_MAX_LEN);
        return false;
    }

    iLen = sprintf(pString, "%02u/%02u/%02u,%02u:%02u:%02u", (pTimeStamp->uYear),
                   (pTimeStamp->uMonth), (pTimeStamp->uDay), (pTimeStamp->uHour),
                   (pTimeStamp->uMinute), (pTimeStamp->uSecond));

    iTimeZone = (pTimeStamp->iTimeZone);
    if (iTimeZone < 0) {
        sprintf((pString + iLen), "%02d", iTimeZone);
    } else {
        sprintf((pString + iLen), "+%02d", iTimeZone);
    }

    RIL_LOG_TRACE("Enter ConvTimeStampToString,SUCCESS. pString:%s", pString);

    return true;
}

/**
 * @brief Convert SMS deliver PDU parameters to text info structure
 * @param pSMSParam Pointer to LIB_SMS_DeliverPDUParamStruct containing PDU parameters
 * @param pTextInfo Pointer to ST_RIL_SMS_DeliverParam structure to store text info
 * @return true if conversion succeeds, false if it fails
 * @details Converts SMS deliver PDU parameters to RIL text format, including
 *          originating address, timestamp, and data coding scheme information.
 */
static bool ConvDeliverSMSParamToTextInfo(LIB_SMS_DeliverPDUParamStruct* pSMSParam,
                                          ST_RIL_SMS_DeliverParam* pTextInfo) {
    bool bResult = false;

    if ((NULL == pSMSParam) || (NULL == pTextInfo)) {
        RIL_LOG_TRACE("Enter ConvDeliverSMSParamToTextInfo,FAIL! Parameter is NULL.");
        return false;
    }

    LIB_SMS_GET_ALPHA_IN_PDU_DCS((pSMSParam->uDCS), (pTextInfo->alpha));

    bResult = ConvPhoneNumberToString(&(pSMSParam->sOA), (pTextInfo->oa), sizeof(pTextInfo->oa));
    if (false == bResult) {
        RIL_LOG_TRACE("Enter ConvDeliverSMSParamToTextInfo,FAIL! ConvPhoneNumberToString FAIL!");
        return false;
    }

    bResult =
        ConvTimeStampToString(&(pSMSParam->sSCTS), (pTextInfo->scts), sizeof(pTextInfo->scts));
    if (false == bResult) {
        RIL_LOG_TRACE("Enter ConvDeliverSMSParamToTextInfo,FAIL! ConvTimeStampToString FAIL!");
        return false;
    }

    RIL_LOG_TRACE("Enter ConvDeliverSMSParamToTextInfo,SUCCESS. alpha:%u,oa:%s,scts:%s",
                  (pTextInfo->alpha), (pTextInfo->oa), (pTextInfo->scts));

    return true;
}

/**
 * @brief Convert SMS submit PDU parameters to text info structure
 * @param pSMSParam Pointer to LIB_SMS_SubmitPDUParamStruct containing PDU parameters
 * @param pTextInfo Pointer to ST_RIL_SMS_SubmitParam structure to store text info
 * @return true if conversion succeeds, false if it fails
 * @details Converts SMS submit PDU parameters to RIL text format, including
 *          destination address and data coding scheme information.
 */
static bool ConvSubmitSMSParamToTextInfo(LIB_SMS_SubmitPDUParamStruct* pSMSParam,
                                         ST_RIL_SMS_SubmitParam* pTextInfo) {
    bool bResult = false;

    if ((NULL == pSMSParam) || (NULL == pTextInfo)) {
        RIL_LOG_TRACE("Enter ConvSubmitSMSParamToTextInfo,FAIL! Parameter is NULL.");
        return false;
    }

    LIB_SMS_GET_ALPHA_IN_PDU_DCS((pSMSParam->uDCS), (pTextInfo->alpha));

    bResult = ConvPhoneNumberToString(&(pSMSParam->sDA), (pTextInfo->da), sizeof(pTextInfo->da));
    if (false == bResult) {
        RIL_LOG_TRACE("Enter ConvSubmitSMSParamToTextInfo,FAIL! ConvPhoneNumberToString FAIL!");
        return false;
    }

    RIL_LOG_TRACE("Enter ConvSubmitSMSParamToTextInfo,SUCCESS. alpha:%u,da:%s", (pTextInfo->alpha),
                  (pTextInfo->da));

    return true;
}

/**
 * @brief Convert SMS PDU parameters to text info structure
 * @param uCharSet Character set value from LIB_SMS_CharSetEnum
 * @param pSMSParam Pointer to LIB_SMS_PDUParamStruct containing SMS parameters
 * @param pTextInfo Pointer to ST_RIL_SMS_TextInfo structure to store text info
 * @return true if conversion succeeds, false if it fails
 * @details Converts SMS PDU parameters to RIL text format, handling both
 *          deliver and submit SMS types, including concatenated SMS support.
 */
static bool ConvSMSParamToTextInfo(uint8_t uCharSet, LIB_SMS_PDUParamStruct* pSMSParam,
                                   ST_RIL_SMS_TextInfo* pTextInfo) {
    bool bResult = false;
    ST_RIL_SMS_DeliverParam* pDeliverTextInfo = NULL;
    ST_RIL_SMS_SubmitParam* pSubmitTextInfo = NULL;

    if ((NULL == pSMSParam) || (NULL == pTextInfo)) {
        RIL_LOG_TRACE("Enter ConvSMSParamToTextInfo,FAIL! Parameter is NULL.");
        return false;
    }

    bResult = ConvPhoneNumberToString(&(pSMSParam->sSCA), (pTextInfo->sca), sizeof(pTextInfo->sca));
    if (false == bResult) {
        RIL_LOG_TRACE("Enter ConvSMSParamToTextInfo,FAIL! ConvPhoneNumberToString FAIL!");
        return false;
    }

    (pTextInfo->type) = LIB_SMS_GET_MSG_TYPE_IN_PDU_FO(pSMSParam->uFO);
    if (LIB_SMS_PDU_TYPE_DELIVER == (pTextInfo->type)) {
        pDeliverTextInfo = &((pTextInfo->param).deliverParam);

        if ((pSMSParam->uFO) & 0x40) // Concatenate SMS
        {
            pDeliverTextInfo->conPres = true;
            pDeliverTextInfo->con.msgType = pSMSParam->sParam.sDeliverParam.sConSMSParam.uMsgType;
            pDeliverTextInfo->con.msgRef = pSMSParam->sParam.sDeliverParam.sConSMSParam.uMsgRef;
            pDeliverTextInfo->con.msgSeg = pSMSParam->sParam.sDeliverParam.sConSMSParam.uMsgSeg;
            pDeliverTextInfo->con.msgTot = pSMSParam->sParam.sDeliverParam.sConSMSParam.uMsgTot;
        }

        bResult =
            ConvDeliverSMSParamToTextInfo(&((pSMSParam->sParam).sDeliverParam), pDeliverTextInfo);
        if (false == bResult) {
            RIL_LOG_TRACE("Enter ConvSMSParamToTextInfo,FAIL! ConvDeliverSMSParamToTextInfo FAIL!");
            return false;
        }

        (pDeliverTextInfo->length) = sizeof(pDeliverTextInfo->data);

        bResult = LIB_SMS_ConvAlphaToCharSet(
            ((pSMSParam->sParam).sDeliverParam.uDCS),
            ((pSMSParam->sParam).sDeliverParam.sUserData.aUserData),
            ((pSMSParam->sParam).sDeliverParam.sUserData.uLen), uCharSet, (pDeliverTextInfo->data),
            (uint16_t*) &(pDeliverTextInfo->length));

        if (false == bResult) {
            RIL_LOG_TRACE("Enter ConvSMSParamToTextInfo,FAIL! LIB_SMS_ConvAlphaToCharSet FAIL!");
            return false;
        }
    } else if (LIB_SMS_PDU_TYPE_SUBMIT == (pTextInfo->type)) {
        pSubmitTextInfo = &((pTextInfo->param).submitParam);

        if ((pSMSParam->uFO) & 0x40) // Concatenate SMS
        {
            pSubmitTextInfo->conPres = true;
            pSubmitTextInfo->con.msgType = pSMSParam->sParam.sSubmitParam.sConSMSParam.uMsgType;
            pSubmitTextInfo->con.msgRef = pSMSParam->sParam.sSubmitParam.sConSMSParam.uMsgRef;
            pSubmitTextInfo->con.msgSeg = pSMSParam->sParam.sSubmitParam.sConSMSParam.uMsgSeg;
            pSubmitTextInfo->con.msgTot = pSMSParam->sParam.sSubmitParam.sConSMSParam.uMsgTot;
        }

        bResult =
            ConvSubmitSMSParamToTextInfo(&((pSMSParam->sParam).sSubmitParam), pSubmitTextInfo);
        if (false == bResult) {
            RIL_LOG_TRACE("Enter ConvSMSParamToTextInfo,FAIL! ConvSubmitSMSParamToTextInfo FAIL!");
            return false;
        }

        (pSubmitTextInfo->length) = sizeof(pSubmitTextInfo->data);

        bResult = LIB_SMS_ConvAlphaToCharSet(((pSMSParam->sParam).sSubmitParam.uDCS),
                                             ((pSMSParam->sParam).sSubmitParam.sUserData.aUserData),
                                             ((pSMSParam->sParam).sSubmitParam.sUserData.uLen),
                                             uCharSet, (pSubmitTextInfo->data),
                                             (uint16_t*) &(pSubmitTextInfo->length));

        if (false == bResult) {
            RIL_LOG_TRACE("Enter ConvSMSParamToTextInfo,FAIL! LIB_SMS_ConvAlphaToCharSet FAIL!");
            return false;
        }
    } else {
        RIL_LOG_TRACE("Enter ConvSMSParamToTextInfo,FAIL! Msg type is INVALID. type:%d",
                      (pTextInfo->type));
        return false;
    }

    RIL_LOG_TRACE("Enter ConvSMSParamToTextInfo,SUCCESS. bResult:%d", bResult);

    return bResult;
}

/**
 * @brief Parse SMS storage type from string
 * @param pStr Pointer to storage type string (e.g., "SM", "ME", "MT")
 * @param pType Pointer to store parsed SMS storage type (Enum_RIL_SMS_StorageType)
 * @return true if parsing succeeds, false if it fails
 * @details Parses SMS storage type string and converts to enumerated value.
 *          Supported types: SM (SIM card), ME (Mobile equipment), MT (Mobile terminal).
 * @note This function is only used in AT command handler functions.
 */
static bool GetStorageType(char* pStr, uint8_t* pType) {
#define SMS_TYPE_STR_LEN (4) // As: "SM"  "ME"

    char aBuf[SMS_TYPE_STR_LEN + 1] = {
        0,
    };

    if ((NULL == pStr) || (NULL == pType)) {
        RIL_LOG_TRACE("Enter GetStorageType FAIL! Parameter is NULL.");
        return false;
    }

    strncpy(aBuf, pStr, SMS_TYPE_STR_LEN);

    if (0 == strcmp(aBuf, "\"SM\"")) {
        (*pType) = RIL_SMS_STORAGE_TYPE_SM;
    } else if (0 == strcmp(aBuf, "\"ME\"")) {
        (*pType) = RIL_SMS_STORAGE_TYPE_ME;
    } else if (0 == strcmp(aBuf, "\"MT\"")) {
        (*pType) = RIL_SMS_STORAGE_TYPE_MT;
    } else {
        RIL_LOG_TRACE("Enter GetStorageType FAIL! Storage type is INVALID. aBuf:%s", aBuf);
        return false;
    }

    RIL_LOG_TRACE("Enter GetStorageType SUCCESS. aBuf:%s,*pType:%d", aBuf, (*pType));

    return true;
}

/**
 * @brief Handler for parsing SMS storage information from AT+CPMS response
 * @param pLine Pointer to the AT response string
 * @param uLen Length of the response string
 * @param pInfo Pointer to ST_SMSStorage structure to store parsed information
 * @return Pointer to pLine on success, NULL on failure
 * @details Parses AT+CPMS command response to extract SMS storage type,
 *          used count, and total capacity information.
 * @note This function is only used in AT command handler functions.
 */
static char* HdlrGetStorageInfo(char* pLine, uint32_t uLen, ST_SMSStorage* pInfo) {
    char* pHead = NULL;
    char* pTail = NULL;

    if ((NULL == pLine) || (0 == uLen) || (NULL == pInfo)) {
        RIL_LOG_TRACE("Enter HdlrGetStorageInfo FAIL! Parameter is NULL.");
        return NULL;
    }

    pHead = strstr(pLine, CPMS_KEY_STR);
    if (NULL == pHead) {
        RIL_LOG_TRACE("Enter HdlrGetStorageInfo FAIL! NOT find \"" CPMS_KEY_STR "\"");
        return NULL;
    }

    // Get <mem1> of +CPMS
    pHead += strlen(CPMS_KEY_STR);
    if (false == GetStorageType(pHead, &(pInfo->storage))) {
        RIL_LOG_TRACE("Enter HdlrGetStorageInfo FAIL! GetStorageType FAIL! string:%c%c%c%c",
                      pHead[0], pHead[1], pHead[2], pHead[3]);
        return NULL;
    }

    // Get <used1> of +CPMS
    pHead = strstr(pHead, STR_COMMA);
    if (NULL == pHead) {
        RIL_LOG_TRACE("Enter HdlrGetStorageInfo FAIL! strstr FAIL! NOT find comma.");
        return NULL;
    }

    pTail = strstr((pHead + 1), STR_COMMA);
    if (NULL == pTail) {
        RIL_LOG_TRACE("Enter HdlrGetStorageInfo FAIL! strstr FAIL! NOT find comma.");
        return NULL;
    }

    CONV_STRING_TO_INTEGER((pHead + 1), (pTail - (pHead + 1)), (pInfo->used));

    // Get <total1> of +CPMS
    pHead = pTail;
    pTail = strstr((pHead + 1), STR_COMMA);
    if (NULL == pTail) {
        RIL_LOG_TRACE("Enter HdlrGetStorageInfo FAIL! strstr FAIL! NOT find comma.");
        return NULL;
    }

    CONV_STRING_TO_INTEGER((pHead + 1), (pTail - (pHead + 1)), (pInfo->total));

    RIL_LOG_TRACE("Enter HdlrGetStorageInfo SUCCESS. storage:%u,used:%u,total:%u", (pInfo->storage),
                  (pInfo->used), (pInfo->total));

    return pLine;
}

/**
 * @brief Handler for parsing SMS storage set response from AT+CPMS command
 * @param pLine Pointer to the AT response string
 * @param uLen Length of the response string
 * @param pInfo Pointer to ST_SMSStorage structure to store parsed information
 * @return Pointer to pLine on success, NULL on failure
 * @details Parses AT+CPMS command response after setting storage type to extract
 *          used count and total capacity information.
 * @note This function is only used in AT command handler functions.
 */
static char* HdlrSetStorage(char* pLine, uint32_t uLen, ST_SMSStorage* pInfo) {
    char* pHead = NULL;
    char* pTail = NULL;

    if ((NULL == pLine) || (0 == uLen) || (NULL == pInfo)) {
        RIL_LOG_TRACE("Enter HdlrSetStorage FAIL! Parameter is NULL.");
        return NULL;
    }

    pHead = strstr(pLine, CPMS_KEY_STR);
    if (NULL == pHead) {
        RIL_LOG_TRACE("Enter HdlrSetStorage FAIL! NOT find \"" CPMS_KEY_STR "\"");
        return NULL;
    }

    // Get <mem1> of +CPMS
    pHead += strlen(CPMS_KEY_STR);
    pTail = strstr(pHead, STR_COMMA);
    if (NULL == pTail) {
        RIL_LOG_TRACE("Enter HdlrSetStorage FAIL! strstr FAIL! NOT find comma.");
        return NULL;
    }

    CONV_STRING_TO_INTEGER(pHead, (pTail - pHead), (pInfo->used));

    pHead = pTail;
    pTail = strstr((pHead + 1), STR_COMMA);
    if (NULL == pTail) {
        RIL_LOG_TRACE("Enter HdlrSetStorage FAIL! strstr FAIL! NOT find comma.");
        return NULL;
    }

    CONV_STRING_TO_INTEGER((pHead + 1), (pTail - (pHead + 1)), (pInfo->total));

    RIL_LOG_TRACE("Enter HdlrSetStorage SUCCESS. used:%u,total:%u", (pInfo->used), (pInfo->total));

    return pLine;
}

/**
 * @brief Handler for parsing PDU message from AT+CMGR response
 * @param pLine Pointer to the AT response string
 * @param uLen Length of the response string
 * @param pPDU Pointer to ST_RIL_SMS_PDUInfo structure to store PDU data
 * @return Pointer to pLine on success, NULL on failure
 * @details Parses AT+CMGR command response to extract SMS PDU data including
 *          status and PDU content. Handles multi-line response format.
 * @note This function is only used in AT command handler functions.
 */
static char* HdlrReadPDUMsg(char* pLine, uint32_t uLen, ST_RIL_SMS_PDUInfo* pPDU) {
    char* pHead = NULL;
    char* pTail = NULL;
    uint32_t uDataLen = 0;

    static bool s_bSMSReadContentFlag =
        false; // false: NOT read SMS content. true: Read SMS content.

    if ((NULL == pLine) || (0 == uLen) || (NULL == pPDU)) {
        RIL_LOG_TRACE("Enter HdlrReadPDUMsg,FAIL! Parameter is NULL.");
        return NULL;
    }

    // Read SMS PDU content
    if (true == s_bSMSReadContentFlag) {
        // Initialize
        memset((pPDU->data), 0x00, sizeof(pPDU->data));

        uDataLen = (((uLen - 2) < (LIB_SMS_PDU_BUF_MAX_LEN * 2)) ? (uLen - 2)
                                                                 : (LIB_SMS_PDU_BUF_MAX_LEN * 2));
        memcpy((pPDU->data), pLine, uDataLen);

        (pPDU->length) = uDataLen; // Get the count of characters in PDU string.

        s_bSMSReadContentFlag = false;

        RIL_LOG_TRACE("Enter HdlrReadPDUMsg,SUCCESS. status:%u,length:%u,s_bSMSReadContentFlag:%d",
                      (pPDU->status), (pPDU->length), s_bSMSReadContentFlag);

        return pLine;
    }

    // Read SMS PDU header info
    pHead = strstr(pLine, CMGR_KEY_STR);
    if (NULL == pHead) {
        RIL_LOG_TRACE("Enter HdlrReadPDUMsg FAIL! NOT find \"" CMGR_KEY_STR "\"");
        return NULL;
    }

    s_bSMSReadContentFlag = true; // Set flag

    // Get <stat> of +CMGR
    pHead += strlen(CMGR_KEY_STR);
    pTail = strstr(pHead, STR_COMMA);
    if (NULL == pTail) {
        RIL_LOG_TRACE("Enter HdlrReadPDUMsg FAIL! strstr FAIL! NOT find comma.");
        return NULL;
    }

    CONV_STRING_TO_INTEGER(pHead, (pTail - pHead), (pPDU->status));

    RIL_LOG_TRACE("Enter HdlrReadPDUMsg SUCCESS. status:%u,s_bSMSReadContentFlag:%d",
                  (pPDU->status), s_bSMSReadContentFlag);

    return pLine;
}

/**
 * @brief Handler for sending PDU message AT command response
 * @param pLine Pointer to the response string
 * @param uLen Length of the response string
 * @param pInfo Pointer to ST_RIL_SMS_SendPDUInfo structure to store result
 * @return Pointer to pLine on success, NULL on failure
 * @details Processes AT+CMGS command response for PDU mode SMS sending.
 *          Extracts message reference number from the response.
 * @note This function is only used in AT command handler functions.
 */
static char* HdlrSendPDUMsg(char* pLine, uint32_t uLen, ST_RIL_SMS_SendPDUInfo* pInfo) {
    char* pHead = NULL;
    char* pTail = NULL;
    uint8_t uCtrlZ = 0x1A;

    if ((NULL == pLine) || (0 == uLen) || (NULL == pInfo)) {
        RIL_LOG_TRACE("Enter HdlrSendPDUMsg FAIL! Parameter is NULL.");
        return NULL;
    }

    pHead = strstr(pLine, STR_CMGS_HINT);
    if (NULL != pHead) {
        // TODO:!!!
        // RIL_WriteDataToCore((uint8_t*)(pInfo->pduString), (pInfo->pduLen));
        // RIL_WriteDataToCore(&uCtrlZ, 1);

        return pLine;
    }

    pHead = strstr(pLine, CMGS_KEY_STR);
    if (NULL == pHead) {
        RIL_LOG_TRACE("Enter HdlrSendPDUMsg FAIL! NOT find \"" CMGS_KEY_STR "\"");
        return NULL;
    }

    pHead += strlen(CMGR_KEY_STR);
    pTail = strstr(pHead, STR_CR_LF);
    if (NULL == pTail) {
        RIL_LOG_TRACE("Enter HdlrSendPDUMsg FAIL! NOT find \"" CMGS_KEY_STR "\"");
        return NULL;
    }

    CONV_STRING_TO_INTEGER(pHead, (pTail - pHead), (pInfo->mr));

    RIL_LOG_TRACE("Enter HdlrSendPDUMsg SUCCESS. mr:%u", (pInfo->mr));

    return pLine;
}

/**
 * @brief General AT command response handler for SMS operations
 * @param pLine Pointer to the AT response line string
 * @param uLen Length of the response line string
 * @param pUserData Pointer to ST_SMS_HdlrUserData structure containing handler parameters
 * @return RIL_ATRSP_CONTINUE if waiting for more responses
 * @return RIL_ATRSP_SUCCESS if command completed successfully
 * @return RIL_ATRSP_FAILED if command failed
 * @return Other values indicate function error
 * @details Dispatches AT command responses to appropriate specific handlers based on
 *          the handler type specified in pUserData. Supports CPMS read/set, CMGR PDU,
 *          and CMGS PDU command types.
 */
static int32_t SMS_CMD_GeneralHandler(char* pLine, uint32_t uLen, void* pUserData) {
    uint32_t uType = 0;
    ST_SMS_HdlrUserData* pParam = (ST_SMS_HdlrUserData*) pUserData;

    if ((NULL == pLine) || (0 == uLen) || (NULL == pUserData)) {
        RIL_LOG_TRACE("Enter SMS_CMD_GeneralHandler,FAIL! Parameter is NULL.");
        return RIL_AT_INVALID_PARAM;
    }

    uType = (pParam->uHdlrType);

    switch (uType) {
    case HDLR_TYPE_CPMS_READ_CMD: {
        ST_SMSStorage* pInfo = (ST_SMSStorage*) (pParam->pUserData);

        if (NULL != HdlrGetStorageInfo(pLine, uLen, pInfo)) {
            RIL_LOG_TRACE("Enter SMS_CMD_GeneralHandler,SUCCESS. "
                          "uType:HDLR_TYPE_CPMS_READ_CMD,storage:%u,used:%u,total:%u",
                          (pInfo->storage), (pInfo->used), (pInfo->total));
            return RIL_ATRSP_CONTINUE;
        }
    } break;

    case HDLR_TYPE_CPMS_SET_CMD: {
        ST_SMSStorage* pInfo = (ST_SMSStorage*) (pParam->pUserData);

        if (NULL != HdlrSetStorage(pLine, uLen, pInfo)) {
            RIL_LOG_TRACE("Enter SMS_CMD_GeneralHandler,SUCCESS. "
                          "uType:HDLR_TYPE_CPMS_SET_CMD,used:%u,total:%u",
                          (pInfo->used), (pInfo->total));
            return RIL_ATRSP_CONTINUE;
        }
    } break;

    case HDLR_TYPE_CMGR_PDU_CMD: {
        ST_RIL_SMS_PDUInfo* pInfo = (ST_RIL_SMS_PDUInfo*) (pParam->pUserData);

        if (NULL != HdlrReadPDUMsg(pLine, uLen, pInfo)) {
            RIL_LOG_TRACE("Enter SMS_CMD_GeneralHandler,SUCCESS. "
                          "uType:HDLR_TYPE_CMGR_PDU_CMD,status:%u,length:%u",
                          (pInfo->status), (pInfo->length));
            return RIL_ATRSP_CONTINUE;
        }
    } break;

    case HDLR_TYPE_CMGS_PDU_CMD: {
        ST_RIL_SMS_SendPDUInfo* pInfo = (ST_RIL_SMS_SendPDUInfo*) (pParam->pUserData);

        if (NULL != HdlrSendPDUMsg(pLine, uLen, pInfo)) {
            RIL_LOG_TRACE(
                "Enter SMS_CMD_GeneralHandler,SUCCESS. uType:HDLR_TYPE_CMGS_PDU_CMD,length:%u",
                (pInfo->pduLen));
            return RIL_ATRSP_CONTINUE;
        }
    } break;

    default: {
        RIL_LOG_TRACE("Enter SMS_CMD_GeneralHandler,SUCCESS. Warning: NOT support type! uType:%u",
                      uType);
    } break;
    }

    SMS_HDLR_CHECK_ERROR(pLine, uLen, "SMS_CMD_GeneralHandler");

    RIL_LOG_TRACE("Enter SMS_CMD_GeneralHandler,SUCCESS. uType:%u", uType);

    return RIL_ATRSP_CONTINUE; // continue wait
}

/**
 * @brief Get SMS storage information using AT+CPMS? command
 * @param pCurrMem Pointer to store current SMS storage type (Enum_RIL_SMS_StorageType)
 * @param pUsed Pointer to store number of used SMS storage slots
 * @param pTotal Pointer to store total SMS storage capacity
 * @return RIL_ATRSP_CONTINUE if waiting for AT response
 * @return RIL_ATRSP_SUCCESS if command completed successfully
 * @return RIL_ATRSP_FAILED if command failed
 * @return Other values indicate function error
 * @details Executes AT+CPMS? command to query current SMS storage status.
 *          Updates the provided pointers with storage information.
 * @note If you don't want to retrieve a certain parameter, set its pointer to NULL.
 *       This function is only used for sending AT commands.
 */
static int32_t CmdGetStorageInfo(uint8_t* pCurrMem, uint32_t* pUsed, uint32_t* pTotal) {
    int32_t iResult = 0;
    char aCmd[SMS_CMD_MAX_LEN] = {
        0,
    };
    ST_SMSStorage sInfo;
    ST_SMS_HdlrUserData sUserData;

    // Initialize
    memset(&sInfo, 0x00, sizeof(sInfo));
    memset(&sUserData, 0x00, sizeof(sUserData));

    strcpy(aCmd, "AT+CPMS?");

    (sUserData.uHdlrType) = HDLR_TYPE_CPMS_READ_CMD;
    (sUserData.pUserData) = (void*) (&sInfo);

    iResult = RIL_SendATCmd(aCmd, strlen(aCmd), SMS_CMD_GeneralHandler, &sUserData, 0);
    if (RIL_ATRSP_SUCCESS != iResult) {
        RIL_LOG_TRACE("Enter CmdSetStorageInfo,FAIL! AT+CPMS? execute error! Code:%d\r\n", iResult);
        return iResult;
    }

    LIB_SMS_SET_POINTER_VAL(pCurrMem, (sInfo.storage));
    LIB_SMS_SET_POINTER_VAL(pUsed, (sInfo.used));
    LIB_SMS_SET_POINTER_VAL(pTotal, (sInfo.total));

    RIL_LOG_TRACE("Enter CmdGetStorageInfo,SUCCESS. storage:%d,used:%d,total:%d", (sInfo.storage),
                  (sInfo.used), (sInfo.total));

    return RIL_ATRSP_SUCCESS;
}

/**
 * @brief Set SMS storage type and get storage information using AT+CPMS command
 * @param uStoType SMS storage type (Enum_RIL_SMS_StorageType)
 * @param pUsed Pointer to store number of used SMS storage slots
 * @param pTotal Pointer to store total SMS storage capacity
 * @return RIL_ATRSP_CONTINUE if waiting for AT response
 * @return RIL_ATRSP_SUCCESS if command completed successfully
 * @return RIL_ATRSP_FAILED if command failed
 * @return Other values indicate function error
 * @details Executes AT+CPMS command to set SMS storage type for reading,
 *          writing, and received message storage, then retrieves storage info.
 * @note This function is only used for sending AT commands.
 */
static int32_t CmdSetStorageInfo(uint8_t uStoType, uint32_t* pUsed, uint32_t* pTotal) {
    char aCmd[SMS_CMD_MAX_LEN] = {
        0,
    };
    char aStoName[4] = {0}; // SMS storage name
    int32_t iLen = 0;
    int32_t iResult = 0;

    ST_SMSStorage sInfo;
    ST_SMS_HdlrUserData sUserData;

    // Initialize
    memset(&sInfo, 0x00, sizeof(sInfo));
    memset(&sUserData, 0x00, sizeof(sUserData));

    // Check SMS storage type is valid or not
    if (false == ADP_IS_SUPPORT_STORAGE_TYPE(uStoType)) {
        RIL_LOG_TRACE("Enter CmdSetStorageInfo,FAIL! ADP_IS_SUPPORT_STORAGE_TYPE FAIL!");
        return RIL_AT_INVALID_PARAM;
    }

    // Get SMS storage's name refer to <uStoType>
    SMS_GET_STORAGE_NAME(uStoType, aStoName);

    iLen = sprintf(aCmd, "AT+CPMS=\"%s\",\"%s\",\"%s\"", aStoName, aStoName, aStoName);

    (sUserData.uHdlrType) = HDLR_TYPE_CPMS_SET_CMD;
    (sUserData.pUserData) = (void*) (&sInfo);

    iResult = RIL_SendATCmd(aCmd, iLen, SMS_CMD_GeneralHandler, &sUserData, 0);
    if (RIL_ATRSP_SUCCESS != iResult) {
        RIL_LOG_TRACE("Enter CmdSetStorageInfo,FAIL! AT+CPMS? execute error! iResult:%d\r\n",
                      iResult);
        return iResult;
    }

    LIB_SMS_SET_POINTER_VAL(pUsed, (sInfo.used));
    LIB_SMS_SET_POINTER_VAL(pTotal, (sInfo.total));

    RIL_LOG_TRACE("Enter CmdGetStorageInfo,SUCCESS. uStoType:%d,used:%d,total:%d", uStoType,
                  (sInfo.used), (sInfo.total));

    return RIL_ATRSP_SUCCESS;
}

/**
 * @brief Read a PDU format SMS message using AT+CMGR command
 * @param uIndex SMS index in current SMS storage
 * @param pPDU Pointer to ST_RIL_SMS_PDUInfo structure to store the PDU data
 * @return RIL_ATRSP_CONTINUE if waiting for AT response
 * @return RIL_ATRSP_SUCCESS if command completed successfully
 * @return RIL_ATRSP_FAILED if command failed
 * @return Other values indicate function error
 * @details Executes AT+CMGR command to read SMS message in PDU format from
 *          specified storage index. Sets modem to PDU mode before reading.
 * @note This function is only used for sending AT commands.
 */
static int32_t CmdReadPDUMsg(uint32_t uIndex, ST_RIL_SMS_PDUInfo* pPDU) {
    char aCmd[SMS_CMD_MAX_LEN] = {
        0,
    };
    int32_t iResult = 0;
    uint8_t uStoType = 0;
    uint32_t uStoTot = 0;
    int32_t iLen = 0;
    uint32_t uCoreIdx = 0;

    ST_SMS_HdlrUserData sUserData;

    if (NULL == pPDU) {
        RIL_LOG_TRACE("Enter CmdReadPDUMsg,FAIL! Parameter is NULL.");
        return RIL_AT_INVALID_PARAM;
    }

    // Initialize
    memset(&sUserData, 0x00, sizeof(sUserData));

    // Check parameter <uIndex>
    iResult = CmdGetStorageInfo(&uStoType, NULL, &uStoTot);
    if (RIL_ATRSP_SUCCESS != iResult) {
        RIL_LOG_TRACE("Enter CmdReadPDUMsg,FAIL! iResult:%d", iResult);
        return iResult;
    }

    if ((uIndex < 1) || (uIndex > uStoTot)) {
        RIL_LOG_TRACE("Enter CmdReadPDUMsg,FAIL! uIndex is INVALID. uIndex:%d", uIndex);
        return RIL_AT_INVALID_PARAM;
    }

    uCoreIdx = ADP_SMS_ConvIdxToCoreIdx(uStoType, uIndex, uStoTot);
    if (0 == uCoreIdx) {
        RIL_LOG_TRACE("Enter CmdReadPDUMsg,FAIL! ADP_SMS_ConvIdxToCoreIdx FAIL! "
                      "uStoType:%u,index:%u,uStoTot:%u",
                      uStoType, uIndex, uStoTot);
        return RIL_AT_INVALID_PARAM;
    }

    // Set to PDU mode
    SMS_SET_PDU_MODE(iResult, "CmdReadPDUMsg");

    (sUserData.uHdlrType) = HDLR_TYPE_CMGR_PDU_CMD;
    (sUserData.pUserData) = pPDU;

    iLen = sprintf(aCmd, "AT+CMGR=%u", uCoreIdx);
    iResult = RIL_SendATCmd(aCmd, iLen, SMS_CMD_GeneralHandler, &sUserData, 0);
    if (iResult != RIL_ATRSP_SUCCESS) {
        RIL_LOG_TRACE("Enter CmdReadPDUMsg,FAIL! CMD: %s execute FAIL!", aCmd);
        return iResult;
    }

    RIL_LOG_TRACE("Enter CmdReadPDUMsg,SUCCESS. CMD: %s,uStoType:%u,uStoTot:%u,index:%u,length:%u",
                  aCmd, uStoType, uStoTot, uIndex, (pPDU->length));

    return iResult;
}

/**
 * @brief Send SMS message in PDU format using AT+CMGS command
 * @param pPDUStr Pointer to PDU string (hexadecimal format)
 * @param uPDUStrLen Length of PDU string
 * @param pMsgRef Pointer to store message reference number (can be NULL)
 * @return RIL_AT_SUCCESS if AT command sent successfully
 * @return RIL_AT_FAILED if AT command failed
 * @return RIL_AT_TIMEOUT if AT command timeout
 * @return RIL_AT_BUSY if AT interface is busy
 * @return RIL_AT_INVALID_PARAM if input parameters are invalid
 * @return RIL_AT_UNINITIALIZED if RIL is not ready
 * @details Sends SMS message in PDU format. Sets modem to PDU mode first,
 *          then executes AT+CMGS command with TPDU length parameter.
 * @note If you don't want to get message reference value, set pMsgRef to NULL.
 *       PDU string must be valid hexadecimal with even length.
 */
static int32_t CmdSendPDUMsg(char* pPDUStr, uint32_t uPDUStrLen, uint32_t* pMsgRef) {
    char aCmd[SMS_CMD_MAX_LEN] = {
        0,
    };
    uint32_t uTPDULen = 0;
    uint32_t uSCALen = 0;
    bool bResult = false;
    int32_t iResult = 0;
    int32_t iLen = 0;
    ST_RIL_SMS_SendPDUInfo sInfo;
    ST_SMS_HdlrUserData sUserData;

    // Initialize
    memset(&sInfo, 0x00, sizeof(sInfo));
    memset(&sUserData, 0x00, sizeof(sUserData));

    if ((NULL == pPDUStr) || (0 == uPDUStrLen)) {
        RIL_LOG_TRACE("Enter CmdSendPDUMsg,FAIL! Parameter is NULL.");
        return RIL_AT_INVALID_PARAM;
    }

    bResult = LIB_SMS_IsValidHexStr(pPDUStr, uPDUStrLen);
    if (false == bResult) {
        RIL_LOG_TRACE("Enter CmdSendPDUMsg,FAIL! LIB_SMS_IsValidHexStr FAIL!");
        return RIL_AT_INVALID_PARAM;
    }

    if ((uPDUStrLen % 2) != 0) {
        RIL_LOG_TRACE("Enter CmdSendPDUMsg,FAIL! (uPDUStrLen % 2) != 0");
        return RIL_AT_INVALID_PARAM;
    }

    CONV_STRING_TO_INTEGER(pPDUStr, 2, uSCALen);
    uTPDULen = ((uPDUStrLen / 2) - (uSCALen + 1));

    // Set to PDU mode
    SMS_SET_PDU_MODE(iResult, "RIL_SMS_ReadSMS_PDU");

    (sInfo.pduString) = pPDUStr;
    (sInfo.pduLen) = uPDUStrLen;
    (sUserData.uHdlrType) = HDLR_TYPE_CMGS_PDU_CMD;
    (sUserData.pUserData) = &sInfo;

    iLen = sprintf(aCmd, "AT+CMGS=%d", uTPDULen);
    iResult = RIL_SendATCmd(aCmd, iLen, SMS_CMD_GeneralHandler, &sUserData, 0);
    if (iResult != RIL_ATRSP_SUCCESS) {
        RIL_LOG_TRACE("Enter CmdSendPDUMsg,FAIL! CMD: %s execute FAIL!", aCmd);
        return iResult;
    }

    LIB_SMS_SET_POINTER_VAL(pMsgRef, (sInfo.mr));

    RIL_LOG_TRACE("Enter CmdSendPDUMsg,SUCCESS. mr:%u", (sInfo.mr));

    return RIL_ATRSP_SUCCESS;
}

/**
 * @brief Get current SMS storage information
 * @param pCurrMem Pointer to store current memory/storage setting (can be NULL)
 * @param pUsed Pointer to store number of used messages (can be NULL)
 * @param pTotal Pointer to store total message capacity (can be NULL)
 * @return RIL_ATRSP_SUCCESS on success, error code on failure
 * @details Retrieves current SMS storage configuration and usage statistics
 *          using AT+CPMS? command. Any output parameter can be NULL if not needed.
 */
int32_t RIL_SMS_GetStorage(uint8_t* pCurrMem, uint32_t* pUsed, uint32_t* pTotal) {
    int32_t iResult = 0;

    iResult = CmdGetStorageInfo((uint8_t*) pCurrMem, pUsed, pTotal);

    RIL_LOG_TRACE("Enter RIL_SMS_GetStorage,SUCCESS. iResult:%d", iResult);

    return iResult;
}

/**
 * @brief Set SMS storage type and get storage information
 * @param eStorage SMS storage type (SM/ME/MT)
 * @param pUsed Pointer to store number of used messages (can be NULL)
 * @param pTotal Pointer to store total message capacity (can be NULL)
 * @return RIL_ATRSP_SUCCESS on success, error code on failure
 * @details Sets the SMS storage type using AT+CPMS command and retrieves
 *          storage usage statistics. Storage types include:
 *          - SM: SIM card storage
 *          - ME: Mobile equipment storage
 *          - MT: Mobile terminal storage (SM + ME)
 */
int32_t RIL_SMS_SetStorage(Enum_RIL_SMS_StorageType eStorage, uint32_t* pUsed, uint32_t* pTotal) {
    int32_t iResult = 0;

    iResult = CmdSetStorageInfo(eStorage, pUsed, pTotal);

    RIL_LOG_TRACE("Enter RIL_SMS_SetStorage,SUCCESS. iResult:%d", iResult);

    return iResult;
}

/**
 * @brief Read SMS message in PDU format
 * @param uIndex SMS index in current storage
 * @param pPDUInfo Pointer to store PDU information (required)
 * @return RIL_AT_SUCCESS on success, error code on failure
 * @details Reads an SMS message in PDU (Protocol Data Unit) format using
 *          AT+CMGR command. The PDU format contains all SMS parameters
 *          in hexadecimal encoding including sender, timestamp, and message body.
 * @note PDU format provides more complete SMS information than text format
 */
int32_t RIL_SMS_ReadSMS_PDU(uint32_t uIndex, ST_RIL_SMS_PDUInfo* pPDUInfo) {
    int32_t iResult = 0;
    bool bResult = false;

    if (NULL == pPDUInfo) {
        RIL_LOG_TRACE("Enter RIL_SMS_ReadSMS_PDU,FAIL! Parameter is NULL.");
        return RIL_AT_INVALID_PARAM;
    }

    // Initialize
    memset(pPDUInfo, 0x00, sizeof(ST_RIL_SMS_PDUInfo));

    iResult = CmdReadPDUMsg(uIndex, pPDUInfo);
    if (iResult != RIL_ATRSP_SUCCESS) {
        RIL_LOG_TRACE("Enter RIL_SMS_ReadSMS_PDU,FAIL! CmdReadPDUMsg FAIL!");
        return iResult;
    }

    // The message of <uIndex> position is NOT exist.
    if (0 == (pPDUInfo->length)) {
        SMS_SET_INVALID_PDU_INFO(pPDUInfo);

        RIL_LOG_TRACE("Enter RIL_SMS_ReadSMS_PDU,SUCCESS. NOTE:(pPDUInfo->length) is 0.");
        return RIL_ATRSP_SUCCESS;
    }

    // Check the <pPDUInfo>
    if (false == IS_VALID_PDU_INFO(pPDUInfo)) {
        RIL_LOG_TRACE("Enter RIL_SMS_ReadSMS_PDU,FAIL! IS_VALID_PDU_INFO FAIL!");
        return RIL_AT_FAILED;
    }

    LIB_SMS_CHECK_PDU_STR((pPDUInfo->data), (pPDUInfo->length), bResult);
    if (false == bResult) {
        RIL_LOG_TRACE("Enter RIL_SMS_ReadSMS_PDU,WARNING! LIB_SMS_CHECK_PDU_STR FAIL!");
    }

    RIL_LOG_TRACE("Enter RIL_SMS_ReadSMS_PDU,SUCCESS. status:%u,length:%u", (pPDUInfo->status),
                  (pPDUInfo->length));

    return RIL_ATRSP_SUCCESS;
}

/**
 * @brief Read SMS message in text format
 * @param uIndex SMS index in current storage
 * @param eCharset Character set enumeration value for text encoding
 * @param pTextInfo Pointer to ST_RIL_SMS_TextInfo structure to store SMS data
 * @return RIL_AT_SUCCESS if AT command sent successfully
 * @return RIL_AT_FAILED if AT command failed
 * @return RIL_AT_TIMEOUT if AT command timeout
 * @return RIL_AT_BUSY if AT interface is busy
 * @return RIL_AT_INVALID_PARAM if input parameters are invalid
 * @return RIL_AT_UNINITIALIZED if RIL is not ready
 * @details Reads SMS message in text format by first reading PDU format
 *          then converting to text format using specified character set.
 */
int32_t RIL_SMS_ReadSMS_Text(uint32_t uIndex, LIB_SMS_CharSetEnum eCharset,
                             ST_RIL_SMS_TextInfo* pTextInfo) {
    ST_RIL_SMS_PDUInfo* pPDUInfo = NULL;
    LIB_SMS_PDUParamStruct* pSMSParam = NULL;
    int32_t iResult = 0;
    bool bResult = false;

    if (NULL == pTextInfo) {
        RIL_LOG_TRACE("Enter RIL_SMS_ReadSMS_Text,FAIL! Parameter is NULL.");
        return RIL_AT_INVALID_PARAM;
    }

    // Initialize
    memset(pTextInfo, 0x00, sizeof(ST_RIL_SMS_TextInfo));

    if (false == LIB_SMS_IS_SUPPORT_CHARSET(eCharset)) {
        RIL_LOG_TRACE(
            "Enter RIL_SMS_ReadSMS_Text,FAIL! LIB_SMS_IS_SUPPORT_CHARSET FAIL! eCharset:%d",
            eCharset);
        return RIL_AT_INVALID_PARAM;
    }

    pPDUInfo = (ST_RIL_SMS_PDUInfo*) malloc(sizeof(ST_RIL_SMS_PDUInfo));
    if (NULL == pPDUInfo) {
        RIL_LOG_TRACE("Enter RIL_SMS_ReadSMS_Text,FAIL! malloc FAIL! size:%u",
                      sizeof(ST_RIL_SMS_PDUInfo));
        return RIL_AT_FAILED;
    }

    pSMSParam = (LIB_SMS_PDUParamStruct*) malloc(sizeof(LIB_SMS_PDUParamStruct));
    if (NULL == pSMSParam) {
        free(pPDUInfo);

        RIL_LOG_TRACE("Enter RIL_SMS_ReadSMS_Text,FAIL! malloc FAIL! size:%u",
                      sizeof(LIB_SMS_PDUParamStruct));
        return RIL_AT_FAILED;
    }

    // Initialize
    memset(pPDUInfo, 0x00, sizeof(ST_RIL_SMS_PDUInfo));
    memset(pSMSParam, 0x00, sizeof(LIB_SMS_PDUParamStruct));

    iResult = CmdReadPDUMsg(uIndex, pPDUInfo);
    if (iResult != RIL_ATRSP_SUCCESS) {
        free(pPDUInfo);
        free(pSMSParam);

        RIL_LOG_TRACE("Enter RIL_SMS_ReadSMS_Text,FAIL! CmdReadPDUMsg FAIL!");
        return iResult;
    }

    // The message of <uIndex> position is NOT exist.
    if (0 == (pPDUInfo->length)) {
        SMS_SET_INVALID_TEXT_INFO(pTextInfo);

        free(pPDUInfo);
        free(pSMSParam);

        RIL_LOG_TRACE("Enter RIL_SMS_ReadSMS_PDU,SUCCESS. NOTE: (pPDUInfo->length) is 0.");
        return RIL_ATRSP_SUCCESS;
    }

    // Check the <pPDUInfo>
    if (false == IS_VALID_PDU_INFO(pPDUInfo)) {
        free(pPDUInfo);
        free(pSMSParam);

        RIL_LOG_TRACE("Enter RIL_SMS_ReadSMS_Text,FAIL! IS_VALID_PDU_INFO FAIL!");
        return RIL_AT_FAILED;
    }

    bResult = LIB_SMS_DecodePDUStr((pPDUInfo->data), (pPDUInfo->length), pSMSParam);
    if (false == bResult) {
        RIL_LOG_TRACE("Enter RIL_SMS_ReadSMS_Text,FAIL! LIB_SMS_DecodePDUStr FAIL! PDU length:%u",
                      (pPDUInfo->length));

        free(pPDUInfo);
        free(pSMSParam);

        return RIL_AT_FAILED;
    }

    (pTextInfo->status) = (pPDUInfo->status);

    bResult = ConvSMSParamToTextInfo(eCharset, pSMSParam, pTextInfo);
    if (false == bResult) {
        free(pPDUInfo);
        free(pSMSParam);

        RIL_LOG_TRACE("Enter RIL_SMS_ReadSMS_Text,FAIL! ConvSMSParamToTextInfo FAIL!");
        return RIL_AT_FAILED;
    }

    free(pPDUInfo);
    free(pSMSParam);

    RIL_LOG_TRACE("Enter RIL_SMS_ReadSMS_Text,SUCCESS. status:%u", (pTextInfo->status));

    return RIL_AT_SUCCESS;
}

/**
 * @brief Send SMS message in PDU format
 * @param pPDUStr Pointer to PDU string (hex-encoded)
 * @param uPDUStrLen Length of PDU string
 * @param pMsgRef Pointer to store message reference number (can be NULL)
 * @return RIL_AT_SUCCESS on success, error code on failure
 * @details Sends an SMS message using PDU format via AT+CMGS command.
 *          The PDU string should be hex-encoded and contain complete
 *          SMS data including headers and message body.
 * @note PDU format allows full control over SMS parameters
 */
int32_t RIL_SMS_SendSMS_PDU(char* pPDUStr, uint32_t uPDUStrLen, uint32_t* pMsgRef) {
    bool bResult = false;
    int32_t iResult = 0;

    if ((NULL == pPDUStr) || (0 == uPDUStrLen)) {
        RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_PDU,FAIL! Parameter is NULL.");
        return RIL_AT_INVALID_PARAM;
    }

    LIB_SMS_CHECK_SUBMIT_PDU_STR_FOR_SEND(pPDUStr, uPDUStrLen, bResult);
    if (false == bResult) {
        RIL_LOG_TRACE(
            "Enter RIL_SMS_SendSMS_PDU,FAIL! LIB_SMS_CHECK_SUBMIT_PDU_STR_FOR_SEND FAIL!");
        return RIL_AT_INVALID_PARAM;
    }

    iResult = CmdSendPDUMsg(pPDUStr, uPDUStrLen, pMsgRef);

    RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_PDU,SUCCESS. uPDUStrLen:%u,iResult:%d", uPDUStrLen,
                  iResult);

    return iResult;
}

/**
 * @brief Send SMS message in text format
 * @param pNumber Pointer to destination phone number
 * @param uNumberLen Length of phone number
 * @param eCharset Character set for message encoding
 * @param pMsg Pointer to message content
 * @param uMsgLen Length of message content
 * @param pMsgRef Pointer to store message reference number (can be NULL)
 * @return RIL_AT_SUCCESS on success, error code on failure
 * @details Sends an SMS message in text format by converting to PDU internally.
 *          Supports various character sets and handles message encoding automatically.
 *          Phone number can include international prefix (+).
 * @note Text format is more convenient but less flexible than PDU format
 */
int32_t RIL_SMS_SendSMS_Text(char* pNumber, uint8_t uNumberLen, LIB_SMS_CharSetEnum eCharset,
                             uint8_t* pMsg, uint32_t uMsgLen, uint32_t* pMsgRef) {
    LIB_SMS_PDUParamStruct* pParam = NULL;
    LIB_SMS_SubmitPDUParamStruct* pSubmitParam = NULL;
    LIB_SMS_PDUInfoStruct* pInfo = NULL;
    bool bResult = false;
    char* pPDUStr = NULL;
    uint32_t uPDUStrLen = 0;
    int32_t iResult = 0;

    if ((NULL == pNumber) || (0 == uNumberLen) || (NULL == pMsg)) {
        RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_Text,FAIL! Parameter is NULL.");
        return RIL_AT_INVALID_PARAM;
    }

    if (false == LIB_SMS_IS_SUPPORT_CHARSET(eCharset)) {
        RIL_LOG_TRACE(
            "Enter RIL_SMS_SendSMS_Text,FAIL! LIB_SMS_IS_SUPPORT_CHARSET FAIL! eCharset:%u",
            eCharset);
        return RIL_AT_INVALID_PARAM;
    }

    pParam = (LIB_SMS_PDUParamStruct*) malloc(sizeof(LIB_SMS_PDUParamStruct));
    if (NULL == pParam) {
        RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_Text,FAIL! malloc FAIL! size:%u",
                      sizeof(LIB_SMS_PDUParamStruct));
        return RIL_AT_FAILED;
    }

    pInfo = (LIB_SMS_PDUInfoStruct*) malloc(sizeof(LIB_SMS_PDUInfoStruct));
    if (NULL == pInfo) {
        free(pParam);

        RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_Text,FAIL! malloc FAIL! size:%u",
                      sizeof(LIB_SMS_PDUInfoStruct));
        return RIL_AT_FAILED;
    }

    // Initialize
    memset(pParam, 0x00, sizeof(LIB_SMS_PDUParamStruct));
    memset(pInfo, 0x00, sizeof(LIB_SMS_PDUInfoStruct));
    pSubmitParam = &((pParam->sParam).sSubmitParam);

    // Set <FO>
    (pParam->uFO) = LIB_SMS_DEFAULT_FO_IN_SUBMIT_PDU;

    // Set <DA>
    bResult = ConvStringToPhoneNumber(pNumber, uNumberLen, &(pSubmitParam->sDA));
    if (false == bResult) {
        free(pParam);
        free(pInfo);

        RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_Text,FAIL! ConvStringToPhoneNumber FAIL!");
        return RIL_AT_FAILED;
    }

    // Set <PID>
    (pSubmitParam->uPID) = LIB_SMS_PDU_DEFAULT_PID;
    // Set <DCS>
    LIB_SMS_SET_DEFAULT_DCS_IN_SUBMIT_PDU(eCharset, (pSubmitParam->uDCS));
    // Set <VP>
    ((pSubmitParam->sVP).uRelative) = LIB_SMS_SUBMIT_PDU_DEFAULT_VP_RELATIVE;

    // Set <UD>
    ((pSubmitParam->sUserData).uLen) = sizeof((pSubmitParam->sUserData).aUserData);
    bResult = LIB_SMS_ConvCharSetToAlpha(eCharset, pMsg, uMsgLen, (pSubmitParam->uDCS),
                                         ((pSubmitParam->sUserData).aUserData),
                                         &((pSubmitParam->sUserData).uLen));

    if (false == bResult) {
        free(pParam);
        free(pInfo);

        RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_Text,FAIL! LIB_SMS_ConvCharSetToAlpha FAIL!");
        return RIL_AT_FAILED;
    }

    bResult = LIB_SMS_EncodeSubmitPDU(pParam, pInfo);
    if (false == bResult) {
        free(pParam);
        free(pInfo);

        RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_Text,FAIL! LIB_SMS_EncodeSubmitPDU FAIL!");
        return RIL_AT_FAILED;
    }

    free(pParam); // Not need <pParam>,free it directly
    pParam = NULL;

    pPDUStr = (char*) malloc((pInfo->uLen) * 2);
    if (NULL == pPDUStr) {
        free(pInfo);

        RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_Text,FAIL! malloc FAIL! size:%d", ((pInfo->uLen) * 2));
        return RIL_AT_FAILED;
    }

    uPDUStrLen = ((pInfo->uLen) * 2);
    bResult = LIB_SMS_ConvHexOctToHexStr((pInfo->aPDUOct), (pInfo->uLen), pPDUStr,
                                         (uint16_t*) &uPDUStrLen);
    if (NULL == pPDUStr) {
        free(pInfo);
        free(pPDUStr);

        RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_Text,FAIL! malloc FAIL! size:%d", ((pInfo->uLen) * 2));
        return RIL_AT_FAILED;
    }

    // Now send SUBMIT-PDU message
    LIB_SMS_CHECK_SUBMIT_PDU_STR_FOR_SEND(pPDUStr, uPDUStrLen, bResult);
    if (false == bResult) {
        free(pInfo);
        free(pPDUStr);

        RIL_LOG_TRACE(
            "Enter RIL_SMS_SendSMS_PDU,FAIL! LIB_SMS_CHECK_SUBMIT_PDU_STR_FOR_SEND FAIL!");
        return RIL_AT_INVALID_PARAM;
    }

    iResult = CmdSendPDUMsg(pPDUStr, uPDUStrLen, pMsgRef);

    RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_PDU,SUCCESS. iResult:%d", iResult);

    free(pInfo);
    free(pPDUStr);

    return iResult;
}

/**
 * @brief Send SMS message in text format with extended options
 * @param pNumber Pointer to phone number string
 * @param uNumberLen Length of phone number
 * @param eCharset Character set value from LIB_SMS_CharSetEnum
 * @param pMsg Pointer to message content
 * @param uMsgLen Length of message content
 * @param pMsgRef Pointer to store message reference number (can be NULL)
 * @param pExt Pointer to ST_RIL_SMS_SendExt structure for extended options:
 *             - conPres: Concatenated SMS present flag
 *             - con: ST_RIL_SMS_Con data for concatenated SMS
 * @return RIL_AT_SUCCESS if AT command sent successfully
 * @return RIL_AT_FAILED if AT command failed
 * @return RIL_AT_TIMEOUT if AT command timeout
 * @return RIL_AT_BUSY if AT interface is busy
 * @return RIL_AT_INVALID_PARAM if input parameters are invalid
 * @return RIL_AT_UNINITIALIZED if RIL is not ready
 * @details Sends SMS message in text format with support for concatenated SMS.
 *          Converts text to PDU format internally for transmission.
 * @note If you don't want message reference value, set pMsgRef to NULL.
 *       For normal SMS, you can set pExt to NULL.
 */
int32_t RIL_SMS_SendSMS_Text_Ext(char* pNumber, uint8_t uNumberLen, LIB_SMS_CharSetEnum eCharset,
                                 uint8_t* pMsg, uint32_t uMsgLen, uint32_t* pMsgRef,
                                 ST_RIL_SMS_SendExt* pExt) {
    LIB_SMS_PDUParamStruct* pParam = NULL;
    LIB_SMS_SubmitPDUParamStruct* pSubmitParam = NULL;
    LIB_SMS_PDUInfoStruct* pInfo = NULL;
    bool bResult = false;
    char* pPDUStr = NULL;
    uint32_t uPDUStrLen = 0;
    int32_t iResult = 0;

    if ((NULL == pNumber) || (0 == uNumberLen) || (NULL == pMsg)) {
        RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_Text_Ext,FAIL! Parameter is NULL.");
        return RIL_AT_INVALID_PARAM;
    }

    if (false == LIB_SMS_IS_SUPPORT_CHARSET(eCharset)) {
        RIL_LOG_TRACE(
            "Enter RIL_SMS_SendSMS_Text_Ext,FAIL! LIB_SMS_IS_SUPPORT_CHARSET FAIL! eCharset:%u",
            eCharset);
        return RIL_AT_INVALID_PARAM;
    }

    if ((pExt != NULL) && (true == pExt->conPres)) {
        if (((pExt->con.msgType) != LIB_SMS_UD_TYPE_CON_6_BYTE) &&
            ((pExt->con.msgType) != LIB_SMS_UD_TYPE_CON_7_BYTE)) {
            RIL_LOG_TRACE(
                "Enter RIL_SMS_SendSMS_Text_Ext,WARNING! msgType:%d is INVALID,use default!",
                pExt->con.msgType);
            (pExt->con.msgType) = LIB_SMS_UD_TYPE_CON_DEFAULT;
        }

        if (false == IsValidConParam(&(pExt->con))) {
            RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_Text_Ext,FAIL! IsValidConParam FAIL!");
            return RIL_AT_INVALID_PARAM;
        }
    }

    pParam = (LIB_SMS_PDUParamStruct*) malloc(sizeof(LIB_SMS_PDUParamStruct));
    if (NULL == pParam) {
        RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_Text_Ext,FAIL! malloc FAIL! size:%u",
                      sizeof(LIB_SMS_PDUParamStruct));
        return RIL_AT_FAILED;
    }

    pInfo = (LIB_SMS_PDUInfoStruct*) malloc(sizeof(LIB_SMS_PDUInfoStruct));
    if (NULL == pInfo) {
        free(pParam);

        RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_Text_Ext,FAIL! malloc FAIL! size:%u",
                      sizeof(LIB_SMS_PDUInfoStruct));
        return RIL_AT_FAILED;
    }

    // Initialize
    memset(pParam, 0x00, sizeof(LIB_SMS_PDUParamStruct));
    memset(pInfo, 0x00, sizeof(LIB_SMS_PDUInfoStruct));
    pSubmitParam = &((pParam->sParam).sSubmitParam);

    // Set <FO>
    (pParam->uFO) = LIB_SMS_DEFAULT_FO_IN_SUBMIT_PDU;
    if ((pExt != NULL) && (true == pExt->conPres)) {
        (pParam->uFO) |= (LIB_SMS_PDU_FO_UDHI_BIT_HAS_UDH << 6);
        pSubmitParam->sConSMSParam.uMsgType = pExt->con.msgType;
        pSubmitParam->sConSMSParam.uMsgRef = pExt->con.msgRef;
        pSubmitParam->sConSMSParam.uMsgSeg = pExt->con.msgSeg;
        pSubmitParam->sConSMSParam.uMsgTot = pExt->con.msgTot;
    }

    // Set <DA>
    bResult = ConvStringToPhoneNumber(pNumber, uNumberLen, &(pSubmitParam->sDA));
    if (false == bResult) {
        free(pParam);
        free(pInfo);

        RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_Text,FAIL! ConvStringToPhoneNumber FAIL!");
        return RIL_AT_FAILED;
    }

    // Set <PID>
    (pSubmitParam->uPID) = LIB_SMS_PDU_DEFAULT_PID;
    // Set <DCS>
    LIB_SMS_SET_DEFAULT_DCS_IN_SUBMIT_PDU(eCharset, (pSubmitParam->uDCS));
    // Set <VP>
    ((pSubmitParam->sVP).uRelative) = LIB_SMS_SUBMIT_PDU_DEFAULT_VP_RELATIVE;

    // Set <UD>
    ((pSubmitParam->sUserData).uLen) = sizeof((pSubmitParam->sUserData).aUserData);
    bResult = LIB_SMS_ConvCharSetToAlpha(eCharset, pMsg, uMsgLen, (pSubmitParam->uDCS),
                                         ((pSubmitParam->sUserData).aUserData),
                                         &((pSubmitParam->sUserData).uLen));

    if (false == bResult) {
        free(pParam);
        free(pInfo);

        RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_Text,FAIL! LIB_SMS_ConvCharSetToAlpha FAIL!");
        return RIL_AT_FAILED;
    }

    bResult = LIB_SMS_EncodeSubmitPDU(pParam, pInfo);
    if (false == bResult) {
        free(pParam);
        free(pInfo);

        RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_Text,FAIL! LIB_SMS_EncodeSubmitPDU FAIL!");
        return RIL_AT_FAILED;
    }

    free(pParam); // Not need <pParam>,free it directly
    pParam = NULL;

    pPDUStr = (char*) malloc((pInfo->uLen) * 2);
    if (NULL == pPDUStr) {
        free(pInfo);

        RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_Text,FAIL! malloc FAIL! size:%d", ((pInfo->uLen) * 2));
        return RIL_AT_FAILED;
    }

    uPDUStrLen = ((pInfo->uLen) * 2);
    bResult = LIB_SMS_ConvHexOctToHexStr((pInfo->aPDUOct), (pInfo->uLen), pPDUStr,
                                         (uint16_t*) &uPDUStrLen);
    if (NULL == pPDUStr) {
        free(pInfo);
        free(pPDUStr);

        RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_Text,FAIL! malloc FAIL! size:%d", ((pInfo->uLen) * 2));
        return RIL_AT_FAILED;
    }

    // Now send SUBMIT-PDU message
    LIB_SMS_CHECK_SUBMIT_PDU_STR_FOR_SEND(pPDUStr, uPDUStrLen, bResult);
    if (false == bResult) {
        free(pInfo);
        free(pPDUStr);

        RIL_LOG_TRACE(
            "Enter RIL_SMS_SendSMS_PDU,FAIL! LIB_SMS_CHECK_SUBMIT_PDU_STR_FOR_SEND FAIL!");
        return RIL_AT_INVALID_PARAM;
    }

    iResult = CmdSendPDUMsg(pPDUStr, uPDUStrLen, pMsgRef);

    RIL_LOG_TRACE("Enter RIL_SMS_SendSMS_PDU,SUCCESS. iResult:%d", iResult);

    free(pInfo);
    free(pPDUStr);

    return iResult;
}

/**
 * @brief Delete SMS messages from current SMS storage
 * @param uIndex Index number of SMS message to delete
 * @param eDelFlag Delete flag from Enum_RIL_SMS_DeleteFlag enumeration
 * @return RIL_AT_SUCCESS if AT command sent successfully
 * @return RIL_AT_FAILED if AT command failed
 * @return RIL_AT_TIMEOUT if AT command timeout
 * @return RIL_AT_BUSY if AT interface is busy
 * @return RIL_AT_INVALID_PARAM if input parameters are invalid
 * @return RIL_AT_UNINITIALIZED if RIL is not ready
 * @details Deletes SMS messages using AT+CMGD command. Supports deletion of
 *          individual messages by index or bulk deletion operations.
 */
int32_t RIL_SMS_DeleteSMS(uint32_t uIndex, Enum_RIL_SMS_DeleteFlag eDelFlag) {
    int32_t iResult = 0;
    uint8_t uStoType = 0; // Current memory type,same as: 'Enum_RIL_SMS_SMSStorage'
    uint32_t uStoTot = 0; // The total count of current memory
    uint32_t uCoreIdx = 0;

    int32_t iLen = 0;
    char aCmd[SMS_CMD_MAX_LEN] = {
        0,
    };

    // Check parameter <eDelFlag>
    if (eDelFlag > RIL_SMS_DEL_ALL_MSG) {
        RIL_LOG_TRACE("Enter RIL_SMS_DeleteSMS,FAIL! eDelFlag is INVALID. eDelFlag:%d", eDelFlag);
        return RIL_AT_INVALID_PARAM;
    }

    // Get the total count of current memory type
    iResult = CmdGetStorageInfo(&uStoType, NULL, &uStoTot);
    if (iResult != RIL_ATRSP_SUCCESS) {
        RIL_LOG_TRACE(
            "Enter RIL_SMS_DeleteSMS,FAIL! CmdGetStorageInfo FAIL! AT+CPMS? execute FAL!");
        return RIL_AT_INVALID_PARAM;
    }

    // Check parameter <uIndex>
    if ((RIL_SMS_DEL_INDEXED_MSG == eDelFlag) && ((uIndex < 1) || (uIndex > uStoTot))) {
        RIL_LOG_TRACE(
            "Enter RIL_SMS_DeleteSMS,FAIL! uIndex is INVALID. eDelFlag:%d,uIndex:%u,uCurrStoTot:%u",
            eDelFlag, uIndex, uStoTot);
        return RIL_AT_INVALID_PARAM;
    }

    // Covert RIL SMS index to core SMS index
    if (RIL_SMS_DEL_INDEXED_MSG == eDelFlag) {
        uCoreIdx = ADP_SMS_ConvIdxToCoreIdx(uStoType, uIndex, uStoTot);
        if (0 == uCoreIdx) {
            RIL_LOG_TRACE("Enter RIL_SMS_DeleteSMS,FAIL! ADP_SMS_ConvIdxToCoreIdx FAIL! "
                          "uCurrStoType:%u,uIndex:%u,uCurrStoTot:%u",
                          uStoType, uIndex, uStoTot);
            return RIL_AT_INVALID_PARAM;
        }
    } else {
        uCoreIdx = uIndex;
    }

    // Set to PDU mode
    SMS_SET_PDU_MODE(iResult, "RIL_SMS_DeleteSMS");

    // Delete SMS use AT+CMGD=<index>,<delflag>
    iLen = sprintf(aCmd, "AT+CMGD=%d,%d", uIndex, eDelFlag);
    iResult = RIL_SendATCmd(aCmd, iLen, NULL, NULL, 0);

    RIL_LOG_TRACE(
        "Enter RIL_SMS_DeleteSMS,SUCCESS. CMD: %s,uStoType:%u,uStoTot:%u,eDelFlag:%u,uIndex:%u",
        aCmd, uStoType, uStoTot, eDelFlag, uIndex);

    return iResult;
}
