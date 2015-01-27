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

static U16 g_malloc_count = 0;

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
		lumDebug("**** lum_malloc g_malloc_count=%d\n", g_malloc_count);
	}
	return pData;
}


void USER_FUNC lum_free(void* pData)
{
	
	os_free(pData);
	g_malloc_count--;
	lumDebug("**** lum_free g_malloc_count=%d\n", g_malloc_count);
}


