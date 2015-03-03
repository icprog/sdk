/*
******************************
*Company:Lumlink
*Data:2015-03-01
*Author:Meiyusong
******************************
*/

#ifndef __LUMLINK_SEND_LIST_H__
#define __LUMLINK_SEND_LIST_H__



#define MAX_RESEND_COUNT			4		//4¥Œ
#define MAX_RESEND_INTERVAL			8		//8√Î
#define RESEND_TIMER_GAP			4000	//4S
#define NEXT_SOCKET_SEND_DELAY		100


typedef struct
{
	U8 sendCount;
	U8 dataLen;
	U8 bReback;
	MSG_ORIGIN msgOrigin;
	U16 cmdData;
	U16 snIndex;
	U8* pData;
	U32 socketIp;
	U32 nextSendTime;	
} SEND_NODE_DATA;


typedef struct send_node
{
	SEND_NODE_DATA nodeBody;
	struct send_node* pNodeNext;
} SEND_NODE;


typedef struct
{
	U16 noteCount;
	SEND_NODE* firstNodePtr;
} SEND_LIST_HEADER;


void USER_FUNC lum_sendListInit(void);
BOOL USER_FUNC lum_deleteRequstSendNode(U8* pSocketDataRecv);
BOOL USER_FUNC lum_addSendDataToNode(SEND_NODE_DATA* pSendData);
void USER_FUNC lum_checkSendList(void);


#endif
