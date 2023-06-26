#include "ril_dateTime.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define RIL_DEFAULT_TIMEOUT 300

// Response handler for AT+CCLK (Get Date and Time)
static int32_t ATResponse_CCLK_Handler(char* line, uint32_t len, void* userdata) {
    if (strncmp(line, "+CCLK:", 6) == 0) {
        RIL_DateTime* dateTime = (RIL_DateTime*) userdata;
        // EC200U format: +CCLK: <YYYY/MM/DD,hh:mm:ss±zz>
        sscanf(line,
               "+CCLK: \"%" SCNu8 "/%" SCNu8 "/%" SCNu8 ",%" SCNu8 ":%" SCNu8 ":%" SCNu8 "%" SCNd8
               "\"",
               &dateTime->year, &dateTime->month, &dateTime->day, &dateTime->hour,
               &dateTime->minute, &dateTime->second, &dateTime->localTimezone);
        return RIL_ATRSP_CONTINUE;
    } else if (strncmp(line, "OK", 2) == 0) {
        return RIL_ATRSP_SUCCESS;
    }
    return RIL_ATRSP_FAILED;
}

// Response handler for AT+CTZU (Time Zone Update)
static int32_t ATResponse_CTZU_Handler(char* line, uint32_t len, void* userdata) {
    if (strncmp(line, "+CTZU:", 6) == 0) {
        uint8_t* enable = (uint8_t*) userdata;
        // Format: +CTZU: <n>
        sscanf(line, "+CTZU: %" SCNu8, enable);
        return RIL_ATRSP_CONTINUE;
    } else if (strncmp(line, "OK", 2) == 0) {
        return RIL_ATRSP_SUCCESS;
    }
    return RIL_ATRSP_FAILED;
}

// Response handler for AT+CTZR (Time Zone Report)
static int32_t ATResponse_CTZR_Handler(char* line, uint32_t len, void* userdata) {
    if (strncmp(line, "+CTZR:", 6) == 0) {
        RIL_TimeZone* timeZone = (RIL_TimeZone*) userdata;
        // Format: +CTZR: <timezone>,<dst>
        sscanf(line, "+CTZR: %" SCNu8 ",%" SCNu8, &timeZone->timezone, &timeZone->dst);
        return RIL_ATRSP_CONTINUE;
    } else if (strncmp(line, "OK", 2) == 0) {
        return RIL_ATRSP_SUCCESS;
    }
    return RIL_ATRSP_FAILED;
}

// Generic OK/ERROR response handler
static int32_t ATResponse_Generic_Handler(char* line, uint32_t len, void* userdata) {
    (void) userdata; // Unused parameter
    if (strncmp(line, "OK", 2) == 0) {
        return RIL_ATRSP_SUCCESS;
    }
    return RIL_ATRSP_CONTINUE;
}

RIL_ATSndError RIL_DateTime_Get(RIL_DateTime* dateTime) {
    static const char cmd[] = "AT+CCLK?";
    RIL_ATSndError ret = RIL_AT_SUCCESS;
    ret =
        RIL_SendATCmd(cmd, sizeof(cmd) - 1, ATResponse_CCLK_Handler, dateTime, RIL_DEFAULT_TIMEOUT);
    return ret;
}

RIL_ATSndError RIL_DateTime_Set(RIL_DateTime* dateTime) {
    char cmd[64];
    RIL_ATSndError ret = RIL_AT_SUCCESS;

    // Format: AT+CCLK="YY/MM/DD,HH:MM:SS±ZZ"
    snprintf(cmd, sizeof(cmd),
             "AT+CCLK=\"%02" PRIu8 "/%02" PRIu8 "/%02" PRIu8 ",%02" PRIu8 ":%02" PRIu8 ":%02" PRIu8
             "%+02" PRId8 "\"",
             dateTime->year, dateTime->month, dateTime->day, dateTime->hour, dateTime->minute,
             dateTime->second, dateTime->localTimezone);

    ret = RIL_SendATCmd(cmd, strlen(cmd), ATResponse_Generic_Handler, NULL, RIL_DEFAULT_TIMEOUT);
    return ret;
}

RIL_ATSndError RIL_TimeZoneUpdate_Set(uint8_t enable) {
    char cmd[16];
    // Format: AT+CTZU=<n> where n=0 (disable) or n=1 (enable)
    snprintf(cmd, sizeof(cmd), "AT+CTZU=%" PRIu8, enable);

    RIL_ATSndError ret = RIL_AT_SUCCESS;
    ret = RIL_SendATCmd(cmd, strlen(cmd), ATResponse_Generic_Handler, NULL, RIL_DEFAULT_TIMEOUT);
    return ret;
}

RIL_ATSndError RIL_TimeZoneUpdate_Get(uint8_t* enable) {
    RIL_ATSndError ret = RIL_AT_SUCCESS;
    static const char cmd[] = "AT+CTZU?";
    ret = RIL_SendATCmd(cmd, sizeof(cmd) - 1, ATResponse_CTZU_Handler, enable, RIL_DEFAULT_TIMEOUT);
    return ret;
}

RIL_ATSndError RIL_TimeZone_Get(RIL_TimeZone* timeZone) {
    RIL_ATSndError ret = RIL_AT_SUCCESS;
    static const char cmd[] = "AT+CTZR?";

    ret =
        RIL_SendATCmd(cmd, sizeof(cmd) - 1, ATResponse_CTZR_Handler, timeZone, RIL_DEFAULT_TIMEOUT);
    return ret;
}