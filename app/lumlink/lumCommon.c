/*
******************************
*Company:Lumitek
*Data:2015-01-26
*Author:Meiyusong
******************************
*/


#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"

#include "user_interface.h"
#include "lumlink/lumCommon.h"
#include "lumlink/lumPlatform.h"


static U16 g_malloc_count = 0;
static GLOBAL_CONFIG_DATA g_deviceConfig;


BOOL USER_FUNC lum_checkRevcSocket(U8* recvData, U8 RecvLen)
{
	SOCKET_HEADER_OPEN* pOpenData;
	U8 macAddr[DEVICE_MAC_LEN];
	U8 noMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	pOpenData = (SOCKET_HEADER_OPEN*)recvData;
	lum_getDeviceMac(macAddr);
	if((pOpenData->dataLen + SOCKET_OPEN_DATA_LEN) != RecvLen) //check Data length
	{
		return FALSE;
	}
	else if(os_memcmp(macAddr, pOpenData->mac, DEVICE_MAC_LEN) != 0)
	{
		if(os_memcmp(noMac, pOpenData->mac, DEVICE_MAC_LEN) != 0)
		{
			return FALSE;
		}
	}
	return TRUE;
}


void USER_FUNC lum_showHexData(U8* data, U8 dataLen)
{
	U16 i;
	U8 strData[512];
	U16 index = 0;

	memset(strData, 0, sizeof(strData));

	for(i=0; i<dataLen; i++)
	{
		os_sprintf(strData+os_strlen(strData), "%02X ", data[i]);
	}
	lumDebug("%d data=%s\n", dataLen, strData);
}


void USER_FUNC lum_getDeviceMac(U8* macAddr)
{
	wifi_get_macaddr(STATION_IF, macAddr);
}


U8* USER_FUNC lum_malloc(U32 mllocSize)
{
	U8* pData;
	
	pData = (U8*)os_malloc(mllocSize);
	if(pData != NULL)
	{
		g_malloc_count++;
		//lumDebug("**** lum_malloc g_malloc_count=%d\n", g_malloc_count);
	}
	return pData;
}


void USER_FUNC lum_free(void* pData)
{
	
	os_free(pData);
	g_malloc_count--;
	lumDebug("**** lum_free g_malloc_count=%d\n", g_malloc_count);
}


void USER_FUNC lum_setDeviceName(DEVICE_NAME_DATA* nameData)
{
	os_memcpy(&g_deviceConfig.deviceConfigData.deviceName, nameData, sizeof(DEVICE_NAME_DATA));
	lum_SaveConfigData(&g_deviceConfig.deviceConfigData);
}


DEVICE_NAME_DATA* USER_FUNC lum_getDeviceName(void)
{
	return &g_deviceConfig.deviceConfigData.deviceName;
}


static void USER_FUNC lum_initDeviceNameData(void)
{
	U8 defaultNameLen = os_strlen(DEFAULT_MODUAL_NAME);


	//Device name init
	os_memcpy(g_deviceConfig.deviceConfigData.deviceName.nameData, DEFAULT_MODUAL_NAME, defaultNameLen);
	g_deviceConfig.deviceConfigData.deviceName.nameLen = defaultNameLen;

}


void USER_FUNC lum_globalConfigDataInit(void)
{
	os_memset(&g_deviceConfig, 0, sizeof(GLOBAL_CONFIG_DATA));
	lum_LoadConfigData(&g_deviceConfig.deviceConfigData);
	if(g_deviceConfig.deviceConfigData.lumitekFlag != LUMITEK_SW_FLAG)
	{
		//Device  first power on flag
		os_memset(&g_deviceConfig, 0, sizeof(GLOBAL_CONFIG_DATA));
		g_deviceConfig.deviceConfigData.lumitekFlag = LUMITEK_SW_FLAG;

		lum_initDeviceNameData();

	}
	lum_SaveConfigData(&g_deviceConfig.deviceConfigData);
}



