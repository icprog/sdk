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


#include "lumlink/lumCommon.h"
#include "lumlink/lumSocketAes.h"
#include "lumlink/lumMessageTask.h"




static struct espconn g_UDPespconn;


static void USER_FUNC lum_udpSocketRecv(void *arg, char *pusrdata, unsigned short length)
{
	 U32 ipAddr;

	 
   // lumDebug("Go into lum_udpRecv recvLen=%d\n", length);
	os_memcpy(&ipAddr, g_UDPespconn.proto.udp->remote_ip, SOCKET_IP_LEN);
	lum_sockRecvData(pusrdata, length, MSG_FROM_UDP, ipAddr);
}


void USER_FUNC lum_sendUdpData(U8* socketData, U8 dataLen, U32 ipData)
{
	os_memcpy(g_UDPespconn.proto.udp->remote_ip, &ipData, SOCKET_IP_LEN);
	g_UDPespconn.proto.udp->remote_port = UDP_SOCKET_PORT;
	//lum_showHexData(socketData, dataLen);
	espconn_sent(&g_UDPespconn, socketData, dataLen);
}


void USER_FUNC lum_udpSocketInit(void)
{
    g_UDPespconn.type = ESPCONN_UDP;
    g_UDPespconn.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	g_UDPespconn.proto.tcp->local_port = UDP_SOCKET_PORT;
    espconn_regist_recvcb(&g_UDPespconn, lum_udpSocketRecv);
    espconn_create(&g_UDPespconn);
}




