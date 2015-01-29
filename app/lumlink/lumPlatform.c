/*
******************************
*Company:Lumitek
*Data:2015-01-26
*Author:Meiyusong
******************************
*/


#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "lumlink/lumCommon.h"
#include "lumlink/lumPlatform.h"


static os_timer_t g_APConnectTimer;



static void USER_FUNC lum_checkConnectStatus(U8 reset_flag)
{
    struct ip_info ipconfig;
	U8 status;

    os_timer_disarm(&g_APConnectTimer);
    wifi_get_ip_info(STATION_IF, &ipconfig);    
    os_timer_setfn(&g_APConnectTimer, (os_timer_func_t *)lum_checkConnectStatus, NULL);
	status = wifi_station_get_connect_status();
	if(status != STATION_GOT_IP)
	{
    	os_timer_arm(&g_APConnectTimer, 1000, 0);
	}
	else
	{
		os_timer_arm(&g_APConnectTimer, 20000, 0);
	}
	//wifi_station_scan(struct scan_config * config,scan_done_cb_t cb)
	lumDebug("******IP=0x%X, status=%d\n", ipconfig.ip.addr, status);
}


void USER_FUNC lum_platformInit(void)
{
	struct station_config staConfig;
	//char* staSSID = "TP-LINK_47D6";
	//char* staPSWD = "13736098070";
	char* staSSID = "KKKK";
	char* staPSWD = "12340000";
	U8 macaddr[6];


	lumDebug("\nSW_Version=%s HW_Version= %s\n",SW_VERSION, HW_VERSION);
	// user_esp_platform_load_param(&esp_param);
	memset(&staConfig, 0, sizeof(struct station_config));
	strcpy(staConfig.ssid, staSSID);
	strcpy(staConfig.password, staPSWD);
	wifi_station_set_config(&staConfig);
	wifi_set_opmode(STATION_MODE);
	os_printf("staSSID=%s ssidLen=%d staPSWD=%s pswdLen=%d\n", staConfig.ssid, strlen(staConfig.ssid), staConfig.password, strlen(staConfig.password));

	lum_getDeviceMac(macaddr);
	lumDebug("mac="MACSTR"\n", MAC2STR(macaddr));


	os_timer_disarm(&g_APConnectTimer);
	os_timer_setfn(&g_APConnectTimer, (os_timer_func_t *)lum_checkConnectStatus, 1);
	os_timer_arm(&g_APConnectTimer, 1000, 0);
}


void USER_FUNC lum_SaveConfigData(DEVICE_CONFIG_DATA* configData)
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


void USER_FUNC lum_LoadConfigData(DEVICE_CONFIG_DATA* configData)
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


