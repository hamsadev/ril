#ifndef _LIB_RIL_SMS_H_
#define _LIB_RIL_SMS_H_

#include "stdbool.h"
#include "stdint.h"

/***********************************************************************
 * MACRO CONSTANT DEFINITIONS
 ************************************************************************/
// Base characters definition
#define LIB_SMS_CHAR_STAR '*'
#define LIB_SMS_CHAR_POUND '#'
#define LIB_SMS_CHAR_PLUS '+'
#define LIB_SMS_CHAR_MINUS '-'

#define LIB_SMS_CHAR_QM '?'

#define LIB_SMS_CHAR_A 'A'
#define LIB_SMS_CHAR_B 'B'
#define LIB_SMS_CHAR_C 'C'
#define LIB_SMS_CHAR_D 'D'
#define LIB_SMS_CHAR_E 'E'
#define LIB_SMS_CHAR_F 'F'

#define LIB_SMS_CHAR_a 'a'
#define LIB_SMS_CHAR_b 'b'
#define LIB_SMS_CHAR_c 'c'
#define LIB_SMS_CHAR_d 'd'
#define LIB_SMS_CHAR_e 'e'
#define LIB_SMS_CHAR_f 'f'

#define LIB_SMS_CHAR_0 '0'
#define LIB_SMS_CHAR_1 '1'
#define LIB_SMS_CHAR_2 '2'
#define LIB_SMS_CHAR_3 '3'
#define LIB_SMS_CHAR_4 '4'
#define LIB_SMS_CHAR_5 '5'
#define LIB_SMS_CHAR_6 '6'
#define LIB_SMS_CHAR_7 '7'
#define LIB_SMS_CHAR_8 '8'
#define LIB_SMS_CHAR_9 '9'

#define LIB_SMS_PHONE_NUMBER_MAX_LEN (20) // Unit is: one character
#define LIB_SMS_USER_DATA_MAX_LEN (160)   // Unit is: one character

#define LIB_SMS_PDU_BUF_MAX_LEN (180)

#define LIB_SMS_PHONE_NUMBER_TYPE_INTERNATIONAL (0x91) // 145
#define LIB_SMS_PHONE_NUMBER_TYPE_NATIONAL (0xA1)      // 161
#define LIB_SMS_PHONE_NUMBER_TYPE_UNKNOWN (0x81)       // 129
//<2015/03/23-ROTVG00006-P04-Vicent.Gao,<[SMS] Segment 4==>Fix issues of RIL SMS LIB.>
#define LIB_SMS_PHONE_NUMBER_TYPE_ALPHANUMERIC (0x50) // 80
//>2015/03/23-ROTVG00006-P04-Vicent.Gao

#define LIB_SMS_PDU_FO_UDHI_BIT_NO_UDH (0)
#define LIB_SMS_PDU_FO_UDHI_BIT_HAS_UDH (1)

#define LIB_SMS_PDU_DCS_NO_MSG_CLASS (0)
#define LIB_SMS_PDU_DCS_HAS_MSG_CLASS (1)

#define LIB_SMS_PDU_DEFAULT_PID (0x00)

#define LIB_SMS_SUBMIT_PDU_FO_SRR_BIT_NO_STATUS_REPORT (0)
#define LIB_SMS_SUBMIT_PDU_FO_SRR_BIT_HAS_STATUS_REPORT (1)

#define LIB_SMS_SUBMIT_PDU_DEFAULT_VP_RELATIVE (167)

#define LIB_SMS_UD_TYPE_CON_DEFAULT (LIB_SMS_UD_TYPE_CON_6_BYTE)

#define LIB_SMS_DEFAULT_FO_IN_SUBMIT_PDU                                                           \
    (((LIB_SMS_PDU_TYPE_SUBMIT) << 0) | ((LIB_SMS_VPF_TYPE_RELATIVE) << 3) |                       \
     ((LIB_SMS_SUBMIT_PDU_FO_SRR_BIT_NO_STATUS_REPORT) << 5) |                                     \
     ((LIB_SMS_PDU_FO_UDHI_BIT_NO_UDH) << 6))

/***********************************************************************
 * ENUM TYPE DEFINITIONS
 ************************************************************************/

// This enum is refer to 'GSM 03.40 Clause 9.2.3.1 TP-Message-Type-Indicator (TP-MTI)'
// Warning: Please NOT-CHANGE this enum!
typedef enum {
    LIB_SMS_PDU_TYPE_DELIVER = 0x00,
    LIB_SMS_PDU_TYPE_SUBMIT = 0x01,
    LIB_SMS_PDU_TYPE_STATUS_REPORT = 0x02,
    LIB_SMS_PDU_TYPE_RESERVED = 0x03,

    LIB_SMS_PDU_TYPE_INVALID = 0xFF,
} LIB_SMS_PDUTypeEnum;

// This enum is refer to 'GSM 03.40 Clause 9.2.3.3 TP-Validity-Period-Format (TP-VPF)'
// Warning: Please NOT-CHANGE this enum!
typedef enum {
    LIB_SMS_VPF_TYPE_NOT_PRESENT = 0x00,
    LIB_SMS_VPF_TYPE_RELATIVE = 0x02,
    LIB_SMS_VPF_TYPE_ENHANCED = 0x01,
    LIB_SMS_VPF_TYPE_ABSOLUTE = 0x03,

    VPF_TYPE_INVALID = 0xFF
} LIB_SMS_VPFTypeEnum;

typedef enum {
    LIB_SMS_DCS_ALPHA_DEFAULT = 0,
    LIB_SMS_DCS_ALPHA_8BIT_DATA = 1,
    LIB_SMS_DCS_ALPHA_UCS2 = 2,

    LIB_SMS_DCS_ALPHA_INVALID = 0xFF
} LIB_SMS_DCSAlphaEnum;

typedef enum {
    LIB_SMS_DCS_MSG_CLASS_0 = 0,        // PLS NOT CHANGE!!
    LIB_SMS_DCS_MSG_CLASS_1 = 1,        // PLS NOT CHANGE!!
    LIB_SMS_DCS_MSG_CLASS_2 = 2,        // PLS NOT CHANGE!!
    LIB_SMS_DCS_MSG_CLASS_3 = 3,        // PLS NOT CHANGE!!
    LIB_SMS_DCS_MSG_CLASS_RESERVED = 4, // PLS NOT CHANGE!!

    MSG_CLASS_TYPE_INVALID = 0xFF,
} LIB_SMS_DCSMsgClassEnum;

typedef enum {
    LIB_SMS_DCS_COMP_UNCOMPRESSED = 0x00, // PLS NOT CHANGE!!
    LIB_SMS_DCS_COMP_COMPRESSED = 0x01,   // PLS NOT CHANGE!!

    LIB_SMS_DCS_COMP_INVALID = 0xFF,
} LIB_SMS_DCSCompressEnum;

// NOTE: This enum ONLY in send/read/write text message.
typedef enum {
    LIB_SMS_CHARSET_GSM = 0, // See 3GPP TS 23.038 Clause '6.2.1 GSM 7 bit Default Alphabet'
    LIB_SMS_CHARSET_HEX = 1,
    LIB_SMS_CHARSET_UCS2 = 2,
    LIB_SMS_CHARSET_IRA = 3,
    LIB_SMS_CHARSET_8859_1 = 4,

    //==> Warning: Please add new charset upper this line!
    LIB_SMS_CHARSET_INVALID = 0xFF
} LIB_SMS_CharSetEnum;

typedef enum {
    LIB_SMS_UD_TYPE_NORMAL = 0,
    LIB_SMS_UD_TYPE_CON_6_BYTE = 1,
    LIB_SMS_UD_TYPE_CON_7_BYTE = 2,

    //==> Waring: Please add new UDH Type upper thils line!!
    LIB_SMS_UD_TYPE_INVALID = 0xFF
} LIB_SMS_UDTypeEnum;

/***********************************************************************
 * STRUCT TYPE DEFINITIONS
 ************************************************************************/
typedef struct {
    uint8_t uType;
    uint8_t aNumber[LIB_SMS_PHONE_NUMBER_MAX_LEN];
    uint8_t uLen;
} LIB_SMS_PhoneNumberStruct;

typedef struct {
    uint8_t uYear;
    uint8_t uMonth;
    uint8_t uDay;
    uint8_t uHour;
    uint8_t uMinute;
    uint8_t uSecond;
    int8_t iTimeZone;
} LIB_SMS_TimeStampStruct;

typedef struct {
    uint8_t uMsgType; // LIB_SMS_UDH_TYPE_CON_8_BIT or LIB_SMS_UDH_TYPE_CON_16_BIT
    uint16_t uMsgRef;
    uint8_t uMsgSeg;
    uint8_t uMsgTot;
} LIB_SMS_ConSMSParamStruct;

typedef struct {
    uint8_t aUserData[LIB_SMS_USER_DATA_MAX_LEN];
    uint16_t uLen;
} LIB_SMS_UserDataStruct;

typedef union {
    uint8_t uRelative;
    LIB_SMS_TimeStampStruct sAbsolute;
} LIB_SMS_ValidityPeriodUnion;

typedef struct {
    LIB_SMS_ConSMSParamStruct sConSMSParam;

    LIB_SMS_PhoneNumberStruct sOA;
    uint8_t uPID;
    uint8_t uDCS;

    LIB_SMS_TimeStampStruct sSCTS;
    LIB_SMS_UserDataStruct sUserData;
} LIB_SMS_DeliverPDUParamStruct;

typedef struct {
    LIB_SMS_ConSMSParamStruct sConSMSParam;

    LIB_SMS_PhoneNumberStruct sDA;
    uint8_t uPID;
    uint8_t uDCS;

    LIB_SMS_ValidityPeriodUnion sVP;
    LIB_SMS_UserDataStruct sUserData;
} LIB_SMS_SubmitPDUParamStruct;

typedef struct {
    LIB_SMS_PhoneNumberStruct sSCA;
    uint8_t uFO;

    union {
        LIB_SMS_DeliverPDUParamStruct sDeliverParam;
        LIB_SMS_SubmitPDUParamStruct sSubmitParam;
    } sParam;

} LIB_SMS_PDUParamStruct;

typedef struct {
    uint8_t aPDUOct[LIB_SMS_PDU_BUF_MAX_LEN];
    uint16_t uLen;
} LIB_SMS_PDUInfoStruct;

/***********************************************************************
 * MACRO FUNCTION DEFINITIONS
 ************************************************************************/
#define LIB_SMS_SET_POINTER_VAL(p, v)                                                              \
    do {                                                                                           \
        if ((p) != NULL) {                                                                         \
            (*(p)) = (v);                                                                          \
        }                                                                                          \
    } while (0)

#define LIB_SMS_GET_MSG_TYPE_IN_PDU_FO(FirstOctet) ((FirstOctet) & 0x03)
#define LIB_SMS_GET_UDHI_IN_PDU(FirstOctet) (((FirstOctet) & 0x40) >> 6)
#define LIB_SMS_GET_VPF_IN_SUBMIT_PDU_FO(FirstOctet) (((FirstOctet) & 0x18) >> 3)

#define LIB_SMS_SET_DEFAULT_DCS_IN_SUBMIT_PDU(CharSet, DCS)                                        \
    do {                                                                                           \
        uint8_t uAlphaZ = 0;                                                                       \
                                                                                                   \
        if (LIB_SMS_CHARSET_HEX == (CharSet)) {                                                    \
            uAlphaZ = LIB_SMS_DCS_ALPHA_8BIT_DATA;                                                 \
        } else if (LIB_SMS_CHARSET_UCS2 == (CharSet)) {                                            \
            uAlphaZ = LIB_SMS_DCS_ALPHA_UCS2;                                                      \
        } else if (LIB_SMS_CHARSET_IRA == (CharSet)) {                                             \
            uAlphaZ = LIB_SMS_DCS_ALPHA_DEFAULT;                                                   \
        } else if (LIB_SMS_CHARSET_8859_1 == (CharSet)) {                                          \
            uAlphaZ = LIB_SMS_DCS_ALPHA_DEFAULT;                                                   \
        } else {                                                                                   \
            uAlphaZ = LIB_SMS_DCS_ALPHA_DEFAULT;                                                   \
        }                                                                                          \
                                                                                                   \
        (DCS) = (((uAlphaZ) << 2) | ((LIB_SMS_PDU_DCS_NO_MSG_CLASS) << 4) |                        \
                 ((LIB_SMS_DCS_COMP_UNCOMPRESSED) << 5));                                          \
    } while (0);

#define LIB_SMS_GET_ALPHA_IN_PDU_DCS(DataCodeScheme, Alphabet)                                     \
    LIB_SMS_DecodeDCS((DataCodeScheme), NULL, &(Alphabet), NULL, NULL);

#define LIB_SMS_IS_SUPPORT_CHARSET(CharSet)                                                        \
    (((LIB_SMS_CHARSET_GSM == (CharSet)) || (LIB_SMS_CHARSET_HEX == (CharSet)) ||                  \
      (LIB_SMS_CHARSET_UCS2 == (CharSet)) || (LIB_SMS_CHARSET_IRA == (CharSet)) ||                 \
      (LIB_SMS_CHARSET_8859_1 == (CharSet)))                                                       \
         ? true                                                                                    \
         : false)

#define LIB_SMS_CHECK_PDU_STR(PDUStr, PDUStrLen, Result)                                           \
    do {                                                                                           \
        LIB_SMS_PDUParamStruct* pTmpZA = NULL;                                                     \
                                                                                                   \
        pTmpZA = (LIB_SMS_PDUParamStruct*) malloc(sizeof(LIB_SMS_PDUParamStruct));                 \
        if (NULL == pTmpZA) {                                                                      \
            (Result) = false;                                                                      \
            break;                                                                                 \
        }                                                                                          \
                                                                                                   \
        (Result) = LIB_SMS_DecodePDUStr((PDUStr), (PDUStrLen), pTmpZA);                            \
        free(pTmpZA);                                                                              \
        (Result) = true;                                                                           \
    } while (0)

#define LIB_SMS_CHECK_SUBMIT_PDU_STR_FOR_SEND(PDUStr, PDUStrLen, Result)                           \
    do {                                                                                           \
        LIB_SMS_PDUParamStruct* pTmpZA = NULL;                                                     \
        LIB_SMS_SubmitPDUParamStruct* pSubmitParamZA = NULL;                                       \
                                                                                                   \
        pTmpZA = (LIB_SMS_PDUParamStruct*) malloc(sizeof(LIB_SMS_PDUParamStruct));                 \
        if (NULL == pTmpZA) {                                                                      \
            (Result) = false;                                                                      \
            break;                                                                                 \
        }                                                                                          \
                                                                                                   \
        (Result) = LIB_SMS_DecodePDUStr((PDUStr), (PDUStrLen), pTmpZA);                            \
        if (false == (Result)) {                                                                   \
            free(pTmpZA);                                                                          \
            break;                                                                                 \
        }                                                                                          \
                                                                                                   \
        if (LIB_SMS_PDU_TYPE_SUBMIT != LIB_SMS_GET_MSG_TYPE_IN_PDU_FO(pTmpZA->uFO)) {              \
            free(pTmpZA);                                                                          \
            (Result) = false;                                                                      \
            break;                                                                                 \
        }                                                                                          \
                                                                                                   \
        pSubmitParamZA = &((pTmpZA->sParam).sSubmitParam);                                         \
                                                                                                   \
        if (0 == ((pSubmitParamZA->sDA).uLen)) {                                                   \
            free(pTmpZA);                                                                          \
            (Result) = false;                                                                      \
            break;                                                                                 \
        }                                                                                          \
                                                                                                   \
        free(pTmpZA);                                                                              \
        (Result) = true;                                                                           \
    } while (0)

#define LIB_SMS_IS_VALID_ASCII_NUMBER_CHAR(Char)                                                   \
    (((((Char) >= LIB_SMS_CHAR_0) && ((Char) <= LIB_SMS_CHAR_9)) ||                                \
      (LIB_SMS_CHAR_STAR == (Char)) || (LIB_SMS_CHAR_POUND == (Char)) ||                           \
      (((Char) >= LIB_SMS_CHAR_A) && ((Char) <= LIB_SMS_CHAR_C)) ||                                \
      (((Char) >= LIB_SMS_CHAR_a) && ((Char) <= LIB_SMS_CHAR_c)))                                  \
         ? true                                                                                    \
         : false)

/***********************************************************************
 * FUNCTION DECLARATIONS
 ************************************************************************/
/**
 * @brief This function is used to Check the hex string is VALID or not.
 *
 * @param pHexStr [In] The pointer of hex string
 * @param uHexStrLen [In] The length of hex string
 * @return true true,  This function works SUCCESS.
 * @return false false, This function works FAIL!
 *
 * NOTE:
 *              1. This is a library function.
 *              2. Please ensure all parameters are VALID.
 */
bool LIB_SMS_IsValidHexStr(char* pHexStr, uint16_t uHexStrLen);

/**
 * @brief This function is used to convert hex octet to hex string.
 *
 * @param pSrc [In] The pointer of source buffer
 * @param uSrcLen [In] The length of source buffer
 * @param pDest [In] The pointer of destination buffer
 * @param pDestLen [In] The length of destination buffer
 * @return true true,  This function works SUCCESS.
 * @return false false, This function works FAIL!
 *
 * NOTE:
 *              1. This is a library function.
 *              2. Please ensure all parameters are VALID.
 */
bool LIB_SMS_ConvHexOctToHexStr(const uint8_t* pSrc, uint16_t uSrcLen, char* pDest,
                                uint16_t* pDestLen);

/**
 * @brief This function is used to convert hex string to hex octet.
 *
 * @param pSrc [In] The pointer of source buffer
 * @param uSrcLen [In] The length of source buffer
 * @param pDest [In] The pointer of destination buffer
 * @param pDestLen [In] The length of destination buffer
 * @return true true,  This function works SUCCESS.
 * @return false false, This function works FAIL!
 *
 * NOTE:
 *              1. This is a library function.
 *              2. Please ensure all parameters are VALID.
 */
bool LIB_SMS_ConvHexStrToHexOct(const char* pSrc, uint16_t uSrcLen, uint8_t* pDest,
                                uint16_t* pDestLen);

/**
 * @brief This function is used to convert 'LIB_SMS_CharSetEnum' data to 'LIB_SMS_DCSAlphaEnum'
 * data.
 *
 * @param eCharSet [In] Character set,It's value is in 'LIB_SMS_CharSetEnum'
 * @param pSrc [In] The pointer of source data
 * @param uSrcLen [In] The length of source data
 * @param uDCS [In] <DCS> element in TPDU
 * @param pDest [In] The pointer of destination buffer
 * @param pDestLen [In] The length of destination buffer
 * @return true true,  This function works SUCCESS.
 * @return false false, This function works FAIL!
 *
 * NOTE:
 *              1. This is a library function.
 *              2. Please ensure all parameters are VALID.
 *              3. Warning: Before call this function,MUST set <*pDestLen> to a value which specify
 * the rest bytes of <pDest> buffer. [In] <DCS> element in TPDU <pDest> [In] The length of
 * destination data <pDestLen> [In] The length of destination length Return: true,  This function
 * works SUCCESS. false, This function works FAIL!
 *
 * NOTE:
 *              1. This is a library function.
 *              2. Please ensure all parameters are VALID.
 *              3. Warning: Before call this function,MUST set <*pDestLen> to a value which specify
 * the rest bytes of <pDest> buffer.
 *              4. If this function works SUCCESS,<*pDestLen> will return the bytes of <pDest>
 * buffer that has been written.
 */
bool LIB_SMS_ConvCharSetToAlpha(LIB_SMS_CharSetEnum eCharSet, uint8_t* pSrc, uint16_t uSrcLen,
                                uint8_t uDCS, uint8_t* pDest, uint16_t* pDestLen);

/**
 * @brief This function is used to convert 'LIB_SMS_DCSAlphaEnum' data to 'LIB_SMS_CharSetEnum'
 * data.
 *
 * @param uDCS [In] <DCS> element in TPDU
 * @param pSrc [In] The pointer of source data
 * @param uSrcLen [In] The length of source data
 * @param eCharSet [In] Character set,It's value is in 'LIB_SMS_CharSetEnum'
 * @param pDest [In] The pointer of destination buffer
 * @param pDestLen [In] The length of destination buffer
 * @return true true,  This function works SUCCESS.
 * @return false false, This function works FAIL!
 *
 * NOTE:
 *              1. This is a library function.
 *              2. Please ensure all parameters are VALID.
 *              3. Warning: Before call this function,MUST set <*pDestLen> to a value which specify
 * the rest bytes of <pDest> buffer.
 *              4. If this function works SUCCESS,<*pDestLen> will return the bytes of <pDest>
 * buffer that has been written.
 */
bool LIB_SMS_ConvAlphaToCharSet(uint8_t uDCS, uint8_t* pSrc, uint16_t uSrcLen,
                                LIB_SMS_CharSetEnum eCharSet, uint8_t* pDest, uint16_t* pDestLen);

/**
 * @brief This function is used to decode PDU string to 'LIB_SMS_PDUParamStruct' data.
 *
 * @param pPDUStr [In] The pointer of PDU string
 * @param uPDUStrLen [In] The length of PDU string
 * @param pParam [In] The pointer of 'LIB_SMS_PDUParamStruct' data
 * @return true true,  This function works SUCCESS.
 * @return false false, This function works FAIL!
 *
 * NOTE:
 *              1. This is a library function.
 *              2. Please ensure all parameters are VALID.
 */
bool LIB_SMS_DecodePDUStr(char* pPDUStr, uint16_t uPDUStrLen, LIB_SMS_PDUParamStruct* pParam);

/**
 * @brief This function is used to decode <DCS> in TPDU
 *
 * @param uDCS [In] <DCS> element in TPDU
 * @param pMsgType [In] The pointer of message type which is same as 'DCSMsgTypeEnum'
 * @param pAlpha [In] The pointer of alphabet which is same as 'LIB_SMS_DCSAlphaEnum'
 * @param pMsgClass [In] The pointer of message class which is same as 'DCSMsgClassEnum'
 * @param pCompress [In] The pointer of compress flag which is same as 'DCSCompressEnum'
 * @return true true,  This function works SUCCESS.
 * @return false false, This function works FAIL!
 *
 * NOTE:
 *              1. This is a library function.
 *              2. Please ensure all parameters are VALID.
 *              3. If you DONOT want to get certain parameter's value,please set it to NULL.
 */
void LIB_SMS_DecodeDCS(uint8_t uDCS, uint8_t* pMsgType, uint8_t* pAlpha, uint8_t* pMsgClass,
                       uint8_t* pCompress);

/**
 * @brief This function is used to encode SUBMIT-PDU according to given parameters
 *
 * @param pParam [In] The pointer of 'LIB_SMS_PDUParamStruct' data
 * @param pInfo [In] The pointer of 'LIB_SMS_PDUInfoStruct' data
 * @return true true,  This function works SUCCESS.
 * @return false false, This function works FAIL!
 *
 * NOTE:
 *              1. This is a library function.
 *              2. Please ensure all parameters are VALID.
 */
bool LIB_SMS_EncodeSubmitPDU(LIB_SMS_PDUParamStruct* pParam, LIB_SMS_PDUInfoStruct* pInfo);

#endif // #ifndef _LIB_RIL_SMS_H_
