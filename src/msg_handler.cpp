#include "zmd_msg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h> //htonl
#include "plog.h"
#include "msg_handler.h"
#include "device_operation.h"

typedef int (*handler_t)(trans_msg_s*, void*, int, PrivateMsgHandler*); 

typedef struct _msg_handler_t {
    int cmd;
    handler_t hdl;   
} msg_handler_t;

struct common_resp_t
{
	trans_msg_s head;
	char result;
	char reserved[3];
	char uid[0];
};

//add new handler here
#define MSG_HANDLER_LIST(XX)\
	XX(IPC_TRANS_HEART_SYN, handle_heart_beat_syn)\
	XX(IPC_TRANS_HEART_ACK, handle_heart_beat_ack)\
	XX(PC_IPC_HEART_SYN, handle_heart_beat_syn)\
	XX(PC_IPC_TRANSFER_DATA, handle_talk_data)\
	XX(PC_IPC_MIC_STATUS_SYN, handle_mic_switch_syn)\
	XX(IPC_PC_PLAYBACK_STOP_ACK, handle_playback_stop_ack)\
	XX(PC_IPC_PTZ_SYN, handle_ptz_syn)\
	XX(PC_IPC_GET_PRESETPOINT_SYN, handle_get_preset_syn)\
	XX(PC_IPC_REPLY_DOORBELL_SYN, handle_doorbell_answered)\
	XX(IPC_PC_TALK_HEART_SYN, handle_talk_heartbeat_syn)
	
#define MSG_HANDLER_DECLARE(MSG, FUNC)\
	int FUNC(trans_msg_s* , void* , int , PrivateMsgHandler*);
#define MSG_HANDLER_ARRAY_DEFINE(MSG, FUNC)\
	{MSG, FUNC},

MSG_HANDLER_LIST(MSG_HANDLER_DECLARE)

static msg_handler_t msg_hdls[] =
{
	MSG_HANDLER_LIST(MSG_HANDLER_ARRAY_DEFINE)
    {0, NULL}
};

#undef MSG_HANDLER_LIST
#undef MSG_HANDLER_DECLARE
#undef MSG_HANDLER_ARRAY_DEFINE

int PrivateMsgHandler::HandleMsg(trans_msg_s* head, void* data, int len)
{
    //plog("\n");
    return handle_common_msg(head, data, len, this);
}

PrivateMsgHandler::~PrivateMsgHandler()
{
	/* 强制释放 */
	if(m_is_hold_mic)
		p2p_free_mic(this, (char*)this);
}

int handle_common_msg(trans_msg_s* head, void* data, int len, PrivateMsgHandler* sender)
{
    //plog("handle msg %d\n", ntohl(head->cmd));
	
    for (int i = 0; msg_hdls[i].cmd; i++) 
    {
        if((unsigned int)msg_hdls[i].cmd == ntohl(head->cmd))
        {
            return msg_hdls[i].hdl(head, data, len, sender);
        }
    }
    plog("no found handler for msg(%d)!!!\n", ntohl(head->cmd));
    return 0; 
}

int handle_heart_beat_syn(trans_msg_s* head, void* data, int len, PrivateMsgHandler* sender)
{
	plogfn();
    head->cmd = htonl(PC_IPC_HEART_ACK);

    //plog("sending heart beat ack...\n");
    return sender->SendReply(head, P2P_HEAD_LEN);
}
int handle_heart_beat_ack(trans_msg_s* head, void* data, int len, PrivateMsgHandler* sender)
{
    //plog("%s\n", "recv heart beat ack");
    plogfn();
    return 0;
}

int handle_mic_switch_syn(trans_msg_s* head, void* data, int len, PrivateMsgHandler* sender)
{
    plogfn();

    int status = *(char*)data;
	char buf[1024]= {0};
	
	if(len >= 256) 
		return -1;

	common_resp_t *resp = (common_resp_t *)buf;

	memcpy(resp, head, P2P_HEAD_LEN);
	memcpy(buf+P2P_HEAD_LEN, data, len);
	
    resp->head.cmd = htonl(PC_IPC_MIC_STATUS_ACK);

    resp->head.length = htonl(len);
    if(status)
    {
        plog("try to open mic\n");
		if(len>64)
			resp->result= p2p_hold_mic(sender, resp->uid);
		else
			resp->result= p2p_hold_mic(sender, NULL);
		if(!sender->m_is_hold_mic)
			sender->m_is_hold_mic = !resp->result;
		if(sender->m_is_hold_mic)
			time(&sender->m_last_talk_heartbeat);
    }
    else if(!status)
    {
        plog("free mic\n");
        resp->result=0;
		if(sender->m_is_hold_mic)
		{
			if(len>64)
				sender->m_is_hold_mic = !p2p_free_mic(sender, resp->uid);
			else
				sender->m_is_hold_mic = !p2p_free_mic(sender, NULL);
		}	
    }
    else
        resp->result=1;
    return sender->SendReply(resp, P2P_HEAD_LEN+len);
}

int handle_talk_data(trans_msg_s* head, void* data, int len, PrivateMsgHandler* sender)
{
	static unsigned int count = 0;

	if(count++%50 == 0)
		plogfn();
	p2p_decode_audio(data, len);

    return 0;
}
int handle_playback_stop_ack(trans_msg_s* head, void* data, int len, PrivateMsgHandler* sender)
{
    plogfn();
    return 0;
}

int handle_ptz_syn(trans_msg_s* head, void* data, int len, PrivateMsgHandler* sender)
{
	plogfn();
    struct ptz_data_t
    {
        char  channed_id;
        char  reserved[2];
        char  ptz_cmd;
        short para0;
        short para1;
        char  uid[64];
    };
    struct ptz_resp_t
    {
        trans_msg_s head;
        char result;
        char reserved[3];
        char uid[64];
    };

    if(len < sizeof(ptz_data_t))
    {
        plog("data is less than expected!!\n");
        return 0;
    }
    ptz_data_t *pdata = (ptz_data_t*)data;

    p2p_ptz_operat(head->channel_id, pdata->ptz_cmd, pdata->para0, pdata->para1);

    ptz_resp_t resp;
    memcpy(&resp.head, head, P2P_HEAD_LEN);
    resp.head.cmd = htonl(PC_IPC_PTZ_ACK);
    resp.head.length = htonl( sizeof(ptz_resp_t) - P2P_HEAD_LEN);
    resp.result = 0;
    memcpy(resp.uid, pdata->uid, 64);
    sender->SendReply(&resp, sizeof(resp));
    return 0;
}

int handle_get_preset_syn(trans_msg_s* head, void* data, int len, PrivateMsgHandler* sender)
{
	plogfn();
    struct get_preset_resp_t
    {
        trans_msg_s head;
        char result;
        char reserved;
        char residece_time;
        char speed;
        char uid[64];
        int  preset_point_num;
        char preset_point_info[0];
    };
    if(len < 64)
    {
        plog("data is less than expected!!\n");
        return 0;
    }
    char readbuf[512] = {0};
    int  readLen = 0;

    char *sendbuf = (char*)malloc(sizeof(get_preset_resp_t)+ 512);
    
    if(!sendbuf) return 0;

    get_preset_resp_t* resp = (get_preset_resp_t*)sendbuf;

    memset(resp, 0, sizeof(get_preset_resp_t));

    if(p2p_ptz_get_preset(readbuf, &readLen) != -1)
    {
        if(readLen > 0 && readLen < sizeof(readbuf))
        {
            resp->result = 0;
            resp->preset_point_num = readLen;
            memcpy( resp->preset_point_info , readbuf , readLen) ; //n字节预置点信息+1字节停留时间+1字节滑竿速度
            resp->residece_time = readbuf[readLen-2];
            resp->speed = readbuf[readLen-1];
        }
        else
        {
            plog("read data error:%d\n",readLen);
            resp->result = 1;
            readLen = 0;
        }
    }
    else
    {
        resp->result = 1;
    }
    memcpy(&resp->head, head, P2P_HEAD_LEN);
    memcpy(resp->uid, data, 64);
    resp->head.cmd = htonl(PC_IPC_GET_PRESETPOINT_ACK);
    resp->head.length = htonl(sizeof(get_preset_resp_t) + readLen - P2P_HEAD_LEN);
    sender->SendReply(resp, sizeof(get_preset_resp_t)+readLen);

    free(sendbuf);
	return 0;
}
/*
* @fuction Press the doorbell events (answer)
*/
int handle_doorbell_answered(trans_msg_s* head, void* data, int len, PrivateMsgHandler* sender)
{
	plogfn();

	common_resp_t resp;
	memset(&resp, 0x0, sizeof(common_resp_t));

	memcpy(&resp.head, head, P2P_HEAD_LEN);
   	resp.head.cmd = htonl(PC_IPC_REPLY_DOORBELL_ACK);
    resp.head.length = htonl(sizeof(common_resp_t) - P2P_HEAD_LEN);
   	sender->SendReply(&resp, sizeof(common_resp_t));
	
	p2p_handle_answer_call();
	return 0;
}
int handle_talk_heartbeat_syn(trans_msg_s* head, void* data, int len, PrivateMsgHandler* sender)
{
	plogfn();
	time(&sender->m_last_talk_heartbeat);
	return 0;
}

