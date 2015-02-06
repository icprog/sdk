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
#include "lumlink/lumTimeDate.h"


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
		lumDebug("Recv lenth error! dataLen=%d RecvLen=%d\n", pOpenData->dataLen, RecvLen);
		return FALSE;
	}
	else if(os_memcmp(macAddr, pOpenData->mac, DEVICE_MAC_LEN) != 0)
	{
		if(os_memcmp(noMac, pOpenData->mac, DEVICE_MAC_LEN) != 0)
		{
			lumDebug("Recv mac error mac=%X:%X:%X:%X:%X:%X\n", pOpenData->mac[0], pOpenData->mac[1],
				pOpenData->mac[2], pOpenData->mac[3],pOpenData->mac[4], pOpenData->mac[5]);
			return FALSE;
		}
	}
	return TRUE;
}


void USER_FUNC lum_showHexData(S8* header, U8* data, U8 dataLen)
{
	U16 i;
	U8 strData[1024];
	U16 index = 0;

	os_memset(strData, 0, sizeof(strData));

	for(i=0; i<dataLen; i++)
	{
		if(i == 10)
		{
			os_sprintf(strData+os_strlen(strData), "¡¾%02X ", data[i]);
		}
		else if(i == 11)
		{
			os_sprintf(strData+os_strlen(strData), "%02X¡¿ ", data[i]);
		}
		else
		{
			os_sprintf(strData+os_strlen(strData), "%02X ", data[i]);
		}
	}
	lumDebug("%s %s\n", header, strData);
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
		g_deviceConfig.globalData.mallocCount++;
		//lumDebug("**** lum_malloc g_malloc_count=%d\n", g_deviceConfig.globalData.mallocCount);
	}
	return pData;
}


void USER_FUNC lum_free(void* pData)
{
	
	os_free(pData);
	g_deviceConfig.globalData.mallocCount--;
	//lumDebug("**** lum_free g_malloc_count=%d\n", g_deviceConfig.globalData.mallocCount);
}

static void USER_FUNC lum_writeConfigData(DEVICE_CONFIG_DATA* configData)
{
	U32 configLen;

	configLen = sizeof(DEVICE_CONFIG_DATA);
	if(configLen > SPI_FLASH_SEC_SIZE)
	{
		lumError("Save Data too long! \n");
		return;
	}
	spi_flash_erase_sector(USER_CONFIG_DATA_FLASH_SECTOR);
	spi_flash_write(USER_CONFIG_DATA_FLASH_SECTOR*SPI_FLASH_SEC_SIZE, (U32*)configData, configLen);
}


static void USER_FUNC lum_readConfigData(DEVICE_CONFIG_DATA* configData)
{
	U32 configLen;

	configLen = sizeof(DEVICE_CONFIG_DATA);
	if(configLen > SPI_FLASH_SEC_SIZE)
	{
		lumError("Read Data too long! \n");
		return;
	}
	spi_flash_read(USER_CONFIG_DATA_FLASH_SECTOR*SPI_FLASH_SEC_SIZE, (U32*)configData, configLen);
}


static void USER_FUNC lum_SaveConfigData(void)
{
	lum_writeConfigData(&g_deviceConfig.deviceConfigData);
}


static void USER_FUNC lum_loadConfigData(void)
{
	lum_readConfigData(&g_deviceConfig.deviceConfigData);
}


U8 USER_FUNC lum_getDeviceLockStatus(void)
{
	return g_deviceConfig.deviceConfigData.bLocked;
}


void USER_FUNC lum_setDeviceLockStatus(U8 lockStatus)
{
	g_deviceConfig.deviceConfigData.bLocked = lockStatus;
	lum_SaveConfigData();
}


void USER_FUNC lum_setDeviceName(DEVICE_NAME_DATA* nameData)
{
	os_memcpy(&g_deviceConfig.deviceConfigData.deviceName, nameData, sizeof(DEVICE_NAME_DATA));
	lum_SaveConfigData();
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


U16 USER_FUNC lum_getSocketSn(BOOL needIncrease)
{
	if(needIncrease)
	{
		g_deviceConfig.globalData.socketSn++;
		if(g_deviceConfig.globalData.socketSn >= INVALID_SN_NUM)
		{
			g_deviceConfig.globalData.socketSn = 0;
		}
	}
	return g_deviceConfig.globalData.socketSn;
}


void USER_FUNC lum_setServerAddr(SOCKET_ADDR* pSocketAddr)
{
	os_memcpy(&g_deviceConfig.globalData.tcpServerAddr, pSocketAddr, sizeof(SOCKET_ADDR));
}


void USER_FUNC lum_getServerAddr(SOCKET_ADDR* pSocketAddr)
{
	os_memcpy(pSocketAddr, &g_deviceConfig.globalData.tcpServerAddr, sizeof(SOCKET_ADDR));
}


void USER_FUNC lum_clearServerAesKey(void)
{
	g_deviceConfig.globalData.keyData.serverAesKeyValid = FALSE;
	os_memset(g_deviceConfig.globalData.keyData.serverKey, 0, AES_KEY_LEN);
}


void USER_FUNC lum_setServerAesKey(U8* serverKey)
{
	os_memcpy(g_deviceConfig.globalData.keyData.serverKey, serverKey, AES_KEY_LEN);
	g_deviceConfig.globalData.keyData.serverAesKeyValid = TRUE;
}


U8* USER_FUNC lum_getServerAesKey(U8* serverKey)
{
	U8* pAesKey;


	if(g_deviceConfig.globalData.keyData.serverAesKeyValid)
	{
		pAesKey = g_deviceConfig.globalData.keyData.serverKey;
	}
	else
	{
		pAesKey = DEFAULT_AES_KEY;
	}
	if(serverKey != NULL)
	{
		os_memcpy(serverKey, g_deviceConfig.globalData.keyData.serverKey, AES_KEY_LEN);
	}
	return pAesKey;
}


void USER_FUNC lum_setAlarmData(ALARM_DATA_INFO* alarmData, U8 index)
{
	if(index >= MAX_ALARM_COUNT)
	{
		return;
	}

	os_memcpy(&g_deviceConfig.deviceConfigData.alarmData[index], alarmData, sizeof(ALARM_DATA_INFO));
	lum_SaveConfigData();

	lumDebug("AlarmData index=%d m=%d T=%d W=%d T=%d F=%d S=%d Sun=%d active=%d startHour=%d, startMinute=%d stopHour=%d stopMinute=%d size=%d\n",
			 index,
	         g_deviceConfig.deviceConfigData.alarmData[index].repeatData.monday,
	         g_deviceConfig.deviceConfigData.alarmData[index].repeatData.tuesday,
	         g_deviceConfig.deviceConfigData.alarmData[index].repeatData.wednesday,
	         g_deviceConfig.deviceConfigData.alarmData[index].repeatData.thursday,
	         g_deviceConfig.deviceConfigData.alarmData[index].repeatData.firday,
	         g_deviceConfig.deviceConfigData.alarmData[index].repeatData.saturday,
	         g_deviceConfig.deviceConfigData.alarmData[index].repeatData.sunday,
	         g_deviceConfig.deviceConfigData.alarmData[index].repeatData.bActive,
	         g_deviceConfig.deviceConfigData.alarmData[index].startHour,
	         g_deviceConfig.deviceConfigData.alarmData[index].startMinute,
	         g_deviceConfig.deviceConfigData.alarmData[index].stopHour,
	         g_deviceConfig.deviceConfigData.alarmData[index].stopMinute,
	         sizeof(ALARM_DATA_INFO));
}


void USER_FUNC lum_deleteAlarmData(U8 index, BOOL needSave)
{
	if(index >= MAX_ALARM_COUNT)
	{
		return;
	}
	g_deviceConfig.deviceConfigData.alarmData[index].repeatData.bActive = (U8)EVENT_INCATIVE;
	g_deviceConfig.deviceConfigData.alarmData[index].startHour= 0xFF;
	g_deviceConfig.deviceConfigData.alarmData[index].startMinute= 0xFF;
	g_deviceConfig.deviceConfigData.alarmData[index].stopHour= 0xFF;
	g_deviceConfig.deviceConfigData.alarmData[index].stopMinute= 0xFF;
	g_deviceConfig.deviceConfigData.alarmData[index].reserved = 0;

	if(needSave)
	{
		lum_SaveConfigData();
	}
}



ALARM_DATA_INFO* USER_FUNC lum_getAlarmData(U8 index)
{
	if(index >= MAX_ALARM_COUNT)
	{
		return NULL;
	}
	else
	{
		return &g_deviceConfig.deviceConfigData.alarmData[index];
	}
}


static void USER_FUNC lum_initAlarmData(void)
{
	U8 i;


	for(i=0; i<MAX_ALARM_COUNT; i++)
	{
		lum_deleteAlarmData(i, FALSE);
	}
}



void USER_FUNC lum_deleteAbsenceData(U8 index, BOOL needSave)
{
	if(index >= MAX_ABSENCE_COUNT)
	{
		return;
	}
	os_memset(&g_deviceConfig.deviceConfigData.absenceData[index], 0, sizeof(ASBENCE_DATA_INFO));
	g_deviceConfig.deviceConfigData.absenceData[index].startHour = 0xFF;

	if(needSave)
	{
		//checkAbsenceTimerAfterChange(index);
		lum_SaveConfigData();
	}
}


static void USER_FUNC lum_initAbsenceData(void)
{
	U8 i;


	os_memset(&g_deviceConfig.deviceConfigData.absenceData, 0, sizeof(ASBENCE_DATA_INFO)*MAX_ABSENCE_COUNT);
	for(i=0; i<MAX_ABSENCE_COUNT; i++)
	{
		lum_deleteAbsenceData(i, FALSE);
	}
}



void USER_FUNC lum_setAbsenceData(ASBENCE_DATA_INFO* absenceData, U8 index)
{
	if(index >= MAX_ABSENCE_COUNT)
	{
		return;
	}

	os_memcpy(&g_deviceConfig.deviceConfigData.absenceData[index], absenceData, sizeof(ASBENCE_DATA_INFO));
	lum_SaveConfigData();
	//checkAbsenceTimerAfterChange(index);

	lumDebug("AbsenceData  index=%d m=%d T=%d W=%d T=%d F=%d S=%d Sun=%d active=%d Shour=%d, Sminute=%d Ehour=%d, Eminute=%d time=%d size=%d\n",
			 index,
	         g_deviceConfig.deviceConfigData.absenceData[index].repeatData.monday,
	         g_deviceConfig.deviceConfigData.absenceData[index].repeatData.tuesday,
	         g_deviceConfig.deviceConfigData.absenceData[index].repeatData.wednesday,
	         g_deviceConfig.deviceConfigData.absenceData[index].repeatData.thursday,
	         g_deviceConfig.deviceConfigData.absenceData[index].repeatData.firday,
	         g_deviceConfig.deviceConfigData.absenceData[index].repeatData.saturday,
	         g_deviceConfig.deviceConfigData.absenceData[index].repeatData.sunday,
	         g_deviceConfig.deviceConfigData.absenceData[index].repeatData.bActive,
	         g_deviceConfig.deviceConfigData.absenceData[index].startHour,
	         g_deviceConfig.deviceConfigData.absenceData[index].startMinute,
	         g_deviceConfig.deviceConfigData.absenceData[index].endHour,
	         g_deviceConfig.deviceConfigData.absenceData[index].endMinute,
	         g_deviceConfig.deviceConfigData.absenceData[index].timeData,
	         sizeof(ASBENCE_DATA_INFO));
}


ASBENCE_DATA_INFO* USER_FUNC lum_getAbsenceData(U8 index)
{
	if(index >= MAX_ABSENCE_COUNT)
	{
		return NULL;
	}
	else
	{
		return &g_deviceConfig.deviceConfigData.absenceData[index];
	}
}


static void USER_FUNC lum_initCountDownData(void)
{
	os_memset(&g_deviceConfig.deviceConfigData.countDownData, 0, sizeof(COUNTDOWN_DATA_INFO)*MAX_COUNTDOWN_COUNT);
}



void USER_FUNC lum_setCountDownData(COUNTDOWN_DATA_INFO* countDownData, U8 index)
{
	if(index >= MAX_COUNTDOWN_COUNT)
	{
		return;
	}

	os_memcpy(&g_deviceConfig.deviceConfigData.countDownData[index], countDownData, sizeof(COUNTDOWN_DATA_INFO));
	lum_SaveConfigData();

	//checkCountDownTimerAfterChange(index);
	lumDebug("countDownData active=%d, action=%d, count=%0xX\n", countDownData->flag.bActive,
	         countDownData->action, countDownData->count);
}



void USER_FUNC lum_deleteCountDownData(U8 index)
{
	if(index >= MAX_COUNTDOWN_COUNT)
	{
		return;
	}
	
	os_memset(&g_deviceConfig.deviceConfigData.countDownData[index], 0, sizeof(COUNTDOWN_DATA_INFO));
	//checkCountDownTimerAfterChange(index);
	lum_SaveConfigData();
}



COUNTDOWN_DATA_INFO* USER_FUNC lum_getCountDownData(U8 index)
{
	if(index >= MAX_COUNTDOWN_COUNT)
	{
		return NULL;
	}
	else
	{
		return &g_deviceConfig.deviceConfigData.countDownData[index];
	}
}


void USER_FUNC lum_globalConfigDataInit(void)
{
	os_memset(&g_deviceConfig, 0, sizeof(GLOBAL_CONFIG_DATA));
	lum_loadConfigData();
	if(g_deviceConfig.deviceConfigData.lumitekFlag != LUMITEK_SW_FLAG+1)
	{
		//Device  first power on flag
		os_memset(&g_deviceConfig, 0, sizeof(GLOBAL_CONFIG_DATA));
		g_deviceConfig.deviceConfigData.lumitekFlag = LUMITEK_SW_FLAG;

		lum_initDeviceNameData();
		lum_initAlarmData();
		lum_initAbsenceData();
		lum_initCountDownData();

		lum_SaveConfigData();
	}
}


U8* USER_FUNC lum_showSendType(MSG_ORIGIN socketFrom, BOOL bSend, U8 cmd)
{
	static U8 showData[60];
	U8* sendType;
	U8* dir; 

	os_memset(showData, 0, sizeof(showData));
	
	if(socketFrom == MSG_FROM_UDP)
	{
		sendType = "UDP";
	}
	else if(socketFrom == MSG_FROM_TCP)
	{
		sendType = "TCP";
	}
	else if(socketFrom == MSG_LOCAL_EVENT)
	{
		sendType = "Local";
	}

	if(bSend)
	{
		dir = "===>";
	}
	else
	{
		dir = "<===";
	}

	lum_getStringTime(showData);
	os_sprintf((showData + os_strlen(showData)), " (%s)¡¾0x%02X¡¿%s", sendType, cmd, dir);
	return showData;
}



