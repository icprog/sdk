/*
******************************
*Company:Lumlink
*Data:2015-01-30
*Author:Meiyusong
******************************
*/

#ifndef __LUMLINK_GPIO__H__
#define __LUMLINK_GPIO__H__

#define GPIO_0_ADDR			PERIPHS_IO_MUX_GPIO0_U
#define GPIO_1_ADDR			PERIPHS_IO_MUX_U0TXD_U
#define GPIO_2_ADDR			PERIPHS_IO_MUX_GPIO2_U
#define GPIO_3_ADDR			PERIPHS_IO_MUX_U0RXD_U
#define GPIO_4_ADDR			PERIPHS_IO_MUX_GPIO4_U
#define GPIO_5_ADDR			PERIPHS_IO_MUX_GPIO5_U
#define GPIO_9_ADDR			PERIPHS_IO_MUX_SD_DATA2_U
#define GPIO_10_ADDR		PERIPHS_IO_MUX_SD_DATA3_U
#define GPIO_12_ADDR		PERIPHS_IO_MUX_MTDI_U
#define GPIO_13_ADDR		PERIPHS_IO_MUX_MTCK_U
#define GPIO_14_ADDR		PERIPHS_IO_MUX_MTMS_U
#define GPIO_15_ADDR		PERIPHS_IO_MUX_MTDO_U



#define RED_LED_IO_MUX     GPIO_15_ADDR
#define RED_LED_IO_NUM     15
#define RED_LED_IO_FUNC    FUNC_GPIO15

#define GREEN_LED_IO_MUX     GPIO_12_ADDR
#define GREEN_LED_IO_NUM     12
#define GREEN_LED_IO_FUNC    FUNC_GPIO12

#define BLUE_LED_IO_MUX     GPIO_13_ADDR
#define BLUE_LED_IO_NUM     13
#define BLUE_LED_IO_FUNC    FUNC_GPIO13



typedef enum
{
	SWITCH_CLOSE = 0,
	SWITCH_OPEN = 1
} SWITCH_STATUS;


void lum_initIOPin(void);
void lum_setSwitchStatus(SWITCH_STATUS action);
SWITCH_STATUS lum_getSwitchStatus(void);

#endif

