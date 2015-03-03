/*
******************************
*Company:Lumlink
*Data:2015-02-01
*Author:Meiyusong
******************************
*/

#ifndef __LUMLINK_TCP_SOCKET_H__
#define __LUMLINK_TCP_SOCKET_H__


#define TCP_RECONNECT_TIMER_GAP		(5000)


typedef enum
{
	TCP_NONE_CONNECT,
	TCP_BALANCE_CONNECTED,
	TCP_SERVER_CONNECTED,
}TCP_CONN_STATUS;


void USER_FUNC lum_connBalanceServer(void);
BOOL USER_FUNC lum_sendTcpData(U8* socketData, U8 dataLen);
void USER_FUNC lum_connActualServer(void);
void USER_FUNC lum_disconnectBalanceServer(void);


#endif
