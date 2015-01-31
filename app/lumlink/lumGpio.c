/*
******************************
*Company:Lumitek
*Data:2015-01-30
*Author:Meiyusong
******************************
*/


#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "gpio.h"
#include "user_interface.h"

#include "lumlink/lumCommon.h"
#include "lumlink/lumSocketAes.h"
#include "lumlink/lumMessageTask.h"
#include "lumlink/lumGpio.h"



static SWITCH_STATUS g_switchStatus = SWITCH_CLOSE;
static U8 g_ledIndex;
static os_timer_t g_led_timer;




void USER_FUNC lum_initIOPin(void)
{
	PIN_FUNC_SELECT(RED_LED_IO_MUX, RED_LED_IO_FUNC);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(RED_LED_IO_NUM), 0);

	PIN_FUNC_SELECT(BLUE_LED_IO_MUX, BLUE_LED_IO_FUNC);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(BLUE_LED_IO_NUM), 0);

	PIN_FUNC_SELECT(GREEN_LED_IO_MUX, GREEN_LED_IO_FUNC);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(GREEN_LED_IO_NUM), 0);

	g_ledIndex = 0;
}


static void USER_FUNC lum_changeLedStatus(U8 ledStatus)
{
	if(ledStatus > 3)
	{
		return;
	}

	if(ledStatus == 1)
	{
		GPIO_OUTPUT_SET(GPIO_ID_PIN(RED_LED_IO_NUM), 1);
	}
	else
	{
		GPIO_OUTPUT_SET(GPIO_ID_PIN(RED_LED_IO_NUM), 0);
	}

	if(ledStatus == 2)
	{
		GPIO_OUTPUT_SET(GPIO_ID_PIN(GREEN_LED_IO_NUM), 1);
	}
	else
	{
		GPIO_OUTPUT_SET(GPIO_ID_PIN(GREEN_LED_IO_NUM), 0);
	}

	if(ledStatus == 3)
	{
		GPIO_OUTPUT_SET(GPIO_ID_PIN(BLUE_LED_IO_NUM), 1);
	}
	else
	{
		GPIO_OUTPUT_SET(GPIO_ID_PIN(BLUE_LED_IO_NUM), 0);
	}
}


static void USER_FUNC lum_ledTimerCallback(void *arg)
{
	if(g_ledIndex < 3)
	{
		g_ledIndex++;
	}
	else
	{
		g_ledIndex = 1;
	}
	lum_changeLedStatus(g_ledIndex);
	lumDebug("g_ledIndex=%d\n", g_ledIndex);
}


static void USER_FUNC lum_openLedStatus(void)
{
	os_timer_disarm(&g_led_timer);
    os_timer_setfn(&g_led_timer, (os_timer_func_t *)lum_ledTimerCallback, NULL);
    os_timer_arm(&g_led_timer, 1000, 1);
	g_ledIndex = 1;
	lum_changeLedStatus(g_ledIndex);
}


static void USER_FUNC lum_closeLedStatus(void)
{
	os_timer_disarm(&g_led_timer);
	g_ledIndex = 0;
	lum_changeLedStatus(g_ledIndex);
}


void USER_FUNC lum_setSwitchStatus(SWITCH_STATUS action)
{
	U8 switchLevel;


	if(action == SWITCH_OPEN)
	{
		lum_openLedStatus();
		g_switchStatus = SWITCH_CLOSE;
	}
	else
	{
		lum_closeLedStatus();
		g_switchStatus = SWITCH_OPEN;
	}
}


SWITCH_STATUS USER_FUNC lum_getSwitchStatus(void)
{
	return g_switchStatus;
}


