#ifndef __RIL_UTIL_H__
#define __RIL_UTIL_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    CHAR_0 = '0',
    CHAR_9 = '9',
    CHAR_A = 'A',
    CHAR_F = 'F',
    END_OF_STR = '\0'
}Enum_Char;

#define IS_NUMBER(alpha_char)   (((alpha_char >= CHAR_0) && (alpha_char <= CHAR_9)))

extern int32_t  Ql_StrPrefixMatch(const char* str, const char* prefix);
extern bool Ql_HexStrToInt(uint8_t* str, uint32_t* val);
extern char* Ql_StrToUpper(char* str);
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
extern char* Ql_RIL_FindString(char* line, uint32_t len, char* str);

#endif
