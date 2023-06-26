#ifndef __RIL_UTIL_H__
#define __RIL_UTIL_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    CHAR_0 = '0',
    CHAR_9 = '9',
    CHAR_A = 'A',
    CHAR_F = 'F',
    END_OF_STR = '\0'
} Enum_Char;

#define IS_NUMBER(alpha_char) (((alpha_char >= CHAR_0) && (alpha_char <= CHAR_9)))

int32_t Ql_StrPrefixMatch(const char* str, const char* prefix);
bool Ql_HexStrToInt(uint8_t* str, uint32_t* val);
char* Ql_StrToUpper(char* str);

#endif
