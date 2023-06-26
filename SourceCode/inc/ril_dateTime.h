#ifndef _RIL_DATE_TIME_H_
#define _RIL_DATE_TIME_H_

#include "ril.h"

typedef struct {
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    int8_t localTimezone;
} RIL_DateTime;

typedef struct {
    int8_t timezone;
    uint8_t dst;  // Daylight Saving Time: 0=disabled, 1=enabled
} RIL_TimeZone;

/**
 * @brief Get the date and time from the modem
 * 
 * @param dateTime Pointer to the RIL_DateTime structure to store the date and time
 * @return RIL_ATSndError Error code
 */
RIL_ATSndError RIL_DateTime_Get(RIL_DateTime* dateTime);

/**
 * @brief Set the date and time on the modem
 * 
 * @param dateTime Pointer to the RIL_DateTime structure containing the date and time to set
 * @return RIL_ATSndError Error code
 */
RIL_ATSndError RIL_DateTime_Set(RIL_DateTime* dateTime);

/**
 * @brief Enable/disable automatic time zone update
 * 
 * @param enable 1 to enable, 0 to disable
 * @return RIL_ATSndError Error code
 */
RIL_ATSndError RIL_TimeZoneUpdate_Set(uint8_t enable);

/**
 * @brief Get the current time zone update setting
 * 
 * @param enable Pointer to store the current setting (1=enabled, 0=disabled)
 * @return RIL_ATSndError Error code
 */
RIL_ATSndError RIL_TimeZoneUpdate_Get(uint8_t* enable);

/**
 * @brief Get the current time zone information
 * 
 * @param timeZone Pointer to the RIL_TimeZone structure to store the time zone info
 * @return RIL_ATSndError Error code
 */
RIL_ATSndError RIL_TimeZone_Get(RIL_TimeZone* timeZone);

#endif //_RIL_DATE_TIME_H_