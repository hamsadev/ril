#include "ril_util.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

int32_t Ql_StrPrefixMatch(const char* str, const char* prefix) {
    for (; *str != '\0' && *prefix != '\0'; str++, prefix++) {
        if (*str != *prefix) {
            return 0;
        }
    }
    return *prefix == '\0';
}

char* Ql_StrToUpper(char* str) {
    char* pCh = str;
    if (!str) {
        return NULL;
    }
    for (; *pCh != '\0'; pCh++) {
        if (((*pCh) >= 'a') && ((*pCh) <= 'z')) {
            *pCh = toupper(*pCh);
        }
    }
    return str;
}

bool Ql_HexStrToInt(uint8_t* str, uint32_t* val) {
    uint16_t i = 0;
    uint32_t temp = 0;

    // ASSERT((str != NULL) && (val != NULL));
    if (NULL == str || NULL == val) {
        return false;
    }
    Ql_StrToUpper((char*) str);

    while (str[i] != '\0') {
        if (IS_NUMBER(str[i])) {
            temp = (temp << 4) + (str[i] - CHAR_0);
        } else if ((str[i] >= CHAR_A) && (str[i] <= CHAR_F)) {
            temp = (temp << 4) + ((str[i] - CHAR_A) + 10);
        } else {
            return false;
        }
        i++;
    }
    *val = temp;
    return true;
}

/******************************************************************************
* Function:     Ql_RIL_FindString
*
* Description:
*                This function is used to match string within a specified length.
*                This function is very much like strstr.
*
* Parameters:
*                line:
*                    [in]The address of the string.
*                len:
*                    [in]The length of the string.
*                str:
*                    [in]The specified item which you want to look for in the string.
*
* Return:
                The function returns a pointer to the located string,
                or a  null  pointer  if  the specified string is not found.
******************************************************************************/
char* Ql_RIL_FindString(char* line, uint32_t len, char* str) {
    int32_t i;
    int32_t slen;
    char* p;

    if ((NULL == line) || (NULL == str))
        return NULL;

    slen = strlen(str);
    if (slen > len) {
        return NULL;
    }

    p = line;
    for (i = 0; i < len - slen + 1; i++) {
        if (0 == strncmp(p, str, slen)) {
            return p;
        } else {
            p++;
        }
    }
    return NULL;
}

uint32_t Ql_GenHash(char* strSrc, uint32_t len) {
    uint32_t h, v;
    uint32_t i;
    for (h = 0, i = 0; i < len; i++) {
        h = (uint32_t) (5527 * h + 7 * strSrc[i]);
        v = h & 0x0000ffff;
        h ^= v * v;
    }
    return h;
}
