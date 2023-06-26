#include "lib_ril_sms.h"
#include <stdlib.h>
#include <string.h>

bool LIB_SMS_IsValidHexStr(char* pHexStr, uint16_t uHexStrLen) {
    uint16_t i = 0;

    // Check input parameters
    if ((NULL == pHexStr) || (0 == uHexStrLen)) {
        return false;
    }

    // Check if the length of the string is even
    if (0 != (uHexStrLen % 2)) {
        return false;
    }

    // Check if each character is a valid hex character
    for (i = 0; i < uHexStrLen; i++) {
        if (!((pHexStr[i] >= LIB_SMS_CHAR_0 && pHexStr[i] <= LIB_SMS_CHAR_9) ||
              (pHexStr[i] >= LIB_SMS_CHAR_A && pHexStr[i] <= LIB_SMS_CHAR_F) ||
              (pHexStr[i] >= LIB_SMS_CHAR_a && pHexStr[i] <= LIB_SMS_CHAR_f))) {
            return false;
        }
    }

    return true;
}

bool LIB_SMS_ConvHexOctToHexStr(const uint8_t* pSrc, uint16_t uSrcLen, char* pDest,
                                uint16_t* pDestLen) {
    uint16_t i = 0;
    uint8_t uHigh = 0;
    uint8_t uLow = 0;
    uint16_t uRequiredLen = 0;

    // Check input parameters
    if ((NULL == pSrc) || (0 == uSrcLen) || (NULL == pDest) || (NULL == pDestLen)) {
        return false;
    }

    // Calculate required destination buffer length (each byte needs 2 characters)
    uRequiredLen = (uSrcLen * 2);

    // Check if destination buffer has enough space
    if (*pDestLen < uRequiredLen) {
        return false;
    }

    // Convert each byte to two hex characters
    for (i = 0; i < uSrcLen; i++) {
        // Extract high nibble and convert to hex char
        uHigh = (pSrc[i] >> 4) & 0x0F;
        if (uHigh <= 9) {
            pDest[i * 2] = uHigh + LIB_SMS_CHAR_0;
        } else {
            pDest[i * 2] = (uHigh - 10) + LIB_SMS_CHAR_A;
        }

        // Extract low nibble and convert to hex char
        uLow = pSrc[i] & 0x0F;
        if (uLow <= 9) {
            pDest[i * 2 + 1] = uLow + LIB_SMS_CHAR_0;
        } else {
            pDest[i * 2 + 1] = (uLow - 10) + LIB_SMS_CHAR_A;
        }
    }

    // Update destination length
    *pDestLen = uRequiredLen;

    return true;
}

bool LIB_SMS_ConvHexStrToHexOct(const char* pSrc, uint16_t uSrcLen, uint8_t* pDest,
                                uint16_t* pDestLen) {
    uint16_t i = 0;
    uint8_t uHigh = 0;
    uint8_t uLow = 0;
    uint16_t uRequiredLen = 0;

    // Check input parameters
    if ((NULL == pSrc) || (0 == uSrcLen) || (NULL == pDest) || (NULL == pDestLen)) {
        return false;
    }

    // Check if source string length is even (each byte needs 2 characters)
    if (0 != (uSrcLen % 2)) {
        return false;
    }

    // Calculate required destination buffer length
    uRequiredLen = (uSrcLen / 2);

    // Check if destination buffer has enough space
    if (*pDestLen < uRequiredLen) {
        return false;
    }

    // Convert each pair of hex characters to one byte
    for (i = 0; i < uSrcLen; i += 2) {
        // Convert high nibble character to value
        if (pSrc[i] >= LIB_SMS_CHAR_0 && pSrc[i] <= LIB_SMS_CHAR_9) {
            uHigh = pSrc[i] - LIB_SMS_CHAR_0;
        } else if (pSrc[i] >= LIB_SMS_CHAR_A && pSrc[i] <= LIB_SMS_CHAR_F) {
            uHigh = pSrc[i] - LIB_SMS_CHAR_A + 10;
        } else if (pSrc[i] >= LIB_SMS_CHAR_a && pSrc[i] <= LIB_SMS_CHAR_f) {
            uHigh = pSrc[i] - LIB_SMS_CHAR_a + 10;
        } else {
            return false;
        }

        // Convert low nibble character to value
        if (pSrc[i + 1] >= LIB_SMS_CHAR_0 && pSrc[i + 1] <= LIB_SMS_CHAR_9) {
            uLow = pSrc[i + 1] - LIB_SMS_CHAR_0;
        } else if (pSrc[i + 1] >= LIB_SMS_CHAR_A && pSrc[i + 1] <= LIB_SMS_CHAR_F) {
            uLow = pSrc[i + 1] - LIB_SMS_CHAR_A + 10;
        } else if (pSrc[i + 1] >= LIB_SMS_CHAR_a && pSrc[i + 1] <= LIB_SMS_CHAR_f) {
            uLow = pSrc[i + 1] - LIB_SMS_CHAR_a + 10;
        } else {
            return false;
        }

        // Combine high and low nibbles into one byte
        pDest[i / 2] = (uHigh << 4) | uLow;
    }

    // Update destination length
    *pDestLen = uRequiredLen;

    return true;
}

bool LIB_SMS_ConvCharSetToAlpha(LIB_SMS_CharSetEnum eCharSet, uint8_t* pSrc, uint16_t uSrcLen,
                                uint8_t uDCS, uint8_t* pDest, uint16_t* pDestLen) {
    uint16_t i = 0;
    uint8_t uAlpha = 0;
    uint16_t uRequiredLen = 0;

    // Check input parameters
    if ((NULL == pSrc) || (0 == uSrcLen) || (NULL == pDest) || (NULL == pDestLen)) {
        return false;
    }

    // Check if character set is supported
    if (!LIB_SMS_IS_SUPPORT_CHARSET(eCharSet)) {
        return false;
    }

    // Get alphabet type from DCS
    LIB_SMS_DecodeDCS(uDCS, NULL, &uAlpha, NULL, NULL);

    // Calculate required destination buffer length based on character set and alphabet
    switch (eCharSet) {
    case LIB_SMS_CHARSET_GSM:
        if (LIB_SMS_DCS_ALPHA_DEFAULT == uAlpha) {
            uRequiredLen = uSrcLen;
        } else if (LIB_SMS_DCS_ALPHA_8BIT_DATA == uAlpha) {
            uRequiredLen = uSrcLen;
        } else if (LIB_SMS_DCS_ALPHA_UCS2 == uAlpha) {
            uRequiredLen = uSrcLen * 2; // Each GSM character needs 2 bytes in UCS2
        }
        break;

    case LIB_SMS_CHARSET_HEX:
        if (LIB_SMS_DCS_ALPHA_8BIT_DATA == uAlpha) {
            uRequiredLen = uSrcLen;
        } else {
            return false; // HEX can only be converted to 8-bit data
        }
        break;

    case LIB_SMS_CHARSET_UCS2:
        if (LIB_SMS_DCS_ALPHA_UCS2 == uAlpha) {
            uRequiredLen = uSrcLen;
        } else {
            return false; // UCS2 can only be converted to UCS2
        }
        break;

    case LIB_SMS_CHARSET_IRA:
    case LIB_SMS_CHARSET_8859_1:
        if (LIB_SMS_DCS_ALPHA_DEFAULT == uAlpha) {
            uRequiredLen = uSrcLen;
        } else if (LIB_SMS_DCS_ALPHA_8BIT_DATA == uAlpha) {
            uRequiredLen = uSrcLen;
        } else if (LIB_SMS_DCS_ALPHA_UCS2 == uAlpha) {
            uRequiredLen = uSrcLen * 2; // Each character needs 2 bytes in UCS2
        }
        break;

    default:
        return false;
    }

    // Check if destination buffer has enough space
    if (*pDestLen < uRequiredLen) {
        return false;
    }

    // Perform the conversion based on character set and alphabet
    switch (eCharSet) {
    case LIB_SMS_CHARSET_GSM:
        if (LIB_SMS_DCS_ALPHA_DEFAULT == uAlpha || LIB_SMS_DCS_ALPHA_8BIT_DATA == uAlpha) {
            // Direct copy for GSM to default or 8-bit
            for (i = 0; i < uSrcLen; i++) {
                pDest[i] = pSrc[i];
            }
        } else if (LIB_SMS_DCS_ALPHA_UCS2 == uAlpha) {
            // Convert GSM to UCS2 (basic ASCII subset)
            for (i = 0; i < uSrcLen; i++) {
                pDest[i * 2] = 0x00;        // UCS2 high byte
                pDest[i * 2 + 1] = pSrc[i]; // UCS2 low byte (ASCII)
            }
        }
        break;

    case LIB_SMS_CHARSET_HEX:
        // Direct copy for HEX to 8-bit
        for (i = 0; i < uSrcLen; i++) {
            pDest[i] = pSrc[i];
        }
        break;

    case LIB_SMS_CHARSET_UCS2:
        // Direct copy for UCS2 to UCS2
        for (i = 0; i < uSrcLen; i++) {
            pDest[i] = pSrc[i];
        }
        break;

    case LIB_SMS_CHARSET_IRA:
    case LIB_SMS_CHARSET_8859_1:
        if (LIB_SMS_DCS_ALPHA_DEFAULT == uAlpha || LIB_SMS_DCS_ALPHA_8BIT_DATA == uAlpha) {
            // Direct copy for IRA/8859-1 to default or 8-bit
            for (i = 0; i < uSrcLen; i++) {
                pDest[i] = pSrc[i];
            }
        } else if (LIB_SMS_DCS_ALPHA_UCS2 == uAlpha) {
            // Convert IRA/8859-1 to UCS2
            for (i = 0; i < uSrcLen; i++) {
                pDest[i * 2] = 0x00;        // UCS2 high byte
                pDest[i * 2 + 1] = pSrc[i]; // UCS2 low byte
            }
        }
        break;
    }

    // Update destination length
    *pDestLen = uRequiredLen;

    return true;
}

bool LIB_SMS_ConvAlphaToCharSet(uint8_t uDCS, uint8_t* pSrc, uint16_t uSrcLen,
                                LIB_SMS_CharSetEnum eCharSet, uint8_t* pDest, uint16_t* pDestLen) {
    uint16_t i = 0;
    uint8_t uAlpha = 0;
    uint16_t uRequiredLen = 0;

    // Check input parameters
    if ((NULL == pSrc) || (0 == uSrcLen) || (NULL == pDest) || (NULL == pDestLen)) {
        return false;
    }

    // Check if character set is supported
    if (!LIB_SMS_IS_SUPPORT_CHARSET(eCharSet)) {
        return false;
    }

    // Get alphabet type from DCS
    LIB_SMS_DecodeDCS(uDCS, NULL, &uAlpha, NULL, NULL);

    // Calculate required destination buffer length based on alphabet and character set
    switch (uAlpha) {
    case LIB_SMS_DCS_ALPHA_DEFAULT:
        if (LIB_SMS_CHARSET_GSM == eCharSet || LIB_SMS_CHARSET_IRA == eCharSet ||
            LIB_SMS_CHARSET_8859_1 == eCharSet) {
            uRequiredLen = uSrcLen;
        } else {
            return false; // Default alphabet can only be converted to GSM/IRA/8859-1
        }
        break;

    case LIB_SMS_DCS_ALPHA_8BIT_DATA:
        if (LIB_SMS_CHARSET_GSM == eCharSet || LIB_SMS_CHARSET_HEX == eCharSet ||
            LIB_SMS_CHARSET_IRA == eCharSet || LIB_SMS_CHARSET_8859_1 == eCharSet) {
            uRequiredLen = uSrcLen;
        } else {
            return false; // 8-bit data can only be converted to GSM/HEX/IRA/8859-1
        }
        break;

    case LIB_SMS_DCS_ALPHA_UCS2:
        if (LIB_SMS_CHARSET_UCS2 == eCharSet) {
            uRequiredLen = uSrcLen;
        } else if (LIB_SMS_CHARSET_GSM == eCharSet || LIB_SMS_CHARSET_IRA == eCharSet ||
                   LIB_SMS_CHARSET_8859_1 == eCharSet) {
            if (0 != (uSrcLen % 2)) {
                return false; // UCS2 data length must be even
            }
            uRequiredLen = uSrcLen / 2; // Each UCS2 character takes 2 bytes
        } else {
            return false; // UCS2 can only be converted to UCS2/GSM/IRA/8859-1
        }
        break;

    default:
        return false;
    }

    // Check if destination buffer has enough space
    if (*pDestLen < uRequiredLen) {
        return false;
    }

    // Perform the conversion based on alphabet and character set
    switch (uAlpha) {
    case LIB_SMS_DCS_ALPHA_DEFAULT:
    case LIB_SMS_DCS_ALPHA_8BIT_DATA:
        // Direct copy for default or 8-bit data
        for (i = 0; i < uSrcLen; i++) {
            pDest[i] = pSrc[i];
        }
        break;

    case LIB_SMS_DCS_ALPHA_UCS2:
        if (LIB_SMS_CHARSET_UCS2 == eCharSet) {
            // Direct copy for UCS2 to UCS2
            for (i = 0; i < uSrcLen; i++) {
                pDest[i] = pSrc[i];
            }
        } else {
            // Convert UCS2 to single byte character sets (only for basic ASCII range)
            for (i = 0; i < uSrcLen; i += 2) {
                if (pSrc[i] == 0x00) // Only handle Basic Latin range (U+0000 to U+007F)
                {
                    pDest[i / 2] = pSrc[i + 1];
                } else {
                    return false; // Character outside basic Latin range
                }
            }
        }
        break;
    }

    // Update destination length
    *pDestLen = uRequiredLen;

    return true;
}

void LIB_SMS_DecodeDCS(uint8_t uDCS, uint8_t* pMsgType, uint8_t* pAlpha, uint8_t* pMsgClass,
                       uint8_t* pCompress) {
    uint8_t uCoding = 0;
    uint8_t uMsgType = 0;
    uint8_t uAlpha = 0;
    uint8_t uMsgClass = 0;
    uint8_t uCompress = 0;

    // Get coding group from high nibble
    uCoding = (uDCS >> 4) & 0x0F;

    // Decode based on coding group
    switch (uCoding) {
    case 0x00: // General Data Coding group
    case 0x02: // Message Marked for Automatic Deletion group
    case 0x03: // Message Waiting Indication group (Store Message)
        uMsgType = uCoding;
        uAlpha = ((uDCS >> 2) & 0x03);    // Bits 2-3
        uCompress = ((uDCS >> 5) & 0x01); // Bit 5
        if (uDCS & 0x10)                  // Bit 4 - Message Class present
        {
            uMsgClass = (uDCS & 0x03); // Bits 0-1
        } else {
            uMsgClass = LIB_SMS_DCS_MSG_CLASS_RESERVED;
        }
        break;

    case 0x04: // Message Waiting Indication group (Discard Message)
    case 0x05: // Message Waiting Indication group (Store Message, GSM 7 bit)
    case 0x06: // Message Waiting Indication group (Store Message, UCS2)
    case 0x07: // Message Waiting Indication group (Store Message)
        uMsgType = uCoding;
        uAlpha = (uCoding == 0x06) ? LIB_SMS_DCS_ALPHA_UCS2 : LIB_SMS_DCS_ALPHA_DEFAULT;
        uCompress = LIB_SMS_DCS_COMP_UNCOMPRESSED;
        uMsgClass = LIB_SMS_DCS_MSG_CLASS_RESERVED;
        break;

    case 0x0F: // Data coding/message class
        uMsgType = uCoding;
        uAlpha = ((uDCS >> 2) & 0x03); // Bits 2-3
        uCompress = LIB_SMS_DCS_COMP_UNCOMPRESSED;
        uMsgClass = (uDCS & 0x03); // Bits 0-1
        break;

    default: // Reserved coding groups
        uMsgType = uCoding;
        uAlpha = LIB_SMS_DCS_ALPHA_DEFAULT;
        uCompress = LIB_SMS_DCS_COMP_UNCOMPRESSED;
        uMsgClass = LIB_SMS_DCS_MSG_CLASS_RESERVED;
        break;
    }

    // Update output parameters if pointers are not NULL
    LIB_SMS_SET_POINTER_VAL(pMsgType, uMsgType);
    LIB_SMS_SET_POINTER_VAL(pAlpha, uAlpha);
    LIB_SMS_SET_POINTER_VAL(pMsgClass, uMsgClass);
    LIB_SMS_SET_POINTER_VAL(pCompress, uCompress);
}

bool LIB_SMS_DecodePDUStr(char* pPDUStr, uint16_t uPDUStrLen, LIB_SMS_PDUParamStruct* pParam) {
    uint16_t uOffset = 0;
    uint8_t uLen = 0;
    uint8_t uType = 0;
    uint8_t aOct[LIB_SMS_PDU_BUF_MAX_LEN] = {0};
    uint16_t uOctLen = sizeof(aOct);

    // Check input parameters
    if ((NULL == pPDUStr) || (0 == uPDUStrLen) || (NULL == pParam)) {
        return false;
    }

    // Convert PDU string to octets
    if (!LIB_SMS_ConvHexStrToHexOct(pPDUStr, uPDUStrLen, aOct, &uOctLen)) {
        return false;
    }

    // Get SMSC address length and type
    uLen = aOct[uOffset++];
    if (uLen > 0) {
        uType = aOct[uOffset++];
        (pParam->sSCA).uType = uType;

        // Convert SMSC number
        if (LIB_SMS_PHONE_NUMBER_TYPE_ALPHANUMERIC == (uType & 0x70)) {
            // Handle alphanumeric address
            uint16_t i;
            for (i = 0; i < (uLen - 1); i++) {
                (pParam->sSCA).aNumber[i] = aOct[uOffset++];
            }
            (pParam->sSCA).uLen = uLen - 1;
        } else {
            // Handle numeric address
            uint16_t i;
            uint8_t uNumLen = (uLen - 1) * 2;
            for (i = 0; i < uNumLen; i += 2) {
                (pParam->sSCA).aNumber[i] = ((aOct[uOffset] & 0x0F) + '0');
                if (i + 1 < uNumLen) {
                    (pParam->sSCA).aNumber[i + 1] = ((aOct[uOffset] >> 4) + '0');
                }
                uOffset++;
            }
            (pParam->sSCA).uLen = uNumLen;
        }
    }

    // Get First Octet
    pParam->uFO = aOct[uOffset++];

    // Process based on PDU type
    switch (LIB_SMS_GET_MSG_TYPE_IN_PDU_FO(pParam->uFO)) {
    case LIB_SMS_PDU_TYPE_DELIVER: {
        LIB_SMS_DeliverPDUParamStruct* pDeliver = &(pParam->sParam.sDeliverParam);

        // Get originator address
        uLen = aOct[uOffset++];
        uType = aOct[uOffset++];
        pDeliver->sOA.uType = uType;

        if (LIB_SMS_PHONE_NUMBER_TYPE_ALPHANUMERIC == (uType & 0x70)) {
            // Handle alphanumeric address
            uint16_t i;
            uint8_t uBytes = (uLen + 1) / 2;
            for (i = 0; i < uBytes; i++) {
                pDeliver->sOA.aNumber[i] = aOct[uOffset++];
            }
            pDeliver->sOA.uLen = uLen;
        } else {
            // Handle numeric address
            uint16_t i;
            for (i = 0; i < uLen; i += 2) {
                pDeliver->sOA.aNumber[i] = ((aOct[uOffset] & 0x0F) + '0');
                if (i + 1 < uLen) {
                    pDeliver->sOA.aNumber[i + 1] = ((aOct[uOffset] >> 4) + '0');
                }
                uOffset++;
            }
            pDeliver->sOA.uLen = uLen;
        }

        // Get PID
        pDeliver->uPID = aOct[uOffset++];

        // Get DCS
        pDeliver->uDCS = aOct[uOffset++];

        // Get SCTS (7 octets)
        uint8_t i;
        uint8_t* pSCTS = (uint8_t*) &(pDeliver->sSCTS);
        for (i = 0; i < 7; i++) {
            pSCTS[i] = ((aOct[uOffset] & 0x0F) * 10) + (aOct[uOffset] >> 4);
            uOffset++;
        }

        // Get user data
        uLen = aOct[uOffset++];
        if (LIB_SMS_GET_UDHI_IN_PDU(pParam->uFO)) {
            // Handle UDH (User Data Header)
            uint8_t uUDHLen = aOct[uOffset++];
            if (uUDHLen >= 5) // Minimum UDH length for concatenated messages
            {
                if (0x00 == aOct[uOffset] || 0x08 == aOct[uOffset]) // IEI for concatenated messages
                {
                    pDeliver->sConSMSParam.uMsgRef = aOct[uOffset + 1];
                    pDeliver->sConSMSParam.uMsgTot = aOct[uOffset + 2];
                    pDeliver->sConSMSParam.uMsgSeg = aOct[uOffset + 3];
                }
            }
            uOffset += uUDHLen;
            uLen -= (uUDHLen + 1);
        }

        // Copy user data
        if (uLen > 0) {
            uint16_t i;
            for (i = 0; i < uLen; i++) {
                pDeliver->sUserData.aUserData[i] = aOct[uOffset++];
            }
            pDeliver->sUserData.uLen = uLen;
        }
        break;
    }

    case LIB_SMS_PDU_TYPE_SUBMIT: {
        LIB_SMS_SubmitPDUParamStruct* pSubmit = &(pParam->sParam.sSubmitParam);

        // Get Message Reference (MR)
        uOffset++; // Skip MR as it's not used in decoding

        // Get destination address
        uLen = aOct[uOffset++];
        uType = aOct[uOffset++];
        pSubmit->sDA.uType = uType;

        if (LIB_SMS_PHONE_NUMBER_TYPE_ALPHANUMERIC == (uType & 0x70)) {
            // Handle alphanumeric address
            uint16_t i;
            uint8_t uBytes = (uLen + 1) / 2;
            for (i = 0; i < uBytes; i++) {
                pSubmit->sDA.aNumber[i] = aOct[uOffset++];
            }
            pSubmit->sDA.uLen = uLen;
        } else {
            // Handle numeric address
            uint16_t i;
            for (i = 0; i < uLen; i += 2) {
                pSubmit->sDA.aNumber[i] = ((aOct[uOffset] & 0x0F) + '0');
                if (i + 1 < uLen) {
                    pSubmit->sDA.aNumber[i + 1] = ((aOct[uOffset] >> 4) + '0');
                }
                uOffset++;
            }
            pSubmit->sDA.uLen = uLen;
        }

        // Get PID
        pSubmit->uPID = aOct[uOffset++];

        // Get DCS
        pSubmit->uDCS = aOct[uOffset++];

        // Get VP (Validity Period)
        switch (LIB_SMS_GET_VPF_IN_SUBMIT_PDU_FO(pParam->uFO)) {
        case LIB_SMS_VPF_TYPE_RELATIVE:
            pSubmit->sVP.uRelative = aOct[uOffset++];
            break;

        case LIB_SMS_VPF_TYPE_ABSOLUTE: {
            uint8_t i;
            uint8_t* pVP = (uint8_t*) &(pSubmit->sVP.sAbsolute);
            for (i = 0; i < 7; i++) {
                pVP[i] = ((aOct[uOffset] & 0x0F) * 10) + (aOct[uOffset] >> 4);
                uOffset++;
            }
            break;
        }

        case LIB_SMS_VPF_TYPE_NOT_PRESENT:
        default:
            break;
        }

        // Get user data
        uLen = aOct[uOffset++];
        if (LIB_SMS_GET_UDHI_IN_PDU(pParam->uFO)) {
            // Handle UDH (User Data Header)
            uint8_t uUDHLen = aOct[uOffset++];
            if (uUDHLen >= 5) // Minimum UDH length for concatenated messages
            {
                if (0x00 == aOct[uOffset] || 0x08 == aOct[uOffset]) // IEI for concatenated messages
                {
                    pSubmit->sConSMSParam.uMsgRef = aOct[uOffset + 1];
                    pSubmit->sConSMSParam.uMsgTot = aOct[uOffset + 2];
                    pSubmit->sConSMSParam.uMsgSeg = aOct[uOffset + 3];
                }
            }
            uOffset += uUDHLen;
            uLen -= (uUDHLen + 1);
        }

        // Copy user data
        if (uLen > 0) {
            uint16_t i;
            for (i = 0; i < uLen; i++) {
                pSubmit->sUserData.aUserData[i] = aOct[uOffset++];
            }
            pSubmit->sUserData.uLen = uLen;
        }
        break;
    }

    default:
        return false;
    }

    return true;
}

bool LIB_SMS_EncodeSubmitPDU(LIB_SMS_PDUParamStruct* pParam, LIB_SMS_PDUInfoStruct* pInfo) {
    if ((NULL == pParam) || (NULL == pInfo)) {
        return false;
    }

    uint8_t* pPDUOct = pInfo->aPDUOct;
    uint32_t uPDULen = 0;

    // 1. Encode First Octet (FO)
    pPDUOct[uPDULen++] = pParam->uFO;

    // 2. Encode Message Reference (MR)
    pPDUOct[uPDULen++] = pParam->sParam.sSubmitParam.sConSMSParam.uMsgRef;

    // 3. Encode Destination Address (DA)
    pPDUOct[uPDULen++] = pParam->sParam.sSubmitParam.sDA.uLen;
    pPDUOct[uPDULen++] = pParam->sParam.sSubmitParam.sDA.uType;
    memcpy(&pPDUOct[uPDULen], pParam->sParam.sSubmitParam.sDA.aNumber,
           pParam->sParam.sSubmitParam.sDA.uLen);
    uPDULen += pParam->sParam.sSubmitParam.sDA.uLen;

    // 4. Encode Protocol Identifier (PID)
    pPDUOct[uPDULen++] = pParam->sParam.sSubmitParam.uPID;

    // 5. Encode Data Coding Scheme (DCS)
    pPDUOct[uPDULen++] = pParam->sParam.sSubmitParam.uDCS;

    // 6. Encode Validity Period (VP)
    if (LIB_SMS_VPF_TYPE_RELATIVE == LIB_SMS_GET_VPF_IN_SUBMIT_PDU_FO(pParam->uFO)) {
        pPDUOct[uPDULen++] = pParam->sParam.sSubmitParam.sVP.uRelative;
    } else if (LIB_SMS_VPF_TYPE_ABSOLUTE == LIB_SMS_GET_VPF_IN_SUBMIT_PDU_FO(pParam->uFO)) {
        // Encode absolute VP (7 octets)
        uint8_t i;
        uint8_t* pVP = (uint8_t*) &(pParam->sParam.sSubmitParam.sVP.sAbsolute);
        for (i = 0; i < 7; i++) {
            pPDUOct[uPDULen++] = pVP[i];
        }
    }

    // 7. Encode User Data Length (UDL)
    pPDUOct[uPDULen++] = pParam->sParam.sSubmitParam.sUserData.uLen;

    // 8. Encode User Data (UD)
    memcpy(&pPDUOct[uPDULen], pParam->sParam.sSubmitParam.sUserData.aUserData,
           pParam->sParam.sSubmitParam.sUserData.uLen);
    uPDULen += pParam->sParam.sSubmitParam.sUserData.uLen;

    // Set the total PDU length
    pInfo->uLen = uPDULen;

    return true;
}
