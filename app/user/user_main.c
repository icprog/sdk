/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/1/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"

#include "user_interface.h"
#include "driver/uart.h"
#include "lumlink/lumCommon.h"
#include "lumlink/lumUdpSocket.h"
#include "lumlink/lumPlatform.h"
#include "lumlink/lumGpio.h"
#include "lumlink/lumTimeDate.h"



/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_printf("SDK version:%s\n", system_get_sdk_version());

	lum_initIOPin();
	lum_initSystemTime();
	lum_globalConfigDataInit();
	lum_platformInit();
    lum_udpSocketInit();
	lum_messageTaskInit();
}

