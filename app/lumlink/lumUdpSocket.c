/*
******************************
*Company:Lumitek
*Data:2015-01-26
*Author:Meiyusong
******************************
*/


#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "espconn.h"


#include "user_interface.h"
#include "lumlink/lumCommon.h"

static struct espconn g_UDPespconn;

static void USER_FUNC lum_udpSocketRecv(void *arg, char *pusrdata, unsigned short length)
{
    lumDebug("Go into lum_udpRecv recvLen=%d\n", length);
}



void USER_FUNC lum_udpSocketInit(void)
{
    g_UDPespconn.type = ESPCONN_UDP;
    g_UDPespconn.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
    g_UDPespconn.proto.udp->local_port = UDP_SOCKET_PORT;
    espconn_regist_recvcb(&g_UDPespconn, lum_udpSocketRecv);
    espconn_create(&g_UDPespconn);
}




