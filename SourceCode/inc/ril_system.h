#ifndef _RIL_SYSTEM_H_
#define _RIL_SYSTEM_H_

#include "ril.h"
#include <stdint.h>


typedef enum {
    RIL_SYS_POWER_OFF_MODE_NORMAL,
    RIL_SYS_POWER_OFF_MODE_IMMEDIATE,
} RIL_SYS_PowerOffMode;

/**
 * @brief This function queries the battery balance, and the battery voltage.
 * @param capacity [out] battery balance, a percent, ranges from 1 to 100.
 * @param voltage [out] battery voltage, unit in mV
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_SYS_GetPowerSupply(uint32_t* capacity, uint32_t* voltage);

/**
 * @brief Retrieves the IMEI (International Mobile Equipment Identity) number.
 * @param imei [out] A string where the IMEI value will be stored.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_SYS_GetIMEI(char* imei);

/**
 * @brief Retrieves the firmware version of the module.
 * @param firmware [out] A string where the firmware version will be stored.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_SYS_GetFirmwareVersion(char* firmware);

/**
 * @brief Retrieves the manufacturer name.
 * @param manufacturer [out] A string where the manufacturer name will be stored.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_SYS_GetManufacturer(char* manufacturer);

/**
 * @brief Retrieves the module model.
 * @param model [out] A string where the module model will be stored.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_SYS_GetModel(char* model);

/**
 * @brief Retrieves the serial number or hardware UID if available.
 * @param serial [out] A string where the serial number or hardware UID will be stored.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_SYS_GetSerialNumber(char* serial);

/**
 * @brief Retrieves the SIM card CCID (Integrated Circuit Card Identifier).
 * @param ccid [out] A string where the SIM card CCID will be stored.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_SYS_GetCCID(char* ccid);

/**
 * @brief Retrieves the software core version.
 * @param coreVersion [out] A string where the core version will be stored.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_SYS_GetCoreVersion(char* coreVersion);

/**
 * @brief Retrieves a complete system report with all available information.
 * @param report [out] A formatted string containing all system details.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_SYS_GetFullSystemReport(char* report);

/**
 * @brief Powers off the module.
 * @param mode [in] The power off mode.
 * @return RIL_ATSndError
 */
RIL_ATSndError RIL_SYS_PowerOff(RIL_SYS_PowerOffMode mode);

#endif //__RIL_SYSTEM_H__
