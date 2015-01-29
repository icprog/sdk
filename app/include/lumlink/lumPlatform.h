/*
******************************
*Company:Lumlink
*Data:2015-01-26
*Author:Meiyusong
******************************
*/

#ifndef __LUMLINK_PLATFORM__H__
#define __LUMLINK_PLATFORM__H__

#include "lumlink/lumCommon.h"


#define USER_CONFIG_DATA_FLASH_SECTOR	0x3C


void lum_platformInit(void);
void lum_SaveConfigData(DEVICE_CONFIG_DATA* configData);
void lum_LoadConfigData(DEVICE_CONFIG_DATA* configData);

#endif
